/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDummyController.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDummyController
 * @brief   Dummy controller for single process applications
 *
 * This is a dummy controller which can be used by applications which always
 * require a controller but are also compile on systems without threads
 * or mpi.
 * @sa
 * svtkMultiProcessController
 */

#ifndef svtkDummyController_h
#define svtkDummyController_h

#include "svtkMultiProcessController.h"
#include "svtkParallelCoreModule.h" // For export macro

class SVTKPARALLELCORE_EXPORT svtkDummyController : public svtkMultiProcessController
{
public:
  static svtkDummyController* New();
  svtkTypeMacro(svtkDummyController, svtkMultiProcessController);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method is for setting up the processes.
   */
  void Initialize(int*, char***, int) override {}
  void Initialize(int*, char***) override {}
  void Finalize() override {}
  void Finalize(int) override {}

  /**
   * This method always returns 0.
   */
  int GetLocalProcessId() { return 0; }

  /**
   * Directly calls the single method.
   */
  void SingleMethodExecute() override;

  /**
   * Directly calls multiple method 0.
   */
  void MultipleMethodExecute() override;

  /**
   * Does nothing.
   */
  void CreateOutputWindow() override {}

  //@{
  /**
   * If you don't need any special functionality from the controller, you
   * can swap out the dummy communicator for another one.
   */
  svtkGetObjectMacro(RMICommunicator, svtkCommunicator);
  virtual void SetCommunicator(svtkCommunicator*);
  virtual void SetRMICommunicator(svtkCommunicator*);
  //@}

protected:
  svtkDummyController();
  ~svtkDummyController() override;

private:
  svtkDummyController(const svtkDummyController&) = delete;
  void operator=(const svtkDummyController&) = delete;
};

#endif
