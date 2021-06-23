// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProcessGroup.h

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
 * @class   svtkProcessGroup
 * @brief   A subgroup of processes from a communicator.
 *
 *
 *
 * This class is used for creating groups of processes.  A svtkProcessGroup is
 * initialized by passing the controller or communicator on which the group is
 * based off of.  You can then use the group to subset and reorder the
 * processes.  Eventually, you can pass the group object to the
 * CreateSubController method of svtkMultiProcessController to create a
 * controller for the defined group of processes.  You must use the same
 * controller (or attached communicator) from which this group was initialized
 * with.
 *
 * @sa
 * svtkMultiProcessController, svtkCommunicator
 *
 * @par Thanks:
 * This class was originally written by Kenneth Moreland (kmorel@sandia.gov)
 * from Sandia National Laboratories.
 *
 */

#ifndef svtkProcessGroup_h
#define svtkProcessGroup_h

#include "svtkObject.h"
#include "svtkParallelCoreModule.h" // For export macro

class svtkMultiProcessController;
class svtkCommunicator;

class SVTKPARALLELCORE_EXPORT svtkProcessGroup : public svtkObject
{
public:
  svtkTypeMacro(svtkProcessGroup, svtkObject);
  static svtkProcessGroup* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Initialize the group to the given controller or communicator.  The group
   * will be set to contain all of the processes in the controller/communicator
   * in the same order.
   */
  void Initialize(svtkMultiProcessController* controller);
  void Initialize(svtkCommunicator* communicator);
  //@}

  //@{
  /**
   * Get the communicator on which this group is based on.
   */
  svtkGetObjectMacro(Communicator, svtkCommunicator);
  //@}

  /**
   * Set the communicator.  This has the same effect as Initialize except that
   * the contents of the group will not be modified (although they may be
   * truncated if the new communicator is smaller than the current group).
   * Note that this can lead to an invalid group if there are values in the
   * group that are not valid in the new communicator.
   */
  void SetCommunicator(svtkCommunicator* communicator);

  //@{
  /**
   * Returns the size of this group (the number of processes defined in it).
   */
  svtkGetMacro(NumberOfProcessIds, int);
  //@}

  /**
   * Given a position in the group, returns the id of the process in the
   * communicator this group is based on.  For example, if this group contains
   * {6, 2, 8, 1}, then GetProcessId(2) will return 8 and GetProcessId(3) will
   * return 1.
   */
  int GetProcessId(int pos) { return this->ProcessIds[pos]; }

  /**
   * Get the process id for the local process (as defined by the group's
   * communicator).  Returns -1 if the local process is not in the group.
   */
  int GetLocalProcessId();

  /**
   * Given a process id in the communicator, this method returns its location in
   * the group or -1 if it is not in the group.  For example, if this group
   * contains {6, 2, 8, 1}, then FindProcessId(2) will return 1 and
   * FindProcessId(3) will return -1.
   */
  int FindProcessId(int processId);

  /**
   * Add a process id to the end of the group (if it is not already in the
   * group).  Returns the location where the id was stored.
   */
  int AddProcessId(int processId);

  /**
   * Remove the given process id from the group (assuming it is in the group).
   * All ids to the "right" of the removed id are shifted over.  Returns 1
   * if the process id was removed, 0 if the process id was not in the group
   * in the first place.
   */
  int RemoveProcessId(int processId);

  /**
   * Removes all the processes ids from the group, leaving the group empty.
   */
  void RemoveAllProcessIds();

  /**
   * Copies the given group's communicator and process ids.
   */
  void Copy(svtkProcessGroup* group);

protected:
  svtkProcessGroup();
  ~svtkProcessGroup() override;

  int* ProcessIds;
  int NumberOfProcessIds;

  svtkCommunicator* Communicator;

private:
  svtkProcessGroup(const svtkProcessGroup&) = delete;
  void operator=(const svtkProcessGroup&) = delete;
};

#endif // svtkProcessGroup_h
