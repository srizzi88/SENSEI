/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPICommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMPICommunicator.h"

#include "svtkImageData.h"
#include "svtkMPI.h"
#include "svtkMPIController.h"
#include "svtkObjectFactory.h"
#include "svtkProcessGroup.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkToolkits.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include <cassert>
#include <vector>

static inline void svtkMPICommunicatorDebugBarrier(MPI_Comm* handle)
{
  // If NDEBUG is defined, do nothing.
#ifdef NDEBUG
  (void)handle; // to avoid warning about unused parameter
#else
  MPI_Barrier(*handle);
#endif
}

svtkStandardNewMacro(svtkMPICommunicator);

svtkMPICommunicator* svtkMPICommunicator::WorldCommunicator = 0;

svtkMPICommunicatorOpaqueComm::svtkMPICommunicatorOpaqueComm(MPI_Comm* handle)
{
  this->Handle = handle;
}

MPI_Comm* svtkMPICommunicatorOpaqueComm::GetHandle()
{
  return this->Handle;
}

//-----------------------------------------------------------------------------
// This MPI error handler basically does the same thing as the default error
// handler, but also provides a convenient place to attach a debugger
// breakpoint.
extern "C" void svtkMPICommunicatorMPIErrorHandler(MPI_Comm* comm, int* errorcode, ...)
{
  char ErrorMessage[MPI_MAX_ERROR_STRING];
  int len;
  MPI_Error_string(*errorcode, ErrorMessage, &len);
  svtkGenericWarningMacro(<< "MPI had an error" << endl
                         << "------------------------------------------------" << endl
                         << ErrorMessage << endl
                         << "------------------------------------------------");
  MPI_Abort(*comm, *errorcode);
}

//----------------------------------------------------------------------------
// I wish I could think of a better way to convert a SVTK type enum to an MPI
// type enum and back.

// Make sure MPI_LONG_LONG and MPI_UNSIGNED_LONG_LONG are defined, if at all
// possible.
#ifndef MPI_LONG_LONG
#ifdef MPI_LONG_LONG_INT
// lampi only has MPI_LONG_LONG_INT, not MPI_LONG_LONG
#define MPI_LONG_LONG MPI_LONG_LONG_INT
#endif
#endif

#ifndef MPI_UNSIGNED_LONG_LONG
#ifdef MPI_UNSIGNED_LONG_LONG_INT
#define MPI_UNSIGNED_LONG_LONG MPI_UNSIGNED_LONG_LONG_INT
#elif defined(MPI_LONG_LONG)
// mpich does not have an unsigned long long.  Just using signed should
// be OK.  Everything uses 2's complement nowadays, right?
#define MPI_UNSIGNED_LONG_LONG MPI_LONG_LONG
#endif
#endif

inline MPI_Datatype svtkMPICommunicatorGetMPIType(int svtkType)
{

  switch (svtkType)
  {
    case SVTK_CHAR:
      return MPI_CHAR;
#ifdef MPI_SIGNED_CHAR
    case SVTK_SIGNED_CHAR:
      return MPI_SIGNED_CHAR;
#else
    case SVTK_SIGNED_CHAR:
      return MPI_CHAR;
#endif
    case SVTK_UNSIGNED_CHAR:
      return MPI_UNSIGNED_CHAR;
    case SVTK_SHORT:
      return MPI_SHORT;
    case SVTK_UNSIGNED_SHORT:
      return MPI_UNSIGNED_SHORT;
    case SVTK_INT:
      return MPI_INT;
    case SVTK_UNSIGNED_INT:
      return MPI_UNSIGNED;
    case SVTK_LONG:
      return MPI_LONG;
    case SVTK_UNSIGNED_LONG:
      return MPI_UNSIGNED_LONG;
    case SVTK_FLOAT:
      return MPI_FLOAT;
    case SVTK_DOUBLE:
      return MPI_DOUBLE;

#ifdef SVTK_USE_64BIT_IDS
#if SVTK_SIZEOF_LONG == 8
    case SVTK_ID_TYPE:
      return MPI_LONG;
#elif defined(MPI_LONG_LONG)
    case SVTK_ID_TYPE:
      return MPI_LONG_LONG;
#else
    case SVTK_ID_TYPE:
      svtkGenericWarningMacro(
        "This systems MPI doesn't seem to support 64 bit ids and you have 64 bit IDs turned on. "
        "Please seek assistance on the SVTK Discourse (https://discourse.svtk.org/).");
      return MPI_LONG;
#endif
#else  // SVTK_USE_64BIT_IDS
    case SVTK_ID_TYPE:
      return MPI_INT;
#endif // SVTK_USE_64BIT_IDS

#ifdef MPI_LONG_LONG
    case SVTK_LONG_LONG:
      return MPI_LONG_LONG;
    case SVTK_UNSIGNED_LONG_LONG:
      return MPI_UNSIGNED_LONG_LONG;
#endif

#if !defined(SVTK_LEGACY_REMOVE)
#if SVTK_SIZEOF_LONG == 8
    case SVTK___INT64:
      return MPI_LONG;
    case SVTK_UNSIGNED___INT64:
      return MPI_UNSIGNED_LONG;
#elif defined(MPI_LONG_LONG)
    case SVTK___INT64:
      return MPI_LONG_LONG;
    case SVTK_UNSIGNED___INT64:
      return MPI_UNSIGNED_LONG_LONG;
#endif
#endif

    default:
      svtkGenericWarningMacro("Could not find a supported MPI type for SVTK type " << svtkType);
      return MPI_BYTE;
  }
}

inline int svtkMPICommunicatorGetSVTKType(MPI_Datatype type)
{
  if (type == MPI_FLOAT)
    return SVTK_FLOAT;
  if (type == MPI_DOUBLE)
    return SVTK_DOUBLE;
  if (type == MPI_BYTE)
    return SVTK_CHAR;
  if (type == MPI_CHAR)
    return SVTK_CHAR;
  if (type == MPI_UNSIGNED_CHAR)
    return SVTK_UNSIGNED_CHAR;
#ifdef MPI_SIGNED_CHAR
  if (type == MPI_SIGNED_CHAR)
    return SVTK_SIGNED_CHAR;
#endif
  if (type == MPI_SHORT)
    return SVTK_SHORT;
  if (type == MPI_UNSIGNED_SHORT)
    return SVTK_UNSIGNED_SHORT;
  if (type == MPI_INT)
    return SVTK_INT;
  if (type == MPI_UNSIGNED)
    return SVTK_UNSIGNED_INT;
  if (type == MPI_LONG)
    return SVTK_LONG;
  if (type == MPI_UNSIGNED_LONG)
    return SVTK_UNSIGNED_LONG;
#ifdef MPI_LONG_LONG
  if (type == MPI_LONG_LONG)
    return SVTK_LONG_LONG;
#endif
#ifdef MPI_UNSIGNED_LONG_LONG
  if (type == MPI_UNSIGNED_LONG_LONG)
    return SVTK_UNSIGNED_LONG_LONG;
#endif

  svtkGenericWarningMacro("Received unrecognized MPI type.");
  return SVTK_CHAR;
}

inline int svtkMPICommunicatorCheckSize(svtkIdType length)
{
  if (length > SVTK_INT_MAX)
  {
    svtkGenericWarningMacro(<< "This operation not yet supported for more than " << SVTK_INT_MAX
                           << " objects");
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
template <class T>
int svtkMPICommunicatorSendData(const T* data, int length, int sizeoftype, int remoteProcessId,
  int tag, MPI_Datatype datatype, MPI_Comm* Handle, int useCopy, int useSsend)
{
  if (useCopy)
  {
    int retVal;

    char* tmpData = svtkMPICommunicator::Allocate(length * sizeoftype);
    memcpy(tmpData, data, length * sizeoftype);
    if (useSsend)
    {
      retVal = MPI_Ssend(tmpData, length, datatype, remoteProcessId, tag, *(Handle));
    }
    else
    {
      retVal = MPI_Send(tmpData, length, datatype, remoteProcessId, tag, *(Handle));
    }
    svtkMPICommunicator::Free(tmpData);
    return retVal;
  }
  else
  {
    if (useSsend)
    {
      return MPI_Ssend(const_cast<T*>(data), length, datatype, remoteProcessId, tag, *(Handle));
    }
    else
    {
      return MPI_Send(const_cast<T*>(data), length, datatype, remoteProcessId, tag, *(Handle));
    }
  }
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::ReceiveDataInternal(char* data, int length, int sizeoftype,
  int remoteProcessId, int tag, svtkMPICommunicatorReceiveDataInfo* info, int useCopy, int& senderId)
{
  if (remoteProcessId == svtkMultiProcessController::ANY_SOURCE)
  {
    remoteProcessId = MPI_ANY_SOURCE;
  }

  int retVal;

  if (useCopy)
  {
    char* tmpData = svtkMPICommunicator::Allocate(length * sizeoftype);
    retVal = MPI_Recv(
      tmpData, length, info->DataType, remoteProcessId, tag, *(info->Handle), &(info->Status));
    memcpy(data, tmpData, length * sizeoftype);
    svtkMPICommunicator::Free(tmpData);
  }
  else
  {
    retVal = MPI_Recv(
      data, length, info->DataType, remoteProcessId, tag, *(info->Handle), &(info->Status));
  }

  if (retVal == MPI_SUCCESS)
  {
    senderId = info->Status.MPI_SOURCE;
  }
  return retVal;
}

//----------------------------------------------------------------------------
template <class T>
int svtkMPICommunicatorNoBlockSendData(const T* data, int length, int remoteProcessId, int tag,
  MPI_Datatype datatype, svtkMPICommunicator::Request& req, MPI_Comm* Handle)
{
  return MPI_Isend(
    const_cast<T*>(data), length, datatype, remoteProcessId, tag, *(Handle), &req.Req->Handle);
}
//----------------------------------------------------------------------------
template <class T>
int svtkMPICommunicatorNoBlockReceiveData(T* data, int length, int remoteProcessId, int tag,
  MPI_Datatype datatype, svtkMPICommunicator::Request& req, MPI_Comm* Handle)
{
  if (remoteProcessId == svtkMultiProcessController::ANY_SOURCE)
  {
    remoteProcessId = MPI_ANY_SOURCE;
  }

  return MPI_Irecv(data, length, datatype, remoteProcessId, tag, *(Handle), &req.Req->Handle);
}
//----------------------------------------------------------------------------
int svtkMPICommunicatorReduceData(const void* sendBuffer, void* recvBuffer, svtkIdType length,
  int type, MPI_Op operation, int destProcessId, MPI_Comm* comm)
{
  if (!svtkMPICommunicatorCheckSize(length))
    return 0;
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  return MPI_Reduce(
    const_cast<void*>(sendBuffer), recvBuffer, length, mpiType, operation, destProcessId, *comm);
}
//----------------------------------------------------------------------------
int svtkMPICommunicatorAllReduceData(const void* sendBuffer, void* recvBuffer, svtkIdType length,
  int type, MPI_Op operation, MPI_Comm* comm)
{
  if (!svtkMPICommunicatorCheckSize(length))
    return 0;
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  return MPI_Allreduce(
    const_cast<void*>(sendBuffer), recvBuffer, length, mpiType, operation, *comm);
}

//----------------------------------------------------------------------------
int svtkMPICommunicatorIprobe(int source, int tag, int* flag, int* actualSource,
  MPI_Datatype datatype, int* size, MPI_Comm* handle)
{
  if (source == svtkMultiProcessController::ANY_SOURCE)
  {
    source = MPI_ANY_SOURCE;
  }
  MPI_Status status;
  int retVal = MPI_Iprobe(source, tag, *handle, flag, &status);
  if (retVal == MPI_SUCCESS && *flag == 1)
  {
    if (actualSource)
    {
      *actualSource = status.MPI_SOURCE;
    }
    if (size)
    {
      return MPI_Get_count(&status, datatype, size);
    }
  }
  return retVal;
}

//-----------------------------------------------------------------------------
// Method for converting an MPI operation to a
// svtkMultiProcessController::Operation.
// MPIAPI is defined in the microsoft mpi.h which decorates
// with the __stdcall decoration.
static svtkCommunicator::Operation* CurrentOperation;
#ifdef MPIAPI
extern "C" void MPIAPI svtkMPICommunicatorUserFunction(
  void* invec, void* inoutvec, int* len, MPI_Datatype* datatype)
#else
extern "C" void svtkMPICommunicatorUserFunction(
  void* invec, void* inoutvec, int* len, MPI_Datatype* datatype)
#endif
{
  int svtkType = svtkMPICommunicatorGetSVTKType(*datatype);
  CurrentOperation->Function(invec, inoutvec, *len, svtkType);
}

//----------------------------------------------------------------------------
// Return the world communicator (i.e. MPI_COMM_WORLD).
// Create one if necessary (singleton).
svtkMPICommunicator* svtkMPICommunicator::GetWorldCommunicator()
{
  int err, size;

  if (svtkMPICommunicator::WorldCommunicator == 0)
  {
    // Install an error handler
    MPI_Errhandler errhandler;
#if (MPI_VERSION > 2) || ((MPI_VERSION == 2) && (MPI_SUBVERSION >= 0))
    MPI_Comm_create_errhandler(svtkMPICommunicatorMPIErrorHandler, &errhandler);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, errhandler);
#else
    MPI_Errhandler_create(svtkMPICommunicatorMPIErrorHandler, &errhandler);
    MPI_Errhandler_set(MPI_COMM_WORLD, errhandler);
#endif
    MPI_Errhandler_free(&errhandler);

    svtkMPICommunicator* comm = svtkMPICommunicator::New();
    comm->MPIComm->Handle = new MPI_Comm;
    *(comm->MPIComm->Handle) = MPI_COMM_WORLD;
    if ((err = MPI_Comm_size(MPI_COMM_WORLD, &size)) != MPI_SUCCESS)
    {
      char* msg = svtkMPIController::ErrorString(err);
      svtkGenericWarningMacro("MPI error occurred: " << msg);
      delete[] msg;
      delete comm->MPIComm->Handle;
      comm->MPIComm = 0;
      comm->Delete();
      return 0;
    }
    comm->InitializeNumberOfProcesses();
    comm->Initialized = 1;
    comm->KeepHandleOn();
    svtkMPICommunicator::WorldCommunicator = comm;
  }
  return svtkMPICommunicator::WorldCommunicator;
}

//----------------------------------------------------------------------------
void svtkMPICommunicator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MPI Communicator handler: ";
  if (this->MPIComm->Handle)
  {
    os << this->MPIComm->Handle << endl;
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "UseSsend: " << (this->UseSsend ? "On" : " Off") << endl;
  os << indent << "Initialized: " << (this->Initialized ? "On\n" : "Off\n");
  os << indent << "Keep handle: " << (this->KeepHandle ? "On\n" : "Off\n");
  if (this != svtkMPICommunicator::WorldCommunicator)
  {
    os << indent << "World communicator: ";
    if (svtkMPICommunicator::WorldCommunicator)
    {
      os << endl;
      svtkMPICommunicator::WorldCommunicator->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
      os << "(none)";
    }
    os << endl;
  }
  return;
}

//----------------------------------------------------------------------------
svtkMPICommunicator::svtkMPICommunicator()
{
  this->MPIComm = new svtkMPICommunicatorOpaqueComm;
  this->Initialized = 0;
  this->KeepHandle = 0;
  this->LastSenderId = -1;
  this->UseSsend = 0;
}

//----------------------------------------------------------------------------
svtkMPICommunicator::~svtkMPICommunicator()
{
  // Free the handle if required and asked for.
  if (this->MPIComm)
  {
    if (this->MPIComm->Handle && !this->KeepHandle)
    {
      if (*(this->MPIComm->Handle) != MPI_COMM_NULL)
      {
        MPI_Comm_free(this->MPIComm->Handle);
      }
    }
    delete this->MPIComm->Handle;
    delete this->MPIComm;
  }
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::Initialize(svtkProcessGroup* group)
{
  if (this->Initialized)
  {
    return 0;
  }

  svtkMPICommunicator* mpiComm = svtkMPICommunicator::SafeDownCast(group->GetCommunicator());
  if (!mpiComm)
  {
    svtkErrorMacro("The group is not attached to an MPI communicator!");
    return 0;
  }

  // If mpiComm has been initialized, it is guaranteed (unless the MPI calls
  // return an error somewhere) to have valid Communicator.
  if (!mpiComm->Initialized)
  {
    svtkWarningMacro("The communicator passed has not been initialized!");
    return 0;
  }

  if (group->GetNumberOfProcessIds() == 0)
  {
    // Based on interpreting the MPI documentation it doesn't seem like a
    // requirement but in practical terms it doesn't seem to make sense
    // to create an MPI communicator with 0 processes. Also, some
    // implementations of MPI crash if this is the case.
    svtkWarningMacro("The group doesn't contain any process ids!");
    return 0;
  }

  this->KeepHandleOff();

  // Select the new processes
  int nProcIds = group->GetNumberOfProcessIds();
  int* ranks = new int[nProcIds];
  for (int i = 0; i < nProcIds; i++)
  {
    ranks[i] = group->GetProcessId(i);
  }

  MPI_Group superGroup;
  MPI_Group subGroup;

  // Get the super group
  int err;
  if ((err = MPI_Comm_group(*(mpiComm->MPIComm->Handle), &superGroup)) != MPI_SUCCESS)
  {
    delete[] ranks;
    MPI_Group_free(&superGroup);

    char* msg = svtkMPIController::ErrorString(err);
    svtkErrorMacro("MPI error occurred: " << msg);
    delete[] msg;

    return 0;
  }

  // Create a new group by including the process ids in group
  if ((err = MPI_Group_incl(superGroup, nProcIds, ranks, &subGroup)) != MPI_SUCCESS)
  {
    delete[] ranks;
    MPI_Group_free(&superGroup);
    MPI_Group_free(&subGroup);

    char* msg = svtkMPIController::ErrorString(err);
    svtkErrorMacro("MPI error occurred: " << msg);
    delete[] msg;

    return 0;
  }

  delete[] ranks;
  MPI_Group_free(&superGroup);

  this->MPIComm->Handle = new MPI_Comm;
  // Create the communicator from the group
  if ((err = MPI_Comm_create(*(mpiComm->MPIComm->Handle), subGroup, this->MPIComm->Handle)) !=
    MPI_SUCCESS)
  {
    MPI_Group_free(&subGroup);
    delete this->MPIComm->Handle;
    this->MPIComm->Handle = 0;

    char* msg = svtkMPIController::ErrorString(err);
    svtkErrorMacro("MPI error occurred: " << msg);
    delete[] msg;

    return 0;
  }

  MPI_Group_free(&subGroup);

  // MPI is kind of funny in that in order to create a communicator from a
  // subgroup of another communicator, it is a collective operation involving
  // all of the processes in the original communicator, not just those belonging
  // to the group.  In any process not part of the group, the communicator is
  // created with MPI_COMM_NULL.  Check for that and only finish initialization
  // when the controller is not MPI_COMM_NULL.
  if (*(this->MPIComm->Handle) != MPI_COMM_NULL)
  {
    this->InitializeNumberOfProcesses();
    this->Initialized = 1;
  }

  this->Modified();

  return 1;
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::SplitInitialize(svtkCommunicator* oldcomm, int color, int key)
{
  if (this->Initialized)
    return 0;

  svtkMPICommunicator* mpiComm = svtkMPICommunicator::SafeDownCast(oldcomm);
  if (!mpiComm)
  {
    svtkErrorMacro("Split communicator must be an MPI communicator.");
    return 0;
  }

  // If mpiComm has been initialized, it is guaranteed (unless the MPI calls
  // return an error somewhere) to have valid Communicator.
  if (!mpiComm->Initialized)
  {
    svtkWarningMacro("The communicator passed has not been initialized!");
    return 0;
  }

  this->KeepHandleOff();

  this->MPIComm->Handle = new MPI_Comm;
  int err;
  if ((err = MPI_Comm_split(*(mpiComm->MPIComm->Handle), color, key, this->MPIComm->Handle)) !=
    MPI_SUCCESS)
  {
    delete this->MPIComm->Handle;
    this->MPIComm->Handle = 0;

    char* msg = svtkMPIController::ErrorString(err);
    svtkErrorMacro("MPI error occurred: " << msg);
    delete[] msg;

    return 0;
  }

  this->InitializeNumberOfProcesses();
  this->Initialized = 1;

  this->Modified();

  return 1;
}

int svtkMPICommunicator::InitializeExternal(svtkMPICommunicatorOpaqueComm* comm)
{
  this->KeepHandleOn();

  delete this->MPIComm->Handle;
  this->MPIComm->Handle = new MPI_Comm(*(comm->GetHandle()));
  this->InitializeNumberOfProcesses();
  this->Initialized = 1;

  this->Modified();

  return 1;
}

//----------------------------------------------------------------------------
// Start the copying process
void svtkMPICommunicator::InitializeCopy(svtkMPICommunicator* source)
{
  if (!source)
  {
    return;
  }

  if (this->MPIComm->Handle && !this->KeepHandle)
  {
    MPI_Comm_free(this->MPIComm->Handle);
  }
  delete this->MPIComm->Handle;
  this->MPIComm->Handle = 0;

  this->LocalProcessId = source->LocalProcessId;
  this->NumberOfProcesses = source->NumberOfProcesses;

  this->Initialized = source->Initialized;
  this->Modified();
}

//-----------------------------------------------------------------------------
// Set the number of processes and maximum number of processes
// to the size obtained from MPI.
int svtkMPICommunicator::InitializeNumberOfProcesses()
{
  int err;

  this->Modified();

  if ((err = MPI_Comm_size(*(this->MPIComm->Handle), &(this->MaximumNumberOfProcesses))) !=
    MPI_SUCCESS)
  {
    char* msg = svtkMPIController::ErrorString(err);
    svtkErrorMacro("MPI error occurred: " << msg);
    delete[] msg;
    return 0;
  }

  this->NumberOfProcesses = this->MaximumNumberOfProcesses;

  if ((err = MPI_Comm_rank(*(this->MPIComm->Handle), &(this->LocalProcessId))) != MPI_SUCCESS)
  {
    char* msg = svtkMPIController::ErrorString(err);
    svtkErrorMacro("MPI error occurred: " << msg);
    delete[] msg;
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
// Copy the MPI handle
void svtkMPICommunicator::CopyFrom(svtkMPICommunicator* source)
{
  this->InitializeCopy(source);

  if (source->MPIComm->Handle)
  {
    this->KeepHandleOn();
    this->MPIComm->Handle = new MPI_Comm;
    *(this->MPIComm->Handle) = *(source->MPIComm->Handle);
  }
}

//----------------------------------------------------------------------------
// Duplicate the MPI handle
void svtkMPICommunicator::Duplicate(svtkMPICommunicator* source)
{
  this->InitializeCopy(source);

  this->KeepHandleOff();

  if (source->MPIComm->Handle)
  {
    this->MPIComm->Handle = new MPI_Comm;
    int err;
    if ((err = MPI_Comm_dup(*(source->MPIComm->Handle), this->MPIComm->Handle)) != MPI_SUCCESS)
    {
      char* msg = svtkMPIController::ErrorString(err);
      svtkErrorMacro("MPI error occurred: " << msg);
      delete[] msg;
    }
  }
}

//----------------------------------------------------------------------------
char* svtkMPICommunicator::Allocate(size_t size)
{
#ifdef MPIPROALLOC
  char* ptr;
  MPI_Alloc_mem(size, nullptr, &ptr);
  return ptr;
#else
  return new char[size];
#endif
}

//----------------------------------------------------------------------------
void svtkMPICommunicator::Free(char* ptr)
{
#ifdef MPIPROALLOC
  MPI_Free_mem(ptr);
#else
  delete[] ptr;
#endif
}

//----------------------------------------------------------------------------
int svtkMPICommunicator::CheckForMPIError(int err)
{

  if (err == MPI_SUCCESS)
  {
    return 1;
  }
  else
  {
    char* msg = svtkMPIController::ErrorString(err);
    svtkGenericWarningMacro("MPI error occurred: " << msg);
    delete[] msg;
    return 0;
  }
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::SendVoidArray(
  const void* data, svtkIdType length, int type, int remoteProcessId, int tag)
{
  const char* byteData = static_cast<const char*>(data);
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  int sizeOfType;
  switch (type)
  {
    svtkTemplateMacro(sizeOfType = sizeof(SVTK_TT));
    default:
      svtkWarningMacro(<< "Invalid data type " << type);
      sizeOfType = 1;
      break;
  }

  int maxSend = SVTK_INT_MAX;
  while (length >= maxSend)
  {
    if (CheckForMPIError(svtkMPICommunicatorSendData(byteData, maxSend, sizeOfType, remoteProcessId,
          tag, mpiType, this->MPIComm->Handle, svtkCommunicator::UseCopy, this->UseSsend)) == 0)
    {
      // Failed to send.
      return 0;
    }
    byteData += maxSend * sizeOfType;
    length -= maxSend;
  }
  return CheckForMPIError(svtkMPICommunicatorSendData(byteData, length, sizeOfType, remoteProcessId,
    tag, mpiType, this->MPIComm->Handle, svtkCommunicator::UseCopy, this->UseSsend));
}

//-----------------------------------------------------------------------------
inline svtkIdType svtkMPICommunicatorMin(svtkIdType a, svtkIdType b)
{
  return (a > b) ? b : a;
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::ReceiveVoidArray(
  void* data, svtkIdType maxlength, int type, int remoteProcessId, int tag)
{
  this->Count = 0;
  char* byteData = static_cast<char*>(data);
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  int sizeOfType;
  switch (type)
  {
    svtkTemplateMacro(sizeOfType = sizeof(SVTK_TT));
    default:
      svtkWarningMacro(<< "Invalid data type " << type);
      sizeOfType = 1;
      break;
  }

  // maxReceive is the maximum size of data that can be fetched in a one atomic
  // receive. If when sending the data-length >= maxReceive, then the sender
  // splits it into multiple packets of at most maxReceive size each.  (Note
  // that when the sending exactly maxReceive length message, it is split into 2
  // packets of sizes maxReceive and 0 respectively).
  int maxReceive = SVTK_INT_MAX;
  svtkMPICommunicatorReceiveDataInfo info;
  info.Handle = this->MPIComm->Handle;
  info.DataType = mpiType;
  while (CheckForMPIError(this->ReceiveDataInternal(byteData,
           svtkMPICommunicatorMin(maxlength, maxReceive), sizeOfType, remoteProcessId, tag, &info,
           svtkCommunicator::UseCopy, this->LastSenderId)) != 0)
  {
    remoteProcessId = this->LastSenderId;

    int words_received = 0;
    if (CheckForMPIError(MPI_Get_count(&info.Status, mpiType, &words_received)) == 0)
    {
      // Failed.
      return 0;
    }
    this->Count += words_received;
    byteData += words_received * sizeOfType;
    maxlength -= words_received;
    if (words_received < maxReceive)
    {
      // words_received in this packet is exactly equal to maxReceive, then it
      // means that the sender is sending at least one more packet for this
      // message. Otherwise, we have received all the packets for this message
      // and we no longer need to iterate.
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockSend(
  const int* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockSendData(
    data, length, remoteProcessId, tag, MPI_INT, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockSend(
  const unsigned long* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockSendData(
    data, length, remoteProcessId, tag, MPI_UNSIGNED_LONG, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockSend(
  const char* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockSendData(
    data, length, remoteProcessId, tag, MPI_CHAR, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockSend(
  const unsigned char* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockSendData(
    data, length, remoteProcessId, tag, MPI_UNSIGNED_CHAR, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockSend(
  const float* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockSendData(
    data, length, remoteProcessId, tag, MPI_FLOAT, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockSend(
  const double* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockSendData(
    data, length, remoteProcessId, tag, MPI_DOUBLE, req, this->MPIComm->Handle));
}
#ifdef SVTK_USE_64BIT_IDS
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockSend(
  const svtkIdType* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockSendData(data, length, remoteProcessId, tag,
    svtkMPICommunicatorGetMPIType(SVTK_ID_TYPE), req, this->MPIComm->Handle));
}
#endif

//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockReceive(
  int* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockReceiveData(
    data, length, remoteProcessId, tag, MPI_INT, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockReceive(
  unsigned long* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockReceiveData(
    data, length, remoteProcessId, tag, MPI_UNSIGNED_LONG, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockReceive(
  char* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockReceiveData(
    data, length, remoteProcessId, tag, MPI_CHAR, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockReceive(
  unsigned char* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockReceiveData(
    data, length, remoteProcessId, tag, MPI_UNSIGNED_CHAR, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockReceive(
  float* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockReceiveData(
    data, length, remoteProcessId, tag, MPI_FLOAT, req, this->MPIComm->Handle));
}
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockReceive(
  double* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockReceiveData(
    data, length, remoteProcessId, tag, MPI_DOUBLE, req, this->MPIComm->Handle));
}
#ifdef SVTK_USE_64BIT_IDS
//----------------------------------------------------------------------------
int svtkMPICommunicator::NoBlockReceive(
  svtkIdType* data, int length, int remoteProcessId, int tag, Request& req)
{

  return CheckForMPIError(svtkMPICommunicatorNoBlockReceiveData(data, length, remoteProcessId, tag,
    svtkMPICommunicatorGetMPIType(SVTK_ID_TYPE), req, this->MPIComm->Handle));
}
#endif

//----------------------------------------------------------------------------
svtkMPICommunicator::Request::Request()
{
  this->Req = new svtkMPICommunicatorOpaqueRequest;
}

//----------------------------------------------------------------------------
svtkMPICommunicator::Request::Request(const svtkMPICommunicator::Request& src)
{
  this->Req = new svtkMPICommunicatorOpaqueRequest;
  this->Req->Handle = src.Req->Handle;
}

//----------------------------------------------------------------------------
svtkMPICommunicator::Request& svtkMPICommunicator::Request::operator=(
  const svtkMPICommunicator::Request& src)
{
  if (this == &src)
  {
    return *this;
  }
  this->Req->Handle = src.Req->Handle;
  return *this;
}

//----------------------------------------------------------------------------
svtkMPICommunicator::Request::~Request()
{
  delete this->Req;
}

//----------------------------------------------------------------------------
int svtkMPICommunicator::Request::Test()
{
  MPI_Status status;
  int retVal;

  int err = MPI_Test(&this->Req->Handle, &retVal, &status);

  if (err == MPI_SUCCESS)
  {
    return retVal;
  }
  else
  {
    char* msg = svtkMPIController::ErrorString(err);
    svtkGenericWarningMacro("MPI error occurred: " << msg);
    delete[] msg;
    return 0;
  }
}

//----------------------------------------------------------------------------
void svtkMPICommunicator::Request::Wait()
{
  MPI_Status status;

  int err = MPI_Wait(&this->Req->Handle, &status);

  if (err != MPI_SUCCESS)
  {
    char* msg = svtkMPIController::ErrorString(err);
    svtkGenericWarningMacro("MPI error occurred: " << msg);
    delete[] msg;
  }
}

//----------------------------------------------------------------------------
void svtkMPICommunicator::Request::Cancel()
{
  int err = MPI_Cancel(&this->Req->Handle);

  if (err != MPI_SUCCESS)
  {
    char* msg = svtkMPIController::ErrorString(err);
    svtkGenericWarningMacro("MPI error occurred: " << msg);
    delete[] msg;
  }

  err = MPI_Request_free(&this->Req->Handle);

  if (err != MPI_SUCCESS)
  {
    char* msg = svtkMPIController::ErrorString(err);
    svtkGenericWarningMacro("MPI error occurred: " << msg);
    delete[] msg;
  }
}

//-----------------------------------------------------------------------------
void svtkMPICommunicator::Barrier()
{
  CheckForMPIError(MPI_Barrier(*this->MPIComm->Handle));
}

//----------------------------------------------------------------------------
int svtkMPICommunicator::BroadcastVoidArray(void* data, svtkIdType length, int type, int root)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!svtkMPICommunicatorCheckSize(length))
    return 0;
  return CheckForMPIError(
    MPI_Bcast(data, length, svtkMPICommunicatorGetMPIType(type), root, *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::GatherVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int destProcessId)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  int numProc;
  MPI_Comm_size(*this->MPIComm->Handle, &numProc);
  if (!svtkMPICommunicatorCheckSize(length * numProc))
    return 0;
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  return CheckForMPIError(MPI_Gather(const_cast<void*>(sendBuffer), length, mpiType, recvBuffer,
    length, mpiType, destProcessId, *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::GatherVVoidArray(const void* sendBuffer, void* recvBuffer,
  svtkIdType sendLength, svtkIdType* recvLengths, svtkIdType* offsets, int type, int destProcessId)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!svtkMPICommunicatorCheckSize(sendLength))
    return 0;
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  // We have to jump through several hoops to make sure svtkIdType arrays
  // become int arrays.
  int rank;
  MPI_Comm_rank(*this->MPIComm->Handle, &rank);
  if (rank == destProcessId)
  {
    int numProc;
    MPI_Comm_size(*this->MPIComm->Handle, &numProc);
#if defined(OPEN_MPI) && (OMPI_MAJOR_VERSION <= 1) && (OMPI_MINOR_VERSION <= 2)
    // I found a bug in OpenMPI 1.2 and earlier where MPI_Gatherv sometimes
    // fails with only one process.  I am told that they will fix it in a later
    // version, but here is a workaround for now.
    if (numProc == 1)
    {
      switch (type)
      {
        svtkTemplateMacro(std::copy(reinterpret_cast<const SVTK_TT*>(sendBuffer),
          reinterpret_cast<const SVTK_TT*>(sendBuffer) + sendLength,
          reinterpret_cast<SVTK_TT*>(recvBuffer) + offsets[0]));
      }
      return 1;
    }
#endif // OPEN_MPI
    std::vector<int> mpiRecvLengths, mpiOffsets;
    mpiRecvLengths.resize(numProc);
    mpiOffsets.resize(numProc);
    for (int i = 0; i < numProc; i++)
    {
      if (!svtkMPICommunicatorCheckSize(recvLengths[i] + offsets[i]))
      {
        return 0;
      }
      mpiRecvLengths[i] = recvLengths[i];
      mpiOffsets[i] = offsets[i];
    }
    return CheckForMPIError(
      MPI_Gatherv(const_cast<void*>(sendBuffer), sendLength, mpiType, recvBuffer,
        &mpiRecvLengths[0], &mpiOffsets[0], mpiType, destProcessId, *this->MPIComm->Handle));
  }
  else
  {
    return CheckForMPIError(MPI_Gatherv(const_cast<void*>(sendBuffer), sendLength, mpiType, nullptr,
      nullptr, nullptr, mpiType, destProcessId, *this->MPIComm->Handle));
  }
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::ScatterVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int srcProcessId)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!svtkMPICommunicatorCheckSize(length))
    return 0;
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  return CheckForMPIError(MPI_Scatter(const_cast<void*>(sendBuffer), length, mpiType, recvBuffer,
    length, mpiType, srcProcessId, *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::ScatterVVoidArray(const void* sendBuffer, void* recvBuffer,
  svtkIdType* sendLengths, svtkIdType* offsets, svtkIdType recvLength, int type, int srcProcessId)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!svtkMPICommunicatorCheckSize(recvLength))
    return 0;
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  // We have to jump through several hoops to make sure svtkIdType arrays
  // become int arrays.
  int rank;
  MPI_Comm_rank(*this->MPIComm->Handle, &rank);
  if (rank == srcProcessId)
  {
    int numProc;
    MPI_Comm_size(*this->MPIComm->Handle, &numProc);
#if defined(OPEN_MPI) && (OMPI_MAJOR_VERSION <= 1) && (OMPI_MINOR_VERSION <= 2)
    // I found a bug in OpenMPI 1.2 and earlier where MPI_Scatterv sometimes
    // fails with only one process.  I am told that they will fix it in a later
    // version, but here is a workaround for now.
    if (numProc == 1)
    {
      switch (type)
      {
        svtkTemplateMacro(std::copy(reinterpret_cast<const SVTK_TT*>(sendBuffer) + offsets[0],
          reinterpret_cast<const SVTK_TT*>(sendBuffer) + offsets[0] + sendLengths[0],
          reinterpret_cast<SVTK_TT*>(recvBuffer)));
      }
      return 1;
    }
#endif // OPEN_MPI
    std::vector<int> mpiSendLengths, mpiOffsets;
    mpiSendLengths.resize(numProc);
    mpiOffsets.resize(numProc);
    for (int i = 0; i < numProc; i++)
    {
      if (!svtkMPICommunicatorCheckSize(sendLengths[i] + offsets[i]))
      {
        return 0;
      }
      mpiSendLengths[i] = sendLengths[i];
      mpiOffsets[i] = offsets[i];
    }
    return CheckForMPIError(
      MPI_Scatterv(const_cast<void*>(sendBuffer), &mpiSendLengths[0], &mpiOffsets[0], mpiType,
        recvBuffer, recvLength, mpiType, srcProcessId, *this->MPIComm->Handle));
  }
  else
  {
    return CheckForMPIError(MPI_Scatterv(nullptr, nullptr, nullptr, mpiType, recvBuffer, recvLength,
      mpiType, srcProcessId, *this->MPIComm->Handle));
  }
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::AllGatherVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  int numProc;
  MPI_Comm_size(*this->MPIComm->Handle, &numProc);
  if (!svtkMPICommunicatorCheckSize(length * numProc))
    return 0;
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  return CheckForMPIError(MPI_Allgather(const_cast<void*>(sendBuffer), length, mpiType, recvBuffer,
    length, mpiType, *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::AllGatherVVoidArray(const void* sendBuffer, void* recvBuffer,
  svtkIdType sendLength, svtkIdType* recvLengths, svtkIdType* offsets, int type)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  if (!svtkMPICommunicatorCheckSize(sendLength))
    return 0;
  MPI_Datatype mpiType = svtkMPICommunicatorGetMPIType(type);
  // We have to jump through several hoops to make sure svtkIdType arrays
  // become int arrays.
  int numProc;
  MPI_Comm_size(*this->MPIComm->Handle, &numProc);
#if defined(OPEN_MPI) && (OMPI_MAJOR_VERSION <= 1) && (OMPI_MINOR_VERSION <= 2)
  // I found a bug in OpenMPI 1.2 and earlier where MPI_Allgatherv sometimes
  // fails with only one process.  I am told that they will fix it in a later
  // version, but here is a workaround for now.
  if (numProc == 1)
  {
    switch (type)
    {
      svtkTemplateMacro(std::copy(reinterpret_cast<const SVTK_TT*>(sendBuffer),
        reinterpret_cast<const SVTK_TT*>(sendBuffer) + sendLength,
        reinterpret_cast<SVTK_TT*>(recvBuffer) + offsets[0]));
    }
    return 1;
  }

  // I found another bug with Allgatherv where it sometimes fails to send data
  // to all processes when one process is sending nothing.  That is not
  // sufficient and I have not figured out what else needs to occur (although
  // the TestMPIController test's random tries eventually catches it if you
  // run it a bunch).  I sent a report to the OpenMPI mailing list.  Hopefully
  // they will have it fixed in a future version.
  for (int i = 0; i < this->NumberOfProcesses; i++)
  {
    if (recvLengths[i] < 1)
    {
      this->Superclass::AllGatherVVoidArray(
        sendBuffer, recvBuffer, sendLength, recvLengths, offsets, type);
    }
  }
#endif // OPEN_MPI
  std::vector<int> mpiRecvLengths, mpiOffsets;
  mpiRecvLengths.resize(numProc);
  mpiOffsets.resize(numProc);
  for (int i = 0; i < numProc; i++)
  {
    if (!svtkMPICommunicatorCheckSize(recvLengths[i] + offsets[i]))
    {
      return 0;
    }
    mpiRecvLengths[i] = recvLengths[i];
    mpiOffsets[i] = offsets[i];
  }
  return CheckForMPIError(MPI_Allgatherv(const_cast<void*>(sendBuffer), sendLength, mpiType,
    recvBuffer, &mpiRecvLengths[0], &mpiOffsets[0], mpiType, *this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::ReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length,
  int type, int operation, int destProcessId)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  MPI_Op mpiOp;
  switch (operation)
  {
    case MAX_OP:
      mpiOp = MPI_MAX;
      break;
    case MIN_OP:
      mpiOp = MPI_MIN;
      break;
    case SUM_OP:
      mpiOp = MPI_SUM;
      break;
    case PRODUCT_OP:
      mpiOp = MPI_PROD;
      break;
    case LOGICAL_AND_OP:
      mpiOp = MPI_LAND;
      break;
    case BITWISE_AND_OP:
      mpiOp = MPI_BAND;
      break;
    case LOGICAL_OR_OP:
      mpiOp = MPI_LOR;
      break;
    case BITWISE_OR_OP:
      mpiOp = MPI_BOR;
      break;
    case LOGICAL_XOR_OP:
      mpiOp = MPI_LXOR;
      break;
    case BITWISE_XOR_OP:
      mpiOp = MPI_BXOR;
      break;
    default:
      svtkWarningMacro(<< "Operation number " << operation << " not supported.");
      return 0;
  }
  return CheckForMPIError(svtkMPICommunicatorReduceData(
    sendBuffer, recvBuffer, length, type, mpiOp, destProcessId, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::ReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length,
  int type, Operation* operation, int destProcessId)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  MPI_Op mpiOp;
  MPI_Op_create(svtkMPICommunicatorUserFunction, operation->Commutative(), &mpiOp);
  // Setting a static global variable like this is not thread safe, but I
  // cannot think of another alternative.
  CurrentOperation = operation;

  int res;
  res = CheckForMPIError(svtkMPICommunicatorReduceData(
    sendBuffer, recvBuffer, length, type, mpiOp, destProcessId, this->MPIComm->Handle));

  MPI_Op_free(&mpiOp);

  return res;
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::AllReduceVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int operation)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  MPI_Op mpiOp;
  switch (operation)
  {
    case MAX_OP:
      mpiOp = MPI_MAX;
      break;
    case MIN_OP:
      mpiOp = MPI_MIN;
      break;
    case SUM_OP:
      mpiOp = MPI_SUM;
      break;
    case PRODUCT_OP:
      mpiOp = MPI_PROD;
      break;
    case LOGICAL_AND_OP:
      mpiOp = MPI_LAND;
      break;
    case BITWISE_AND_OP:
      mpiOp = MPI_BAND;
      break;
    case LOGICAL_OR_OP:
      mpiOp = MPI_LOR;
      break;
    case BITWISE_OR_OP:
      mpiOp = MPI_BOR;
      break;
    case LOGICAL_XOR_OP:
      mpiOp = MPI_LXOR;
      break;
    case BITWISE_XOR_OP:
      mpiOp = MPI_BXOR;
      break;
    default:
      svtkWarningMacro(<< "Operation number " << operation << " not supported.");
      return 0;
  }
  return CheckForMPIError(svtkMPICommunicatorAllReduceData(
    sendBuffer, recvBuffer, length, type, mpiOp, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::AllReduceVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, Operation* operation)
{
  svtkMPICommunicatorDebugBarrier(this->MPIComm->Handle);
  MPI_Op mpiOp;
  MPI_Op_create(svtkMPICommunicatorUserFunction, operation->Commutative(), &mpiOp);
  // Setting a static global variable like this is not thread safe, but I
  // cannot think of another alternative.
  CurrentOperation = operation;

  int res;
  res = CheckForMPIError(svtkMPICommunicatorAllReduceData(
    sendBuffer, recvBuffer, length, type, mpiOp, this->MPIComm->Handle));

  MPI_Op_free(&mpiOp);

  return res;
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::WaitAll(const int count, Request requests[])
{
  if (count < 1)
  {
    return -1;
  }

  MPI_Request* r = new MPI_Request[count];
  for (int i = 0; i < count; ++i)
  {
    r[i] = requests[i].Req->Handle;
  }

  int rc = CheckForMPIError(MPI_Waitall(count, r, MPI_STATUSES_IGNORE));
  delete[] r;
  return rc;
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::WaitAny(const int count, Request requests[], int& idx)
{
  if (count < 1)
  {
    return 0;
  }

  MPI_Request* r = new MPI_Request[count];
  for (int i = 0; i < count; ++i)
  {
    r[i] = requests[i].Req->Handle;
  }

  int rc = CheckForMPIError(MPI_Waitany(count, r, &idx, MPI_STATUS_IGNORE));
  assert("post: index from MPI_Waitany is out-of-bounds!" && (idx >= 0) && (idx < count));
  delete[] r;
  return (rc);
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::WaitSome(
  const int count, Request requests[], int& NCompleted, int* completed)
{
  if (count < 1)
  {
    return 0;
  }

  MPI_Request* r = new MPI_Request[count];
  for (int i = 0; i < count; ++i)
  {
    r[i] = requests[i].Req->Handle;
  }

  int rc = CheckForMPIError(MPI_Waitsome(count, r, &NCompleted, completed, MPI_STATUSES_IGNORE));
  delete[] r;
  return (rc);
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::TestAll(const int count, svtkMPICommunicator::Request requests[], int& flag)
{
  if (count < 1)
  {
    flag = 0;
    return 0;
  }

  MPI_Request* r = new MPI_Request[count];
  for (int i = 0; i < count; ++i)
  {
    r[i] = requests[i].Req->Handle;
  }

  int rc = CheckForMPIError(MPI_Testall(count, r, &flag, MPI_STATUSES_IGNORE));
  delete[] r;
  return (rc);
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::TestAny(
  const int count, svtkMPICommunicator::Request requests[], int& idx, int& flag)
{
  if (count < 1)
  {
    flag = 0;
    return 0;
  }

  MPI_Request* r = new MPI_Request[count];
  for (int i = 0; i < count; ++i)
  {
    r[i] = requests[i].Req->Handle;
  }

  int rc = CheckForMPIError(MPI_Testany(count, r, &idx, &flag, MPI_STATUS_IGNORE));
  delete[] r;
  return (rc);
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::TestSome(
  const int count, Request requests[], int& NCompleted, int* completed)
{
  if (count < 1)
  {
    NCompleted = 0;
    return 0;
  }

  MPI_Request* r = new MPI_Request[count];
  for (int i = 0; i < count; ++i)
  {
    r[i] = requests[i].Req->Handle;
  }

  int rc = CheckForMPIError(MPI_Testsome(count, r, &NCompleted, completed, MPI_STATUSES_IGNORE));
  delete[] r;
  return (rc);
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::Iprobe(int source, int tag, int* flag, int* actualSource)
{
  return CheckForMPIError(svtkMPICommunicatorIprobe(
    source, tag, flag, actualSource, MPI_INT, NULL, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource, int* svtkNotUsed(type), int* size)
{
  return CheckForMPIError(svtkMPICommunicatorIprobe(
    source, tag, flag, actualSource, MPI_INT, size, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource, unsigned long* svtkNotUsed(type), int* size)
{
  return CheckForMPIError(svtkMPICommunicatorIprobe(
    source, tag, flag, actualSource, MPI_UNSIGNED_LONG, size, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource, const char* svtkNotUsed(type), int* size)
{
  return CheckForMPIError(svtkMPICommunicatorIprobe(
    source, tag, flag, actualSource, MPI_CHAR, size, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource, float* svtkNotUsed(type), int* size)
{
  return CheckForMPIError(svtkMPICommunicatorIprobe(
    source, tag, flag, actualSource, MPI_FLOAT, size, this->MPIComm->Handle));
}

//-----------------------------------------------------------------------------
int svtkMPICommunicator::Iprobe(
  int source, int tag, int* flag, int* actualSource, double* svtkNotUsed(type), int* size)
{
  return CheckForMPIError(svtkMPICommunicatorIprobe(
    source, tag, flag, actualSource, MPI_DOUBLE, size, this->MPIComm->Handle));
}
