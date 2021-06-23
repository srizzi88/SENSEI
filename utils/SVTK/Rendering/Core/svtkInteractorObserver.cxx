/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorObserver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorObserver.h"

#include "svtkAbstractPropPicker.h"
#include "svtkCallbackCommand.h"
#include "svtkObjectFactory.h"
#include "svtkObserverMediator.h"
#include "svtkPickingManager.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

svtkCxxSetObjectMacro(svtkInteractorObserver, DefaultRenderer, svtkRenderer);

//----------------------------------------------------------------------------
svtkInteractorObserver::svtkInteractorObserver()
{
  this->Enabled = 0;

  this->Interactor = nullptr;

  this->EventCallbackCommand = svtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this);

  // subclass has to invoke SetCallback()

  this->KeyPressCallbackCommand = svtkCallbackCommand::New();
  this->KeyPressCallbackCommand->SetClientData(this);
  this->KeyPressCallbackCommand->SetCallback(svtkInteractorObserver::ProcessEvents);

  this->CurrentRenderer = nullptr;
  this->DefaultRenderer = nullptr;

  this->Priority = 0.0f;
  this->PickingManaged = true;

  this->KeyPressActivation = 1;
  this->KeyPressActivationValue = 'i';

  this->CharObserverTag = 0;
  this->DeleteObserverTag = 0;

  this->ObserverMediator = nullptr;
}

//----------------------------------------------------------------------------
svtkInteractorObserver::~svtkInteractorObserver()
{
  this->UnRegisterPickers();

  this->SetEnabled(0);
  this->SetCurrentRenderer(nullptr);
  this->SetDefaultRenderer(nullptr);
  this->EventCallbackCommand->Delete();
  this->KeyPressCallbackCommand->Delete();
  this->SetInteractor(nullptr);
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::SetCurrentRenderer(svtkRenderer* _arg)
{
  if (this->CurrentRenderer == _arg)
  {
    return;
  }

  if (this->CurrentRenderer != nullptr)
  {
    this->CurrentRenderer->UnRegister(this);
  }

  // WARNING: see .h, if the DefaultRenderer is set, whatever the value
  // of _arg (except nullptr), we are going to use DefaultRenderer
  // Normally when the widget is activated (SetEnabled(1) or when
  // keypress activation takes place), the renderer over which the mouse
  // pointer is positioned is used to call SetCurrentRenderer().
  // Alternatively, we may want to specify a user-defined renderer to bind the
  // interactor to when the interactor observer is activated.
  // The problem is that in many 3D widgets, when SetEnabled(0) is called,
  // the CurrentRender is set to nullptr. In that case, the next time
  // SetEnabled(1) is called, the widget will try to set CurrentRenderer
  // to the renderer over which the mouse pointer is positioned, and we
  // will use our user-defined renderer. To solve that, we introduced the
  // DefaultRenderer ivar, which will be used to force the value of
  // CurrentRenderer each time SetCurrentRenderer is called (i.e., no matter
  // if SetCurrentRenderer is called with the renderer that was poked at
  // the mouse coords, the DefaultRenderer will be used).

  if (_arg && this->DefaultRenderer)
  {
    _arg = this->DefaultRenderer;
  }

  this->CurrentRenderer = _arg;

  if (this->CurrentRenderer != nullptr)
  {
    this->CurrentRenderer->Register(this);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
// This adds the keypress event observer and the delete event observer
void svtkInteractorObserver::SetInteractor(svtkRenderWindowInteractor* i)
{
  if (i == this->Interactor)
  {
    return;
  }

  // Since the observer mediator is bound to the interactor, reset it to
  // 0 so that the next time it is requested, it is queried from the
  // new interactor.
  // Furthermore, remove ourself from the mediator queue.

  if (this->ObserverMediator)
  {
    this->ObserverMediator->RemoveAllCursorShapeRequests(this);
    this->ObserverMediator = nullptr;
  }

  // if we already have an Interactor then stop observing it
  if (this->Interactor)
  {
    this->SetEnabled(0); // disable the old interactor
    this->Interactor->RemoveObserver(this->CharObserverTag);
    this->CharObserverTag = 0;
    this->Interactor->RemoveObserver(this->DeleteObserverTag);
    this->DeleteObserverTag = 0;
  }

  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if (i)
  {
    this->CharObserverTag =
      i->AddObserver(svtkCommand::CharEvent, this->KeyPressCallbackCommand, this->Priority);
    this->DeleteObserverTag =
      i->AddObserver(svtkCommand::DeleteEvent, this->KeyPressCallbackCommand, this->Priority);

    this->RegisterPickers();
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::SetPickingManaged(bool managed)
{
  if (this->PickingManaged == managed)
  {
    return;
  }
  this->UnRegisterPickers();
  this->PickingManaged = managed;
  if (this->PickingManaged)
  {
    this->RegisterPickers();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::RegisterPickers() {}

//----------------------------------------------------------------------------
void svtkInteractorObserver::UnRegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }

  pm->RemoveObject(this);
}

//----------------------------------------------------------------------------
svtkPickingManager* svtkInteractorObserver::GetPickingManager()
{
  return this->Interactor ? this->Interactor->GetPickingManager() : nullptr;
}

//------------------------------------------------------------------------------
svtkAssemblyPath* svtkInteractorObserver::GetAssemblyPath(
  double X, double Y, double Z, svtkAbstractPropPicker* picker)
{
  if (!this->GetPickingManager())
  {
    picker->Pick(X, Y, Z, this->CurrentRenderer);
    return picker->GetPath();
  }

  return this->GetPickingManager()->GetAssemblyPath(X, Y, Z, picker, this->CurrentRenderer, this);
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::ProcessEvents(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientdata, void* svtkNotUsed(calldata))
{
  if (event == svtkCommand::CharEvent || event == svtkCommand::DeleteEvent)
  {
    svtkObject* vobj = reinterpret_cast<svtkObject*>(clientdata);
    svtkInteractorObserver* self = svtkInteractorObserver::SafeDownCast(vobj);
    if (self)
    {
      if (event == svtkCommand::CharEvent)
      {
        self->OnChar();
      }
      else // delete event
      {
        self->SetInteractor(nullptr);
      }
    }
    else
    {
      svtkGenericWarningMacro(
        "Process Events received a bad client data. The client data class name was "
        << vobj->GetClassName());
    }
  }
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::StartInteraction()
{
  this->Interactor->GetRenderWindow()->SetDesiredUpdateRate(
    this->Interactor->GetDesiredUpdateRate());
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::EndInteraction()
{
  this->Interactor->GetRenderWindow()->SetDesiredUpdateRate(this->Interactor->GetStillUpdateRate());
}

//----------------------------------------------------------------------------
// Description:
// Transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void svtkInteractorObserver::ComputeDisplayToWorld(
  svtkRenderer* ren, double x, double y, double z, double worldPt[4])
{
  ren->SetDisplayPoint(x, y, z);
  ren->DisplayToWorld();
  ren->GetWorldPoint(worldPt);
  if (worldPt[3])
  {
    worldPt[0] /= worldPt[3];
    worldPt[1] /= worldPt[3];
    worldPt[2] /= worldPt[3];
    worldPt[3] = 1.0;
  }
}

//----------------------------------------------------------------------------
// Description:
// Transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void svtkInteractorObserver::ComputeWorldToDisplay(
  svtkRenderer* ren, double x, double y, double z, double displayPt[3])
{
  ren->SetWorldPoint(x, y, z, 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(displayPt);
}

//----------------------------------------------------------------------------
// Description:
// Transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void svtkInteractorObserver::ComputeDisplayToWorld(double x, double y, double z, double worldPt[4])
{
  if (!this->CurrentRenderer)
  {
    return;
  }

  this->ComputeDisplayToWorld(this->CurrentRenderer, x, y, z, worldPt);
}

//----------------------------------------------------------------------------
// Description:
// Transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void svtkInteractorObserver::ComputeWorldToDisplay(double x, double y, double z, double displayPt[3])
{
  if (!this->CurrentRenderer)
  {
    return;
  }

  this->ComputeWorldToDisplay(this->CurrentRenderer, x, y, z, displayPt);
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::OnChar()
{
  // catch additional keycodes otherwise
  if (this->KeyPressActivation)
  {
    if (this->Interactor->GetKeyCode() == this->KeyPressActivationValue)
    {
      if (!this->Enabled)
      {
        this->On();
      }
      else
      {
        this->Off();
      }
      this->KeyPressCallbackCommand->SetAbortFlag(1);
    }
  } // if activation enabled
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::GrabFocus(svtkCommand* mouseEvents, svtkCommand* keypressEvents)
{
  if (this->Interactor)
  {
    this->Interactor->GrabFocus(mouseEvents, keypressEvents);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::ReleaseFocus()
{
  if (this->Interactor)
  {
    this->Interactor->ReleaseFocus();
  }
}

//----------------------------------------------------------------------------
int svtkInteractorObserver::RequestCursorShape(int requestedShape)
{
  if (!this->Interactor)
  {
    return 0;
  }

  if (!this->ObserverMediator)
  {
    this->ObserverMediator = this->Interactor->GetObserverMediator();
  }
  int status = this->ObserverMediator->RequestCursorShape(this, requestedShape);
  if (status)
  {
    this->InvokeEvent(svtkCommand::CursorChangedEvent, nullptr);
  }
  return status;
}

//----------------------------------------------------------------------------
void svtkInteractorObserver::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Current Renderer: " << this->CurrentRenderer << "\n";
  os << indent << "Default Renderer: " << this->DefaultRenderer << "\n";
  os << indent << "Enabled: " << this->Enabled << "\n";
  os << indent << "Priority: " << this->Priority << "\n";
  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Key Press Activation: " << (this->KeyPressActivation ? "On" : "Off") << "\n";
  os << indent << "Key Press Activation Value: " << this->KeyPressActivationValue << "\n";
}
