/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointInterpolator.h"

#include "svtkAbstractPointLocator.h"
#include "svtkArrayListTemplate.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLinearKernel.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticPointLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <vector>

svtkStandardNewMacro(svtkPointInterpolator);
svtkCxxSetObjectMacro(svtkPointInterpolator, Locator, svtkAbstractPointLocator);
svtkCxxSetObjectMacro(svtkPointInterpolator, Kernel, svtkInterpolationKernel);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{
// The threaded core of the algorithm
struct ProbePoints
{
  svtkPointInterpolator* PointInterpolator;
  svtkDataSet* Input;
  svtkInterpolationKernel* Kernel;
  svtkAbstractPointLocator* Locator;
  svtkPointData* InPD;
  svtkPointData* OutPD;
  ArrayList Arrays;
  char* Valid;
  int Strategy;
  bool Promote;

  // Don't want to allocate these working arrays on every thread invocation,
  // so make them thread local.
  svtkSMPThreadLocalObject<svtkIdList> PIds;
  svtkSMPThreadLocalObject<svtkDoubleArray> Weights;

  ProbePoints(svtkPointInterpolator* ptInt, svtkDataSet* input, svtkPointData* inPD,
    svtkPointData* outPD, char* valid)
    : PointInterpolator(ptInt)
    , Input(input)
    , InPD(inPD)
    , OutPD(outPD)
    , Valid(valid)
  {
    // Gather information from the interpolator
    this->Kernel = ptInt->GetKernel();
    this->Locator = ptInt->GetLocator();
    this->Strategy = ptInt->GetNullPointsStrategy();
    double nullV = ptInt->GetNullValue();
    this->Promote = ptInt->GetPromoteOutputArrays();

    // Manage arrays for interpolation
    for (int i = 0; i < ptInt->GetNumberOfExcludedArrays(); ++i)
    {
      const char* arrayName = ptInt->GetExcludedArray(i);
      svtkDataArray* array = this->InPD->GetArray(arrayName);
      if (array != nullptr)
      {
        outPD->RemoveArray(array->GetName());
        this->Arrays.ExcludeArray(array);
      }
    }
    this->Arrays.AddArrays(input->GetNumberOfPoints(), inPD, outPD, nullV, this->Promote);
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    svtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
    svtkDoubleArray*& weights = this->Weights.Local();
    weights->Allocate(128);
  }

  // When null point is encountered
  void AssignNullPoint(const double x[3], svtkIdList* pIds, svtkDoubleArray* weights, svtkIdType ptId)
  {
    if (this->Strategy == svtkPointInterpolator::MASK_POINTS)
    {
      this->Valid[ptId] = 0;
      this->Arrays.AssignNullValue(ptId);
    }
    else if (this->Strategy == svtkPointInterpolator::NULL_VALUE)
    {
      this->Arrays.AssignNullValue(ptId);
    }
    else // svtkPointInterpolator::CLOSEST_POINT:
    {
      pIds->SetNumberOfIds(1);
      svtkIdType pId = this->Locator->FindClosestPoint(x);
      pIds->SetId(0, pId);
      weights->SetNumberOfTuples(1);
      weights->SetValue(0, 1.0);
      this->Arrays.Interpolate(1, pIds->GetPointer(0), weights->GetPointer(0), ptId);
    }
  }

  // Threaded interpolation method
  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double x[3];
    svtkIdList*& pIds = this->PIds.Local();
    svtkIdType numWeights;
    svtkDoubleArray*& weights = this->Weights.Local();

    for (; ptId < endPtId; ++ptId)
    {
      this->Input->GetPoint(ptId, x);

      if (this->Kernel->ComputeBasis(x, pIds) > 0)
      {
        numWeights = this->Kernel->ComputeWeights(x, pIds, weights);
        this->Arrays.Interpolate(numWeights, pIds->GetPointer(0), weights->GetPointer(0), ptId);
      }
      else
      {
        this->AssignNullPoint(x, pIds, weights, ptId);
      } // null point
    }   // for all dataset points
  }

  void Reduce() {}

}; // ProbePoints

// Probe points using an image. Uses a more efficient iteration scheme.
struct ImageProbePoints : public ProbePoints
{
  int Dims[3];
  double Origin[3];
  double Spacing[3];

  ImageProbePoints(svtkPointInterpolator* ptInt, svtkImageData* image, int dims[3], double origin[3],
    double spacing[3], svtkPointData* inPD, svtkPointData* outPD, char* valid)
    : ProbePoints(ptInt, image, inPD, outPD, valid)
  {
    for (int i = 0; i < 3; ++i)
    {
      this->Dims[i] = dims[i];
      this->Origin[i] = origin[i];
      this->Spacing[i] = spacing[i];
    }
  }

  // Threaded interpolation method specialized to image traversal
  void operator()(svtkIdType slice, svtkIdType sliceEnd)
  {
    double x[3];
    svtkIdType numWeights;
    double* origin = this->Origin;
    double* spacing = this->Spacing;
    int* dims = this->Dims;
    svtkIdType ptId, jOffset, kOffset, sliceSize = dims[0] * dims[1];
    svtkIdList*& pIds = this->PIds.Local();
    svtkDoubleArray*& weights = this->Weights.Local();

    for (; slice < sliceEnd; ++slice)
    {
      x[2] = origin[2] + slice * spacing[2];
      kOffset = slice * sliceSize;

      for (int j = 0; j < dims[1]; ++j)
      {
        x[1] = origin[1] + j * spacing[1];
        jOffset = j * dims[0];

        for (int i = 0; i < dims[0]; ++i)
        {
          x[0] = origin[0] + i * spacing[0];
          ptId = i + jOffset + kOffset;

          if (this->Kernel->ComputeBasis(x, pIds) > 0)
          {
            numWeights = this->Kernel->ComputeWeights(x, pIds, weights);
            this->Arrays.Interpolate(numWeights, pIds->GetPointer(0), weights->GetPointer(0), ptId);
          }
          else
          {
            this->AssignNullPoint(x, pIds, weights, ptId);
          } // null point

        } // over i
      }   // over j
    }     // over slices
  }
}; // ImageProbePoints

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkPointInterpolator::svtkPointInterpolator()
{
  this->SetNumberOfInputPorts(2);

  this->Locator = svtkStaticPointLocator::New();

  this->Kernel = svtkLinearKernel::New();

  this->NullPointsStrategy = svtkPointInterpolator::NULL_VALUE;
  this->NullValue = 0.0;

  this->ValidPointsMask = nullptr;
  this->ValidPointsMaskArrayName = "svtkValidPointMask";

  this->PromoteOutputArrays = true;

  this->PassPointArrays = true;
  this->PassCellArrays = true;
  this->PassFieldArrays = true;
}

//----------------------------------------------------------------------------
svtkPointInterpolator::~svtkPointInterpolator()
{
  this->SetLocator(nullptr);
  this->SetKernel(nullptr);
}

//----------------------------------------------------------------------------
void svtkPointInterpolator::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void svtkPointInterpolator::SetSourceData(svtkDataObject* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPointInterpolator::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
void svtkPointInterpolator::ExtractImageDescription(
  svtkImageData* input, int dims[3], double origin[3], double spacing[3])
{
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);
}

//----------------------------------------------------------------------------
// The driver of the algorithm
void svtkPointInterpolator::Probe(svtkDataSet* input, svtkDataSet* source, svtkDataSet* output)
{
  // Make sure there is a kernel
  if (!this->Kernel)
  {
    svtkErrorMacro(<< "Interpolation kernel required\n");
    return;
  }

  // Start by building the locator
  if (!this->Locator)
  {
    svtkErrorMacro(<< "Point locator required\n");
    return;
  }
  this->Locator->SetDataSet(source);
  this->Locator->BuildLocator();

  // Set up the interpolation process
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPointData* inPD = source->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD, numPts);

  // Masking if requested
  char* mask = nullptr;
  if (this->NullPointsStrategy == svtkPointInterpolator::MASK_POINTS)
  {
    this->ValidPointsMask = svtkCharArray::New();
    this->ValidPointsMask->SetNumberOfTuples(numPts);
    mask = this->ValidPointsMask->GetPointer(0);
    std::fill_n(mask, numPts, 1);
  }

  // Now loop over input points, finding closest points and invoking kernel.
  if (this->Kernel->GetRequiresInitialization())
  {
    this->Kernel->Initialize(this->Locator, source, inPD);
  }

  // If the input is image data then there is a faster path
  svtkImageData* imgInput = svtkImageData::SafeDownCast(input);
  if (imgInput)
  {
    int dims[3];
    double origin[3], spacing[3];
    this->ExtractImageDescription(imgInput, dims, origin, spacing);
    ImageProbePoints imageProbe(this, imgInput, dims, origin, spacing, inPD, outPD, mask);
    svtkSMPTools::For(0, dims[2], imageProbe); // over slices
  }
  else
  {
    ProbePoints probe(this, input, inPD, outPD, mask);
    svtkSMPTools::For(0, numPts, probe);
  }

  // Clean up
  if (mask)
  {
    this->ValidPointsMask->SetName(this->ValidPointsMaskArrayName);
    outPD->AddArray(this->ValidPointsMask);
    this->ValidPointsMask->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkPointInterpolator::PassAttributeData(
  svtkDataSet* input, svtkDataObject* svtkNotUsed(source), svtkDataSet* output)
{
  // copy point data arrays
  if (this->PassPointArrays)
  {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i = 0; i < numPtArrays; ++i)
    {
      output->GetPointData()->AddArray(input->GetPointData()->GetArray(i));
    }
  }

  // copy cell data arrays
  if (this->PassCellArrays)
  {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i = 0; i < numCellArrays; ++i)
    {
      output->GetCellData()->AddArray(input->GetCellData()->GetArray(i));
    }
  }

  if (this->PassFieldArrays)
  {
    // nothing to do, svtkDemandDrivenPipeline takes care of that.
  }
  else
  {
    output->GetFieldData()->Initialize();
  }
}

//----------------------------------------------------------------------------
int svtkPointInterpolator::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* source = svtkDataSet::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!source || source->GetNumberOfPoints() < 1)
  {
    svtkWarningMacro(<< "No source points to interpolate from");
    return 1;
  }

  // Copy the input geometry and topology to the output
  output->CopyStructure(input);

  // Perform the probing
  this->Probe(input, source, output);

  // Pass attribute data as requested
  this->PassAttributeData(input, source, output);

  return 1;
}

//----------------------------------------------------------------------------
int svtkPointInterpolator::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (svtkImageData::HasScalarType(sourceInfo))
  {
    svtkImageData::SetScalarType(svtkImageData::GetScalarType(sourceInfo), outInfo);
  }
  if (svtkImageData::HasNumberOfScalarComponents(sourceInfo))
  {
    svtkImageData::SetNumberOfScalarComponents(
      svtkImageData::GetNumberOfScalarComponents(sourceInfo), outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPointInterpolator::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    sourceInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  return 1;
}

//--------------------------------------------------------------------------
svtkMTimeType svtkPointInterpolator::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType mTime2;
  if (this->Locator != nullptr)
  {
    mTime2 = this->Locator->GetMTime();
    mTime = (mTime2 > mTime ? mTime2 : mTime);
  }
  if (this->Kernel != nullptr)
  {
    mTime2 = this->Kernel->GetMTime();
    mTime = (mTime2 > mTime ? mTime2 : mTime);
  }
  return mTime;
}

//----------------------------------------------------------------------------
void svtkPointInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkDataObject* source = this->GetSource();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Kernel: " << this->Kernel << "\n";

  os << indent << "Null Points Strategy: " << this->NullPointsStrategy << endl;
  os << indent << "Null Value: " << this->NullValue << "\n";
  os << indent << "Valid Points Mask Array Name: "
     << (this->ValidPointsMaskArrayName ? this->ValidPointsMaskArrayName : "(none)") << "\n";

  os << indent << "Number of Excluded Arrays:" << this->GetNumberOfExcludedArrays() << endl;
  svtkIndent nextIndent = indent.GetNextIndent();
  for (int i = 0; i < this->GetNumberOfExcludedArrays(); ++i)
  {
    os << nextIndent << "Excluded Array: " << this->ExcludedArrays[i] << endl;
  }

  os << indent << "Promote Output Arrays: " << (this->PromoteOutputArrays ? "On" : " Off") << "\n";

  os << indent << "Pass Point Arrays: " << (this->PassPointArrays ? "On" : " Off") << "\n";
  os << indent << "Pass Cell Arrays: " << (this->PassCellArrays ? "On" : " Off") << "\n";
  os << indent << "Pass Field Arrays: " << (this->PassFieldArrays ? "On" : " Off") << "\n";
}
