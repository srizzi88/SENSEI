/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MPIController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtk_mpi.h>

#include "svtkMPIController.h"
#include "svtkProcessGroup.h"

#include "ExerciseMultiProcessController.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

namespace
{
int mpiTag = 5678;
int data = -1;
}

// returns 0 for success
int CheckNoBlockSends(svtkMPIController* controller)
{
  int retVal = 0;
  // processes send their rank to the next, higher ranked process
  int myRank = controller->GetLocalProcessId();
  int numRanks = controller->GetNumberOfProcesses();
  svtkMPICommunicator::Request sendRequest;
  data = myRank;
  if (myRank != numRanks - 1)
  {
    if (controller->NoBlockSend(&data, 1, myRank + 1, mpiTag, sendRequest) == 0)
    {
      svtkGenericWarningMacro("Problem with NoBlockSend.");
      retVal = 1;
    }
  }
  return retVal;
}

// returns 0 for success
int CheckNoBlockRecvs(
  svtkMPIController* controller, int sendSource, int wasMessageSent, const char* info)
{
  int myRank = controller->GetLocalProcessId();
  int retVal = 0;
  if (myRank)
  {
    int flag = -1, actualSource = -1, size = -1;
    if (controller->Iprobe(sendSource, mpiTag, &flag, &actualSource, &size, &size) == 0)
    {
      svtkGenericWarningMacro("Problem with Iprobe " << info);
      retVal = 1;
    }
    if (flag != wasMessageSent)
    {
      if (wasMessageSent)
      {
        svtkGenericWarningMacro("Did not receive the message yet but should have " << info);
      }
      else
      {
        svtkGenericWarningMacro(" Received a message I shouldn't have " << info);
      }
      retVal = 1;
    }
    if (wasMessageSent == 0)
    { // no message sent so no need to check if we can receive it
      return retVal;
    }
    else
    {
      if (actualSource != myRank - 1)
      {
        svtkGenericWarningMacro("Did not receive the proper source id " << info);
        retVal = 1;
      }
      if (size != 1)
      {
        svtkGenericWarningMacro("Did not receive the proper size " << info);
        retVal = 1;
      }
    }
    int recvData = -1;
    svtkMPICommunicator::Request recvRequest;
    if (controller->NoBlockReceive(&recvData, 1, sendSource, mpiTag, recvRequest) == 0)
    {
      svtkGenericWarningMacro("Problem with NoBlockReceive " << info);
      retVal = 1;
    }
    recvRequest.Wait();
    if (recvData != myRank - 1)
    {
      svtkGenericWarningMacro("Did not receive the proper information " << info);
      retVal = 1;
    }
  }
  return retVal;
}

// returns 0 for success
int ExerciseNoBlockCommunications(svtkMPIController* controller)
{
  if (controller->GetNumberOfProcesses() == 1)
  {
    return 0;
  }
  int retVal = CheckNoBlockRecvs(controller, svtkMultiProcessController::ANY_SOURCE, 0, "case 1");

  // barrier to make sure there's really no message to receive
  controller->Barrier();

  retVal = retVal | CheckNoBlockSends(controller);

  // barrier to make sure it's really a non-blocking send
  controller->Barrier();

  int myRank = controller->GetLocalProcessId();
  retVal = retVal | CheckNoBlockRecvs(controller, myRank - 1, 1, "case 2");

  // do it again with doing an any_source receive
  controller->Barrier();

  retVal = retVal | CheckNoBlockSends(controller);

  // barrier to make sure it's really a non-blocking send
  controller->Barrier();

  retVal =
    retVal | CheckNoBlockRecvs(controller, svtkMultiProcessController::ANY_SOURCE, 1, "case 3");

  return retVal;
}

int MPIController(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  SVTK_CREATE(svtkMPIController, controller);

  controller->Initialize(&argc, &argv, 1);

  int retval = ExerciseMultiProcessController(controller);

  retval = retval | ExerciseNoBlockCommunications(controller);

  // The previous run of ExerciseMultiProcessController used the native MPI
  // collective operations.  There is also a second (inefficient) implementation
  // of these within the base svtkCommunicator class.  This hack should force the
  // class to use the SVTK implementation.  In practice, the collective
  // operations will probably never be used like this, but this is a convenient
  // place to test for completeness.
  SVTK_CREATE(svtkProcessGroup, group);
  group->Initialize(controller);
  svtkSmartPointer<svtkMultiProcessController> genericController;
  genericController.TakeReference(
    controller->svtkMultiProcessController::CreateSubController(group));
  if (!retval)
  {
    retval = ExerciseMultiProcessController(genericController);
  }

  controller->Finalize();

  return retval;
}
