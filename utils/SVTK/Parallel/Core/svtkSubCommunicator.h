// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSubCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkSubCommunicator
 * @brief   Provides communication on a process group.
 *
 *
 *
 * This class provides an implementation for communicating on process groups.
 * In general, you should never use this class directly.  Instead, use the
 * svtkMultiProcessController::CreateSubController method.
 *
 *
 * @bug
 * Because all communication is delegated to the original communicator,
 * any error will report process ids with respect to the original
 * communicator, not this communicator that was actually used.
 *
 * @sa
 * svtkCommunicator, svtkMultiProcessController
 *
 * @par Thanks:
 * This class was originally written by Kenneth Moreland (kmorel@sandia.gov)
 * from Sandia National Laboratories.
 *
 */

#ifndef svtkSubCommunicator_h
#define svtkSubCommunicator_h

#include "svtkCommunicator.h"
#include "svtkParallelCoreModule.h" // For export macro

class svtkProcessGroup;

class SVTKPARALLELCORE_EXPORT svtkSubCommunicator : public svtkCommunicator
{
public:
  svtkTypeMacro(svtkSubCommunicator, svtkCommunicator);
  static svtkSubCommunicator* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the group on which communication will happen.
   */
  svtkGetObjectMacro(Group, svtkProcessGroup);
  virtual void SetGroup(svtkProcessGroup* group);
  //@}

  //@{
  /**
   * Implementation for abstract supercalss.
   */
  int SendVoidArray(
    const void* data, svtkIdType length, int type, int remoteHandle, int tag) override;
  int ReceiveVoidArray(void* data, svtkIdType length, int type, int remoteHandle, int tag) override;
  //@}

protected:
  svtkSubCommunicator();
  ~svtkSubCommunicator() override;

  svtkProcessGroup* Group;

private:
  svtkSubCommunicator(const svtkSubCommunicator&) = delete;
  void operator=(const svtkSubCommunicator&) = delete;
};

#endif // svtkSubCommunicator_h
