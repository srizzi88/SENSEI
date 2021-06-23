/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkReaderExecutive.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkReaderExecutive.h"

#include "svtkAlgorithm.h"
#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkReaderAlgorithm.h"

svtkStandardNewMacro(svtkReaderExecutive);

//----------------------------------------------------------------------------
svtkReaderExecutive::svtkReaderExecutive() {}

//----------------------------------------------------------------------------
svtkReaderExecutive::~svtkReaderExecutive() {}

//----------------------------------------------------------------------------
void svtkReaderExecutive::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkReaderExecutive::CallAlgorithm(svtkInformation* request, int direction,
  svtkInformationVector** inInfo, svtkInformationVector* outInfo)
{
  // Copy default information in the direction of information flow.
  this->CopyDefaultInformation(request, direction, inInfo, outInfo);

  // Invoke the request on the algorithm.
  this->InAlgorithm = 1;
  int result = 1; // this->Algorithm->ProcessRequest(request, inInfo, outInfo);
  svtkReaderAlgorithm* reader = svtkReaderAlgorithm::SafeDownCast(this->Algorithm);
  if (!reader)
  {
    return 0;
  }

  using svtkSDDP = svtkStreamingDemandDrivenPipeline;
  svtkInformation* reqs = outInfo->GetInformationObject(0);
  int hasTime = reqs->Has(svtkSDDP::UPDATE_TIME_STEP());
  double* steps = reqs->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int timeIndex = 0;
  if (hasTime && steps)
  {
    double requestedTimeStep = reqs->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    int length = reqs->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());

    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < length - 1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    timeIndex = cnt;
  }

  if (request->Has(REQUEST_DATA_OBJECT()))
  {
    svtkDataObject* currentOutput = svtkDataObject::GetData(outInfo);
    svtkDataObject* output = reader->CreateOutput(currentOutput);
    if (output)
    {
      result = 1;
      if (output != currentOutput)
      {
        outInfo->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), output);
        output->Delete();
      }
    }
  }
  else if (request->Has(REQUEST_INFORMATION()))
  {
    result = reader->ReadMetaData(outInfo->GetInformationObject(0));
  }
  else if (request->Has(REQUEST_TIME_DEPENDENT_INFORMATION()))
  {
    result = reader->ReadTimeDependentMetaData(timeIndex, outInfo->GetInformationObject(0));
  }
  else if (request->Has(REQUEST_DATA()))
  {
    int piece =
      reqs->Has(svtkSDDP::UPDATE_PIECE_NUMBER()) ? reqs->Get(svtkSDDP::UPDATE_PIECE_NUMBER()) : 0;
    int npieces = reqs->Has(svtkSDDP::UPDATE_NUMBER_OF_PIECES())
      ? reqs->Get(svtkSDDP::UPDATE_NUMBER_OF_PIECES())
      : 1;
    int nghosts = reqs->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
    svtkDataObject* output = svtkDataObject::GetData(outInfo);
    result = reader->ReadMesh(piece, npieces, nghosts, timeIndex, output);
    if (result)
    {
      result = reader->ReadPoints(piece, npieces, nghosts, timeIndex, output);
    }
    if (result)
    {
      result = reader->ReadArrays(piece, npieces, nghosts, timeIndex, output);
    }
  }
  this->InAlgorithm = 0;

  // If the algorithm failed report it now.
  if (!result)
  {
    svtkErrorMacro("Algorithm " << this->Algorithm->GetClassName() << "(" << this->Algorithm
                               << ") returned failure for request: " << *request);
  }

  return result;
}
