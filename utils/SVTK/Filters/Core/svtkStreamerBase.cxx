// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStreamerBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStreamerBase.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkStreamingDemandDrivenPipeline.h"

//=============================================================================
svtkStreamerBase::svtkStreamerBase()
{
  this->NumberOfPasses = 1;
  this->CurrentIndex = 0;
}

//-----------------------------------------------------------------------------
svtkStreamerBase::~svtkStreamerBase() = default;

//-----------------------------------------------------------------------------
void svtkStreamerBase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStreamerBase::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int svtkStreamerBase::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!this->ExecutePass(inputVector, outputVector))
  {
    request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    return 0;
  }

  this->CurrentIndex++;

  if (this->CurrentIndex < this->NumberOfPasses)
  {
    // There is still more to do.
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    // We are done.  Finish up.
    request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    if (!this->PostExecute(inputVector, outputVector))
    {
      return 0;
    }
    this->CurrentIndex = 0;
  }

  return 1;
}
