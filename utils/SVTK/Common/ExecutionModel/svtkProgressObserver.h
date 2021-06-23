/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgressObserver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProgressObserver
 * @brief   Basic class to optionally replace svtkAlgorithm progress functionality.
 *
 * When the basic functionality in svtkAlgorithm that reports progress is
 * not enough, a subclass of svtkProgressObserver can be used to provide
 * custom functionality.
 * The main use case for this is when an algorithm's RequestData() is
 * called from multiple threads in parallel - the basic functionality in
 * svtkAlgorithm is not thread safe. svtkSMPProgressObserver can
 * handle this situation by routing progress from each thread to a
 * thread local svtkProgressObserver, which will invoke events separately
 * for each thread.
 */

#ifndef svtkProgressObserver_h
#define svtkProgressObserver_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkObject.h"

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkProgressObserver : public svtkObject
{
public:
  static svtkProgressObserver* New();
  svtkTypeMacro(svtkProgressObserver, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The default behavior is to update the Progress data member
   * and invoke a ProgressEvent. This is designed to be overwritten.
   */
  virtual void UpdateProgress(double amount);

  //@{
  /**
   * Returns the progress reported by the algorithm.
   */
  svtkGetMacro(Progress, double);
  //@}

protected:
  svtkProgressObserver();
  ~svtkProgressObserver() override;

  double Progress;

private:
  svtkProgressObserver(const svtkProgressObserver&) = delete;
  void operator=(const svtkProgressObserver&) = delete;
};

#endif
