/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkBrokenLineWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBrokenLineWidget.h"

#include "svtkActor.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellPicker.h"
#include "svtkLineSource.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPlaneSource.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTransform.h"

svtkStandardNewMacro(svtkBrokenLineWidget);

svtkCxxSetObjectMacro(svtkBrokenLineWidget, HandleProperty, svtkProperty);
svtkCxxSetObjectMacro(svtkBrokenLineWidget, SelectedHandleProperty, svtkProperty);
svtkCxxSetObjectMacro(svtkBrokenLineWidget, LineProperty, svtkProperty);
svtkCxxSetObjectMacro(svtkBrokenLineWidget, SelectedLineProperty, svtkProperty);

svtkBrokenLineWidget::svtkBrokenLineWidget()
{
  this->State = svtkBrokenLineWidget::Start;
  this->EventCallbackCommand->SetCallback(svtkBrokenLineWidget::ProcessEventsHandler);
  this->ProjectToPlane = 0;   // default off
  this->ProjectionNormal = 0; // default YZ not used
  this->ProjectionPosition = 0.;
  this->PlaneSource = nullptr;

  // Does this widget respond to interaction?
  this->ProcessEvents = 1;

  // Default handle size factor
  this->HandleSizeFactor = 1.;

  // Build the representation of the widget

  // Default bounds to get started
  double bounds[6] = { -.5, .5, -.5, .5, -.5, .5 };

  // Create the handles along a straight line within the bounds of a unit cube
  this->NumberOfHandles = 5;
  this->Handle = new svtkActor*[this->NumberOfHandles];
  this->HandleGeometry = new svtkSphereSource*[this->NumberOfHandles];
  double u;
  double x0 = bounds[0];
  double x1 = bounds[1];
  double y0 = bounds[2];
  double y1 = bounds[3];
  double z0 = bounds[4];
  double z1 = bounds[5];
  double x;
  double y;
  double z;
  svtkPoints* points = svtkPoints::New(SVTK_DOUBLE);
  points->SetNumberOfPoints(this->NumberOfHandles);

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i] = svtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    svtkPolyDataMapper* handleMapper = svtkPolyDataMapper::New();
    handleMapper->SetInputConnection(this->HandleGeometry[i]->GetOutputPort());
    this->Handle[i] = svtkActor::New();
    this->Handle[i]->SetMapper(handleMapper);
    handleMapper->Delete();
    u = i / (this->NumberOfHandles - 1.);
    x = (1. - u) * x0 + u * x1;
    y = (1. - u) * y0 + u * y1;
    z = (1. - u) * z0 + u * z1;
    points->SetPoint(i, x, y, z);
    this->HandleGeometry[i]->SetCenter(x, y, z);
  }

  // Create the broken line
  this->LineSource = svtkLineSource::New();
  this->LineSource->SetPoints(points);
  points->Delete();

  // Represent the broken line
  this->LineMapper = svtkPolyDataMapper::New();
  this->LineMapper->SetInputConnection(this->LineSource->GetOutputPort());
  this->LineMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->LineActor = svtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  // Initial creation of the widget, serves to initialize it
  this->PlaceFactor = 1.;
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = svtkCellPicker::New();
  this->HandlePicker->SetTolerance(.005);

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandlePicker->AddPickList(this->Handle[i]);
  }
  this->HandlePicker->PickFromListOn();

  this->LinePicker = svtkCellPicker::New();
  this->LinePicker->SetTolerance(.01);
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->PickFromListOn();

  this->CurrentHandle = nullptr;
  this->CurrentHandleIndex = -1;

  this->Transform = svtkTransform::New();

  // Set up the initial properties
  this->HandleProperty = nullptr;
  this->SelectedHandleProperty = nullptr;
  this->LineProperty = nullptr;
  this->SelectedLineProperty = nullptr;
  this->CreateDefaultProperties();
}

svtkBrokenLineWidget::~svtkBrokenLineWidget()
{
  this->LineActor->Delete();
  this->LineMapper->Delete();
  this->LineSource->Delete();

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->Delete();
    this->Handle[i]->Delete();
  }
  delete[] this->Handle;
  delete[] this->HandleGeometry;

  this->HandlePicker->Delete();
  this->LinePicker->Delete();

  if (this->HandleProperty)
  {
    this->HandleProperty->Delete();
  }
  if (this->SelectedHandleProperty)
  {
    this->SelectedHandleProperty->Delete();
  }
  if (this->LineProperty)
  {
    this->LineProperty->Delete();
  }
  if (this->SelectedLineProperty)
  {
    this->SelectedLineProperty->Delete();
  }

  this->Transform->Delete();
}

void svtkBrokenLineWidget::SetHandlePosition(int handle, double x, double y, double z)
{
  if (handle < 0 || handle >= this->NumberOfHandles)
  {
    svtkErrorMacro(<< "svtkBrokenLineWidget: handle index out of range.");
    return;
  }
  this->HandleGeometry[handle]->SetCenter(x, y, z);
  this->HandleGeometry[handle]->Update();
  if (this->ProjectToPlane)
  {
    this->ProjectPointsToPlane();
  }
  this->BuildRepresentation();
}

void svtkBrokenLineWidget::SetHandlePosition(int handle, double xyz[3])
{
  this->SetHandlePosition(handle, xyz[0], xyz[1], xyz[2]);
}

void svtkBrokenLineWidget::GetHandlePosition(int handle, double xyz[3])
{
  if (handle < 0 || handle >= this->NumberOfHandles)
  {
    svtkErrorMacro(<< "svtkBrokenLineWidget: handle index out of range.");
    return;
  }

  this->HandleGeometry[handle]->GetCenter(xyz);
}

double* svtkBrokenLineWidget::GetHandlePosition(int handle)
{
  if (handle < 0 || handle >= this->NumberOfHandles)
  {
    svtkErrorMacro(<< "svtkBrokenLineWidget: handle index out of range.");
    return nullptr;
  }

  return this->HandleGeometry[handle]->GetCenter();
}

void svtkBrokenLineWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
  {
    svtkErrorMacro(<< "The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if (enabling) //------------------------------------------------------------
  {
    svtkDebugMacro(<< "Enabling broken line widget");

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

    // Listen for the following events
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
    this->CurrentRenderer->AddActor(this->LineActor);
    this->LineActor->SetProperty(this->LineProperty);

    // Turn on the handles
    for (int j = 0; j < this->NumberOfHandles; ++j)
    {
      this->CurrentRenderer->AddActor(this->Handle[j]);
      this->Handle[j]->SetProperty(this->HandleProperty);
    }
    this->BuildRepresentation();
    this->SizeHandles();
    this->RegisterPickers();

    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling----------------------------------------------------------
  {
    svtkDebugMacro(<< "Disabling broken line widget");

    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // Don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // Turn off the line
    this->CurrentRenderer->RemoveActor(this->LineActor);

    // Turn off the handles
    for (int i = 0; i < this->NumberOfHandles; ++i)
    {
      this->CurrentRenderer->RemoveActor(this->Handle[i]);
    }

    this->CurrentHandle = nullptr;
    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
    this->UnRegisterPickers();
  }

  this->Interactor->Render();
}

void svtkBrokenLineWidget::ProcessEventsHandler(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientdata, void* svtkNotUsed(calldata))
{
  svtkBrokenLineWidget* self = reinterpret_cast<svtkBrokenLineWidget*>(clientdata);

  // if ProcessEvents is Off, we ignore all interaction events.
  if (!self->GetProcessEvents())
  {
    return;
  }

  // Okay, let's do the right thing
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

void svtkBrokenLineWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ProcessEvents: " << (this->ProcessEvents ? "On" : "Off") << "\n";

  if (this->HandleProperty)
  {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
  }
  else
  {
    os << indent << "Handle Property: ( none )\n";
  }
  if (this->SelectedHandleProperty)
  {
    os << indent << "Selected Handle Property: " << this->SelectedHandleProperty << "\n";
  }
  else
  {
    os << indent << "Selected Handle Property: ( none )\n";
  }
  if (this->LineProperty)
  {
    os << indent << "Line Property: " << this->LineProperty << "\n";
  }
  else
  {
    os << indent << "Line Property: ( none )\n";
  }
  if (this->SelectedLineProperty)
  {
    os << indent << "Selected Line Property: " << this->SelectedLineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Line Property: ( none )\n";
  }

  os << indent << "Project To Plane: " << (this->ProjectToPlane ? "On" : "Off") << "\n";
  os << indent << "Projection Normal: " << this->ProjectionNormal << "\n";
  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";
  os << indent << "Number Of Handles: " << this->NumberOfHandles << "\n";
  os << indent << "Handle Size Factor" << this->HandleSizeFactor << "\n";
}

void svtkBrokenLineWidget::ProjectPointsToPlane()
{
  if (this->ProjectionNormal == SVTK_PROJECTION_OBLIQUE)
  {
    if (this->PlaneSource != nullptr)
    {
      this->ProjectPointsToObliquePlane();
    }
    else
    {
      svtkGenericWarningMacro(<< "Set the plane source for oblique projections...");
    }
  }
  else
  {
    this->ProjectPointsToOrthoPlane();
  }
}

void svtkBrokenLineWidget::ProjectPointsToObliquePlane()
{
  double o[3];
  double u[3];
  double v[3];

  this->PlaneSource->GetPoint1(u);
  this->PlaneSource->GetPoint2(v);
  this->PlaneSource->GetOrigin(o);

  int i;
  for (i = 0; i < 3; ++i)
  {
    u[i] = u[i] - o[i];
    v[i] = v[i] - o[i];
  }
  svtkMath::Normalize(u);
  svtkMath::Normalize(v);

  double o_dot_u = svtkMath::Dot(o, u);
  double o_dot_v = svtkMath::Dot(o, v);
  double fac1;
  double fac2;
  double ctr[3];
  for (i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    fac1 = svtkMath::Dot(ctr, u) - o_dot_u;
    fac2 = svtkMath::Dot(ctr, v) - o_dot_v;
    ctr[0] = o[0] + fac1 * u[0] + fac2 * v[0];
    ctr[1] = o[1] + fac1 * u[1] + fac2 * v[1];
    ctr[2] = o[2] + fac1 * u[2] + fac2 * v[2];
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
  }
}

void svtkBrokenLineWidget::ProjectPointsToOrthoPlane()
{
  double ctr[3];
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    ctr[this->ProjectionNormal] = this->ProjectionPosition;
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
  }
}

//------------------------------------------------------------------------------
void svtkBrokenLineWidget::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->HandlePicker, this);
  pm->AddPicker(this->LinePicker, this);
}

void svtkBrokenLineWidget::BuildRepresentation()
{
  // Get points array from line source
  svtkPoints* points = this->LineSource->GetPoints();
  if (points->GetNumberOfPoints() != this->NumberOfHandles)
  {
    points->SetNumberOfPoints(this->NumberOfHandles);
  }

  double pt[3];
  int i;
  for (i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(pt);
    points->SetPoint(i, pt);
  }
  this->LineSource->Modified();
}

int svtkBrokenLineWidget::HighlightHandle(svtkProp* prop)
{
  // First unhighlight anything picked
  if (this->CurrentHandle)
  {
    this->CurrentHandle->SetProperty(this->HandleProperty);
  }

  this->CurrentHandle = static_cast<svtkActor*>(prop);

  if (this->CurrentHandle)
  {
    for (int i = 0; i < this->NumberOfHandles; ++i) // find handle
    {
      if (this->CurrentHandle == this->Handle[i])
      {
        this->ValidPick = 1;
        this->HandlePicker->GetPickPosition(this->LastPickPosition);
        this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
        return i;
      }
    }
  }
  return -1;
}

void svtkBrokenLineWidget::HighlightLine(int highlight)
{
  if (highlight)
  {
    this->ValidPick = 1;
    this->LinePicker->GetPickPosition(this->LastPickPosition);
    this->LineActor->SetProperty(this->SelectedLineProperty);
  }
  else
  {
    this->LineActor->SetProperty(this->LineProperty);
  }
}

void svtkBrokenLineWidget::OnLeftButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkBrokenLineWidget::Outside;
    return;
  }

  this->State = svtkBrokenLineWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the line.
  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    this->CurrentHandleIndex = this->HighlightHandle(path->GetFirstNode()->GetViewProp());
  }
  else
  {
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

    if (path != nullptr)
    {
      this->HighlightLine(1);
    }
    else
    {
      this->CurrentHandleIndex = this->HighlightHandle(nullptr);
      this->State = svtkBrokenLineWidget::Outside;
      return;
    }
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBrokenLineWidget::OnLeftButtonUp()
{
  if (this->State == svtkBrokenLineWidget::Outside || this->State == svtkBrokenLineWidget::Start)
  {
    return;
  }

  this->State = svtkBrokenLineWidget::Start;
  this->HighlightHandle(nullptr);
  this->HighlightLine(0);

  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBrokenLineWidget::OnMiddleButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkBrokenLineWidget::Outside;
    return;
  }

  if (this->Interactor->GetControlKey())
  {
    this->State = svtkBrokenLineWidget::Spinning;
    this->CalculateCentroid();
  }
  else
  {
    this->State = svtkBrokenLineWidget::Moving;
  }

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the line.
  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path == nullptr)
  {
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

    if (path == nullptr)
    {
      this->State = svtkBrokenLineWidget::Outside;
      this->HighlightLine(0);
      return;
    }
    else
    {
      this->HighlightLine(1);
    }
  }
  else // we picked a handle but lets make it look like the line is picked
  {
    this->HighlightLine(1);
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBrokenLineWidget::OnMiddleButtonUp()
{
  if (this->State == svtkBrokenLineWidget::Outside || this->State == svtkBrokenLineWidget::Start)
  {
    return;
  }

  this->State = svtkBrokenLineWidget::Start;
  this->HighlightLine(0);

  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBrokenLineWidget::OnRightButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkBrokenLineWidget::Outside;
    return;
  }

  if (this->Interactor->GetShiftKey())
  {
    this->State = svtkBrokenLineWidget::Inserting;
  }
  else if (this->Interactor->GetControlKey())
  {
    this->State = svtkBrokenLineWidget::Erasing;
  }
  else
  {
    this->State = svtkBrokenLineWidget::Scaling;
  }

  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    switch (this->State)
    {
      // deny insertion over existing handles
      case svtkBrokenLineWidget::Inserting:
        this->State = svtkBrokenLineWidget::Outside;
        return;
      case svtkBrokenLineWidget::Erasing:
        this->CurrentHandleIndex = this->HighlightHandle(path->GetFirstNode()->GetViewProp());
        break;
      case svtkBrokenLineWidget::Scaling:
        this->HighlightLine(1);
        break;
    }
  }
  else
  {
    // trying to erase handle but nothing picked
    if (this->State == svtkBrokenLineWidget::Erasing)
    {
      this->State = svtkBrokenLineWidget::Outside;
      return;
    }

    // try to insert or scale so pick the line
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

    if (path != nullptr)
    {
      this->HighlightLine(1);
    }
    else
    {
      this->State = svtkBrokenLineWidget::Outside;
      return;
    }
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBrokenLineWidget::OnRightButtonUp()
{
  if (this->State == svtkBrokenLineWidget::Outside || this->State == svtkBrokenLineWidget::Start)
  {
    return;
  }

  if (this->State == svtkBrokenLineWidget::Inserting)
  {
    this->InsertHandleOnLine(this->LastPickPosition);
  }
  else if (this->State == svtkBrokenLineWidget::Erasing)
  {
    int index = this->CurrentHandleIndex;
    this->CurrentHandleIndex = this->HighlightHandle(nullptr);
    this->EraseHandle(index);
  }

  this->State = svtkBrokenLineWidget::Start;
  this->HighlightLine(0);

  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBrokenLineWidget::OnMouseMove()
{
  // See whether we're active
  if (this->State == svtkBrokenLineWidget::Outside || this->State == svtkBrokenLineWidget::Start)
  {
    return;
  }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

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
  if (this->State == svtkBrokenLineWidget::Moving)
  {
    // Okay to process
    if (this->CurrentHandle)
    {
      this->MovePoint(prevPickPoint, pickPoint);
    }
    else // Must be moving the spline
    {
      this->Translate(prevPickPoint, pickPoint);
    }
  }
  else if (this->State == svtkBrokenLineWidget::Scaling)
  {
    this->Scale(prevPickPoint, pickPoint, X, Y);
  }
  else if (this->State == svtkBrokenLineWidget::Spinning)
  {
    camera->GetViewPlaneNormal(vpn);
    this->Spin(prevPickPoint, pickPoint, vpn);
  }

  if (this->ProjectToPlane)
  {
    this->ProjectPointsToPlane();
  }

  this->BuildRepresentation();

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBrokenLineWidget::MovePoint(double* p1, double* p2)
{
  if (this->CurrentHandleIndex < 0 || this->CurrentHandleIndex >= this->NumberOfHandles)
  {
    svtkGenericWarningMacro(<< "BrokenLine handle index out of range.");
    return;
  }
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double* ctr = this->HandleGeometry[this->CurrentHandleIndex]->GetCenter();

  double newCtr[3];
  newCtr[0] = ctr[0] + v[0];
  newCtr[1] = ctr[1] + v[1];
  newCtr[2] = ctr[2] + v[2];

  this->HandleGeometry[this->CurrentHandleIndex]->SetCenter(newCtr);
  this->HandleGeometry[this->CurrentHandleIndex]->Update();
}

void svtkBrokenLineWidget::Translate(double* p1, double* p2)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double newCtr[3];
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    double* ctr = this->HandleGeometry[i]->GetCenter();
    for (int j = 0; j < 3; ++j)
    {
      newCtr[j] = ctr[j] + v[j];
    }
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

void svtkBrokenLineWidget::Scale(double* p1, double* p2, int svtkNotUsed(X), int Y)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double center[3] = { 0., 0., 0. };
  double avgdist = 0.;
  double* prevctr = this->HandleGeometry[0]->GetCenter();
  double* ctr;

  center[0] += prevctr[0];
  center[1] += prevctr[1];
  center[2] += prevctr[2];

  int i;
  for (i = 1; i < this->NumberOfHandles; ++i)
  {
    ctr = this->HandleGeometry[i]->GetCenter();
    center[0] += ctr[0];
    center[1] += ctr[1];
    center[2] += ctr[2];
    avgdist += sqrt(svtkMath::Distance2BetweenPoints(ctr, prevctr));
    prevctr = ctr;
  }

  avgdist /= this->NumberOfHandles;

  center[0] /= this->NumberOfHandles;
  center[1] /= this->NumberOfHandles;
  center[2] /= this->NumberOfHandles;

  // Compute the scale factor
  double sf = svtkMath::Norm(v) / avgdist;
  if (Y > this->Interactor->GetLastEventPosition()[1])
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  // Move the handle points
  double newCtr[3];
  for (i = 0; i < this->NumberOfHandles; ++i)
  {
    ctr = this->HandleGeometry[i]->GetCenter();
    for (int j = 0; j < 3; ++j)
    {
      newCtr[j] = sf * (ctr[j] - center[j]) + center[j];
    }
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

void svtkBrokenLineWidget::Spin(double* p1, double* p2, double* vpn)
{
  // Mouse motion vector in world space
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Axis of rotation
  double axis[3] = { 0., 0., 0. };

  if (this->ProjectToPlane)
  {
    if (this->ProjectionNormal == SVTK_PROJECTION_OBLIQUE)
    {
      if (this->PlaneSource != nullptr)
      {
        double* normal = this->PlaneSource->GetNormal();
        axis[0] = normal[0];
        axis[1] = normal[1];
        axis[2] = normal[2];
        svtkMath::Normalize(axis);
      }
      else
      {
        axis[0] = 1.;
      }
    }
    else
    {
      axis[this->ProjectionNormal] = 1.;
    }
  }
  else
  {
    // Create axis of rotation and angle of rotation
    svtkMath::Cross(vpn, v, axis);
    if (svtkMath::Normalize(axis) == 0.)
    {
      return;
    }
  }

  // Radius vector ( from mean center to cursor position )
  double rv[3] = { p2[0] - this->Centroid[0], p2[1] - this->Centroid[1],
    p2[2] - this->Centroid[2] };

  // Distance between center and cursor location
  double rs = svtkMath::Normalize(rv);

  // Spin direction
  double ax_cross_rv[3];
  svtkMath::Cross(axis, rv, ax_cross_rv);

  // Spin angle
  double theta = 360. * svtkMath::Dot(v, ax_cross_rv) / rs;

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(this->Centroid[0], this->Centroid[1], this->Centroid[2]);
  this->Transform->RotateWXYZ(theta, axis);
  this->Transform->Translate(-this->Centroid[0], -this->Centroid[1], -this->Centroid[2]);

  // Set the handle points
  double newCtr[3];
  double ctr[3];
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Transform->TransformPoint(ctr, newCtr);
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

void svtkBrokenLineWidget::CreateDefaultProperties()
{
  if (!this->HandleProperty)
  {
    this->HandleProperty = svtkProperty::New();
    this->HandleProperty->SetColor(1, 1, 1);
  }
  if (!this->SelectedHandleProperty)
  {
    this->SelectedHandleProperty = svtkProperty::New();
    this->SelectedHandleProperty->SetColor(1, 0, 0);
  }

  if (!this->LineProperty)
  {
    this->LineProperty = svtkProperty::New();
    this->LineProperty->SetRepresentationToWireframe();
    this->LineProperty->SetAmbient(1.);
    this->LineProperty->SetColor(1., 1., 0.);
    this->LineProperty->SetLineWidth(2.);
  }
  if (!this->SelectedLineProperty)
  {
    this->SelectedLineProperty = svtkProperty::New();
    this->SelectedLineProperty->SetRepresentationToWireframe();
    this->SelectedLineProperty->SetAmbient(1.);
    this->SelectedLineProperty->SetAmbientColor(0., 1., 0.);
    this->SelectedLineProperty->SetLineWidth(2.);
  }
}

void svtkBrokenLineWidget::PlaceWidget(double bds[6])
{
  double bounds[6], center[3];
  this->AdjustBounds(bds, bounds, center);

  if (this->ProjectToPlane)
  {
    this->ProjectPointsToPlane();
  }
  else // place the center
  {
    // Create a default straight line within the data bounds
    double x0 = bounds[0];
    double x1 = bounds[1];
    double y0 = bounds[2];
    double y1 = bounds[3];
    double z0 = bounds[4];
    double z1 = bounds[5];
    double x;
    double y;
    double z;
    double u;
    for (int i = 0; i < this->NumberOfHandles; ++i)
    {
      u = i / (this->NumberOfHandles - 1.0);
      x = (1.0 - u) * x0 + u * x1;
      y = (1.0 - u) * y0 + u * y1;
      z = (1.0 - u) * z0 + u * z1;
      this->HandleGeometry[i]->SetCenter(x, y, z);
    }
  }

  for (int i = 0; i < 6; ++i)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  this->BuildRepresentation();
  this->SizeHandles();
}

void svtkBrokenLineWidget::SetProjectionPosition(double position)
{
  this->ProjectionPosition = position;
  if (this->ProjectToPlane)
  {
    this->ProjectPointsToPlane();
  }
  this->BuildRepresentation();
}

void svtkBrokenLineWidget::SetPlaneSource(svtkPlaneSource* plane)
{
  if (this->PlaneSource == plane)
  {
    return;
  }
  this->PlaneSource = plane;
}

void svtkBrokenLineWidget::SetNumberOfHandles(int npts)
{
  if (this->NumberOfHandles == npts)
  {
    return;
  }
  if (npts < 2)
  {
    svtkGenericWarningMacro(<< "Minimum of 2 points required to define a broken line.");
    return;
  }

  double radius = this->HandleGeometry[0]->GetRadius();
  this->Initialize();

  this->NumberOfHandles = npts;

  // Create the handles
  this->Handle = new svtkActor*[this->NumberOfHandles];
  this->HandleGeometry = new svtkSphereSource*[this->NumberOfHandles];

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i] = svtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    svtkPolyDataMapper* handleMapper = svtkPolyDataMapper::New();
    handleMapper->SetInputConnection(this->HandleGeometry[i]->GetOutputPort());
    this->Handle[i] = svtkActor::New();
    this->Handle[i]->SetMapper(handleMapper);
    handleMapper->Delete();
    this->Handle[i]->SetProperty(this->HandleProperty);

    this->HandleGeometry[i]->SetRadius(radius);
    this->HandlePicker->AddPickList(this->Handle[i]);
  }

  if (this->Interactor)
  {
    if (!this->CurrentRenderer)
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0], this->Interactor->GetLastEventPosition()[1]));
    }
    if (this->CurrentRenderer != nullptr)
    {
      for (int i = 0; i < this->NumberOfHandles; ++i)
      {
        this->CurrentRenderer->AddViewProp(this->Handle[i]);
      }
      this->SizeHandles();
    }
    this->Interactor->Render();
  }
}

void svtkBrokenLineWidget::Initialize()
{
  int i;
  if (this->Interactor)
  {
    if (!this->CurrentRenderer)
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0], this->Interactor->GetLastEventPosition()[1]));
    }
    if (this->CurrentRenderer != nullptr)
    {
      for (i = 0; i < this->NumberOfHandles; ++i)
      {
        this->CurrentRenderer->RemoveViewProp(this->Handle[i]);
      }
    }
  }

  for (i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandlePicker->DeletePickList(this->Handle[i]);
    this->HandleGeometry[i]->Delete();
    this->Handle[i]->Delete();
  }

  this->NumberOfHandles = 0;

  delete[] this->Handle;
  delete[] this->HandleGeometry;
}

void svtkBrokenLineWidget::GetPolyData(svtkPolyData* pd)
{
  pd->ShallowCopy(this->LineSource->GetOutput());
}

void svtkBrokenLineWidget::SizeHandles()
{
  double radius = this->svtk3DWidget::SizeHandles(this->HandleSizeFactor);
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->SetRadius(radius);
  }
}

double svtkBrokenLineWidget::GetSummedLength()
{
  svtkPoints* points = this->LineSource->GetOutput()->GetPoints();
  if (!points)
  {
    return 0.;
  }

  int npts = points->GetNumberOfPoints();
  if (npts < 2)
  {
    return 0.;
  }

  double a[3];
  double b[3];
  double sum = 0.;
  int i = 0;
  points->GetPoint(i, a);
  int imax = (npts % 2 == 0) ? npts - 2 : npts - 1;

  while (i < imax)
  {
    points->GetPoint(i + 1, b);
    sum += sqrt(svtkMath::Distance2BetweenPoints(a, b));
    i = i + 2;
    points->GetPoint(i, a);
    sum = sum + sqrt(svtkMath::Distance2BetweenPoints(a, b));
  }

  if (npts % 2 == 0)
  {
    points->GetPoint(i + 1, b);
    sum += sqrt(svtkMath::Distance2BetweenPoints(a, b));
  }

  return sum;
}

void svtkBrokenLineWidget::CalculateCentroid()
{
  this->Centroid[0] = 0.;
  this->Centroid[1] = 0.;
  this->Centroid[2] = 0.;

  double ctr[3];
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Centroid[0] += ctr[0];
    this->Centroid[1] += ctr[1];
    this->Centroid[2] += ctr[2];
  }

  this->Centroid[0] /= this->NumberOfHandles;
  this->Centroid[1] /= this->NumberOfHandles;
  this->Centroid[2] /= this->NumberOfHandles;
}

void svtkBrokenLineWidget::InsertHandleOnLine(double* pos)
{
  if (this->NumberOfHandles < 2)
  {
    return;
  }

  svtkIdType id = this->LinePicker->GetCellId();
  if (id == -1)
  {
    return;
  }

  svtkIdType subid = this->LinePicker->GetSubId();
  svtkPoints* newpoints = svtkPoints::New(SVTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles + 1);

  int istart = subid;
  int istop = istart + 1;
  int count = 0;
  int i;
  for (i = 0; i <= istart; ++i)
  {
    newpoints->SetPoint(count++, this->HandleGeometry[i]->GetCenter());
  }

  newpoints->SetPoint(count++, pos);

  for (i = istop; i < this->NumberOfHandles; ++i)
  {
    newpoints->SetPoint(count++, this->HandleGeometry[i]->GetCenter());
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

void svtkBrokenLineWidget::EraseHandle(const int& index)
{
  if (this->NumberOfHandles < 3 || index < 0 || index >= this->NumberOfHandles)
  {
    return;
  }

  svtkPoints* newpoints = svtkPoints::New(SVTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles - 1);
  int count = 0;
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    if (i != index)
    {
      newpoints->SetPoint(count++, this->HandleGeometry[i]->GetCenter());
    }
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

void svtkBrokenLineWidget::InitializeHandles(svtkPoints* points)
{
  if (!points)
  {
    return;
  }

  int npts = points->GetNumberOfPoints();
  if (npts < 2)
  {
    return;
  }

  double p0[3];
  double p1[3];

  points->GetPoint(0, p0);
  points->GetPoint(npts - 1, p1);

  if (svtkMath::Distance2BetweenPoints(p0, p1) == 0.)
  {
    --npts;
  }

  this->SetNumberOfHandles(npts);
  int i;
  for (i = 0; i < npts; ++i)
  {
    this->SetHandlePosition(i, points->GetPoint(i));
  }

  if (this->Interactor && this->Enabled)
  {
    this->Interactor->Render();
  }
}
