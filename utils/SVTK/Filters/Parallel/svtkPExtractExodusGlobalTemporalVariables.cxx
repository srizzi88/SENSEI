/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExtractExodusGlobalTemporalVariables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPExtractExodusGlobalTemporalVariables.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPExtractExodusGlobalTemporalVariables);
svtkCxxSetObjectMacro(
  svtkPExtractExodusGlobalTemporalVariables, Controller, svtkMultiProcessController);
//----------------------------------------------------------------------------
svtkPExtractExodusGlobalTemporalVariables::svtkPExtractExodusGlobalTemporalVariables()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkPExtractExodusGlobalTemporalVariables::~svtkPExtractExodusGlobalTemporalVariables()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkPExtractExodusGlobalTemporalVariables::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  const auto retval = this->Superclass::RequestData(request, inputVector, outputVector);
  if (this->Controller == nullptr || this->Controller->GetNumberOfProcesses() == 1)
  {
    return retval;
  }

  const bool isRoot = (this->Controller->GetLocalProcessId() == 0);
  if (isRoot)
  {
    bool continue_executing = false;
    size_t offset = 0;
    this->GetContinuationState(continue_executing, offset);

    int continue_executing_i = continue_executing ? 1 : 0;
    this->Controller->Broadcast(&continue_executing_i, 1, 0);
    if (continue_executing)
    {
      int offset_i = static_cast<int>(offset);
      this->Controller->Broadcast(&offset_i, 1, 0);
    }
  }
  else
  {
    int continue_executing_i = 0;
    this->Controller->Broadcast(&continue_executing_i, 1, 0);
    const bool continue_executing = (continue_executing_i != 0);

    if (continue_executing)
    {
      int offset_i = 0;
      this->Controller->Broadcast(&offset_i, 1, 0);
      const size_t offset = static_cast<size_t>(offset_i);
      this->SetContinuationState(continue_executing, offset);
      request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
    else
    {
      this->SetContinuationState(false, 0);
    }
  }
  return retval;
}

//----------------------------------------------------------------------------
void svtkPExtractExodusGlobalTemporalVariables::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
