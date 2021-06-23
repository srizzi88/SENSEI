/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSampleFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSampleFunction.h"

#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkGarbageCollector.h"
#include "svtkImageData.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkSampleFunction);
svtkCxxSetObjectMacro(svtkSampleFunction, ImplicitFunction, svtkImplicitFunction);

// The heart of the algorithm plus interface to the SMP tools.
template <class T>
class svtkSampleFunctionAlgorithm
{
public:
  svtkImplicitFunction* ImplicitFunction;
  T* Scalars;
  float* Normals;
  svtkIdType Extent[6];
  svtkIdType Dims[3];
  svtkIdType SliceSize;
  double Origin[3];
  double Spacing[3];
  double CapValue;

  // Constructor
  svtkSampleFunctionAlgorithm();

  // Interface between SVTK and templated functions
  static void SampleAcrossImage(
    svtkSampleFunction* self, svtkImageData* output, int extent[6], T* scalars, float* normals);

  // Cap the boundaries with the specified cap value (if requested).
  void Cap();

  // Interface implicit function computation to SMP tools.
  template <class TT>
  class FunctionValueOp
  {
  public:
    FunctionValueOp(svtkSampleFunctionAlgorithm<TT>* algo) { this->Algo = algo; }
    svtkSampleFunctionAlgorithm* Algo;

    void operator()(svtkIdType k, svtkIdType end)
    {
      double x[3];
      svtkIdType* extent = this->Algo->Extent;
      svtkIdType i, j, jOffset, kOffset;
      for (; k < end; ++k)
      {
        x[2] = this->Algo->Origin[2] + k * this->Algo->Spacing[2];
        kOffset = (k - extent[4]) * this->Algo->SliceSize;
        for (j = extent[2]; j <= extent[3]; ++j)
        {
          x[1] = this->Algo->Origin[1] + j * this->Algo->Spacing[1];
          jOffset = (j - extent[2]) * this->Algo->Dims[0];
          for (i = extent[0]; i <= extent[1]; ++i)
          {
            x[0] = this->Algo->Origin[0] + i * this->Algo->Spacing[0];
            this->Algo->Scalars[(i - extent[0]) + jOffset + kOffset] =
              static_cast<TT>(this->Algo->ImplicitFunction->FunctionValue(x));
          }
        }
      }
    }
  };

  // Interface implicit function graadient computation to SMP tools.
  template <class TT>
  class FunctionGradientOp
  {
  public:
    FunctionGradientOp(svtkSampleFunctionAlgorithm<TT>* algo) { this->Algo = algo; }
    svtkSampleFunctionAlgorithm* Algo;

    void operator()(svtkIdType k, svtkIdType end)
    {
      double x[3], n[3];
      float* nPtr;
      svtkIdType* extent = this->Algo->Extent;
      svtkIdType i, j, jOffset, kOffset;
      for (; k < end; ++k)
      {
        x[2] = this->Algo->Origin[2] + k * this->Algo->Spacing[2];
        kOffset = (k - extent[4]) * this->Algo->SliceSize;
        for (j = extent[2]; j <= extent[3]; ++j)
        {
          x[1] = this->Algo->Origin[1] + j * this->Algo->Spacing[1];
          jOffset = (j - extent[2]) * this->Algo->Dims[0];
          for (i = extent[0]; i <= extent[1]; ++i)
          {
            x[0] = this->Algo->Origin[0] + i * this->Algo->Spacing[0];
            this->Algo->ImplicitFunction->FunctionGradient(x, n);
            svtkMath::Normalize(n);
            nPtr = this->Algo->Normals + 3 * ((i - extent[0]) + jOffset + kOffset);
            nPtr[0] = static_cast<TT>(-n[0]);
            nPtr[1] = static_cast<TT>(-n[1]);
            nPtr[2] = static_cast<TT>(-n[2]);
          } // i
        }   // j
      }     // k
    }
  };
};

//----------------------------------------------------------------------------
// Initialized mainly to eliminate compiler warnings.
template <class T>
svtkSampleFunctionAlgorithm<T>::svtkSampleFunctionAlgorithm()
  : Scalars(nullptr)
  , Normals(nullptr)
{
  for (int i = 0; i < 3; ++i)
  {
    this->Extent[2 * i] = this->Extent[2 * i + 1] = 0;
    this->Dims[i] = 0;
    this->Origin[i] = this->Spacing[i] = 0.0;
  }
  this->SliceSize = 0;
  this->CapValue = 0.0;
}

//----------------------------------------------------------------------------
// Templated class is glue between SVTK and templated algorithms.
template <class T>
void svtkSampleFunctionAlgorithm<T>::SampleAcrossImage(
  svtkSampleFunction* self, svtkImageData* output, int extent[6], T* scalars, float* normals)
{
  // Populate data into local storage
  svtkSampleFunctionAlgorithm<T> algo;
  algo.ImplicitFunction = self->GetImplicitFunction();
  algo.Scalars = scalars;
  algo.Normals = normals;
  for (int i = 0; i < 3; ++i)
  {
    algo.Extent[2 * i] = extent[2 * i];
    algo.Extent[2 * i + 1] = extent[2 * i + 1];
    algo.Dims[i] = extent[2 * i + 1] - extent[2 * i] + 1;
  }
  algo.SliceSize = algo.Dims[0] * algo.Dims[1];
  output->GetOrigin(algo.Origin);
  output->GetSpacing(algo.Spacing);
  algo.CapValue = self->GetCapValue();

  // Okay now generate samples using SMP tools
  FunctionValueOp<T> values(&algo);
  svtkSMPTools::For(extent[4], extent[5] + 1, values);

  // If requested, generate normals
  if (algo.Normals)
  {
    FunctionGradientOp<T> gradient(&algo);
    svtkSMPTools::For(extent[4], extent[5] + 1, gradient);
  }

  // If requested, cap boundaries
  if (self->GetCapping())
  {
    algo.Cap();
  }
}

//----------------------------------------------------------------------------
// Cap the boundaries of the volume if requested.
template <class T>
void svtkSampleFunctionAlgorithm<T>::Cap()
{
  svtkIdType i, j, k;
  svtkIdType idx;

  // i-j planes
  // k = this->Extent[4];
  for (j = this->Extent[2]; j <= this->Extent[3]; j++)
  {
    for (i = this->Extent[0]; i <= this->Extent[1]; i++)
    {
      this->Scalars[i + j * this->Dims[0]] = this->CapValue;
    }
  }

  idx = this->Extent[5] * this->SliceSize;
  for (j = this->Extent[2]; j <= this->Extent[3]; j++)
  {
    for (i = this->Extent[0]; i <= this->Extent[1]; i++)
    {
      this->Scalars[idx + i + j * this->Dims[0]] = this->CapValue;
    }
  }

  // j-k planes
  // i = this->Extent[0];
  for (k = this->Extent[4]; k <= this->Extent[5]; k++)
  {
    for (j = this->Extent[2]; j <= this->Extent[3]; j++)
    {
      this->Scalars[j * this->Dims[0] + k * this->SliceSize] = this->CapValue;
    }
  }

  i = this->Extent[1];
  for (k = this->Extent[4]; k <= this->Extent[5]; k++)
  {
    for (j = this->Extent[2]; j <= this->Extent[3]; j++)
    {
      this->Scalars[i + j * this->Dims[0] + k * this->SliceSize] = this->CapValue;
    }
  }

  // i-k planes
  // j = this->Extent[2];
  for (k = this->Extent[4]; k <= this->Extent[5]; k++)
  {
    for (i = this->Extent[0]; i <= this->Extent[1]; i++)
    {
      this->Scalars[i + k * this->SliceSize] = this->CapValue;
    }
  }

  j = this->Extent[3];
  idx = j * this->Dims[0];
  for (k = this->Extent[4]; k <= this->Extent[5]; k++)
  {
    for (i = this->Extent[0]; i <= this->Extent[1]; i++)
    {
      this->Scalars[idx + i + k * this->SliceSize] = this->CapValue;
    }
  }
}

//----------------------------------------------------------------------------
// Okay define the SVTK class proper
svtkSampleFunction::svtkSampleFunction()
{
  this->ModelBounds[0] = -1.0;
  this->ModelBounds[1] = 1.0;
  this->ModelBounds[2] = -1.0;
  this->ModelBounds[3] = 1.0;
  this->ModelBounds[4] = -1.0;
  this->ModelBounds[5] = 1.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Capping = 0;
  this->CapValue = SVTK_DOUBLE_MAX;

  this->ImplicitFunction = nullptr;

  this->ComputeNormals = 1;
  this->OutputScalarType = SVTK_DOUBLE;

  this->ScalarArrayName = nullptr;
  this->SetScalarArrayName("scalars");

  this->NormalArrayName = nullptr;
  this->SetNormalArrayName("normals");

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkSampleFunction::~svtkSampleFunction()
{
  this->SetImplicitFunction(nullptr);
  this->SetScalarArrayName(nullptr);
  this->SetNormalArrayName(nullptr);
}

//----------------------------------------------------------------------------
// Specify the dimensions of the data on which to sample.
void svtkSampleFunction::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
// Specify the dimensions of the data on which to sample.
void svtkSampleFunction::SetSampleDimensions(int dim[3])
{
  svtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2]
                << ")");

  if (dim[0] != this->SampleDimensions[0] || dim[1] != this->SampleDimensions[1] ||
    dim[2] != this->SampleDimensions[2])
  {
    for (int i = 0; i < 3; i++)
    {
      this->SampleDimensions[i] = (dim[i] > 0 ? dim[i] : 1);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Set the bounds of the model.
void svtkSampleFunction::SetModelBounds(const double bounds[6])
{
  this->SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

//----------------------------------------------------------------------------
void svtkSampleFunction::SetModelBounds(
  double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
  svtkDebugMacro(<< " setting ModelBounds to ("
                << "(" << xMin << "," << xMax << "), "
                << "(" << yMin << "," << yMax << "), "
                << "(" << zMin << "," << zMax << "), ");
  if ((xMin > xMax) || (yMin > yMax) || (zMin > zMax))
  {
    svtkErrorMacro("Invalid bounds: "
      << "(" << xMin << "," << xMax << "), "
      << "(" << yMin << "," << yMax << "), "
      << "(" << zMin << "," << zMax << ")"
      << " Bound mins cannot be larger that bound maxs");
    return;
  }
  if (xMin != this->ModelBounds[0] || xMax != this->ModelBounds[1] ||
    yMin != this->ModelBounds[2] || yMax != this->ModelBounds[3] || zMin != this->ModelBounds[4] ||
    zMax != this->ModelBounds[5])
  {
    this->ModelBounds[0] = xMin;
    this->ModelBounds[1] = xMax;
    this->ModelBounds[2] = yMin;
    this->ModelBounds[3] = yMax;
    this->ModelBounds[4] = zMin;
    this->ModelBounds[5] = zMax;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkSampleFunction::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  int wExt[6];
  wExt[0] = 0;
  wExt[2] = 0;
  wExt[4] = 0;
  wExt[1] = this->SampleDimensions[0] - 1;
  wExt[3] = this->SampleDimensions[1] - 1;
  wExt[5] = this->SampleDimensions[2] - 1;

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt, 6);

  for (i = 0; i < 3; i++)
  {
    origin[i] = this->ModelBounds[2 * i];
    if (this->SampleDimensions[i] <= 1)
    {
      ar[i] = 1;
    }
    else
    {
      ar[i] =
        (this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) / (this->SampleDimensions[i] - 1);
    }
  }
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), ar, 3);

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, 1);

  outInfo->Set(svtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
// Produce the data.
void svtkSampleFunction::ExecuteDataWithInformation(svtkDataObject* outp, svtkInformation* outInfo)
{
  svtkFloatArray* newNormals = nullptr;
  float* normals = nullptr;

  svtkImageData* output = this->GetOutput();
  int* extent = this->GetExecutive()->GetOutputInformation(0)->Get(
    svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  output->SetExtent(extent);
  output = this->AllocateOutputData(outp, outInfo);
  svtkDataArray* newScalars = output->GetPointData()->GetScalars();
  svtkIdType numPts = newScalars->GetNumberOfTuples();

  svtkDebugMacro(<< "Sampling implicit function");

  // Initialize self; create output objects
  //
  if (!this->ImplicitFunction)
  {
    svtkErrorMacro(<< "No implicit function specified");
    return;
  }

  if (this->ComputeNormals)
  {
    newNormals = svtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->SetNumberOfTuples(numPts);
    normals = newNormals->WritePointer(0, numPts);
  }

  void* ptr = output->GetArrayPointerForExtent(newScalars, extent);
  switch (newScalars->GetDataType())
  {
    svtkTemplateMacro(svtkSampleFunctionAlgorithm<SVTK_TT>::SampleAcrossImage(
      this, output, extent, (SVTK_TT*)ptr, normals));
  }

  newScalars->SetName(this->ScalarArrayName);

  // Update self
  //
  if (newNormals)
  {
    // For an unknown reason yet, if the following line is not commented out,
    // it will make ImplicitSum, TestBoxFunction and TestDiscreteMarchingCubes
    // to fail.
    newNormals->SetName(this->NormalArrayName);
    output->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkSampleFunction::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType impFuncMTime;

  if (this->ImplicitFunction != nullptr)
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkSampleFunction::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
     << this->SampleDimensions[1] << ", " << this->SampleDimensions[2] << ")\n";
  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";

  if (this->ImplicitFunction)
  {
    os << indent << "Implicit Function: " << this->ImplicitFunction << "\n";
  }
  else
  {
    os << indent << "No Implicit function defined\n";
  }

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");

  os << indent << "ScalarArrayName: ";
  if (this->ScalarArrayName != nullptr)
  {
    os << this->ScalarArrayName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "NormalArrayName: ";
  if (this->NormalArrayName != nullptr)
  {
    os << this->NormalArrayName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }
}

//----------------------------------------------------------------------------
void svtkSampleFunction::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->ImplicitFunction, "ImplicitFunction");
}
