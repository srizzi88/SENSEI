/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPPainterCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPPainterCommunicator.h"

#include "svtkMPI.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkMultiProcessController.h"

#include <vector>

using std::vector;

// use PImpl to avoid MPI types in public API.
class svtkPPainterCommunicatorInternals
{
public:
  svtkPPainterCommunicatorInternals()
    : Ownership(false)
    , Communicator(MPI_COMM_WORLD)
  {
  }

  ~svtkPPainterCommunicatorInternals();

  // Description:
  // Set the communicator, by default ownership is not taken.
  void SetCommunicator(MPI_Comm comm, bool ownership = false);

  // Description:
  // Duplicate the communicator, ownership of the new
  // communicator is always taken.
  void DuplicateCommunicator(MPI_Comm comm);

  bool Ownership;
  MPI_Comm Communicator;
};

//-----------------------------------------------------------------------------
svtkPPainterCommunicatorInternals::~svtkPPainterCommunicatorInternals()
{
  this->SetCommunicator(MPI_COMM_NULL);
}

//-----------------------------------------------------------------------------
void svtkPPainterCommunicatorInternals::SetCommunicator(MPI_Comm comm, bool ownership)
{
  // avoid unnecessary operations
  if (this->Communicator == comm)
  {
    return;
  }
  // do nothing without mpi
  if (svtkPPainterCommunicator::MPIInitialized() && !svtkPPainterCommunicator::MPIFinalized())
  {
    // release the old communicator if it's ours
    if (this->Ownership && (this->Communicator != MPI_COMM_NULL) &&
      (this->Communicator != MPI_COMM_WORLD))
    {
      MPI_Comm_free(&this->Communicator);
    }
  }
  // assign
  this->Ownership = ownership;
  this->Communicator = comm;
}

//-----------------------------------------------------------------------------
void svtkPPainterCommunicatorInternals::DuplicateCommunicator(MPI_Comm comm)
{
  // avoid unnecessary operations
  if (this->Communicator == comm)
  {
    return;
  }
  // handle no mpi gracefully
  if (!svtkPPainterCommunicator::MPIInitialized() || svtkPPainterCommunicator::MPIFinalized())
  {
    this->Ownership = false;
    this->Communicator = comm;
    return;
  }
  // release the old communicator if it's ours
  this->SetCommunicator(MPI_COMM_NULL);
  if (comm != MPI_COMM_NULL)
  {
    // duplcate
    this->Ownership = true;
    MPI_Comm_dup(comm, &this->Communicator);
  }
}

//-----------------------------------------------------------------------------
svtkPPainterCommunicator::svtkPPainterCommunicator()
{
  this->Internals = new ::svtkPPainterCommunicatorInternals;
}

//-----------------------------------------------------------------------------
svtkPPainterCommunicator::~svtkPPainterCommunicator()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void svtkPPainterCommunicator::Copy(const svtkPainterCommunicator* other, bool ownership)
{
  const svtkPPainterCommunicator* pOther = dynamic_cast<const svtkPPainterCommunicator*>(other);

  if (pOther && (pOther != this))
  {
    this->Internals->SetCommunicator(pOther->Internals->Communicator, ownership);
  }
}

//-----------------------------------------------------------------------------
void svtkPPainterCommunicator::Duplicate(const svtkPainterCommunicator* comm)
{
  const svtkPPainterCommunicator* pcomm = dynamic_cast<const svtkPPainterCommunicator*>(comm);

  if (pcomm)
  {
    this->Internals->DuplicateCommunicator(pcomm->Internals->Communicator);
  }
}

//-----------------------------------------------------------------------------
void svtkPPainterCommunicator::SetCommunicator(svtkMPICommunicatorOpaqueComm* comm)
{
  this->Internals->SetCommunicator(*comm->GetHandle());
}

//-----------------------------------------------------------------------------
void svtkPPainterCommunicator::GetCommunicator(svtkMPICommunicatorOpaqueComm* comm)
{
  *comm = &this->Internals->Communicator;
}

//-----------------------------------------------------------------------------
void* svtkPPainterCommunicator::GetCommunicator()
{
  return &this->Internals->Communicator;
}

//-----------------------------------------------------------------------------
int svtkPPainterCommunicator::GetRank()
{
  if (!this->MPIInitialized() || this->MPIFinalized())
  {
    return 0;
  }
  int rank;
  MPI_Comm_rank(this->Internals->Communicator, &rank);
  return rank;
}

//-----------------------------------------------------------------------------
int svtkPPainterCommunicator::GetSize()
{
  if (!this->MPIInitialized() || this->MPIFinalized())
  {
    return 1;
  }
  int size;
  MPI_Comm_size(this->Internals->Communicator, &size);
  return size;
}

//-----------------------------------------------------------------------------
int svtkPPainterCommunicator::GetWorldRank()
{
  if (!this->MPIInitialized() || this->MPIFinalized())
  {
    return 0;
  }
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  return rank;
}

//-----------------------------------------------------------------------------
int svtkPPainterCommunicator::GetWorldSize()
{
  if (!this->MPIInitialized() || this->MPIFinalized())
  {
    return 1;
  }
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  return size;
}

// ----------------------------------------------------------------------------
svtkMPICommunicatorOpaqueComm* svtkPPainterCommunicator::GetGlobalCommunicator()
{
  static svtkMPICommunicatorOpaqueComm* globalComm = nullptr;
  if (!globalComm)
  {
    if (svtkPPainterCommunicator::MPIInitialized())
    {
      svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();

      svtkMPIController* mpiController = svtkMPIController::SafeDownCast(controller);
      svtkMPICommunicator* mpiCommunicator;

      if (mpiController &&
        (mpiCommunicator = svtkMPICommunicator::SafeDownCast(controller->GetCommunicator())))
      {
        globalComm = new svtkMPICommunicatorOpaqueComm(*mpiCommunicator->GetMPIComm());
      }
      else
      {
        svtkGenericWarningMacro("MPI is required for parallel operations.");
      }
    }
  }
  return globalComm;
}

//-----------------------------------------------------------------------------
bool svtkPPainterCommunicator::MPIInitialized()
{
  int initialized;
  MPI_Initialized(&initialized);
  return initialized == 1;
}

//-----------------------------------------------------------------------------
bool svtkPPainterCommunicator::MPIFinalized()
{
  int finished;
  MPI_Finalized(&finished);
  return finished == 1;
}

//-----------------------------------------------------------------------------
bool svtkPPainterCommunicator::GetIsNull()
{
  return this->Internals->Communicator == MPI_COMM_NULL;
}

//-----------------------------------------------------------------------------
void svtkPPainterCommunicator::SubsetCommunicator(svtkMPICommunicatorOpaqueComm* comm, int include)
{
#if defined(svtkPPainterCommunicatorDEBUG)
  cerr << "=====svtkPPainterCommunicator::SubsetCommunicator" << endl
       << "creating communicator " << (include ? "with" : "WITHOUT") << this->GetWorldRank()
       << endl;
#endif

  if (this->MPIInitialized() && !this->MPIFinalized())
  {
    MPI_Comm defaultComm = *((MPI_Comm*)comm->GetHandle());

    // exchange include status
    // make list of active ranks
    int worldSize = 0;
    MPI_Comm_size(defaultComm, &worldSize);

    vector<int> included(worldSize, 0);
    MPI_Allgather(&include, 1, MPI_INT, &included[0], 1, MPI_INT, defaultComm);

    vector<int> activeRanks;
    activeRanks.reserve(worldSize);
    for (int i = 0; i < worldSize; ++i)
    {
      if (included[i] != 0)
      {
        activeRanks.push_back(i);
      }
    }

    int nActive = (int)activeRanks.size();
    if (nActive == 0)
    {
      // no active ranks
      // no rendering will occur so no communicator
      // is needed
      this->Internals->SetCommunicator(MPI_COMM_NULL);
    }
    else if (nActive == worldSize)
    {
      // all ranks are active
      // use the default communicator.
      this->Internals->SetCommunicator(defaultComm);
    }
    else
    {
      // a subset of the ranks are active
      // make a new communicator
      MPI_Group wholeGroup;
      MPI_Comm_group(defaultComm, &wholeGroup);

      MPI_Group activeGroup;
      MPI_Group_incl(wholeGroup, nActive, &activeRanks[0], &activeGroup);

      MPI_Comm subsetComm;
      MPI_Comm_create(defaultComm, activeGroup, &subsetComm);
      MPI_Group_free(&activeGroup);

      this->Internals->SetCommunicator(subsetComm, true);
    }
  }
}
