/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSPHInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSPHInterpolator.h"

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
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkSPHQuinticKernel.h"
#include "svtkStaticPointLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkVoronoiKernel.h"

svtkStandardNewMacro(svtkSPHInterpolator);
svtkCxxSetObjectMacro(svtkSPHInterpolator, Locator, svtkAbstractPointLocator);
svtkCxxSetObjectMacro(svtkSPHInterpolator, Kernel, svtkSPHKernel);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{
// The threaded core of the algorithm
struct ProbePoints
{
  svtkSPHInterpolator* SPHInterpolator;
  svtkDataSet* Input;
  svtkSPHKernel* Kernel;
  svtkAbstractPointLocator* Locator;
  svtkPointData* InPD;
  svtkPointData* OutPD;
  ArrayList Arrays;
  ArrayList DerivArrays;
  svtkTypeBool ComputeDerivArrays;
  char* Valid;
  int Strategy;
  float* Shepard;
  svtkTypeBool Promote;

  // Don't want to allocate these working arrays on every thread invocation,
  // so make them thread local.
  svtkSMPThreadLocalObject<svtkIdList> PIds;
  svtkSMPThreadLocalObject<svtkDoubleArray> Weights;
  svtkSMPThreadLocalObject<svtkDoubleArray> DerivWeights;

  ProbePoints(svtkSPHInterpolator* sphInt, svtkDataSet* input, svtkPointData* inPD,
    svtkPointData* outPD, char* valid, float* shepCoef)
    : SPHInterpolator(sphInt)
    , Input(input)
    , InPD(inPD)
    , OutPD(outPD)
    , Valid(valid)
    , Shepard(shepCoef)
  {
    // Gather information from the interpolator
    this->Kernel = sphInt->GetKernel();
    this->Locator = sphInt->GetLocator();
    this->Strategy = sphInt->GetNullPointsStrategy();
    double nullV = sphInt->GetNullValue();
    this->Promote = sphInt->GetPromoteOutputArrays();

    // Manage arrays for interpolation
    for (int i = 0; i < sphInt->GetNumberOfExcludedArrays(); ++i)
    {
      const char* arrayName = sphInt->GetExcludedArray(i);
      svtkDataArray* array = this->InPD->GetArray(arrayName);
      if (array != nullptr)
      {
        outPD->RemoveArray(array->GetName());
        this->Arrays.ExcludeArray(array);
        this->DerivArrays.ExcludeArray(array);
      }
    }
    this->Arrays.AddArrays(input->GetNumberOfPoints(), inPD, outPD, nullV, this->Promote);

    // Sometimes derivative arrays are requested
    for (int i = 0; i < sphInt->GetNumberOfDerivativeArrays(); ++i)
    {
      const char* arrayName = sphInt->GetDerivativeArray(i);
      svtkDataArray* array = this->InPD->GetArray(arrayName);
      if (array != nullptr)
      {
        svtkStdString outName = arrayName;
        outName += "_deriv";
        if (svtkDataArray* outArray = this->DerivArrays.AddArrayPair(
              array->GetNumberOfTuples(), array, outName, nullV, this->Promote))
        {
          outPD->AddArray(outArray);
        }
      }
    }
    this->ComputeDerivArrays = (!this->DerivArrays.Arrays.empty() ? true : false);
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    svtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
    svtkDoubleArray*& weights = this->Weights.Local();
    weights->Allocate(128);
    svtkDoubleArray*& gradWeights = this->DerivWeights.Local();
    gradWeights->Allocate(128);
  }

  // Threaded interpolation method
  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double x[3];
    svtkIdList*& pIds = this->PIds.Local();
    svtkIdType numWeights;
    svtkDoubleArray*& weights = this->Weights.Local();
    svtkDoubleArray*& gradWeights = this->DerivWeights.Local();

    for (; ptId < endPtId; ++ptId)
    {
      this->Input->GetPoint(ptId, x);

      if ((numWeights = this->Kernel->ComputeBasis(x, pIds, ptId)) > 0)
      {
        if (!this->ComputeDerivArrays)
        {
          this->Kernel->ComputeWeights(x, pIds, weights);
        }
        else
        {
          this->Kernel->ComputeDerivWeights(x, pIds, weights, gradWeights);
          this->DerivArrays.Interpolate(
            numWeights, pIds->GetPointer(0), gradWeights->GetPointer(0), ptId);
        }
        this->Arrays.Interpolate(numWeights, pIds->GetPointer(0), weights->GetPointer(0), ptId);
      }
      else // no neighborhood points
      {
        this->Arrays.AssignNullValue(ptId);
        if (this->Strategy == svtkSPHInterpolator::MASK_POINTS)
        {
          this->Valid[ptId] = 0;
        }
      } // null point

      // Shepard's coefficient if requested
      if (this->Shepard)
      {
        double sum = 0.0, *w = weights->GetPointer(0);
        for (int i = 0; i < numWeights; ++i)
        {
          sum += w[i];
        }
        this->Shepard[ptId] = sum;
      }
    } // for all dataset points
  }

  void Reduce() {}

}; // ProbePoints

// Used when normalizing arrays by the Shepard coefficient
template <typename T>
struct NormalizeArray
{
  T* Array;
  int NumComp;
  float* ShepardSumArray;

  NormalizeArray(T* array, int numComp, float* ssa)
    : Array(array)
    , NumComp(numComp)
    , ShepardSumArray(ssa)
  {
  }

  void Initialize() {}

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    int i, numComp = this->NumComp;
    T* array = this->Array + ptId * numComp;
    T val;
    const float* ssa = this->ShepardSumArray + ptId;

    // If Shepard coefficient ==0.0 then set values to zero
    for (; ptId < endPtId; ++ptId, ++ssa)
    {
      if (*ssa == 0.0)
      {
        for (i = 0; i < numComp; ++i)
        {
          *array++ = static_cast<T>(0.0);
        }
      }
      else
      {
        for (i = 0; i < numComp; ++i)
        {
          val = static_cast<T>(static_cast<double>(*array) / static_cast<double>(*ssa));
          *array++ = val;
        }
      }
    } // for points in this range
  }

  void Reduce() {}

  static void Execute(svtkIdType numPts, T* ptr, int numComp, float* ssa)
  {
    NormalizeArray normalize(ptr, numComp, ssa);
    svtkSMPTools::For(0, numPts, normalize);
  }
}; // NormalizeArray

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkSPHInterpolator::svtkSPHInterpolator()
{
  this->SetNumberOfInputPorts(2);

  this->Locator = svtkStaticPointLocator::New();

  this->Kernel = svtkSPHQuinticKernel::New();

  this->CutoffArrayName = "";

  this->DensityArrayName = "Rho";
  this->MassArrayName = "";

  this->NullPointsStrategy = svtkSPHInterpolator::NULL_VALUE;
  this->NullValue = 0.0;

  this->ValidPointsMask = nullptr;
  this->ValidPointsMaskArrayName = "svtkValidPointMask";

  this->ComputeShepardSum = true;
  this->ShepardSumArrayName = "Shepard Summation";

  this->PromoteOutputArrays = true;

  this->PassPointArrays = true;
  this->PassCellArrays = true;
  this->PassFieldArrays = true;

  this->ShepardNormalization = false;
}

//----------------------------------------------------------------------------
svtkSPHInterpolator::~svtkSPHInterpolator()
{
  this->SetLocator(nullptr);
  this->SetKernel(nullptr);
}

//----------------------------------------------------------------------------
void svtkSPHInterpolator::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void svtkSPHInterpolator::SetSourceData(svtkDataObject* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkSPHInterpolator::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
// The driver of the algorithm
void svtkSPHInterpolator::Probe(svtkDataSet* input, svtkDataSet* source, svtkDataSet* output)
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
  svtkPointData* sourcePD = source->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  outPD->InterpolateAllocate(sourcePD, numPts);

  // Masking if requested
  char* mask = nullptr;
  if (this->NullPointsStrategy == svtkSPHInterpolator::MASK_POINTS)
  {
    this->ValidPointsMask = svtkCharArray::New();
    this->ValidPointsMask->SetNumberOfTuples(numPts);
    mask = this->ValidPointsMask->GetPointer(0);
    std::fill_n(mask, numPts, 1);
  }

  // Shepard summation if requested
  svtkTypeBool computeShepardSum = this->ComputeShepardSum || this->ShepardNormalization;
  float* shepardArray = nullptr;
  if (computeShepardSum)
  {
    this->ShepardSumArray = svtkFloatArray::New();
    this->ShepardSumArray->SetNumberOfTuples(numPts);
    shepardArray = this->ShepardSumArray->GetPointer(0);
  }

  // Initialize the SPH kernel
  if (this->Kernel->GetRequiresInitialization())
  {
    this->Kernel->SetCutoffArray(sourcePD->GetArray(this->CutoffArrayName));
    this->Kernel->SetDensityArray(sourcePD->GetArray(this->DensityArrayName));
    this->Kernel->SetMassArray(sourcePD->GetArray(this->MassArrayName));
    this->Kernel->Initialize(this->Locator, source, sourcePD);
  }

  // Now loop over input points, finding closest points and invoking kernel.
  ProbePoints probe(this, input, sourcePD, outPD, mask, shepardArray);
  svtkSMPTools::For(0, numPts, probe);

  // If Shepard normalization requested, normalize all arrays except density
  // array.
  if (this->ShepardNormalization)
  {
    svtkDataArray* da;
    for (int i = 0; i < outPD->GetNumberOfArrays(); ++i)
    {
      da = outPD->GetArray(i);
      if (da != nullptr && da != this->Kernel->GetDensityArray())
      {
        void* ptr = da->GetVoidPointer(0);
        switch (da->GetDataType())
        {
          svtkTemplateMacro(NormalizeArray<SVTK_TT>::Execute(
            numPts, (SVTK_TT*)ptr, da->GetNumberOfComponents(), shepardArray));
        }
      } // not denisty array
    }   // for all arrays
  }     // if Shepard normalization

  // Clean up
  if (this->ShepardSumArray)
  {
    this->ShepardSumArray->SetName(this->ShepardSumArrayName);
    outPD->AddArray(this->ShepardSumArray);
    this->ShepardSumArray->Delete();
    this->ShepardSumArray = nullptr;
  }

  if (mask)
  {
    this->ValidPointsMask->SetName(this->ValidPointsMaskArrayName);
    outPD->AddArray(this->ValidPointsMask);
    this->ValidPointsMask->Delete();
    this->ValidPointsMask = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkSPHInterpolator::PassAttributeData(
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
int svtkSPHInterpolator::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDebugMacro(<< "Executing SPH Interpolator");

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
int svtkSPHInterpolator::RequestInformation(svtkInformation* svtkNotUsed(request),
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
int svtkSPHInterpolator::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
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
svtkMTimeType svtkSPHInterpolator::GetMTime()
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
void svtkSPHInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkDataObject* source = this->GetSource();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Kernel: " << this->Kernel << "\n";

  os << indent << "Cutoff Array Name: " << this->CutoffArrayName << "\n";

  os << indent << "Density Array Name: " << this->DensityArrayName << "\n";
  os << indent << "Mass Array Name: " << this->MassArrayName << "\n";

  os << indent << "Null Points Strategy: " << this->NullPointsStrategy << endl;
  os << indent << "Null Value: " << this->NullValue << "\n";
  os << indent << "Valid Points Mask Array Name: "
     << (this->ValidPointsMaskArrayName ? this->ValidPointsMaskArrayName : "(none)") << "\n";

  os << indent << "Compute Shepard Sum: " << (this->ComputeShepardSum ? "On" : " Off") << "\n";
  os << indent << "Shepard Sum Array Name: "
     << (this->ShepardSumArrayName ? this->ShepardSumArrayName : "(none)") << "\n";

  os << indent << "Promote Output Arrays: " << (this->PromoteOutputArrays ? "On" : " Off") << "\n";

  os << indent << "Pass Point Arrays: " << (this->PassPointArrays ? "On" : " Off") << "\n";
  os << indent << "Pass Cell Arrays: " << (this->PassCellArrays ? "On" : " Off") << "\n";
  os << indent << "Pass Field Arrays: " << (this->PassFieldArrays ? "On" : " Off") << "\n";

  os << indent << "Shepard Normalization: " << (this->ShepardNormalization ? "On" : " Off") << "\n";
}
