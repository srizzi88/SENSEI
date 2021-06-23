/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPHardwareSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPHardwareSelector.h"

#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

class svtkPHardwareSelector::svtkObserver : public svtkCommand
{
public:
  static svtkObserver* New() { return new svtkObserver(); }
  void Execute(svtkObject*, unsigned long eventId, void*) override
  {
    if (eventId == svtkCommand::StartEvent)
    {
      this->Target->StartRender();
    }
    else if (eventId == svtkCommand::EndEvent)
    {
      this->Target->EndRender();
    }
  }
  svtkPHardwareSelector* Target;
};

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPHardwareSelector);

//----------------------------------------------------------------------------
svtkPHardwareSelector::svtkPHardwareSelector()
{
  this->ProcessIsRoot = false;
  this->Observer = svtkObserver::New();
  this->Observer->Target = this;
}

//----------------------------------------------------------------------------
svtkPHardwareSelector::~svtkPHardwareSelector()
{
  this->Observer->Target = nullptr;
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
bool svtkPHardwareSelector::CaptureBuffers()
{
  if (this->ProcessIsRoot)
  {
    return this->Superclass::CaptureBuffers();
  }

  this->InvokeEvent(svtkCommand::StartEvent);
  this->BeginSelection();
  svtkRenderWindow* rwin = this->Renderer->GetRenderWindow();
  rwin->AddObserver(svtkCommand::StartEvent, this->Observer);
  rwin->AddObserver(svtkCommand::EndEvent, this->Observer);

  for (this->CurrentPass = MIN_KNOWN_PASS; this->CurrentPass < MAX_KNOWN_PASS; this->CurrentPass++)
  {
    if (this->PassRequired(this->CurrentPass))
    {
      break;
    }
  }

  if (this->CurrentPass == MAX_KNOWN_PASS)
  {
    this->EndRender();
  }
  return false;
}

//----------------------------------------------------------------------------
void svtkPHardwareSelector::StartRender() {}

//----------------------------------------------------------------------------
void svtkPHardwareSelector::EndRender()
{
  this->CurrentPass++;
  for (; this->CurrentPass < MAX_KNOWN_PASS; this->CurrentPass++)
  {
    if (this->PassRequired(this->CurrentPass))
    {
      break;
    }
  }

  if (this->CurrentPass >= MAX_KNOWN_PASS)
  {
    svtkRenderWindow* rwin = this->Renderer->GetRenderWindow();
    rwin->RemoveObserver(this->Observer);
    this->EndSelection();
    this->InvokeEvent(svtkCommand::EndEvent);
  }
}

//----------------------------------------------------------------------------
void svtkPHardwareSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ProcessIsRoot: " << this->ProcessIsRoot << endl;
}
