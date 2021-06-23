/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRMenuWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenVRMenuWidget.h"
#include "svtkOpenVRMenuRepresentation.h"

#include "svtkAssemblyPath.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkEventData.h"
#include "svtkInteractorStyle3D.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPropPicker.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"

#include <map>

svtkStandardNewMacro(svtkOpenVRMenuWidget);

class svtkOpenVRMenuWidget::InternalElement
{
public:
  svtkCommand* Command;
  std::string Name;
  std::string Text;
  InternalElement() {}
};

//----------------------------------------------------------------------
svtkOpenVRMenuWidget::svtkOpenVRMenuWidget()
{
  // Set the initial state
  this->WidgetState = svtkOpenVRMenuWidget::Start;

  this->EventCommand = svtkCallbackCommand::New();
  this->EventCommand->SetClientData(this);
  this->EventCommand->SetCallback(svtkOpenVRMenuWidget::EventCallback);

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::ApplicationMenu);
    ed->SetAction(svtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed, svtkWidgetEvent::Select,
      this, svtkOpenVRMenuWidget::StartMenuAction);
  }

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed, svtkWidgetEvent::Select3D,
      this, svtkOpenVRMenuWidget::SelectMenuAction);
  }

  {
    svtkNew<svtkEventDataMove3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      svtkCommand::Move3DEvent, ed, svtkWidgetEvent::Move3D, this, svtkOpenVRMenuWidget::MoveAction);
  }
}

//----------------------------------------------------------------------
svtkOpenVRMenuWidget::~svtkOpenVRMenuWidget()
{
  this->EventCommand->Delete();
}

void svtkOpenVRMenuWidget::PushFrontMenuItem(const char* name, const char* text, svtkCommand* cmd)
{
  svtkOpenVRMenuWidget::InternalElement* el = new svtkOpenVRMenuWidget::InternalElement();
  el->Text = text;
  el->Command = cmd;
  el->Name = name;
  this->Menus.push_front(el);

  static_cast<svtkOpenVRMenuRepresentation*>(this->WidgetRep)
    ->PushFrontMenuItem(name, text, this->EventCommand);

  this->Modified();
}

void svtkOpenVRMenuWidget::RenameMenuItem(const char* name, const char* text)
{
  for (auto itr : this->Menus)
  {
    if (itr->Name == name)
    {
      itr->Text = text;
    }
  }
  static_cast<svtkOpenVRMenuRepresentation*>(this->WidgetRep)->RenameMenuItem(name, text);
}

void svtkOpenVRMenuWidget::RemoveMenuItem(const char* name)
{
  for (auto itr = this->Menus.begin(); itr != this->Menus.end(); ++itr)
  {
    if ((*itr)->Name == name)
    {
      delete *itr;
      this->Menus.erase(itr);
      break;
    }
  }
  static_cast<svtkOpenVRMenuRepresentation*>(this->WidgetRep)->RemoveMenuItem(name);
}

void svtkOpenVRMenuWidget::RemoveAllMenuItems()
{
  while (this->Menus.size() > 0)
  {
    auto itr = this->Menus.begin();
    delete *itr;
    this->Menus.erase(itr);
  }
  static_cast<svtkOpenVRMenuRepresentation*>(this->WidgetRep)->RemoveAllMenuItems();
}

void svtkOpenVRMenuWidget::EventCallback(svtkObject*, unsigned long, void* clientdata, void* calldata)
{
  svtkOpenVRMenuWidget* self = static_cast<svtkOpenVRMenuWidget*>(clientdata);
  std::string name = static_cast<const char*>(calldata);

  for (auto& menu : self->Menus)
  {
    if (menu->Name == name)
    {
      menu->Command->Execute(
        self, svtkWidgetEvent::Select3D, static_cast<void*>(const_cast<char*>(menu->Name.c_str())));
    }
  }
}

//-------------------------------------------------------------------------
void svtkOpenVRMenuWidget::ShowSubMenu(svtkOpenVRMenuWidget* w)
{
  w->SetInteractor(this->Interactor);
  w->Show(static_cast<svtkEventData*>(this->CallData));
}

void svtkOpenVRMenuWidget::Show(svtkEventData* ed)
{
  this->On();
  if (this->WidgetState == svtkOpenVRMenuWidget::Start)
  {
    if (!this->Parent)
    {
      this->GrabFocus(this->EventCallbackCommand);
    }
    this->CallData = ed;
    this->WidgetRep->StartComplexInteraction(this->Interactor, this, svtkWidgetEvent::Select, ed);

    this->WidgetState = svtkOpenVRMenuWidget::Active;
  }
}

//-------------------------------------------------------------------------
void svtkOpenVRMenuWidget::StartMenuAction(svtkAbstractWidget* w)
{
  svtkOpenVRMenuWidget* self = reinterpret_cast<svtkOpenVRMenuWidget*>(w);

  if (self->WidgetState == svtkOpenVRMenuWidget::Active)
  {
    if (!self->Parent)
    {
      self->ReleaseFocus();
    }

    self->Off();
    self->WidgetState = svtkOpenVRMenuWidget::Start;

    self->WidgetRep->EndComplexInteraction(
      self->Interactor, self, svtkWidgetEvent::Select, self->CallData);
  }
}

//-------------------------------------------------------------------------
void svtkOpenVRMenuWidget::SelectMenuAction(svtkAbstractWidget* w)
{
  svtkOpenVRMenuWidget* self = reinterpret_cast<svtkOpenVRMenuWidget*>(w);

  if (self->WidgetState != svtkOpenVRMenuWidget::Active)
  {
    return;
  }

  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->Off();
  self->WidgetState = svtkOpenVRMenuWidget::Start;

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);
}

//-------------------------------------------------------------------------
void svtkOpenVRMenuWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkOpenVRMenuWidget* self = reinterpret_cast<svtkOpenVRMenuWidget*>(w);

  if (self->WidgetState != svtkOpenVRMenuWidget::Active)
  {
    return;
  }

  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Move3D, self->CallData);
}

//----------------------------------------------------------------------
void svtkOpenVRMenuWidget::SetRepresentation(svtkOpenVRMenuRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(rep));
}

//----------------------------------------------------------------------
void svtkOpenVRMenuWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkOpenVRMenuRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkOpenVRMenuWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
