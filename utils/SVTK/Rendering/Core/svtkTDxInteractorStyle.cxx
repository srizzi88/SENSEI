/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTDxInteractorStyle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTDxInteractorStyle.h"

#include "svtkCommand.h"
#include "svtkTDxInteractorStyleSettings.h"
#include "svtkTDxMotionEventInfo.h" // Borland needs it.

svtkCxxSetObjectMacro(svtkTDxInteractorStyle, Settings, svtkTDxInteractorStyleSettings);

// ----------------------------------------------------------------------------
svtkTDxInteractorStyle::svtkTDxInteractorStyle()
{
  this->Renderer = nullptr;
  this->Settings = svtkTDxInteractorStyleSettings::New();
}

// ----------------------------------------------------------------------------
svtkTDxInteractorStyle::~svtkTDxInteractorStyle()
{
  if (this->Settings != nullptr)
  {
    this->Settings->Delete();
  }
}

// ----------------------------------------------------------------------------
void svtkTDxInteractorStyle::ProcessEvent(svtkRenderer* renderer, unsigned long event, void* calldata)
{
  svtkDebugMacro(<< "svtkTDxInteractorStyle::ProcessEvent()");
  this->Renderer = renderer;

  svtkTDxMotionEventInfo* motionInfo;
  int* buttonInfo;

  switch (event)
  {
    case svtkCommand::TDxMotionEvent:
      svtkDebugMacro(<< "svtkTDxInteractorStyle::ProcessEvent() TDxMotionEvent");
      motionInfo = static_cast<svtkTDxMotionEventInfo*>(calldata);
      this->OnMotionEvent(motionInfo);
      break;
    case svtkCommand::TDxButtonPressEvent:
      svtkDebugMacro(<< "svtkTDxInteractorStyle::ProcessEvent() TDxButtonPressEvent");
      buttonInfo = static_cast<int*>(calldata);
      this->OnButtonPressedEvent(*buttonInfo);
      break;
    case svtkCommand::TDxButtonReleaseEvent:
      svtkDebugMacro(<< "svtkTDxInteractorStyle::ProcessEvent() TDxButtonReleaseEvent");
      buttonInfo = static_cast<int*>(calldata);
      this->OnButtonReleasedEvent(*buttonInfo);
      break;
  }
}

// ----------------------------------------------------------------------------
void svtkTDxInteractorStyle::OnMotionEvent(svtkTDxMotionEventInfo* svtkNotUsed(motionInfo))
{
  svtkDebugMacro(<< "svtkTDxInteractorStyle::OnMotionEvent()");
}

// ----------------------------------------------------------------------------
void svtkTDxInteractorStyle::OnButtonPressedEvent(int svtkNotUsed(button))
{
  svtkDebugMacro(<< "svtkTDxInteractorStyle::OnButtonPressedEvent()");
}

// ----------------------------------------------------------------------------
void svtkTDxInteractorStyle::OnButtonReleasedEvent(int svtkNotUsed(button))
{
  svtkDebugMacro(<< "svtkTDxInteractorStyle::OnButtonReleasedEvent()");
}

//----------------------------------------------------------------------------
void svtkTDxInteractorStyle::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Settings: ";
  if (this->Settings == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << endl;
    this->Settings->PrintSelf(os, indent.GetNextIndent());
  }
}
