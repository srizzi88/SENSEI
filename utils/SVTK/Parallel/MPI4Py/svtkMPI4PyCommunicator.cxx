/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPI4PyCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMPI4PyCommunicator.h"

#include "svtkMPI.h"
#include "svtkMPICommunicator.h"
#include "svtkObjectFactory.h"

#include <mpi4py/mpi4py.h>

svtkStandardNewMacro(svtkMPI4PyCommunicator);

//----------------------------------------------------------------------------
svtkMPI4PyCommunicator::svtkMPI4PyCommunicator() {}

//----------------------------------------------------------------------------
PyObject* svtkMPI4PyCommunicator::ConvertToPython(svtkMPICommunicator* comm)
{
  // Import mpi4py if it does not exist.
  if (!PyMPIComm_New)
  {
    if (import_mpi4py() < 0)
    {
      Py_RETURN_NONE;
    }
  }

  if (!comm || !comm->GetMPIComm())
  {
    Py_RETURN_NONE;
  }

  return PyMPIComm_New(*comm->GetMPIComm()->GetHandle());
}

//----------------------------------------------------------------------------
svtkMPICommunicator* svtkMPI4PyCommunicator::ConvertToSVTK(PyObject* comm)
{
  // Import mpi4py if it does not exist.
  if (!PyMPIComm_Get)
  {
    if (import_mpi4py() < 0)
    {
      return nullptr;
    }
  }

  if (!comm || !PyObject_TypeCheck(comm, &PyMPIComm_Type))
  {
    return nullptr;
  }

  MPI_Comm* mpiComm = PyMPIComm_Get(comm);
  svtkMPICommunicator* svtkComm = svtkMPICommunicator::New();
  svtkMPICommunicatorOpaqueComm opaqueComm(mpiComm);
  if (!svtkComm->InitializeExternal(&opaqueComm))
  {
    svtkComm->Delete();
    return nullptr;
  }

  return svtkComm;
}

//----------------------------------------------------------------------------
void svtkMPI4PyCommunicator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
