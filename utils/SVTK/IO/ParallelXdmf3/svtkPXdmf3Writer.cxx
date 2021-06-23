
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPXdmf3Writer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPXdmf3Writer.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPXdmf3Writer);

//----------------------------------------------------------------------------
svtkPXdmf3Writer::svtkPXdmf3Writer() {}

//----------------------------------------------------------------------------
svtkPXdmf3Writer::~svtkPXdmf3Writer() {}

//----------------------------------------------------------------------------
void svtkPXdmf3Writer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkPXdmf3Writer::CheckParameters()
{
  svtkMultiProcessController* c = svtkMultiProcessController::GetGlobalController();
  int numberOfProcesses = c ? c->GetNumberOfProcesses() : 1;
  int myRank = c ? c->GetLocalProcessId() : 0;

  return this->Superclass::CheckParametersInternal(numberOfProcesses, myRank);
}

//----------------------------------------------------------------------------
int svtkPXdmf3Writer::RequestUpdateExtent(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
  if (svtkMultiProcessController* c = svtkMultiProcessController::GetGlobalController())
  {
    int numberOfProcesses = c->GetNumberOfProcesses();
    int myRank = c->GetLocalProcessId();

    svtkInformation* info = inputVector[0]->GetInformationObject(0);
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), myRank);
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numberOfProcesses);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPXdmf3Writer::GlobalContinueExecuting(int localContinue)
{
  svtkMultiProcessController* c = svtkMultiProcessController::GetGlobalController();
  int globalContinue = localContinue;
  if (c)
  {
    c->AllReduce(&localContinue, &globalContinue, 1, svtkCommunicator::MIN_OP);
  }
  return globalContinue;
}
