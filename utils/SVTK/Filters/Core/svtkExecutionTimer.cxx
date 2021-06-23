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

#include "svtkExecutionTimer.h"

#include "svtkAlgorithm.h"
#include "svtkCallbackCommand.h"
#include "svtkObjectFactory.h"
#include "svtkTimerLog.h"
#include <iostream>

svtkStandardNewMacro(svtkExecutionTimer);

// ----------------------------------------------------------------------

svtkExecutionTimer::svtkExecutionTimer()
{
  this->Filter = nullptr;
  this->Callback = svtkCallbackCommand::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(svtkExecutionTimer::EventRelay);

  this->CPUStartTime = 0;
  this->CPUEndTime = 0;
  this->ElapsedCPUTime = 0;
  this->WallClockStartTime = 0;
  this->WallClockEndTime = 0;
  this->ElapsedWallClockTime = 0;
}

// ----------------------------------------------------------------------

svtkExecutionTimer::~svtkExecutionTimer()
{
  this->SetFilter(nullptr);
  this->Callback->Delete();
}

// ----------------------------------------------------------------------

void svtkExecutionTimer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Observed Filter: ";
  if (this->Filter)
  {
    os << "\n";
    this->Filter->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }
  os << indent << "Most recent CPU start time: " << this->CPUStartTime << "\n";
  os << indent << "Most recent CPU end time: " << this->CPUStartTime << "\n";
  os << indent << "Most recent CPU elapsed time: " << this->ElapsedCPUTime << "\n";

  os << indent << "Most recent wall clock start time: " << this->WallClockStartTime << "\n";
  os << indent << "Most recent wall clock end time: " << this->WallClockStartTime << "\n";
  os << indent << "Most recent wall clock elapsed time: " << this->WallClockEndTime << "\n";
}

// ----------------------------------------------------------------------

void svtkExecutionTimer::SetFilter(svtkAlgorithm* filter)
{
  if (this->Filter)
  {
    this->Filter->RemoveObserver(this->Callback);
    this->Filter->RemoveObserver(this->Callback);
    this->Filter->UnRegister(this);
    this->Filter = nullptr;
  }

  if (filter)
  {
    this->Filter = filter;
    this->Filter->Register(this);
    this->Filter->AddObserver(svtkCommand::StartEvent, this->Callback);
    this->Filter->AddObserver(svtkCommand::EndEvent, this->Callback);
  }
}

// ----------------------------------------------------------------------

void svtkExecutionTimer::EventRelay(svtkObject* svtkNotUsed(caller), unsigned long eventType,
  void* clientData, void* svtkNotUsed(callData))
{
  svtkExecutionTimer* receiver = static_cast<svtkExecutionTimer*>(clientData);

  if (eventType == svtkCommand::StartEvent)
  {
    receiver->StartTimer();
  }
  else if (eventType == svtkCommand::EndEvent)
  {
    receiver->StopTimer();
  }
  else
  {
    svtkGenericWarningMacro("WARNING: Unknown event type "
      << eventType << " in svtkExecutionTimer::EventRelay.  This shouldn't happen.");
  }
}

// ----------------------------------------------------------------------

void svtkExecutionTimer::StartTimer()
{
  this->CPUEndTime = 0;
  this->ElapsedCPUTime = 0;
  this->WallClockEndTime = 0;
  this->ElapsedWallClockTime = 0;

  this->WallClockStartTime = svtkTimerLog::GetUniversalTime();
  this->CPUStartTime = svtkTimerLog::GetCPUTime();
}

// ----------------------------------------------------------------------

void svtkExecutionTimer::StopTimer()
{
  this->WallClockEndTime = svtkTimerLog::GetUniversalTime();
  this->CPUEndTime = svtkTimerLog::GetCPUTime();

  this->ElapsedCPUTime = this->CPUEndTime - this->CPUStartTime;
  this->ElapsedWallClockTime = this->WallClockEndTime - this->WallClockStartTime;

  this->TimerFinished();
}

// ----------------------------------------------------------------------

void svtkExecutionTimer::TimerFinished()
{
  // Nothing to do here
}
