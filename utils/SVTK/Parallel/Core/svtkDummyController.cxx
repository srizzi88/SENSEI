/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDummyController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDummyController.h"
#include "svtkDummyCommunicator.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkDummyController);

svtkCxxSetObjectMacro(svtkDummyController, Communicator, svtkCommunicator);
svtkCxxSetObjectMacro(svtkDummyController, RMICommunicator, svtkCommunicator);

//----------------------------------------------------------------------------
svtkDummyController::svtkDummyController()
{
  this->Communicator = svtkDummyCommunicator::New();
  this->RMICommunicator = svtkDummyCommunicator::New();
}

svtkDummyController::~svtkDummyController()
{
  this->SetCommunicator(nullptr);
  this->SetRMICommunicator(nullptr);
}

void svtkDummyController::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Communicator: " << this->Communicator << endl;
  os << indent << "RMICommunicator: " << this->RMICommunicator << endl;
}

//-----------------------------------------------------------------------------
void svtkDummyController::SingleMethodExecute()
{
  if (this->SingleMethod)
  {
    // Should we set the global controller here?  I'm going to say no since
    // we are not really a parallel job or at the very least not the global
    // controller.

    (this->SingleMethod)(this, this->SingleData);
  }
  else
  {
    svtkWarningMacro("SingleMethod not set.");
  }
}

//-----------------------------------------------------------------------------
void svtkDummyController::MultipleMethodExecute()
{
  int i = this->GetLocalProcessId();

  svtkProcessFunctionType multipleMethod;
  void* multipleData;
  this->GetMultipleMethod(i, multipleMethod, multipleData);
  if (multipleMethod)
  {
    // Should we set the global controller here?  I'm going to say no since
    // we are not really a parallel job or at the very least not the global
    // controller.

    (multipleMethod)(this, multipleData);
  }
  else
  {
    svtkWarningMacro("MultipleMethod " << i << " not set.");
  }
}
