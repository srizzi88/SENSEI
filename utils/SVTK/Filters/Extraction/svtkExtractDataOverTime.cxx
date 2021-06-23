/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractDataOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractDataOverTime.h"

#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkExtractDataOverTime);

//----------------------------------------------------------------------------
svtkExtractDataOverTime::svtkExtractDataOverTime()
{
  this->NumberOfTimeSteps = 0;
  this->CurrentTimeIndex = 0;
  this->PointIndex = 0;
}

//----------------------------------------------------------------------------
void svtkExtractDataOverTime::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Index: " << this->PointIndex << endl;
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
}

//----------------------------------------------------------------------------
int svtkExtractDataOverTime::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }
  // The output of this filter does not contain a specific time, rather
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkExtractDataOverTime::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  else if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    // get the requested update extent
    double* inTimes =
      inputVector[0]->GetInformationObject(0)->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (inTimes)
    {
      double timeReq = inTimes[this->CurrentTimeIndex];
      inputVector[0]->GetInformationObject(0)->Set(
        svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
    }
    return 1;
  }

  // generate the data
  else if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (this->NumberOfTimeSteps == 0)
    {
      svtkErrorMacro("No Time steps in input time data!");
      return 0;
    }

    // get the output data object
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    svtkPointSet* output = svtkPointSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

    // and input data object
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

    // is this the first request
    if (!this->CurrentTimeIndex)
    {
      // Tell the pipeline to start looping.
      request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      this->AllocateOutputData(input, output);
    }

    // extract the actual data
    output->GetPoints()->SetPoint(
      this->CurrentTimeIndex, input->GetPoints()->GetPoint(this->PointIndex));
    output->GetPointData()->CopyData(
      input->GetPointData(), this->PointIndex, this->CurrentTimeIndex);
    if (input->GetPointData()->GetArray("Time"))
    {
      output->GetPointData()
        ->GetArray("TimeData")
        ->SetTuple1(
          this->CurrentTimeIndex, input->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP()));
    }
    else
    {
      output->GetPointData()->GetArray("Time")->SetTuple1(
        this->CurrentTimeIndex, input->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP()));
    }

    // increment the time index
    this->CurrentTimeIndex++;
    if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
    }

    return 1;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkExtractDataOverTime::AllocateOutputData(svtkPointSet* input, svtkPointSet* output)
{
  // by default svtkPointSetAlgorithm::RequestDataObject already
  // created an output of the same type as the input
  if (!output)
  {
    svtkErrorMacro("Output not created as expected!");
    return 0;
  }

  // 1st the points
  svtkPoints* points = output->GetPoints();
  if (!points)
  {
    points = svtkPoints::New();
    output->SetPoints(points);
    points->Delete();
  }
  points->SetNumberOfPoints(this->NumberOfTimeSteps);

  // now the point data
  output->GetPointData()->CopyAllocate(input->GetPointData(), this->NumberOfTimeSteps);

  // and finally add an array to hold the time at each step
  svtkDoubleArray* timeArray = svtkDoubleArray::New();
  timeArray->SetNumberOfComponents(1);
  timeArray->SetNumberOfTuples(this->NumberOfTimeSteps);
  if (input->GetPointData()->GetArray("Time"))
  {
    timeArray->SetName("TimeData");
  }
  else
  {
    timeArray->SetName("Time");
  }
  output->GetPointData()->AddArray(timeArray);
  timeArray->Delete();

  return 1;
}
