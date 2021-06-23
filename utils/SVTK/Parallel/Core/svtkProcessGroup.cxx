// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProcessGroup.cxx

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

#include "svtkProcessGroup.h"

#include "svtkCommunicator.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"

#include <algorithm>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkProcessGroup);

//-----------------------------------------------------------------------------
svtkProcessGroup::svtkProcessGroup()
{
  this->Communicator = nullptr;

  this->ProcessIds = nullptr;
  this->NumberOfProcessIds = 0;
}

svtkProcessGroup::~svtkProcessGroup()
{
  this->SetCommunicator(nullptr);
}

void svtkProcessGroup::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Communicator: " << this->Communicator << endl;
  os << indent << "ProcessIds:";
  for (int i = 0; i < this->NumberOfProcessIds; i++)
  {
    os << " " << this->ProcessIds[i];
  }
  os << endl;
}

//-----------------------------------------------------------------------------
void svtkProcessGroup::Initialize(svtkMultiProcessController* controller)
{
  this->Initialize(controller->GetCommunicator());
}

void svtkProcessGroup::Initialize(svtkCommunicator* communicator)
{
  this->SetCommunicator(communicator);

  this->NumberOfProcessIds = this->Communicator->GetNumberOfProcesses();
  for (int i = 0; i < this->NumberOfProcessIds; i++)
  {
    this->ProcessIds[i] = i;
  }
}

//-----------------------------------------------------------------------------
void svtkProcessGroup::SetCommunicator(svtkCommunicator* communicator)
{
  // Adjust ProcessIds array.
  int* newProcessIds = nullptr;
  int newNumberOfProcessIds = 0;
  if (communicator)
  {
    newProcessIds = new int[communicator->GetNumberOfProcesses()];
    newNumberOfProcessIds = communicator->GetNumberOfProcesses();
    if (newNumberOfProcessIds > this->NumberOfProcessIds)
    {
      newNumberOfProcessIds = this->NumberOfProcessIds;
    }
  }
  if (this->ProcessIds)
  {
    std::copy(newProcessIds, newProcessIds + newNumberOfProcessIds, this->ProcessIds);
  }
  if (this->Communicator)
    delete[] this->ProcessIds;
  this->ProcessIds = newProcessIds;
  this->NumberOfProcessIds = newNumberOfProcessIds;

  svtkSetObjectBodyMacro(Communicator, svtkCommunicator, communicator);
}

//-----------------------------------------------------------------------------
int svtkProcessGroup::GetLocalProcessId()
{
  if (this->Communicator)
  {
    return this->FindProcessId(this->Communicator->GetLocalProcessId());
  }
  else
  {
    return -1;
  }
}

//-----------------------------------------------------------------------------
int svtkProcessGroup::FindProcessId(int processId)
{
  for (int i = 0; i < this->NumberOfProcessIds; i++)
  {
    if (this->ProcessIds[i] == processId)
      return i;
  }
  return -1;
}

//-----------------------------------------------------------------------------
int svtkProcessGroup::AddProcessId(int processId)
{
  int loc = this->FindProcessId(processId);
  if (loc < 0)
  {
    loc = this->NumberOfProcessIds++;
    this->ProcessIds[loc] = processId;
    this->Modified();
  }
  return loc;
}

//-----------------------------------------------------------------------------
int svtkProcessGroup::RemoveProcessId(int processId)
{
  int loc = this->FindProcessId(processId);
  if (loc < 0)
    return 0;

  this->NumberOfProcessIds--;
  for (int i = loc; i < this->NumberOfProcessIds; i++)
  {
    this->ProcessIds[i] = this->ProcessIds[i + 1];
  }
  this->Modified();
  return 1;
}

//-----------------------------------------------------------------------------
void svtkProcessGroup::RemoveAllProcessIds()
{
  if (this->NumberOfProcessIds > 0)
  {
    this->NumberOfProcessIds = 0;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void svtkProcessGroup::Copy(svtkProcessGroup* group)
{
  this->SetCommunicator(group->Communicator);
  this->NumberOfProcessIds = group->NumberOfProcessIds;
  for (int i = 0; i < this->NumberOfProcessIds; i++)
  {
    this->ProcessIds[i] = group->ProcessIds[i];
  }
}
