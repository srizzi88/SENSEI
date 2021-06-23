/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockFromTimeSeriesFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMultiBlockFromTimeSeriesFilter.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkMultiBlockFromTimeSeriesFilter);

svtkMultiBlockFromTimeSeriesFilter::svtkMultiBlockFromTimeSeriesFilter()
{
  this->UpdateTimeIndex = 0;
}

svtkMultiBlockFromTimeSeriesFilter::~svtkMultiBlockFromTimeSeriesFilter() = default;

int svtkMultiBlockFromTimeSeriesFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

int svtkMultiBlockFromTimeSeriesFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inInfo, svtkInformationVector* outInfoVec)
{
  this->UpdateTimeIndex = 0;
  svtkInformation* info = inInfo[0]->GetInformationObject(0);
  int len = info->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* timeSteps = info->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  this->TimeSteps.resize(len);
  std::copy(timeSteps, timeSteps + len, this->TimeSteps.begin());
  this->TempDataset = svtkSmartPointer<svtkMultiBlockDataSet>::New();
  this->TempDataset->SetNumberOfBlocks(len);

  svtkInformation* outInfo = outInfoVec->GetInformationObject(0);
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

int svtkMultiBlockFromTimeSeriesFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inInfo, svtkInformationVector* svtkNotUsed(outInfo))
{
  if (this->TimeSteps.size() > static_cast<size_t>(this->UpdateTimeIndex))
  {
    svtkInformation* info = inInfo[0]->GetInformationObject(0);
    info->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->TimeSteps[this->UpdateTimeIndex]);
  }
  return 1;
}

int svtkMultiBlockFromTimeSeriesFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo)
{
  svtkInformation* info = inInfo[0]->GetInformationObject(0);
  svtkDataObject* data = svtkDataObject::GetData(info);
  svtkSmartPointer<svtkDataObject> clone = svtkSmartPointer<svtkDataObject>::Take(data->NewInstance());
  clone->ShallowCopy(data);
  this->TempDataset->SetBlock(this->UpdateTimeIndex, clone);
  if (this->UpdateTimeIndex < static_cast<svtkTypeInt64>(this->TimeSteps.size()) - 1)
  {
    this->UpdateTimeIndex++;
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outInfo);
    output->ShallowCopy(this->TempDataset);
    for (unsigned i = 0; i < this->TempDataset->GetNumberOfBlocks(); ++i)
    {
      this->TempDataset->SetBlock(i, nullptr);
    }
    request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  }
  return 1;
}

void svtkMultiBlockFromTimeSeriesFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
