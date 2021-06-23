/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericRenderWindowInteractor
 * @brief   platform-independent programmable render window interactor.
 *
 *
 * svtkGenericRenderWindowInteractor provides a way to translate native
 * mouse and keyboard events into svtk Events.   By calling the methods on
 * this class, svtk events will be invoked.   This will allow scripting
 * languages to use svtkInteractorStyles and 3D widgets.
 */

#ifndef svtkGenericRenderWindowInteractor_h
#define svtkGenericRenderWindowInteractor_h

#include "svtkRenderWindowInteractor.h"
#include "svtkRenderingUIModule.h" // For export macro

class SVTKRENDERINGUI_EXPORT svtkGenericRenderWindowInteractor : public svtkRenderWindowInteractor
{
public:
  static svtkGenericRenderWindowInteractor* New();
  svtkTypeMacro(svtkGenericRenderWindowInteractor, svtkRenderWindowInteractor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Fire TimerEvent. SetEventInformation should be called just prior
   * to calling any of these methods. These methods will Invoke the
   * corresponding svtk event.
   */
  virtual void TimerEvent();

  //@{
  /**
   * Flag that indicates whether the TimerEvent method should call ResetTimer
   * to simulate repeating timers with an endless stream of one shot timers.
   * By default this flag is on and all repeating timers are implemented as a
   * stream of sequential one shot timers. If the observer of
   * CreateTimerEvent actually creates a "natively repeating" timer, setting
   * this flag to off will prevent (perhaps many many) unnecessary calls to
   * ResetTimer. Having the flag on by default means that "natively one
   * shot" timers can be either one shot or repeating timers with no
   * additional work. Also, "natively repeating" timers still work with the
   * default setting, but with potentially many create and destroy calls.
   */
  svtkSetMacro(TimerEventResetsTimer, svtkTypeBool);
  svtkGetMacro(TimerEventResetsTimer, svtkTypeBool);
  svtkBooleanMacro(TimerEventResetsTimer, svtkTypeBool);
  //@}

protected:
  svtkGenericRenderWindowInteractor();
  ~svtkGenericRenderWindowInteractor() override;

  //@{
  /**
   * Generic internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  //@}

  svtkTypeBool TimerEventResetsTimer;

private:
  svtkGenericRenderWindowInteractor(const svtkGenericRenderWindowInteractor&) = delete;
  void operator=(const svtkGenericRenderWindowInteractor&) = delete;
};

#endif
