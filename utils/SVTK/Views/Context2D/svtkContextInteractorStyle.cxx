/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextInteractorStyle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkContextInteractorStyle.h"

#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkContextKeyEvent.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"

#include <cassert>

svtkStandardNewMacro(svtkContextInteractorStyle);

//--------------------------------------------------------------------------
svtkContextInteractorStyle::svtkContextInteractorStyle()
{
  this->Scene = nullptr;
  this->ProcessingEvents = 0;
  this->SceneCallbackCommand->SetClientData(this);
  this->SceneCallbackCommand->SetCallback(svtkContextInteractorStyle::ProcessSceneEvents);
  this->InteractorCallbackCommand->SetClientData(this);
  this->InteractorCallbackCommand->SetCallback(svtkContextInteractorStyle::ProcessInteractorEvents);
  this->LastSceneRepaintMTime = 0;
  this->SceneTimerId = 0;
  this->TimerCallbackInitialized = false;
}

//--------------------------------------------------------------------------
svtkContextInteractorStyle::~svtkContextInteractorStyle()
{
  // to remove observers.
  this->SetScene(nullptr);
  if (this->TimerCallbackInitialized && this->Interactor)
  {
    this->Interactor->RemoveObserver(this->InteractorCallbackCommand);
    this->TimerCallbackInitialized = false;
  }
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Scene: " << this->Scene << endl;
  if (this->Scene)
  {
    this->Scene->PrintSelf(os, indent.GetNextIndent());
  }
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::SetScene(svtkContextScene* scene)
{
  if (this->Scene == scene)
  {
    return;
  }
  if (this->Scene)
  {
    this->Scene->RemoveObserver(this->SceneCallbackCommand);
  }

  this->Scene = scene;

  if (this->Scene)
  {
    this->Scene->AddObserver(svtkCommand::ModifiedEvent, this->SceneCallbackCommand, this->Priority);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
svtkContextScene* svtkContextInteractorStyle::GetScene()
{
  return this->Scene;
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::ProcessSceneEvents(
  svtkObject*, unsigned long event, void* clientdata, void* svtkNotUsed(calldata))
{
  svtkContextInteractorStyle* self = reinterpret_cast<svtkContextInteractorStyle*>(clientdata);
  switch (event)
  {
    case svtkCommand::ModifiedEvent:
      self->OnSceneModified();
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::ProcessInteractorEvents(
  svtkObject*, unsigned long eventId, void* clientdata, void* svtkNotUsed(calldata))
{
  svtkContextInteractorStyle* self = reinterpret_cast<svtkContextInteractorStyle*>(clientdata);
  if (eventId == svtkCommand::TimerEvent)
  {
    // This is a timeout. To avoid the self->RenderNow() from destroying a
    // already dead timer, just we just reset it.
    self->SceneTimerId = 0;
  }

  self->RenderNow();
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::RenderNow()
{
  if (this->SceneTimerId > 0)
  {
    this->Interactor->DestroyTimer(this->SceneTimerId);
    this->SceneTimerId = 0;
  }
  if (this->Scene && !this->ProcessingEvents && this->Interactor->GetInitialized())
  {
    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::OnSceneModified()
{
  if (!this->Scene || !this->Scene->GetDirty() || this->ProcessingEvents ||
    this->Scene->GetMTime() == this->LastSceneRepaintMTime || !this->Interactor->GetInitialized())
  {
    return;
  }
  this->BeginProcessingEvent();
  if (!this->TimerCallbackInitialized && this->Interactor)
  {
    this->Interactor->AddObserver(svtkCommand::TimerEvent, this->InteractorCallbackCommand, 0.0);
    this->TimerCallbackInitialized = true;
  }
  this->LastSceneRepaintMTime = this->Scene->GetMTime();
  // If there is no timer, create a one shot timer to render an updated scene
  if (this->SceneTimerId == 0)
  {
    this->SceneTimerId = this->Interactor->CreateOneShotTimer(40);
  }
  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::BeginProcessingEvent()
{
  ++this->ProcessingEvents;
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::EndProcessingEvent()
{
  --this->ProcessingEvents;
  assert(this->ProcessingEvents >= 0);
  if (this->ProcessingEvents == 0)
  {
    this->OnSceneModified();
  }
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::OnMouseMove()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::NO_BUTTON);
    eatEvent = this->Scene->MouseMoveEvent(event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnMouseMove();
  }

  this->EndProcessingEvent();
}

inline bool svtkContextInteractorStyle::ProcessMousePress(const svtkContextMouseEvent& event)
{
  bool eatEvent(false);
  if (this->Interactor->GetRepeatCount())
  {
    eatEvent = this->Scene->DoubleClickEvent(event);
    //
    // The second ButtonRelease event seems not to be processed automatically,
    // need manually processing here so that the following MouseMove event will
    // not think the mouse button is still pressed down, and we don't really
    // care about the return result from the second ButtonRelease.
    //
    if (eatEvent)
    {
      this->Scene->ButtonReleaseEvent(event);
    }
  }
  else
  {
    eatEvent = this->Scene->ButtonPressEvent(event);
  }
  return eatEvent;
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::OnLeftButtonDown()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::LEFT_BUTTON);
    eatEvent = this->ProcessMousePress(event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnLeftButtonDown();
  }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::OnLeftButtonUp()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::LEFT_BUTTON);
    eatEvent = this->Scene->ButtonReleaseEvent(event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnLeftButtonUp();
  }
  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::OnMiddleButtonDown()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::MIDDLE_BUTTON);
    eatEvent = this->ProcessMousePress(event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnMiddleButtonDown();
  }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::OnMiddleButtonUp()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::MIDDLE_BUTTON);
    eatEvent = this->Scene->ButtonReleaseEvent(event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnMiddleButtonUp();
  }
  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::OnRightButtonDown()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::RIGHT_BUTTON);
    eatEvent = this->ProcessMousePress(event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnRightButtonDown();
  }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::OnRightButtonUp()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::RIGHT_BUTTON);
    eatEvent = this->Scene->ButtonReleaseEvent(event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnRightButtonUp();
  }
  this->EndProcessingEvent();
}

//----------------------------------------------------------------------------
void svtkContextInteractorStyle::OnMouseWheelForward()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::MIDDLE_BUTTON);
    eatEvent = this->Scene->MouseWheelEvent(static_cast<int>(this->MouseWheelMotionFactor), event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnMouseWheelForward();
  }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::OnMouseWheelBackward()
{
  this->BeginProcessingEvent();

  bool eatEvent = false;
  if (this->Scene)
  {
    svtkContextMouseEvent event;
    this->ConstructMouseEvent(event, svtkContextMouseEvent::MIDDLE_BUTTON);
    eatEvent = this->Scene->MouseWheelEvent(-static_cast<int>(this->MouseWheelMotionFactor), event);
  }
  if (!eatEvent)
  {
    this->Superclass::OnMouseWheelBackward();
  }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::OnSelection(unsigned int rect[5])
{
  this->BeginProcessingEvent();
  if (this->Scene)
  {
    this->Scene->ProcessSelectionEvent(rect);
  }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::OnChar()
{
  this->Superclass::OnChar();
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::OnKeyPress()
{
  this->BeginProcessingEvent();
  svtkContextKeyEvent event;
  svtkVector2i position(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  event.SetInteractor(this->Interactor);
  event.SetPosition(position);
  bool keepEvent = false;
  if (this->Scene)
  {
    keepEvent = this->Scene->KeyPressEvent(event);
  }
  if (!keepEvent)
  {
    this->Superclass::OnKeyPress();
  }
  this->EndProcessingEvent();
}

//--------------------------------------------------------------------------
void svtkContextInteractorStyle::OnKeyRelease()
{
  this->BeginProcessingEvent();
  svtkContextKeyEvent event;
  svtkVector2i position(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  event.SetInteractor(this->Interactor);
  event.SetPosition(position);
  bool keepEvent = false;
  if (this->Scene)
  {
    keepEvent = this->Scene->KeyReleaseEvent(event);
  }
  if (!keepEvent)
  {
    this->Superclass::OnKeyRelease();
  }
  this->EndProcessingEvent();
}

//-------------------------------------------------------------------------
inline void svtkContextInteractorStyle::ConstructMouseEvent(svtkContextMouseEvent& event, int button)
{
  event.SetInteractor(this->Interactor);
  event.SetPos(
    svtkVector2f(this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]));
  event.SetButton(button);
}
