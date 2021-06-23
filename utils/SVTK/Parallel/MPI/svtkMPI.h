/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkMPI_h
#define svtkMPI_h
#ifndef __SVTK_WRAP__

#ifndef USE_STDARG
#define USE_STDARG
#include "svtk_mpi.h"
#undef USE_STDARG
#else
#include "svtk_mpi.h"
#endif

#include "svtkParallelMPIModule.h" // For export macro
#include "svtkSystemIncludes.h"

class SVTKPARALLELMPI_EXPORT svtkMPICommunicatorOpaqueComm
{
public:
  svtkMPICommunicatorOpaqueComm(MPI_Comm* handle = 0);

  MPI_Comm* GetHandle();

  friend class svtkMPICommunicator;
  friend class svtkMPIController;

protected:
  MPI_Comm* Handle;
};

class SVTKPARALLELMPI_EXPORT svtkMPICommunicatorReceiveDataInfo
{
public:
  svtkMPICommunicatorReceiveDataInfo() { this->Handle = 0; }
  MPI_Datatype DataType;
  MPI_Status Status;
  MPI_Comm* Handle;
};

class SVTKPARALLELMPI_EXPORT svtkMPIOpaqueFileHandle
{
public:
  svtkMPIOpaqueFileHandle()
    : Handle(MPI_FILE_NULL)
  {
  }
  MPI_File Handle;
};

//-----------------------------------------------------------------------------
class svtkMPICommunicatorOpaqueRequest
{
public:
  MPI_Request Handle;
};

#endif
#endif // svtkMPI_h
// SVTK-HeaderTest-Exclude: svtkMPI.h
