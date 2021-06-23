/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSocketController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSocketController.h"

#include "svtkObjectFactory.h"
#include "svtkProcessGroup.h"
#include "svtkSocketCommunicator.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
#define SVTK_WINDOWS_FULL
#include "svtkWindows.h"
#define WSA_VERSION MAKEWORD(1, 1)
#endif

int svtkSocketController::Initialized = 0;

svtkStandardNewMacro(svtkSocketController);

//----------------------------------------------------------------------------
svtkSocketController::svtkSocketController()
{
  this->Communicator = svtkSocketCommunicator::New();
  this->RMICommunicator = this->Communicator;
}

//----------------------------------------------------------------------------
svtkSocketController::~svtkSocketController()
{
  this->Communicator->Delete();
  this->Communicator = this->RMICommunicator = nullptr;
}

//----------------------------------------------------------------------------
void svtkSocketController::Initialize(int*, char***)
{
  if (svtkSocketController::Initialized)
  {
    svtkWarningMacro("Already initialized.");
    return;
  }

#if defined(_WIN32) && !defined(__CYGWIN__)
  WSAData wsaData;
  if (WSAStartup(WSA_VERSION, &wsaData))
  {
    svtkErrorMacro("Could not initialize sockets !");
  }
#endif
  svtkSocketController::Initialized = 1;
}

//----------------------------------------------------------------------------
void svtkSocketController::SetCommunicator(svtkSocketCommunicator* comm)
{
  if (comm == this->Communicator)
  {
    return;
  }
  if (this->Communicator)
  {
    this->Communicator->UnRegister(this);
  }
  this->Communicator = comm;
  this->RMICommunicator = comm;
  if (comm)
  {
    comm->Register(this);
  }
}

//----------------------------------------------------------------------------
void svtkSocketController::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkSocketController::WaitForConnection(int port)
{
  return svtkSocketCommunicator::SafeDownCast(this->Communicator)->WaitForConnection(port);
}

//----------------------------------------------------------------------------
void svtkSocketController::CloseConnection()
{
  svtkSocketCommunicator::SafeDownCast(this->Communicator)->CloseConnection();
}

//----------------------------------------------------------------------------
int svtkSocketController::ConnectTo(const char* hostName, int port)
{
  return svtkSocketCommunicator::SafeDownCast(this->Communicator)->ConnectTo(hostName, port);
}

//----------------------------------------------------------------------------
int svtkSocketController::GetSwapBytesInReceivedData()
{
  return svtkSocketCommunicator::SafeDownCast(this->Communicator)->GetSwapBytesInReceivedData();
}

//-----------------------------------------------------------------------------
svtkMultiProcessController* svtkSocketController::CreateCompliantController()
{
  svtkProcessGroup* group = svtkProcessGroup::New();
  group->Initialize(this->Communicator);
  group->RemoveAllProcessIds();

  // This hack creates sub controllers with differing orders of the processes
  // that will map the ids to be unique on each process.
  if (svtkSocketCommunicator::SafeDownCast(this->Communicator)->GetIsServer())
  {
    group->AddProcessId(1);
    group->AddProcessId(0);
  }
  else
  {
    group->AddProcessId(0);
    group->AddProcessId(1);
  }

  svtkMultiProcessController* compliantController = this->CreateSubController(group);

  group->Delete();

  return compliantController;
}
