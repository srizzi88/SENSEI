/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkExecutionTimer
 * @brief   Time filter execution
 *
 *
 *
 * This object monitors a single filter for StartEvent and EndEvent.
 * Each time it hears StartEvent it records the time.  Each time it
 * hears EndEvent it measures the elapsed time (both CPU and
 * wall-clock) since the most recent StartEvent.  Internally we use
 * svtkTimerLog for measurements.
 *
 * By default we simply store the elapsed time.  You are welcome to
 * subclass and override TimerFinished() to do anything you want.
 */

#ifndef svtkExecutionTimer_h
#define svtkExecutionTimer_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkObject.h"

class svtkAlgorithm;
class svtkCallbackCommand;

class SVTKFILTERSCORE_EXPORT svtkExecutionTimer : public svtkObject
{
public:
  svtkTypeMacro(svtkExecutionTimer, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a new timer with no attached filter.  Use SetFilter()
   * to specify the svtkAlgorithm whose execution you want to time.
   */
  static svtkExecutionTimer* New();

  //@{
  /**
   * Set/get the filter to be monitored.  The only real constraint
   * here is that the svtkExecutive associated with the filter must
   * fire StartEvent and EndEvent before and after the filter is
   * executed.  All SVTK executives should do this.
   */
  void SetFilter(svtkAlgorithm* filter);
  svtkGetObjectMacro(Filter, svtkAlgorithm);
  //@}

  //@{
  /**
   * Get the total CPU time (in seconds) that elapsed between
   * StartEvent and EndEvent.  This is undefined before the filter has
   * finished executing.
   */
  svtkGetMacro(ElapsedCPUTime, double);
  //@}

  //@{
  /**
   * Get the total wall clock time (in seconds) that elapsed between
   * StartEvent and EndEvent.  This is undefined before the filter has
   * finished executing.
   */
  svtkGetMacro(ElapsedWallClockTime, double);
  //@}

protected:
  svtkExecutionTimer();
  ~svtkExecutionTimer() override;

  // This is the observer that will catch StartEvent and hand off to
  // EventRelay
  svtkCallbackCommand* Callback;

  // This is the filter that will be timed
  svtkAlgorithm* Filter;

  // These are where we keep track of the timestamps for start/end
  double CPUStartTime;
  double CPUEndTime;

  double WallClockStartTime;
  double WallClockEndTime;

  double ElapsedCPUTime;
  double ElapsedWallClockTime;

  //@{
  /**
   * Convenience functions -- StartTimer clears out the elapsed times
   * and records start times; StopTimer records end times and computes
   * the elapsed time
   */
  void StartTimer();
  void StopTimer();
  //@}

  /**
   * This is where you can do anything you want with the progress
   * event.  By default this does nothing.
   */
  virtual void TimerFinished();

  /**
   * This is the callback that SVTK will invoke when it sees StartEvent
   * and EndEvent.  Its responsibility is to pass the event on to an
   * instance of this observer class.
   */
  static void EventRelay(
    svtkObject* caller, unsigned long eventId, void* clientData, void* callData);

private:
  svtkExecutionTimer(const svtkExecutionTimer&) = delete;
  void operator=(const svtkExecutionTimer&) = delete;
};

#endif
