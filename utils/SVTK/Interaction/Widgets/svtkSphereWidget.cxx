/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSphereWidget.h"

#include "svtkActor.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkDoubleArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphere.h"
#include "svtkSphereSource.h"

svtkStandardNewMacro(svtkSphereWidget);

//----------------------------------------------------------------------------
svtkSphereWidget::svtkSphereWidget()
{
  this->State = svtkSphereWidget::Start;
  this->EventCallbackCommand->SetCallback(svtkSphereWidget::ProcessEvents);

  this->Representation = SVTK_SPHERE_WIREFRAME;

  // Build the representation of the widget
  // Represent the sphere
  this->SphereSource = svtkSphereSource::New();
  this->SphereSource->SetThetaResolution(16);
  this->SphereSource->SetPhiResolution(8);
  this->SphereSource->LatLongTessellationOn();
  this->SphereMapper = svtkPolyDataMapper::New();
  this->SphereMapper->SetInputConnection(this->SphereSource->GetOutputPort());
  this->SphereActor = svtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

  // controls
  this->Translation = 1;
  this->Scale = 1;

  // handles
  this->HandleVisibility = 0;
  this->HandleDirection[0] = 1.0;
  this->HandleDirection[1] = 0.0;
  this->HandleDirection[2] = 0.0;
  this->HandleSource = svtkSphereSource::New();
  this->HandleSource->SetThetaResolution(16);
  this->HandleSource->SetPhiResolution(8);
  this->HandleMapper = svtkPolyDataMapper::New();
  this->HandleMapper->SetInputConnection(this->HandleSource->GetOutputPort());
  this->HandleActor = svtkActor::New();
  this->HandleActor->SetMapper(this->HandleMapper);

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
  this->Picker = svtkCellPicker::New();
  this->Picker->SetTolerance(0.005); // need some fluff
  this->Picker->AddPickList(this->SphereActor);
  this->Picker->AddPickList(this->HandleActor);
  this->Picker->PickFromListOn();

  // Set up the initial properties
  this->SphereProperty = nullptr;
  this->SelectedSphereProperty = nullptr;
  this->HandleProperty = nullptr;
  this->SelectedHandleProperty = nullptr;
  this->CreateDefaultProperties();
}

//----------------------------------------------------------------------------
svtkSphereWidget::~svtkSphereWidget()
{
  this->SphereActor->Delete();
  this->SphereMapper->Delete();
  this->SphereSource->Delete();

  this->Picker->Delete();

  this->HandleSource->Delete();
  this->HandleMapper->Delete();
  this->HandleActor->Delete();

  if (this->SphereProperty)
  {
    this->SphereProperty->Delete();
  }
  if (this->SelectedSphereProperty)
  {
    this->SelectedSphereProperty->Delete();
  }
  if (this->HandleProperty)
  {
    this->HandleProperty->Delete();
  }
  if (this->SelectedHandleProperty)
  {
    this->SelectedHandleProperty->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkSphereWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
  {
    svtkErrorMacro(<< "The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if (enabling) //----------------------------------------------------------
  {
    svtkDebugMacro(<< "Enabling sphere widget");

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
    i->AddObserver(svtkCommand::RightButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    // Add the sphere
    this->CurrentRenderer->AddActor(this->SphereActor);
    this->SphereActor->SetProperty(this->SphereProperty);

    this->CurrentRenderer->AddActor(this->HandleActor);
    this->HandleActor->SetProperty(this->HandleProperty);
    this->SelectRepresentation();
    this->SizeHandles();
    this->RegisterPickers();

    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling----------------------------------------------------------
  {
    svtkDebugMacro(<< "Disabling sphere widget");

    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the sphere
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    this->CurrentRenderer->RemoveActor(this->HandleActor);

    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
    this->UnRegisterPickers();
  }

  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void svtkSphereWidget::ProcessEvents(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientdata, void* svtkNotUsed(calldata))
{
  svtkSphereWidget* self = reinterpret_cast<svtkSphereWidget*>(clientdata);

  // okay, let's do the right thing
  switch (event)
  {
    case svtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case svtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
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

//----------------------------------------------------------------------------
void svtkSphereWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sphere Representation: ";
  if (this->Representation == SVTK_SPHERE_OFF)
  {
    os << "Off\n";
  }
  else if (this->Representation == SVTK_SPHERE_WIREFRAME)
  {
    os << "Wireframe\n";
  }
  else // if ( this->Representation == SVTK_SPHERE_SURFACE )
  {
    os << "Surface\n";
  }

  if (this->SphereProperty)
  {
    os << indent << "Sphere Property: " << this->SphereProperty << "\n";
  }
  else
  {
    os << indent << "Sphere Property: (none)\n";
  }
  if (this->SelectedSphereProperty)
  {
    os << indent << "Selected Sphere Property: " << this->SelectedSphereProperty << "\n";
  }
  else
  {
    os << indent << "Selected Sphere Property: (none)\n";
  }

  if (this->HandleProperty)
  {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
  }
  else
  {
    os << indent << "Handle Property: (none)\n";
  }
  if (this->SelectedHandleProperty)
  {
    os << indent << "Selected Handle Property: " << this->SelectedHandleProperty << "\n";
  }
  else
  {
    os << indent << "Selected Handle Property: (none)\n";
  }

  os << indent << "Translation: " << (this->Translation ? "On\n" : "Off\n");
  os << indent << "Scale: " << (this->Scale ? "On\n" : "Off\n");

  os << indent << "Handle Visibility: " << (this->HandleVisibility ? "On\n" : "Off\n");
  os << indent << "Handle Direction: (" << this->HandleDirection[0] << ", "
     << this->HandleDirection[1] << ", " << this->HandleDirection[2] << ")\n";
  os << indent << "Handle Position: (" << this->HandlePosition[0] << ", " << this->HandlePosition[1]
     << ", " << this->HandlePosition[2] << ")\n";

  int thetaRes = this->SphereSource->GetThetaResolution();
  int phiRes = this->SphereSource->GetPhiResolution();
  double* center = this->SphereSource->GetCenter();
  double r = this->SphereSource->GetRadius();

  os << indent << "Theta Resolution: " << thetaRes << "\n";
  os << indent << "Phi Resolution: " << phiRes << "\n";
  os << indent << "Center: (" << center[0] << ", " << center[1] << ", " << center[2] << ")\n";
  os << indent << "Radius: " << r << "\n";
}

//----------------------------------------------------------------------------
void svtkSphereWidget::SelectRepresentation()
{
  if (!this->HandleVisibility)
  {
    this->CurrentRenderer->RemoveActor(this->HandleActor);
  }

  if (this->Representation == SVTK_SPHERE_OFF)
  {
    this->CurrentRenderer->RemoveActor(this->SphereActor);
  }
  else if (this->Representation == SVTK_SPHERE_WIREFRAME)
  {
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    this->CurrentRenderer->AddActor(this->SphereActor);
    this->SphereProperty->SetRepresentationToWireframe();
    this->SelectedSphereProperty->SetRepresentationToWireframe();
  }
  else // if ( this->Representation == SVTK_SPHERE_SURFACE )
  {
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    this->CurrentRenderer->AddActor(this->SphereActor);
    this->SphereProperty->SetRepresentationToSurface();
    this->SelectedSphereProperty->SetRepresentationToSurface();
  }
}

//----------------------------------------------------------------------------
void svtkSphereWidget::GetSphere(svtkSphere* sphere)
{
  sphere->SetRadius(this->SphereSource->GetRadius());
  sphere->SetCenter(this->SphereSource->GetCenter());
}

//----------------------------------------------------------------------------
void svtkSphereWidget::HighlightSphere(int highlight)
{
  if (highlight)
  {
    this->ValidPick = 1;
    this->Picker->GetPickPosition(this->LastPickPosition);
    this->SphereActor->SetProperty(this->SelectedSphereProperty);
  }
  else
  {
    this->SphereActor->SetProperty(this->SphereProperty);
  }
}

//----------------------------------------------------------------------------
void svtkSphereWidget::HighlightHandle(int highlight)
{
  if (highlight)
  {
    this->ValidPick = 1;
    this->Picker->GetPickPosition(this->LastPickPosition);
    this->HandleActor->SetProperty(this->SelectedHandleProperty);
  }
  else
  {
    this->HandleActor->SetProperty(this->HandleProperty);
  }
}

//----------------------------------------------------------------------------
void svtkSphereWidget::OnLeftButtonDown()
{
  if (!this->Interactor)
  {
    return;
  }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkSphereWidget::Outside;
    return;
  }

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the sphere.
  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  if (path == nullptr)
  {
    this->State = svtkSphereWidget::Outside;
    return;
  }
  else if (path->GetFirstNode()->GetViewProp() == this->SphereActor)
  {
    this->State = svtkSphereWidget::Moving;
    this->HighlightSphere(1);
  }
  else if (path->GetFirstNode()->GetViewProp() == this->HandleActor)
  {
    this->State = svtkSphereWidget::Positioning;
    this->HighlightHandle(1);
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void svtkSphereWidget::OnMouseMove()
{
  // See whether we're active
  if (this->State == svtkSphereWidget::Outside || this->State == svtkSphereWidget::Start)
  {
    return;
  }

  if (!this->Interactor)
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
  camera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1], focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
    double(this->Interactor->GetLastEventPosition()[1]), z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if (this->State == svtkSphereWidget::Moving)
  {
    this->Translate(prevPickPoint, pickPoint);
  }
  else if (this->State == svtkSphereWidget::Scaling)
  {
    this->ScaleSphere(prevPickPoint, pickPoint, X, Y);
  }
  else if (this->State == svtkSphereWidget::Positioning)
  {
    this->MoveHandle(prevPickPoint, pickPoint, X, Y);
  }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void svtkSphereWidget::OnLeftButtonUp()
{
  if (this->State == svtkSphereWidget::Outside)
  {
    return;
  }

  this->State = svtkSphereWidget::Start;
  this->HighlightSphere(0);
  this->HighlightHandle(0);
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  if (this->Interactor)
  {
    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------------
void svtkSphereWidget::OnRightButtonDown()
{
  if (!this->Interactor)
  {
    return;
  }

  this->State = svtkSphereWidget::Scaling;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkSphereWidget::Outside;
    return;
  }

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  if (path == nullptr)
  {
    this->State = svtkSphereWidget::Outside;
    this->HighlightSphere(0);
    return;
  }
  else
  {
    this->HighlightSphere(1);
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void svtkSphereWidget::OnRightButtonUp()
{
  if (this->State == svtkSphereWidget::Outside)
  {
    return;
  }

  this->State = svtkSphereWidget::Start;
  this->HighlightSphere(0);
  this->HighlightHandle(0);
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  if (this->Interactor)
  {
    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void svtkSphereWidget::Translate(double* p1, double* p2)
{
  if (!this->Translation)
  {
    return;
  }

  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // int res = this->SphereSource->GetResolution();
  double* center = this->SphereSource->GetCenter();

  double center1[3];
  for (int i = 0; i < 3; i++)
  {
    center1[i] = center[i] + v[i];
    this->HandlePosition[i] += v[i];
  }

  this->SphereSource->SetCenter(center1);
  this->HandleSource->SetCenter(HandlePosition);

  this->SelectRepresentation();
}

//----------------------------------------------------------------------------
void svtkSphereWidget::ScaleSphere(double* p1, double* p2, int svtkNotUsed(X), int Y)
{
  if (!this->Scale)
  {
    return;
  }

  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double radius = this->SphereSource->GetRadius();
  double* c = this->SphereSource->GetCenter();

  // Compute the scale factor
  double sf = 0.0;
  if (radius > 0.0)
  {
    sf = svtkMath::Norm(v) / radius;
    if (Y > this->Interactor->GetLastEventPosition()[1])
    {
      sf = 1.0 + sf;
    }
    else
    {
      sf = 1.0 - sf;
    }
    radius *= sf;
  }
  else // Bump the radius >0 otherwise it'll never scale up from 0.0
  {
    radius = SVTK_DBL_EPSILON;
  }

  this->SphereSource->SetRadius(radius);
  this->HandlePosition[0] = c[0] + sf * (this->HandlePosition[0] - c[0]);
  this->HandlePosition[1] = c[1] + sf * (this->HandlePosition[1] - c[1]);
  this->HandlePosition[2] = c[2] + sf * (this->HandlePosition[2] - c[2]);
  this->HandleSource->SetCenter(this->HandlePosition);

  this->SelectRepresentation();
}

//----------------------------------------------------------------------------
void svtkSphereWidget::MoveHandle(double* p1, double* p2, int svtkNotUsed(X), int svtkNotUsed(Y))
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Compute the new location of the sphere
  double* center = this->SphereSource->GetCenter();
  double radius = this->SphereSource->GetRadius();

  // set the position of the sphere
  double p[3];
  for (int i = 0; i < 3; i++)
  {
    p[i] = this->HandlePosition[i] + v[i];
    this->HandleDirection[i] = p[i] - center[i];
  }

  this->PlaceHandle(center, radius);

  this->SelectRepresentation();
}

//----------------------------------------------------------------------------
void svtkSphereWidget::CreateDefaultProperties()
{
  if (!this->SphereProperty)
  {
    this->SphereProperty = svtkProperty::New();
  }
  if (!this->SelectedSphereProperty)
  {
    this->SelectedSphereProperty = svtkProperty::New();
  }

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
}

//----------------------------------------------------------------------------
void svtkSphereWidget::PlaceWidget(double bds[6])
{
  double bounds[6], center[3], radius;

  this->AdjustBounds(bds, bounds, center);

  radius = (bounds[1] - bounds[0]) / 2.0;
  if (radius > ((bounds[3] - bounds[2]) / 2.0))
  {
    radius = (bounds[3] - bounds[2]) / 2.0;
  }
  radius = (bounds[1] - bounds[0]) / 2.0;
  if (radius > ((bounds[5] - bounds[4]) / 2.0))
  {
    radius = (bounds[5] - bounds[4]) / 2.0;
  }

  this->SphereSource->SetCenter(center);
  this->SphereSource->SetRadius(radius);
  this->SphereSource->Update();

  // place the handle
  this->PlaceHandle(center, radius);

  for (int i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  this->SizeHandles();
}

//----------------------------------------------------------------------------
void svtkSphereWidget::PlaceHandle(double* center, double radius)
{
  double sf = radius / svtkMath::Norm(this->HandleDirection);

  this->HandlePosition[0] = center[0] + sf * this->HandleDirection[0];
  this->HandlePosition[1] = center[1] + sf * this->HandleDirection[1];
  this->HandlePosition[2] = center[2] + sf * this->HandleDirection[2];
  this->HandleSource->SetCenter(this->HandlePosition);
}

//----------------------------------------------------------------------------
void svtkSphereWidget::SizeHandles()
{
  double radius = this->svtk3DWidget::SizeHandles(1.25);
  this->HandleSource->SetRadius(radius);
}

//------------------------------------------------------------------------------
void svtkSphereWidget::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}

//----------------------------------------------------------------------------
void svtkSphereWidget::GetPolyData(svtkPolyData* pd)
{
  pd->ShallowCopy(this->SphereSource->GetOutput());
}
