/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderTimerLog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRenderTimerLog.h"

#include "svtkObjectFactory.h"

#include <iomanip>
#include <utility>

svtkObjectFactoryNewMacro(svtkRenderTimerLog);

//------------------------------------------------------------------------------
svtkRenderTimerLog::svtkRenderTimerLog()
  : LoggingEnabled(false)
  , FrameLimit(32)
{
}

//------------------------------------------------------------------------------
svtkRenderTimerLog::~svtkRenderTimerLog() = default;

//------------------------------------------------------------------------------
void svtkRenderTimerLog::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool svtkRenderTimerLog::IsSupported()
{
  return false;
}

//------------------------------------------------------------------------------
void svtkRenderTimerLog::MarkFrame() {}

//------------------------------------------------------------------------------
svtkRenderTimerLog::ScopedEventLogger svtkRenderTimerLog::StartScopedEvent(const std::string& name)
{
  this->MarkStartEvent(name);
  return ScopedEventLogger(this);
}

//------------------------------------------------------------------------------
void svtkRenderTimerLog::MarkStartEvent(const std::string&) {}

//------------------------------------------------------------------------------
void svtkRenderTimerLog::MarkEndEvent() {}

//------------------------------------------------------------------------------
bool svtkRenderTimerLog::FrameReady()
{
  svtkWarningMacro("svtkRenderTimerLog unsupported for the current rendering "
                  "backend.");
  return false;
}

//------------------------------------------------------------------------------
svtkRenderTimerLog::Frame svtkRenderTimerLog::PopFirstReadyFrame()
{
  return Frame();
}

//------------------------------------------------------------------------------
void svtkRenderTimerLog::ReleaseGraphicsResources() {}

//------------------------------------------------------------------------------
svtkRenderTimerLog::ScopedEventLogger::ScopedEventLogger(ScopedEventLogger&& o)
  : Log(nullptr)
{
  std::swap(o.Log, this->Log);
}

//------------------------------------------------------------------------------
svtkRenderTimerLog::ScopedEventLogger& svtkRenderTimerLog::ScopedEventLogger::operator=(
  ScopedEventLogger&& o)
{
  std::swap(o.Log, this->Log);
  return *this;
}

//------------------------------------------------------------------------------
void svtkRenderTimerLog::ScopedEventLogger::Stop()
{
  if (this->Log)
  {
    this->Log->MarkEndEvent();
    this->Log = nullptr;
  }
}

//------------------------------------------------------------------------------
void svtkRenderTimerLog::Frame::Print(std::ostream& os, float threshMs)
{
  svtkIndent indent;
  for (auto event : this->Events)
  {
    event.Print(os, 0.f, threshMs, indent);
  }
}

//------------------------------------------------------------------------------
void svtkRenderTimerLog::Event::Print(
  std::ostream& os, float parentTime, float threshMs, svtkIndent indent)
{
  float thisTime = this->ElapsedTimeMilliseconds();
  if (thisTime < threshMs)
  {
    return;
  }

  float parentPercent = 100.f;
  if (parentTime > 0.f)
  {
    parentPercent = thisTime / parentTime * 100.f;
  }

  os << indent << "- " << std::fixed << std::setw(5) << std::setprecision(1) << parentPercent
     << std::setw(0) << "% " << std::setw(8) << std::setprecision(3) << thisTime << std::setw(0)
     << " ms \"" << this->Name << "\"\n";

  svtkIndent nextIndent = indent.GetNextIndent();
  for (auto event : this->Events)
  {
    event.Print(os, thisTime, threshMs, nextIndent);
  }
}
