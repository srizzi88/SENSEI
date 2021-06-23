/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPI4PyCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMPI4PyCommunicator
 * @brief   Class for bridging MPI4Py with svtkMPICommunicator.
 *
 *
 * This class can be used to convert between SVTK and MPI4Py communicators.
 *
 * @sa
 * svtkMPICommunicator
 */

#ifndef svtkMPI4PyCommunicator_h
#define svtkMPI4PyCommunicator_h
#if !defined(__SVTK_WRAP__) || defined(__SVTK_WRAP_PYTHON__)

#include "svtkPython.h" // For PyObject*; must be first

#include "svtkObject.h"
#include "svtkParallelMPI4PyModule.h" // For export macro

class svtkMPICommunicator;

class SVTKPARALLELMPI4PY_EXPORT svtkMPI4PyCommunicator : public svtkObject
{
public:
  svtkTypeMacro(svtkMPI4PyCommunicator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkMPI4PyCommunicator* New();
  svtkMPI4PyCommunicator();

  /**
   * Convert a SVTK communicator into an mpi4py communicator.
   */
  static PyObject* ConvertToPython(svtkMPICommunicator* comm);

  /**
   * Convert an mpi4py communicator into a SVTK communicator.
   */
  static svtkMPICommunicator* ConvertToSVTK(PyObject* comm);

private:
  svtkMPI4PyCommunicator(const svtkMPI4PyCommunicator&) = delete;
  void operator=(const svtkMPI4PyCommunicator&) = delete;
};

#endif
#endif
