// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSubCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkSubCommunicator.h"

#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkProcessGroup.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkSubCommunicator);

//-----------------------------------------------------------------------------
svtkSubCommunicator::svtkSubCommunicator()
{
  this->Group = nullptr;
}

svtkSubCommunicator::~svtkSubCommunicator()
{
  this->SetGroup(nullptr);
}

void svtkSubCommunicator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Group: " << this->Group << endl;
}

//-----------------------------------------------------------------------------
int svtkSubCommunicator::SendVoidArray(
  const void* data, svtkIdType length, int type, int remoteHandle, int tag)
{
  int realHandle = this->Group->GetProcessId(remoteHandle);
  return this->Group->GetCommunicator()->SendVoidArray(data, length, type, realHandle, tag);
}

//-----------------------------------------------------------------------------
int svtkSubCommunicator::ReceiveVoidArray(
  void* data, svtkIdType length, int type, int remoteHandle, int tag)
{
  int realHandle;
  if (remoteHandle == svtkMultiProcessController::ANY_SOURCE)
  {
    realHandle = svtkMultiProcessController::ANY_SOURCE;
  }
  else
  {
    realHandle = this->Group->GetProcessId(remoteHandle);
  }
  return this->Group->GetCommunicator()->ReceiveVoidArray(data, length, type, realHandle, tag);
}

//-----------------------------------------------------------------------------
void svtkSubCommunicator::SetGroup(svtkProcessGroup* group)
{
  svtkSetObjectBodyMacro(Group, svtkProcessGroup, group);

  if (this->Group)
  {
    this->LocalProcessId = this->Group->GetLocalProcessId();
    if (this->MaximumNumberOfProcesses != this->Group->GetNumberOfProcessIds())
    {
      this->NumberOfProcesses = this->MaximumNumberOfProcesses =
        this->Group->GetNumberOfProcessIds();
    }
  }
  else
  {
    this->LocalProcessId = -1;
    this->NumberOfProcesses = 0;
    this->MaximumNumberOfProcesses = 0;
  }
}
