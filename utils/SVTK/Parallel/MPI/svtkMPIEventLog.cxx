/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPIEventLog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMPIEventLog.h"

// clang-format off
#include "svtk_mpi.h" // required before "mpe.h" to avoid "C vs C++" conflicts
#include "mpe.h"
// clang-format on

#include "svtkMPIController.h"
#include "svtkObjectFactory.h"

int svtkMPIEventLog::LastEventId = 0;

svtkStandardNewMacro(svtkMPIEventLog);

void svtkMPIEventLog::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

svtkMPIEventLog::svtkMPIEventLog()
{

  this->Active = 0;
}

void svtkMPIEventLog::InitializeLogging()
{
  MPE_Init_log();
}

void svtkMPIEventLog::FinalizeLogging(const char* fname)
{
  MPE_Finish_log(const_cast<char*>(fname));
}

int svtkMPIEventLog::SetDescription(const char* name, const char* desc)
{
  int err, processId;
  if ((err = MPI_Comm_rank(MPI_COMM_WORLD, &processId)) != MPI_SUCCESS)
  {
    char* msg = svtkMPIController::ErrorString(err);
    svtkErrorMacro("MPI error occurred: " << msg);
    delete[] msg;
    return 0;
  }

  this->Active = 1;
  if (processId == 0)
  {
    this->BeginId = MPE_Log_get_event_number();
    this->EndId = MPE_Log_get_event_number();
    MPE_Describe_state(
      this->BeginId, this->EndId, const_cast<char*>(name), const_cast<char*>(desc));
  }
  MPI_Bcast(&this->BeginId, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&this->EndId, 1, MPI_INT, 0, MPI_COMM_WORLD);

  return 1;
}

void svtkMPIEventLog::StartLogging()
{
  if (!this->Active)
  {
    svtkWarningMacro("This svtkMPIEventLog has not been initialized. Can not log event.");
    return;
  }

  MPE_Log_event(this->BeginId, 0, "begin");
}

void svtkMPIEventLog::StopLogging()
{
  if (!this->Active)
  {
    svtkWarningMacro("This svtkMPIEventLog has not been initialized. Can not log event.");
    return;
  }
  MPE_Log_event(this->EndId, 0, "end");
}

svtkMPIEventLog::~svtkMPIEventLog() {}
