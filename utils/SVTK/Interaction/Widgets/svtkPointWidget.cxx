/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointWidget.h"

#include "svtkActor.h"
#include "svtkAssemblyNode.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkPointWidget);

svtkPointWidget::svtkPointWidget()
{
  this->State = svtkPointWidget::Start;
  this->EventCallbackCommand->SetCallback(svtkPointWidget::ProcessEvents);

  // Represent the line
  this->Cursor3D = svtkCursor3D::New();
  this->Mapper = svtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Cursor3D->GetOutputPort());
  this->Actor = svtkActor::New();
  this->Actor->SetMapper(this->Mapper);

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->CursorPicker = svtkCellPicker::New();
  this->CursorPicker->PickFromListOn();
  this->CursorPicker->AddPickList(this->Actor);
  this->CursorPicker->SetTolerance(0.005); // need some fluff

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Constraints not set
  this->ConstraintAxis = -1;

  // Override superclass'
  this->PlaceFactor = 1.0;

  // The size of the hot spot
  this->HotSpotSize = 0.05;
  this->WaitingForMotion = 0;
}

svtkPointWidget::~svtkPointWidget()
{
  this->Actor->Delete();
  this->Mapper->Delete();
  this->Cursor3D->Delete();

  this->CursorPicker->Delete();

  this->Property->Delete();
  this->SelectedProperty->Delete();
}

void svtkPointWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
  {
    svtkErrorMacro(<< "The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if (enabling) //-----------------------------------------------------------
  {
    svtkDebugMacro(<< "Enabling point widget");

    if (this->Enabled) // already enabled, just return
    {
      return;
    }

    if (!this->CurrentRenderer)
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0], this->Interactor->GetLastEventPosition()[1]));
      if (this->CurrentRenderer == nullptr)
      {
        return;
      }
    }

    this->Enabled = 1;

    // listen for the following events
    svtkRenderWindowInteractor* i = this->Interactor;
    i->AddObserver(svtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::MiddleButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(
      svtkCommand::MiddleButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::RightButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    // Add the line
    this->CurrentRenderer->AddActor(this->Actor);
    this->Actor->SetProperty(this->Property);
    this->Cursor3D->Update();
    this->RegisterPickers();

    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling----------------------------------------------------------
  {
    svtkDebugMacro(<< "Disabling point widget");

    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the line
    this->CurrentRenderer->RemoveActor(this->Actor);

    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
    this->UnRegisterPickers();
  }

  this->Interactor->Render();
}

//------------------------------------------------------------------------------
void svtkPointWidget::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->CursorPicker, this);
}

void svtkPointWidget::ProcessEvents(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientdata, void* svtkNotUsed(calldata))
{
  svtkPointWidget* self = reinterpret_cast<svtkPointWidget*>(clientdata);

  // okay, let's do the right thing
  switch (event)
  {
    case svtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case svtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case svtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown();
      break;
    case svtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp();
      break;
    case svtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown();
      break;
    case svtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp();
      break;
    case svtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
  }
}

void svtkPointWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Property)
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }
  if (this->SelectedProperty)
  {
    os << indent << "Selected Property: " << this->SelectedProperty << "\n";
  }
  else
  {
    os << indent << "Selected Property: (none)\n";
  }

  double* pos = this->Cursor3D->GetFocalPoint();
  os << indent << "Position: (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ")\n";

  os << indent << "Outline: " << (this->GetOutline() ? "On\n" : "Off\n");
  os << indent << "XShadows: " << (this->GetXShadows() ? "On\n" : "Off\n");
  os << indent << "YShadows: " << (this->GetYShadows() ? "On\n" : "Off\n");
  os << indent << "ZShadows: " << (this->GetZShadows() ? "On\n" : "Off\n");

  os << indent << "Translation Mode: " << (this->Cursor3D->GetTranslationMode() ? "On\n" : "Off\n");

  os << indent << "Hot Spot Size: " << this->HotSpotSize << "\n";
}

void svtkPointWidget::Highlight(int highlight)
{
  if (highlight)
  {
    this->Actor->SetProperty(this->SelectedProperty);
    this->CursorPicker->GetPickPosition(this->LastPickPosition);
    this->ValidPick = 1;
  }
  else
  {
    this->Actor->SetProperty(this->Property);
  }
}

int svtkPointWidget::DetermineConstraintAxis(int constraint, double* x)
{
  // Look for trivial cases
  if (!this->Interactor->GetShiftKey())
  {
    return -1;
  }
  else if (constraint >= 0 && constraint < 3)
  {
    return constraint;
  }

  // Okay, figure out constraint. First see if the choice is
  // outside the hot spot
  if (!this->WaitingForMotion)
  {
    double p[3], d2, tol;
    this->CursorPicker->GetPickPosition(p);
    d2 = svtkMath::Distance2BetweenPoints(p, this->LastPickPosition);
    tol = this->HotSpotSize * this->InitialLength;
    if (d2 > (tol * tol))
    {
      this->WaitingForMotion = 0;
      return this->CursorPicker->GetCellId();
    }
    else
    {
      this->WaitingForMotion = 1;
      this->WaitCount = 0;
      return -1;
    }
  }
  else if (this->WaitingForMotion && x)
  {
    double v[3];
    this->WaitingForMotion = 0;
    v[0] = fabs(x[0] - this->LastPickPosition[0]);
    v[1] = fabs(x[1] - this->LastPickPosition[1]);
    v[2] = fabs(x[2] - this->LastPickPosition[2]);
    return (v[0] > v[1] ? (v[0] > v[2] ? 0 : 2) : (v[1] > v[2] ? 1 : 2));
  }
  else
  {
    return -1;
  }
}

void svtkPointWidget::OnLeftButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkPointWidget::Outside;
    return;
  }

  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->CursorPicker);

  if (path != nullptr)
  {
    this->State = svtkPointWidget::Moving;
    this->Highlight(1);
    this->ConstraintAxis = this->DetermineConstraintAxis(-1, nullptr);
  }
  else
  {
    this->State = svtkPointWidget::Outside;
    this->Highlight(0);
    this->ConstraintAxis = -1;
    return;
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkPointWidget::OnLeftButtonUp()
{
  if (this->State == svtkPointWidget::Outside || this->State == svtkPointWidget::Start)
  {
    return;
  }

  this->State = svtkPointWidget::Start;
  this->Highlight(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkPointWidget::OnMiddleButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkPointWidget::Outside;
    return;
  }

  // Okay, we can process this.
  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->CursorPicker);

  if (path != nullptr)
  {
    this->State = svtkPointWidget::Translating;
    this->Highlight(1);
    this->ConstraintAxis = this->DetermineConstraintAxis(-1, nullptr);
  }
  else
  {
    this->State = svtkPointWidget::Outside;
    this->ConstraintAxis = -1;
    return;
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkPointWidget::OnMiddleButtonUp()
{
  if (this->State == svtkPointWidget::Outside || this->State == svtkPointWidget::Start)
  {
    return;
  }

  this->State = svtkPointWidget::Start;
  this->Highlight(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkPointWidget::OnRightButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkPointWidget::Outside;
    return;
  }

  // Okay, we can process this. Pick the cursor.
  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->CursorPicker);

  if (path != nullptr)
  {
    this->State = svtkPointWidget::Scaling;
    int idx = this->CursorPicker->GetCellId();
    if (idx >= 0 && idx < 3)
    {
      this->ConstraintAxis = idx;
    }
    this->Highlight(1);
  }
  else
  {
    this->State = svtkPointWidget::Outside;
    this->ConstraintAxis = -1;
    return;
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkPointWidget::OnRightButtonUp()
{
  if (this->State == svtkPointWidget::Outside || this->State == svtkPointWidget::Start)
  {
    return;
  }

  this->State = svtkPointWidget::Start;
  this->Highlight(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkPointWidget::OnMouseMove()
{
  // See whether we're active
  if (this->State == svtkPointWidget::Outside || this->State == svtkPointWidget::Start)
  {
    return;
  }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  // Compute the two points defining the motion vector
  this->ComputeWorldToDisplay(
    this->LastPickPosition[0], this->LastPickPosition[1], this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
    double(this->Interactor->GetLastEventPosition()[1]), z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if (this->State == svtkPointWidget::Moving)
  {
    if (!this->WaitingForMotion || this->WaitCount++ > 3)
    {
      this->ConstraintAxis = this->DetermineConstraintAxis(this->ConstraintAxis, pickPoint);
      this->MoveFocus(prevPickPoint, pickPoint);
    }
    else
    {
      return; // avoid the extra render
    }
  }

  else if (this->State == svtkPointWidget::Scaling)
  {
    this->Scale(prevPickPoint, pickPoint, X, Y);
  }

  else if (this->State == svtkPointWidget::Translating)
  {
    if (!this->WaitingForMotion || this->WaitCount++ > 3)
    {
      this->ConstraintAxis = this->DetermineConstraintAxis(this->ConstraintAxis, pickPoint);
      this->Translate(prevPickPoint, pickPoint);
    }
    else
    {
      return; // avoid the extra render
    }
  }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkPointWidget::MoveFocus(double* p1, double* p2)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double focus[3];
  this->Cursor3D->GetFocalPoint(focus);
  if (this->ConstraintAxis >= 0)
  {
    focus[this->ConstraintAxis] += v[this->ConstraintAxis];
  }
  else
  {
    focus[0] += v[0];
    focus[1] += v[1];
    focus[2] += v[2];
  }

  this->Cursor3D->SetFocalPoint(focus);
}

// Translate everything
void svtkPointWidget::Translate(double* p1, double* p2)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double* bounds = this->Cursor3D->GetModelBounds();
  double* pos = this->Cursor3D->GetFocalPoint();
  double newBounds[6], newFocus[3];
  int i;

  if (this->ConstraintAxis >= 0)
  { // move along axis
    for (i = 0; i < 3; i++)
    {
      if (i != this->ConstraintAxis)
      {
        v[i] = 0.0;
      }
    }
  }

  for (i = 0; i < 3; i++)
  {
    newBounds[2 * i] = bounds[2 * i] + v[i];
    newBounds[2 * i + 1] = bounds[2 * i + 1] + v[i];
    newFocus[i] = pos[i] + v[i];
  }

  this->Cursor3D->SetModelBounds(newBounds);
  this->Cursor3D->SetFocalPoint(newFocus);
}

void svtkPointWidget::Scale(double* p1, double* p2, int svtkNotUsed(X), int Y)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // int res = this->Cursor3D->GetResolution();
  double* bounds = this->Cursor3D->GetModelBounds();
  double* focus = this->Cursor3D->GetFocalPoint();

  // Compute the scale factor
  double sf = svtkMath::Norm(v) /
    sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
      (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
      (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  if (Y > this->Interactor->GetLastEventPosition()[1])
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  // Move the end points
  double newBounds[6];
  for (int i = 0; i < 3; i++)
  {
    newBounds[2 * i] = sf * (bounds[2 * i] - focus[i]) + focus[i];
    newBounds[2 * i + 1] = sf * (bounds[2 * i + 1] - focus[i]) + focus[i];
  }

  this->Cursor3D->SetModelBounds(newBounds);
  this->Cursor3D->Update();
}

void svtkPointWidget::CreateDefaultProperties()
{
  this->Property = svtkProperty::New();
  this->Property->SetAmbient(1.0);
  this->Property->SetAmbientColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = svtkProperty::New();
  this->SelectedProperty->SetAmbient(1.0);
  this->SelectedProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

void svtkPointWidget::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);

  this->Cursor3D->SetModelBounds(bounds);
  this->Cursor3D->SetFocalPoint(center);
  this->Cursor3D->Update();

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
}

void svtkPointWidget::GetPolyData(svtkPolyData* pd)
{
  this->Cursor3D->Update();
  pd->DeepCopy(this->Cursor3D->GetFocus());
}
