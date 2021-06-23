/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointHandleRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointHandleRepresentation2D.h"
#include "svtkActor2D.h"
#include "svtkAssemblyPath.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkCursor2D.h"
#include "svtkGlyph2D.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointPlacer.h"
#include "svtkPoints.h"
#include "svtkPolyDataAlgorithm.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"
#include "svtkWindow.h"

svtkStandardNewMacro(svtkPointHandleRepresentation2D);

svtkCxxSetObjectMacro(svtkPointHandleRepresentation2D, Property, svtkProperty2D);
svtkCxxSetObjectMacro(svtkPointHandleRepresentation2D, SelectedProperty, svtkProperty2D);
svtkCxxSetObjectMacro(svtkPointHandleRepresentation2D, PointPlacer, svtkPointPlacer);

//----------------------------------------------------------------------
svtkPointHandleRepresentation2D::svtkPointHandleRepresentation2D()
{
  // Initialize state
  this->InteractionState = svtkHandleRepresentation::Outside;

  // Represent the position of the cursor
  this->FocalPoint = svtkPoints::New();
  this->FocalPoint->SetNumberOfPoints(1);
  this->FocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  this->FocalData = svtkPolyData::New();
  this->FocalData->SetPoints(this->FocalPoint);

  // The transformation of the cursor will be done via svtkGlyph2D
  // By default a svtkGlyphSOurce2D will be used to define the cursor shape
  svtkCursor2D* cursor2D = svtkCursor2D::New();
  cursor2D->AllOff();
  cursor2D->AxesOn();
  cursor2D->PointOn();
  cursor2D->Update();
  this->CursorShape = cursor2D->GetOutput();
  this->CursorShape->Register(this);
  cursor2D->Delete();

  this->Glypher = svtkGlyph2D::New();
  this->Glypher->SetInputData(this->FocalData);
  this->Glypher->SetSourceData(this->CursorShape);
  this->Glypher->SetVectorModeToVectorRotationOff();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  this->MapperCoordinate = svtkCoordinate::New();
  this->MapperCoordinate->SetCoordinateSystemToDisplay();

  this->Mapper = svtkPolyDataMapper2D::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());
  this->Mapper->SetTransformCoordinate(this->MapperCoordinate);

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = svtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  // The size of the hot spot
  this->WaitingForMotion = 0;
}

//----------------------------------------------------------------------
svtkPointHandleRepresentation2D::~svtkPointHandleRepresentation2D()
{
  this->FocalPoint->Delete();
  this->FocalData->Delete();

  this->CursorShape->Delete();
  this->Glypher->Delete();
  this->MapperCoordinate->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();

  this->Property->Delete();
  this->SelectedProperty->Delete();
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::SetCursorShape(svtkPolyData* shape)
{
  if (shape != this->CursorShape)
  {
    if (this->CursorShape)
    {
      this->CursorShape->Delete();
    }
    this->CursorShape = shape;
    if (this->CursorShape)
    {
      this->CursorShape->Register(this);
    }
    this->Glypher->SetSourceData(this->CursorShape);
    this->Modified();
  }
}

//----------------------------------------------------------------------
svtkPolyData* svtkPointHandleRepresentation2D::GetCursorShape()
{
  return this->CursorShape;
}

//-------------------------------------------------------------------------
double* svtkPointHandleRepresentation2D::GetBounds()
{
  return nullptr;
}

//-------------------------------------------------------------------------
void svtkPointHandleRepresentation2D::SetDisplayPosition(double p[3])
{
  this->Superclass::SetDisplayPosition(p);
  this->FocalPoint->SetPoint(0, p);
  this->FocalPoint->Modified();

  if (this->PointPlacer)
  {
    // The point placer will compute the world position for us.
    return;
  }

  double w[4];
  if (this->Renderer)
  {
    svtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, p[0], p[1], p[2], w);
    this->SetWorldPosition(w);
  }
}

//-------------------------------------------------------------------------
int svtkPointHandleRepresentation2D::ComputeInteractionState(int X, int Y, int svtkNotUsed(modify))
{
  double pos[3], xyz[3];
  this->FocalPoint->GetPoint(0, pos);
  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  xyz[2] = pos[2];

  this->VisibilityOn();
  double tol2 = this->Tolerance * this->Tolerance;
  if (svtkMath::Distance2BetweenPoints(xyz, pos) <= tol2)
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

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void svtkPointHandleRepresentation2D::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  this->WaitCount = 0;
  if (this->IsTranslationConstrained())
  {
    this->WaitingForMotion = 1;
  }
  else
  {
    this->WaitingForMotion = 0;
  }
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void svtkPointHandleRepresentation2D::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if (this->InteractionState == svtkHandleRepresentation::Selecting ||
    this->InteractionState == svtkHandleRepresentation::Translating)
  {
    if (!this->WaitingForMotion || this->WaitCount++ > 1)
    {
      this->Translate(eventPos);
    }
  }

  else if (this->InteractionState == svtkHandleRepresentation::Scaling)
  {
    this->Scale(eventPos);
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];

  this->Modified();
}

//----------------------------------------------------------------------
// Translate everything
void svtkPointHandleRepresentation2D::Translate(const double* eventPos)
{
  double pos[3];
  this->FocalPoint->GetPoint(0, pos);
  if (this->IsTranslationConstrained())
  {
    pos[this->TranslationAxis] += eventPos[this->TranslationAxis] - pos[this->TranslationAxis];
  }
  else
  {
    pos[0] += eventPos[0] - pos[0];
    pos[1] += eventPos[1] - pos[1];
  }
  this->SetDisplayPosition(pos);
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::Scale(const double eventPos[2])
{
  // Get the current scale factor
  double sf = this->Glypher->GetScaleFactor();

  // Compute the scale factor
  const int* size = this->Renderer->GetSize();
  double dPos = static_cast<double>(eventPos[1] - this->LastEventPosition[1]);
  sf *= (1.0 + 2.0 * (dPos / size[1])); // scale factor of 2.0 is arbitrary

  // Scale the handle
  this->Glypher->SetScaleFactor(sf);
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::Highlight(int highlight)
{
  if (highlight)
  {
    this->Actor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->Actor->SetProperty(this->Property);
  }
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::CreateDefaultProperties()
{
  this->Property = svtkProperty2D::New();
  this->Property->SetColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(1.0);

  this->SelectedProperty = svtkProperty2D::New();
  this->SelectedProperty->SetColor(0.0, 1.0, 0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetActiveCamera() &&
      this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime) ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {
    double p[3];
    this->GetDisplayPosition(p);
    this->FocalPoint->SetPoint(0, p);
    this->FocalPoint->Modified();
    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::ShallowCopy(svtkProp* prop)
{
  svtkPointHandleRepresentation2D* rep = svtkPointHandleRepresentation2D::SafeDownCast(prop);
  if (rep)
  {
    this->SetCursorShape(rep->GetCursorShape());
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::DeepCopy(svtkProp* prop)
{
  svtkPointHandleRepresentation2D* rep = svtkPointHandleRepresentation2D::SafeDownCast(prop);
  if (rep)
  {
    this->SetCursorShape(rep->GetCursorShape());
    this->Property->DeepCopy(rep->GetProperty());
    this->SelectedProperty->DeepCopy(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);
  }
  this->Superclass::DeepCopy(prop);
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::GetActors2D(svtkPropCollection* pc)
{
  this->Actor->GetActors2D(pc);
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int svtkPointHandleRepresentation2D::RenderOverlay(svtkViewport* viewport)
{
  this->BuildRepresentation();
  return this->Actor->RenderOverlay(viewport);
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::SetVisibility(svtkTypeBool visible)
{
  this->Actor->SetVisibility(visible);
  // Forward to superclass
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------
void svtkPointHandleRepresentation2D::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
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

  if (this->CursorShape)
  {
    os << indent << "Cursor Shape: " << this->CursorShape << "\n";
  }
  else
  {
    os << indent << "Cursor Shape: (none)\n";
  }
}
