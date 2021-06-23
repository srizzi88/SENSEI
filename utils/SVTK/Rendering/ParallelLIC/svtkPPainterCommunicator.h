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
/**
 * @class   svtkPainterCommunicator
 * ranks that will execute a painter chain.
 *
 *
 * A communicator that can safely be used inside a painter.
 * A simple container holding an MPI communicator. The simple API
 * is sufficient to allow serial code (no MPI available) to steer
 * execution.
 */

#ifndef svtkPPainterCommunicator_h
#define svtkPPainterCommunicator_h

#include "svtkPainterCommunicator.h"
#include "svtkRenderingParallelLICModule.h" // for export macro

class svtkPPainterCommunicatorInternals;
class svtkMPICommunicatorOpaqueComm;

class SVTKRENDERINGPARALLELLIC_EXPORT svtkPPainterCommunicator : public svtkPainterCommunicator
{
public:
  svtkPPainterCommunicator();
  virtual ~svtkPPainterCommunicator();

  /**
   * Copier and assignment operators.
   */
  svtkPPainterCommunicator(const svtkPPainterCommunicator& other)
    : svtkPainterCommunicator(other)
  {
    this->Copy(&other, false);
  }

  svtkPPainterCommunicator& operator=(const svtkPPainterCommunicator& other)
  {
    this->Copy(&other, false);
    return *this;
  }

  /**
   * Copy the communicator.
   */
  virtual void Copy(const svtkPainterCommunicator* other, bool ownership);

  /**
   * Duplicate the communicator.
   */
  virtual void Duplicate(const svtkPainterCommunicator* other);

  //@{
  /**
   * Querry MPI for information about the communicator.
   */
  virtual int GetRank();
  virtual int GetSize();
  virtual bool GetIsNull();
  //@}

  //@{
  /**
   * Querry MPI for information about the world communicator.
   */
  virtual int GetWorldRank();
  virtual int GetWorldSize();
  //@}

  /**
   * Querry MPI state.
   */
  virtual bool GetMPIInitialized() { return this->MPIInitialized(); }
  virtual bool GetMPIFinalized() { return this->MPIFinalized(); }

  static bool MPIInitialized();
  static bool MPIFinalized();

  //@{
  /**
   * Set/Get the communicator. Ownership is not assumed
   * thus caller must keep the commuicator alive while
   * this class is in use and free the communicator when
   * finished.
   */
  void SetCommunicator(svtkMPICommunicatorOpaqueComm* comm);
  void GetCommunicator(svtkMPICommunicatorOpaqueComm* comm);
  void* GetCommunicator();
  //@}

  /**
   * Creates a new communicator with/without the calling processes
   * as indicated by the passed in flag, if not 0 the calling process
   * is included in the new communicator. The new communicator is
   * accessed via GetCommunicator. In parallel this call is mpi
   * collective on the world communicator. In serial this is a no-op.
   */
  void SubsetCommunicator(svtkMPICommunicatorOpaqueComm* comm, int include);

  /**
   * Get SVTK's world communicator. Return's a null communictor if
   * MPI was not yet initialized.
   */
  static svtkMPICommunicatorOpaqueComm* GetGlobalCommunicator();

private:
  // PImpl for MPI datatypes
  svtkPPainterCommunicatorInternals* Internals;
};

#endif
// SVTK-HeaderTest-Exclude: svtkPPainterCommunicator.h
