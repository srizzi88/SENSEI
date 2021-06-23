/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleUser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleUser.h"
#include "svtkCellPicker.h"
#include "svtkCommand.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"

svtkStandardNewMacro(svtkInteractorStyleUser);

//----------------------------------------------------------------------------
svtkInteractorStyleUser::svtkInteractorStyleUser()
{
  // Tell the parent class not to handle observers
  // that has to be done here
  this->HandleObserversOff();
  this->LastPos[0] = this->LastPos[1] = 0;
  this->OldPos[0] = this->OldPos[1] = 0;
  this->ShiftKey = 0;
  this->CtrlKey = 0;
  this->Char = '\0';
  this->KeySym = nullptr;
  this->Button = 0;
}

//----------------------------------------------------------------------------
svtkInteractorStyleUser::~svtkInteractorStyleUser() = default;

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LastPos: (" << this->LastPos[0] << ", " << this->LastPos[1] << ")\n";
  os << indent << "OldPos: (" << this->OldPos[0] << ", " << this->OldPos[1] << ")\n";
  os << indent << "ShiftKey: " << this->ShiftKey << "\n";
  os << indent << "CtrlKey: " << this->CtrlKey << "\n";
  os << indent << "Char: " << this->Char << "\n";
  os << indent << "KeySym: " << (this->KeySym ? this->KeySym : "(null)") << "\n";
  os << indent << "Button: " << this->Button << "\n";
}

//----------------------------------------------------------------------------
// checks for USERINTERACTION state, then defers to the superclass modes
void svtkInteractorStyleUser::OnTimer()
{
  if (this->HasObserver(svtkCommand::TimerEvent))
  {
    this->InvokeEvent(svtkCommand::TimerEvent, &(this->TimerId));
  }

  if (this->State == SVTKIS_USERINTERACTION)
  {
    if (this->HasObserver(svtkCommand::UserEvent))
    {
      this->InvokeEvent(svtkCommand::UserEvent, nullptr);
      this->OldPos[0] = this->LastPos[0];
      this->OldPos[1] = this->LastPos[1];
      if (this->UseTimers)
      {
        this->Interactor->ResetTimer(this->TimerId);
      }
    }
  }
  else if (!(this->HasObserver(svtkCommand::MouseMoveEvent) &&
             (this->Button == 0 ||
               (this->HasObserver(svtkCommand::LeftButtonPressEvent) && this->Button == 1) ||
               (this->HasObserver(svtkCommand::MiddleButtonPressEvent) && this->Button == 2) ||
               (this->HasObserver(svtkCommand::RightButtonPressEvent) && this->Button == 3))))
  {
    this->svtkInteractorStyle::OnTimer();
  }
  else if (this->HasObserver(svtkCommand::TimerEvent))
  {
    if (this->UseTimers)
    {
      this->Interactor->ResetTimer(this->TimerId);
    }
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnKeyPress()
{
  if (this->HasObserver(svtkCommand::KeyPressEvent))
  {
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->CtrlKey = this->Interactor->GetControlKey();
    this->KeySym = this->Interactor->GetKeySym();
    this->Char = this->Interactor->GetKeyCode();
    this->InvokeEvent(svtkCommand::KeyPressEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnKeyRelease()
{
  if (this->HasObserver(svtkCommand::KeyReleaseEvent))
  {
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->CtrlKey = this->Interactor->GetControlKey();
    this->KeySym = this->Interactor->GetKeySym();
    this->Char = this->Interactor->GetKeyCode();

    this->InvokeEvent(svtkCommand::KeyReleaseEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnChar()
{
  // otherwise pass the OnChar to the svtkInteractorStyle.
  if (this->HasObserver(svtkCommand::CharEvent))
  {
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->CtrlKey = this->Interactor->GetControlKey();
    this->Char = this->Interactor->GetKeyCode();

    this->InvokeEvent(svtkCommand::CharEvent, nullptr);
  }
  else
  {
    this->svtkInteractorStyle::OnChar();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnRightButtonDown()
{
  this->Button = 3;

  if (this->HasObserver(svtkCommand::RightButtonPressEvent))
  {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    this->CtrlKey = this->Interactor->GetControlKey();
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->InvokeEvent(svtkCommand::RightButtonPressEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
  else
  {
    this->svtkInteractorStyle::OnRightButtonDown();
  }
}
//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnRightButtonUp()
{
  if (this->HasObserver(svtkCommand::RightButtonReleaseEvent))
  {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    this->CtrlKey = this->Interactor->GetControlKey();
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->InvokeEvent(svtkCommand::RightButtonReleaseEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
  else
  {
    this->svtkInteractorStyle::OnRightButtonUp();
  }

  if (this->Button == 3)
  {
    this->Button = 0;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnMouseWheelForward()
{
  if (this->HasObserver(svtkCommand::MouseWheelForwardEvent))
  {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    this->CtrlKey = this->Interactor->GetControlKey();
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->InvokeEvent(svtkCommand::MouseWheelForwardEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
  else
  {
    this->svtkInteractorStyle::OnMouseWheelForward();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnMouseWheelBackward()
{
  if (this->HasObserver(svtkCommand::MouseWheelBackwardEvent))
  {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    this->CtrlKey = this->Interactor->GetControlKey();
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->InvokeEvent(svtkCommand::MouseWheelBackwardEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
  else
  {
    this->svtkInteractorStyle::OnMouseWheelBackward();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnMiddleButtonDown()
{
  this->Button = 2;

  if (this->HasObserver(svtkCommand::MiddleButtonPressEvent))
  {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    this->CtrlKey = this->Interactor->GetControlKey();
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->InvokeEvent(svtkCommand::MiddleButtonPressEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
  else
  {
    this->svtkInteractorStyle::OnMiddleButtonDown();
  }
}
//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnMiddleButtonUp()
{
  if (this->HasObserver(svtkCommand::MiddleButtonReleaseEvent))
  {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    this->CtrlKey = this->Interactor->GetControlKey();
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->InvokeEvent(svtkCommand::MiddleButtonReleaseEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
  else
  {
    this->svtkInteractorStyle::OnMiddleButtonUp();
  }

  if (this->Button == 2)
  {
    this->Button = 0;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnLeftButtonDown()
{
  this->Button = 1;

  if (this->HasObserver(svtkCommand::LeftButtonPressEvent))
  {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    this->CtrlKey = this->Interactor->GetControlKey();
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
  else
  {
    this->svtkInteractorStyle::OnLeftButtonDown();
  }
}
//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnLeftButtonUp()
{
  if (this->HasObserver(svtkCommand::LeftButtonReleaseEvent))
  {
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    this->CtrlKey = this->Interactor->GetControlKey();
    this->ShiftKey = this->Interactor->GetShiftKey();
    this->LastPos[0] = x;
    this->LastPos[1] = y;
    this->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
  else
  {
    this->svtkInteractorStyle::OnLeftButtonUp();
  }

  if (this->Button == 1)
  {
    this->Button = 0;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnMouseMove()
{
  this->svtkInteractorStyle::OnMouseMove();

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->LastPos[0] = x;
  this->LastPos[1] = y;
  this->ShiftKey = this->Interactor->GetShiftKey();
  this->CtrlKey = this->Interactor->GetControlKey();

  if (this->HasObserver(svtkCommand::MouseMoveEvent))
  {
    this->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr);
    this->OldPos[0] = x;
    this->OldPos[1] = y;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnExpose()
{
  if (this->HasObserver(svtkCommand::ExposeEvent))
  {
    this->InvokeEvent(svtkCommand::ExposeEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnConfigure()
{
  if (this->HasObserver(svtkCommand::ConfigureEvent))
  {
    this->InvokeEvent(svtkCommand::ConfigureEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnEnter()
{
  if (this->HasObserver(svtkCommand::EnterEvent))
  {
    this->LastPos[0] = this->Interactor->GetEventPosition()[0];
    this->LastPos[1] = this->Interactor->GetEventPosition()[1];
    this->InvokeEvent(svtkCommand::EnterEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleUser::OnLeave()
{
  if (this->HasObserver(svtkCommand::LeaveEvent))
  {
    this->LastPos[0] = this->Interactor->GetEventPosition()[0];
    this->LastPos[1] = this->Interactor->GetEventPosition()[1];
    this->InvokeEvent(svtkCommand::LeaveEvent, nullptr);
  }
}
