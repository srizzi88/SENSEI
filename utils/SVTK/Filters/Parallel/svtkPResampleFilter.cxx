/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPResampleFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPResampleFilter.h"

#include "svtkCellData.h"
#include "svtkCommunicator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataObject.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPProbeFilter.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPResampleFilter);

svtkCxxSetObjectMacro(svtkPResampleFilter, Controller, svtkMultiProcessController);

//----------------------------------------------------------------------------
svtkPResampleFilter::svtkPResampleFilter()
  : UseInputBounds(0)
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
  this->UseInputBoundsOn();
  this->CustomSamplingBounds[0] = this->CustomSamplingBounds[2] = this->CustomSamplingBounds[4] = 0;
  this->CustomSamplingBounds[1] = this->CustomSamplingBounds[3] = this->CustomSamplingBounds[5] = 1;
  this->SamplingDimension[0] = this->SamplingDimension[1] = this->SamplingDimension[2] = 10;

  svtkMath::UninitializeBounds(this->Bounds);
}

//----------------------------------------------------------------------------
svtkPResampleFilter::~svtkPResampleFilter()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
double* svtkPResampleFilter::CalculateBounds(svtkDataSet* input)
{
  double localBounds[6];
  input->GetBounds(localBounds);

  if (!this->Controller)
  {
    memcpy(this->Bounds, localBounds, 6 * sizeof(double));
  }
  else
  {
    double localBoundsMin[3], globalBoundsMin[3];
    double localBoundsMax[3], globalBoundsMax[3];
    for (int i = 0; i < 3; i++)
    {
      // Change uninitialized bounds to something that will work
      // with collective MPI calls.
      if (localBounds[2 * i] > localBounds[2 * i + 1])
      {
        localBounds[2 * i] = SVTK_DOUBLE_MAX;
        localBounds[2 * i + 1] = -SVTK_DOUBLE_MAX;
      }
      localBoundsMin[i] = localBounds[2 * i];
      localBoundsMax[i] = localBounds[2 * i + 1];
    }
    this->Controller->AllReduce(localBoundsMin, globalBoundsMin, 3, svtkCommunicator::MIN_OP);
    this->Controller->AllReduce(localBoundsMax, globalBoundsMax, 3, svtkCommunicator::MAX_OP);
    for (int i = 0; i < 3; i++)
    {
      if (globalBoundsMin[i] <= globalBoundsMax[i])
      {
        this->Bounds[2 * i] = globalBoundsMin[i];
        this->Bounds[2 * i + 1] = globalBoundsMax[i];
      }
      else
      {
        this->Bounds[2 * i] = 0;
        this->Bounds[2 * i + 1] = 0;
      }
    }
  }

  cout << "Bounds: " << localBounds[0] << " " << localBounds[1] << " " << localBounds[2] << " "
       << localBounds[3] << " " << localBounds[4] << " " << localBounds[5] << " " << endl;
  return this->Bounds;
}

//----------------------------------------------------------------------------
int svtkPResampleFilter::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  int wholeExtent[6] = { 0, this->SamplingDimension[0] - 1, 0, this->SamplingDimension[1] - 1, 0,
    this->SamplingDimension[2] - 1 };

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int svtkPResampleFilter::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  // This needs to be here because input and output extents are not
  // necessarily related. The output extent is controlled by the
  // resampled dataset whereas the input extent is controlled by
  // input data.
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  return 1;
}

//----------------------------------------------------------------------------
int svtkPResampleFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Create Image Data for resampling
  svtkNew<svtkImageData> source;
  double* boundsToSample =
    (this->UseInputBounds == 1) ? this->CalculateBounds(input) : this->CustomSamplingBounds;
  source->SetOrigin(boundsToSample[0], boundsToSample[2], boundsToSample[4]);
  source->SetDimensions(this->SamplingDimension);
  source->SetSpacing(
    (boundsToSample[1] - boundsToSample[0]) / static_cast<double>(this->SamplingDimension[0] - 1),
    (boundsToSample[3] - boundsToSample[2]) / static_cast<double>(this->SamplingDimension[1] - 1),
    (boundsToSample[5] - boundsToSample[4]) / static_cast<double>(this->SamplingDimension[2] - 1));

  // Probe data
  svtkNew<svtkPProbeFilter> probeFilter;
  probeFilter->SetController(this->Controller);
  probeFilter->SetSourceData(input);
  probeFilter->SetInputData(source);
  probeFilter->Update();
  output->ShallowCopy(probeFilter->GetOutput());

  return 1;
}

//----------------------------------------------------------------------------
int svtkPResampleFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPResampleFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller " << this->Controller << endl;
  os << indent << "UseInputBounds " << this->UseInputBounds << endl;
  if (this->UseInputBounds == 0)
  {
    os << indent << "CustomSamplingBounds [" << this->CustomSamplingBounds[0] << ", "
       << this->CustomSamplingBounds[1] << ", " << this->CustomSamplingBounds[2] << ", "
       << this->CustomSamplingBounds[3] << ", " << this->CustomSamplingBounds[4] << ", "
       << this->CustomSamplingBounds[5] << "]" << endl;
  }
  os << indent << "SamplingDimension " << this->SamplingDimension[0] << " x "
     << this->SamplingDimension[1] << " x " << this->SamplingDimension[2] << endl;
}
