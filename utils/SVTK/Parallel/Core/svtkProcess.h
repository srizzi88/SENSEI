/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProcess.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProcess
 * @brief   a process that can be launched by a svtkMultiProcessController
 *
 * svtkProcess is an abstract class representing a process that can be launched
 * by a svtkMultiProcessController. Concrete classes just have to implement
 * Execute() method and make sure it set the proper value in ReturnValue.
 *
 * @par Example:
 *  class MyProcess: public svtkProcess
 *  ...
 *  svtkMultiProcessController *c;
 *  MyProcess *p=new MyProcess::New();
 *  p->SetArgs(argc,argv); // some parameters specific to the process
 *  p->SetX(10.0); // ...
 *  c->SetSingleProcess(p);
 *  c->SingleMethodExecute();
 *  int returnValue=p->GetReturnValue();
 *
 * @sa
 * svtkMultiProcessController
 */

#ifndef svtkProcess_h
#define svtkProcess_h

#include "svtkObject.h"
#include "svtkParallelCoreModule.h" // For export macro

class svtkMultiProcessController;

class SVTKPARALLELCORE_EXPORT svtkProcess : public svtkObject
{
public:
  svtkTypeMacro(svtkProcess, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Entry point of the process.
   * This method is expected to update ReturnValue.
   */
  virtual void Execute() = 0;

  /**
   * Give access to the controller that launched the process.
   * Initial value is nullptr.
   */
  svtkMultiProcessController* GetController();

  /**
   * This method should not be called directly but set by the controller
   * itself.
   */
  void SetController(svtkMultiProcessController* aController);

  /**
   * Value set at the end of a call to Execute.
   */
  int GetReturnValue();

protected:
  svtkProcess();

  svtkMultiProcessController* Controller;
  int ReturnValue;

private:
  svtkProcess(const svtkProcess&) = delete;
  void operator=(const svtkProcess&) = delete;
};

#endif
