/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericRenderWindowInteractor.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkGenericRenderWindowInteractor);

//------------------------------------------------------------------
svtkGenericRenderWindowInteractor::svtkGenericRenderWindowInteractor()
{
  this->TimerEventResetsTimer = 1;
}

//------------------------------------------------------------------
svtkGenericRenderWindowInteractor::~svtkGenericRenderWindowInteractor() = default;

//------------------------------------------------------------------
void svtkGenericRenderWindowInteractor::TimerEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  int timerId = this->GetCurrentTimerId();
  this->InvokeEvent(svtkCommand::TimerEvent, &timerId);

  if (!this->IsOneShotTimer(timerId) && this->GetTimerEventResetsTimer())
  {
    this->ResetTimer(timerId);
  }
}

//------------------------------------------------------------------
int svtkGenericRenderWindowInteractor::InternalCreateTimer(
  int timerId, int timerType, unsigned long duration)
{
  if (this->HasObserver(svtkCommand::CreateTimerEvent))
  {
    this->SetTimerEventId(timerId);
    this->SetTimerEventType(timerType);
    this->SetTimerEventDuration(duration);
    this->SetTimerEventPlatformId(timerId);
    this->InvokeEvent(svtkCommand::CreateTimerEvent, &timerId);
    return this->GetTimerEventPlatformId();
  }
  return 0;
}

//------------------------------------------------------------------
int svtkGenericRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  if (this->HasObserver(svtkCommand::DestroyTimerEvent))
  {
    this->SetTimerEventPlatformId(platformTimerId);
    this->InvokeEvent(svtkCommand::DestroyTimerEvent, &platformTimerId);
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------
void svtkGenericRenderWindowInteractor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TimerEventResetsTimer: " << this->TimerEventResetsTimer << "\n";
}
