/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPProgressObserver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSMPProgressObserver
 * @brief   Progress observer that is thread safe
 *
 * svtkSMPProgressObserver is designed to handle progress events coming
 * from an algorithm in a thread safe way. It does this by using
 * thread local objects that it updates. To receive the progress
 * information, one has to listen to the local observer in the same
 * thread. Since the execution will be somewhat load balanced,
 * it may be enough to do this only on the main thread.
 */

#ifndef svtkSMPProgressObserver_h
#define svtkSMPProgressObserver_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkProgressObserver.h"
#include "svtkSMPThreadLocalObject.h" // For thread local observers.

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkSMPProgressObserver : public svtkProgressObserver
{
public:
  static svtkSMPProgressObserver* New();
  svtkTypeMacro(svtkSMPProgressObserver, svtkProgressObserver);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Passes the progress event to a thread local ProgressObserver
   * instance.
   */
  void UpdateProgress(double amount) override;

  /**
   * Returns the progress observer local to the thread it was
   * called from.
   */
  svtkProgressObserver* GetLocalObserver() { return this->Observers.Local(); }

protected:
  svtkSMPProgressObserver();
  ~svtkSMPProgressObserver() override;

  svtkSMPThreadLocalObject<svtkProgressObserver> Observers;

private:
  svtkSMPProgressObserver(const svtkSMPProgressObserver&) = delete;
  void operator=(const svtkSMPProgressObserver&) = delete;
};

#endif
