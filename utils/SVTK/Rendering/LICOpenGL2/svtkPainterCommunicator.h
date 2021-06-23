/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPainterCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkPainterCommunicator
 * @brief A communicator that can safely be used inside a painter.
 *
 * A simple container holding a handle to an MPI communicator.
 * This API is sufficient to allow for control flow with/without
 * MPI. The parallel parts of the code should use the derived
 * class svtkPPainterCommunicator.
 */

#ifndef svtkPainterCommunicator_h
#define svtkPainterCommunicator_h

#include "svtkRenderingLICOpenGL2Module.h" // for export macro

class SVTKRENDERINGLICOPENGL2_EXPORT svtkPainterCommunicator
{
public:
  svtkPainterCommunicator() {}
  virtual ~svtkPainterCommunicator() {}

  /**
   * Copy and assignment operators. Both use Copy internally
   * and do take ownership.
   */
  svtkPainterCommunicator(const svtkPainterCommunicator& other) { this->Copy(&other, false); }

  svtkPainterCommunicator& operator=(const svtkPainterCommunicator& other)
  {
    this->Copy(&other, false);
    return *this;
  }

  /**
   * Copy the communicator, the flag indicates if ownership
   * should be assumed. The owner is responsible for free'ing
   * the communicator.
   */
  virtual void Copy(const svtkPainterCommunicator*, bool) {}

  /**
   * Duplicate the communicator.
   */
  virtual void Duplicate(const svtkPainterCommunicator*) {}

  /**
   * Querry MPI about the communicator.
   */
  virtual int GetRank() { return 0; }
  virtual int GetSize() { return 1; }
  virtual bool GetIsNull() { return false; }

  /**
   * Querry MPI about the world communicator.
   */
  virtual int GetWorldRank() { return 0; }
  virtual int GetWorldSize() { return 1; }

  /**
   * Querry MPI about its state.
   */
  virtual bool GetMPIInitialized() { return false; }
  virtual bool GetMPIFinalized() { return true; }
};

#endif
// SVTK-HeaderTest-Exclude: svtkPainterCommunicator.h
