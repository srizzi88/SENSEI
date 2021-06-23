/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractPolygonalHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractPolygonalHandleRepresentation3D.h"
#include "svtkActor.h"
#include "svtkAssemblyPath.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkCoordinate.h"
#include "svtkCursor3D.h"
#include "svtkFocalPlanePointPlacer.h"
#include "svtkFollower.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkMatrixToLinearTransform.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkVectorText.h"

#include <assert.h>

svtkCxxSetObjectMacro(svtkAbstractPolygonalHandleRepresentation3D, Property, svtkProperty);
svtkCxxSetObjectMacro(svtkAbstractPolygonalHandleRepresentation3D, SelectedProperty, svtkProperty);

//----------------------------------------------------------------------
svtkAbstractPolygonalHandleRepresentation3D ::svtkAbstractPolygonalHandleRepresentation3D()
{
  this->InteractionState = svtkHandleRepresentation::Outside;

  this->HandleTransformFilter = svtkTransformPolyDataFilter::New();
  this->HandleTransform = svtkMatrixToLinearTransform::New();
  this->HandleTransformMatrix = svtkMatrix4x4::New();
  this->HandleTransformMatrix->Identity();
  this->HandleTransform->SetInput(this->HandleTransformMatrix);
  this->HandleTransformFilter->SetTransform(this->HandleTransform);

  // initialized because it is used in PrintSelf
  this->LastPickPosition[0] = 0.0;
  this->LastPickPosition[1] = 0.0;
  this->LastPickPosition[2] = 0.0;

  // initialized because it is used in PrintSelf
  this->LastEventPosition[0] = 0.0;
  this->LastEventPosition[1] = 0.0;

  this->Mapper = svtkPolyDataMapper::New();
  this->Mapper->ScalarVisibilityOff();
  this->Mapper->SetInputConnection(this->HandleTransformFilter->GetOutputPort());

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = nullptr; // Created by subclass.svtkFollower::New();

  // Manage the picking stuff
  this->HandlePicker = svtkCellPicker::New();
  this->HandlePicker->PickFromListOn();
  this->HandlePicker->SetTolerance(0.01); // need some fluff

  // Override superclass'
  this->PlaceFactor = 1.0;
  this->WaitingForMotion = 0;
  this->ConstraintAxis = -1;

  svtkFocalPlanePointPlacer* pointPlacer = svtkFocalPlanePointPlacer::New();
  this->SetPointPlacer(pointPlacer);
  pointPlacer->Delete();

  // Label stuff.
  this->LabelAnnotationTextScaleInitialized = false;
  this->LabelVisibility = 0;
  this->HandleVisibility = 1;
  this->LabelTextInput = svtkVectorText::New();
  this->LabelTextInput->SetText("0");
  this->LabelTextMapper = svtkPolyDataMapper::New();
  this->LabelTextMapper->SetInputConnection(this->LabelTextInput->GetOutputPort());
  this->LabelTextActor = svtkFollower::New();
  this->LabelTextActor->SetMapper(this->LabelTextMapper);
  this->LabelTextActor->GetProperty()->SetColor(1.0, 0.1, 0.0);

  this->SmoothMotion = 1;
}

//----------------------------------------------------------------------
svtkAbstractPolygonalHandleRepresentation3D ::~svtkAbstractPolygonalHandleRepresentation3D()
{
  this->HandleTransformFilter->Delete();
  this->HandleTransform->Delete();
  this->HandleTransformMatrix->Delete();
  this->HandlePicker->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  this->SelectedProperty->Delete();
  this->LabelTextInput->Delete();
  this->LabelTextMapper->Delete();
  this->LabelTextActor->Delete();
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->HandlePicker, this);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::SetHandle(svtkPolyData* pd)
{
  this->HandleTransformFilter->SetInputData(pd);
}

//----------------------------------------------------------------------
svtkPolyData* svtkAbstractPolygonalHandleRepresentation3D::GetHandle()
{
  return svtkPolyData::SafeDownCast(this->HandleTransformFilter->GetInput());
}

//-------------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::SetWorldPosition(double p[3])
{
  if (!this->Renderer || !this->PointPlacer || this->PointPlacer->ValidateWorldPosition(p))
  {
    this->WorldPosition->SetValue(p);
    this->WorldPositionTime.Modified();
    this->Modified();
  }
}

//-------------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::SetDisplayPosition(double p[3])
{
  if (this->Renderer && this->PointPlacer)
  {
    if (this->PointPlacer->ValidateDisplayPosition(this->Renderer, p))
    {
      double worldPos[3], worldOrient[9];
      if (this->PointPlacer->ComputeWorldPosition(this->Renderer, p, worldPos, worldOrient))
      {
        this->DisplayPosition->SetValue(p);
        this->WorldPosition->SetValue(worldPos);
        this->DisplayPositionTime.Modified();
        this->SetWorldPosition(this->WorldPosition->GetValue());
      }
    }
  }
  else
  {
    this->DisplayPosition->SetValue(p);
    this->DisplayPositionTime.Modified();
  }
}

//-------------------------------------------------------------------------
int svtkAbstractPolygonalHandleRepresentation3D ::ComputeInteractionState(
  int X, int Y, int svtkNotUsed(modify))
{
  this->VisibilityOn(); // actor must be on to be picked
  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    this->InteractionState = svtkHandleRepresentation::Nearby;
  }
  else
  {
    this->InteractionState = svtkHandleRepresentation::Outside;
    if (this->ActiveRepresentation)
    {
      this->VisibilityOff();
    }
  }

  return this->InteractionState;
}

//-------------------------------------------------------------------------
int svtkAbstractPolygonalHandleRepresentation3D::DetermineConstraintAxis(
  int constraint, double* x, double* startPickPoint)
{
  // Look for trivial cases
  if (!this->Constrained)
  {
    return -1;
  }
  else if (constraint >= 0 && constraint < 3)
  {
    return constraint;
  }

  // Okay, figure out constraint. First see if the choice is
  // outside the hot spot
  if (!x)
  {
    double p[3];
    this->HandlePicker->GetPickPosition(p);
    if (svtkMath::Distance2BetweenPoints(p, this->LastPickPosition) > 0.0)
    {
      this->WaitingForMotion = 0;
      return 0;
    }
    else
    {
      this->WaitingForMotion = 1;
      this->WaitCount = 0;
      return -1;
    }
  }
  else if (x)
  {
    this->WaitingForMotion = 0;
    double v[3];
    v[0] = fabs(x[0] - startPickPoint[0]);
    v[1] = fabs(x[1] - startPickPoint[1]);
    v[2] = fabs(x[2] - startPickPoint[2]);
    return (v[0] > v[1] ? (v[0] > v[2] ? 0 : 2) : (v[1] > v[2] ? 1 : 2));
  }
  else
  {
    return -1;
  }
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void svtkAbstractPolygonalHandleRepresentation3D::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  svtkAssemblyPath* path =
    this->GetAssemblyPath(startEventPos[0], startEventPos[1], 0., this->HandlePicker);

  // Did we pick the handle ?
  if (path)
  {
    this->InteractionState = svtkHandleRepresentation::Nearby;
    this->ConstraintAxis = -1;
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
  }
  else
  {
    this->InteractionState = svtkHandleRepresentation::Outside;
    this->ConstraintAxis = -1;
  }
  this->WaitCount = 0;
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void svtkAbstractPolygonalHandleRepresentation3D::WidgetInteraction(double eventPos[2])
{
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4], startPickPoint[4], z;

  // Compute the two points defining the motion vector
  svtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, this->LastPickPosition[0],
    this->LastPickPosition[1], this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  svtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
  svtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, eventPos[0], eventPos[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == svtkHandleRepresentation::Selecting ||
    this->InteractionState == svtkHandleRepresentation::Translating)
  {
    this->WaitCount++;

    if (this->WaitCount > 3 || !this->Constrained)
    {
      svtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, this->StartEventPosition[0],
        this->StartEventPosition[1], z, startPickPoint);

      this->ConstraintAxis =
        this->DetermineConstraintAxis(this->ConstraintAxis, pickPoint, startPickPoint);

      if (this->InteractionState == svtkHandleRepresentation::Selecting)
      {
        // If we are doing axis constrained motion, ignore the placer.
        // Can't have both the placer and an axis constraint dictating
        // handle placement.
        if (this->ConstraintAxis >= 0 || this->Constrained || !this->PointPlacer)
        {
          this->MoveFocus(prevPickPoint, pickPoint);
        }
        else
        {
          double newCenterPointRequested[3]; // displayPosition
          double newCenterPoint[3], worldOrient[9];

          // Make a request for the new position.
          this->MoveFocusRequest(prevPickPoint, pickPoint, eventPos, newCenterPointRequested);

          svtkFocalPlanePointPlacer* fPlacer =
            svtkFocalPlanePointPlacer::SafeDownCast(this->PointPlacer);
          if (fPlacer)
          {
            // Offset the placer plane to one that passes through the current
            // world position and is parallel to the focal plane. Offset =
            // the distance currentWorldPos is from the focal plane
            //
            double currentWorldPos[3], projDir[3], fp[3];
            this->GetWorldPosition(currentWorldPos);
            this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
            double vec[3] = { currentWorldPos[0] - fp[0], currentWorldPos[1] - fp[1],
              currentWorldPos[2] - fp[2] };
            this->Renderer->GetActiveCamera()->GetDirectionOfProjection(projDir);
            fPlacer->SetOffset(svtkMath::Dot(vec, projDir));
          }

          // See what the placer says.
          if (this->PointPlacer->ComputeWorldPosition(
                this->Renderer, newCenterPointRequested, newCenterPoint, worldOrient))
          {
            // Once the placer has validated us, update the handle position
            this->SetWorldPosition(newCenterPoint);
          }
        }
      }
      else
      {
        // If we are doing axis constrained motion, ignore the placer.
        // Can't have both the placer and the axis constraint dictating
        // handle placement.
        if (this->ConstraintAxis >= 0 || this->Constrained || !this->PointPlacer)
        {
          this->Translate(prevPickPoint, pickPoint);
        }
        else
        {
          double newCenterPointRequested[3]; // displayPosition
          double newCenterPoint[3], worldOrient[9];

          // Make a request for the new position.
          this->MoveFocusRequest(prevPickPoint, pickPoint, eventPos, newCenterPointRequested);

          svtkFocalPlanePointPlacer* fPlacer =
            svtkFocalPlanePointPlacer::SafeDownCast(this->PointPlacer);
          if (fPlacer)
          {
            // Offset the placer plane to one that passes through the current
            // world position and is parallel to the focal plane. Offset =
            // the distance currentWorldPos is from the focal plane
            //
            double currentWorldPos[3], projDir[3], fp[3];
            this->GetWorldPosition(currentWorldPos);
            this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
            double vec[3] = { currentWorldPos[0] - fp[0], currentWorldPos[1] - fp[1],
              currentWorldPos[2] - fp[2] };
            this->Renderer->GetActiveCamera()->GetDirectionOfProjection(projDir);
            fPlacer->SetOffset(svtkMath::Dot(vec, projDir));
          }

          // See what the placer says.
          if (this->PointPlacer->ComputeWorldPosition(
                this->Renderer, newCenterPointRequested, newCenterPoint, worldOrient))
          {
            this->SetWorldPosition(newCenterPoint);
          }
        }
      }
    }
  }

  else if (this->InteractionState == svtkHandleRepresentation::Scaling)
  {
    // Scaling does not change the position of the handle, we needn't
    // ask the placer..
    this->Scale(prevPickPoint, pickPoint, eventPos);
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];

  this->Modified();
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D ::MoveFocusRequest(
  const double* p1, const double* p2, const double currPos[2], double center[3])
{
  if (this->SmoothMotion)
  {
    double focus[4], v[3] = { 0, 0, 0 };
    this->GetWorldPosition(focus);

    // Move the center of the handle along the motion vector
    if (!this->IsTranslationConstrained())
    {
      v[0] = p2[0] - p1[0];
      v[1] = p2[1] - p1[1];
      v[2] = p2[2] - p1[2];
    }
    else
    {
      assert(this->TranslationAxis > -1 && this->TranslationAxis < 3 &&
        "this->TranslationAxis out of bounds");
      v[this->TranslationAxis] = p2[this->TranslationAxis] - p1[this->TranslationAxis];
    }

    focus[0] += v[0];
    focus[1] += v[1];
    focus[2] += v[2];
    focus[3] = 1.0;

    // Get the display position that this center would fall on.
    this->Renderer->SetWorldPoint(focus);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(center);
  }
  else
  {
    center[0] = currPos[0];
    center[1] = currPos[1];
    center[2] = 1.0;
  }
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::MoveFocus(const double* p1, const double* p2)
{
  this->Translate(p1, p2);
}

//----------------------------------------------------------------------
// Translate everything
void svtkAbstractPolygonalHandleRepresentation3D::Translate(const double* p1, const double* p2)
{
  // Get the motion vector
  double v[3] = { 0, 0, 0 }, pos[3];
  if (!this->IsTranslationConstrained())
  {
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
  }
  else
  {
    assert(this->TranslationAxis > -1 && this->TranslationAxis < 3 &&
      "this->TranslationAxis out of bounds");
    v[this->TranslationAxis] = p2[this->TranslationAxis] - p1[this->TranslationAxis];
  }

  this->GetWorldPosition(pos);
  double newFocus[3];

  for (int i = 0; i < 3; i++)
  {
    newFocus[i] = pos[i] + v[i];
  }

  this->SetWorldPosition(newFocus);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D ::Scale(
  const double*, const double*, const double eventPos[2])
{
  double sf = 1.0 + (eventPos[1] - this->LastEventPosition[1]) / this->Renderer->GetSize()[1];
  if (sf == 1.0)
  {
    return;
  }

  double handleSize = this->HandleTransformMatrix->GetElement(0, 0) * sf;
  handleSize = (handleSize < 0.001 ? 0.001 : handleSize);

  this->SetUniformScale(handleSize);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D ::SetUniformScale(double handleSize)
{
  this->HandleTransformMatrix->SetElement(0, 0, handleSize);
  this->HandleTransformMatrix->SetElement(1, 1, handleSize);
  this->HandleTransformMatrix->SetElement(2, 2, handleSize);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::Highlight(int highlight)
{
  this->Actor->SetProperty(highlight ? this->SelectedProperty : this->Property);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::CreateDefaultProperties()
{
  this->Property = svtkProperty::New();
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = svtkProperty::New();
  this->SelectedProperty->SetAmbient(1.0);
  this->SelectedProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::UpdateHandle()
{
  // Subclasses should override this.
  this->HandleTransformFilter->Update();
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::BuildRepresentation()
{
  // The net effect is to resize the handle
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {

    // Update the handle.
    this->UpdateHandle();

    // Update the label,
    this->UpdateLabel();

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::UpdateLabel()
{
  // Display the label if needed.
  if (this->LabelVisibility)
  {
    if (this->Renderer)
    {
      this->LabelTextActor->SetCamera(this->Renderer->GetActiveCamera());
    }
    else
    {
      svtkErrorMacro("UpdateLabel: no renderer has been set!");
      return;
    }

    // Place the label on the North east of the handle. We need to take into
    // account the viewup vector and the direction of the camera, so that we
    // can bring it on the closest plane of the widget facing the camera.
    double labelPosition[3], vup[3], directionOfProjection[3], xAxis[3], bounds[6];
    this->Renderer->GetActiveCamera()->GetViewUp(vup);
    this->Renderer->GetActiveCamera()->GetDirectionOfProjection(directionOfProjection);
    svtkMath::Cross(directionOfProjection, vup, xAxis);
    this->Mapper->GetBounds(bounds);
    double width = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
      (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
      (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
    this->GetWorldPosition(labelPosition);
    labelPosition[0] += width / 2.0 * xAxis[0];
    labelPosition[1] += width / 2.0 * xAxis[1];
    labelPosition[2] += width / 2.0 * xAxis[2];

    this->LabelTextActor->SetPosition(labelPosition);

    if (!this->LabelAnnotationTextScaleInitialized)
    {
      // If a font size hasn't been specified by the user, scale the text
      // (font size) according to the size of the handle .
      this->LabelTextActor->SetScale(width / 3.0, width / 3.0, width / 3.0);
    }
  }
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::ShallowCopy(svtkProp* prop)
{
  svtkAbstractPolygonalHandleRepresentation3D* rep =
    svtkAbstractPolygonalHandleRepresentation3D::SafeDownCast(prop);
  if (rep)
  {
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);

    // copy the handle shape
    this->HandleTransformFilter->SetInputConnection(
      rep->HandleTransformFilter->GetInputConnection(0, 0));

    this->LabelVisibility = rep->LabelVisibility;
    this->SetLabelText(rep->GetLabelText());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::DeepCopy(svtkProp* prop)
{
  svtkAbstractPolygonalHandleRepresentation3D* rep =
    svtkAbstractPolygonalHandleRepresentation3D::SafeDownCast(prop);
  if (rep)
  {
    this->Property->DeepCopy(rep->GetProperty());
    this->SelectedProperty->DeepCopy(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);

    // copy the handle shape
    svtkPolyData* pd = svtkPolyData::New();
    pd->DeepCopy(rep->HandleTransformFilter->GetInput());
    this->HandleTransformFilter->SetInputData(pd);
    pd->Delete();

    this->LabelVisibility = rep->LabelVisibility;
    this->SetLabelText(rep->GetLabelText());
  }
  this->Superclass::DeepCopy(prop);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::GetActors(svtkPropCollection* pc)
{
  this->Actor->GetActors(pc);
  this->LabelTextActor->GetActors(pc);
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->LabelTextActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int svtkAbstractPolygonalHandleRepresentation3D::RenderOpaqueGeometry(svtkViewport* viewport)
{
  int count = 0;
  this->BuildRepresentation();
  if (this->HandleVisibility)
  {
    count += this->Actor->RenderOpaqueGeometry(viewport);
  }
  if (this->LabelVisibility)
  {
    count += this->LabelTextActor->RenderOpaqueGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int svtkAbstractPolygonalHandleRepresentation3D::RenderTranslucentPolygonalGeometry(
  svtkViewport* viewport)
{
  // The internal actor needs to share property keys. This allows depth peeling
  // etc to work.
  this->Actor->SetPropertyKeys(this->GetPropertyKeys());

  int count = 0;
  if (this->HandleVisibility)
  {
    count += this->Actor->RenderTranslucentPolygonalGeometry(viewport);
  }
  if (this->LabelVisibility)
  {
    count += this->LabelTextActor->RenderTranslucentPolygonalGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkAbstractPolygonalHandleRepresentation3D::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  this->BuildRepresentation();
  if (this->HandleVisibility)
  {
    result |= this->Actor->HasTranslucentPolygonalGeometry();
  }
  if (this->LabelVisibility)
  {
    result |= this->LabelTextActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

//-----------------------------------------------------------------------------
double* svtkAbstractPolygonalHandleRepresentation3D::GetBounds()
{
  this->BuildRepresentation();
  return this->Actor ? this->Actor->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
svtkAbstractTransform* svtkAbstractPolygonalHandleRepresentation3D::GetTransform()
{
  return this->HandleTransform;
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::SetLabelText(const char* s)
{
  this->LabelTextInput->SetText(s);
}

//----------------------------------------------------------------------
char* svtkAbstractPolygonalHandleRepresentation3D::GetLabelText()
{
  return this->LabelTextInput->GetText();
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::SetLabelTextScale(double scale[3])
{
  this->LabelTextActor->SetScale(scale);
  this->LabelAnnotationTextScaleInitialized = true;
}

//----------------------------------------------------------------------
double* svtkAbstractPolygonalHandleRepresentation3D::GetLabelTextScale()
{
  return this->LabelTextActor->GetScale();
}

//----------------------------------------------------------------------
void svtkAbstractPolygonalHandleRepresentation3D::PrintSelf(ostream& os, svtkIndent indent)
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
  os << indent << "LabelVisibility: " << this->LabelVisibility << endl;
  os << indent << "HandleVisibility: " << this->HandleVisibility << endl;
  os << indent << "Actor: " << this->Actor << "\n";
  this->Actor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LabelTextActor: " << this->LabelTextActor << endl;
  this->LabelTextActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper: " << this->Mapper << "\n";
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "HandleTransformFilter: " << this->HandleTransformFilter << "\n";
  this->HandleTransformFilter->PrintSelf(os, indent.GetNextIndent());
  os << indent << "HandleTransform: " << this->HandleTransform << "\n";
  this->HandleTransform->PrintSelf(os, indent.GetNextIndent());
  os << indent << "HandleTransformMatrix: " << this->HandleTransformMatrix << "\n";
  this->HandleTransformMatrix->PrintSelf(os, indent.GetNextIndent());
  os << indent << "HandlePicker: " << this->HandlePicker << "\n";
  this->HandlePicker->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LastPickPosition: (" << this->LastPickPosition[0] << ","
     << this->LastPickPosition[1] << ")\n";
  os << indent << "LastEventPosition: (" << this->LastEventPosition[0] << ","
     << this->LastEventPosition[1] << ")\n";
  os << indent << "SmoothMotion: " << this->SmoothMotion << endl;
  // ConstraintAxis;
  // WaitingForMotion;
  // WaitCount;
}
