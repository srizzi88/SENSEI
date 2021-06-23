/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearWipeRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRectilinearWipeRepresentation.h"
#include "svtkActor2D.h"
#include "svtkCellArray.h"
#include "svtkCoordinate.h"
#include "svtkDoubleArray.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageRectilinearWipe.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkRectilinearWipeRepresentation);

svtkCxxSetObjectMacro(svtkRectilinearWipeRepresentation, RectilinearWipe, svtkImageRectilinearWipe);
svtkCxxSetObjectMacro(svtkRectilinearWipeRepresentation, ImageActor, svtkImageActor);

//----------------------------------------------------------------------
svtkRectilinearWipeRepresentation::svtkRectilinearWipeRepresentation()
{
  this->RectilinearWipe = nullptr;
  this->ImageActor = nullptr;

  this->InteractionState = svtkRectilinearWipeRepresentation::Outside;
  this->Tolerance = 5; // pick tolerance in pixels

  this->Property = svtkProperty2D::New();
  this->Property->SetColor(1, 0, 0);

  this->Points = svtkPoints::New();
  this->Points->SetDataTypeToDouble();
  this->Points->SetNumberOfPoints(9);
  this->ActiveParts = -1; // indicates that the widget is uninitialized

  this->Lines = svtkCellArray::New();
  this->Lines->AllocateEstimate(8, 2);

  this->Wipe = svtkPolyData::New();
  this->Wipe->SetPoints(this->Points);
  this->Wipe->SetLines(this->Lines);

  svtkCoordinate* coordinate = svtkCoordinate::New();
  coordinate->SetCoordinateSystemToWorld();

  this->WipeMapper = svtkPolyDataMapper2D::New();
  this->WipeMapper->SetInputData(this->Wipe);
  this->WipeMapper->SetTransformCoordinate(coordinate);
  coordinate->Delete();

  this->WipeActor = svtkActor2D::New();
  this->WipeActor->SetMapper(this->WipeMapper);
  this->WipeActor->SetProperty(this->Property);
}

//----------------------------------------------------------------------
svtkRectilinearWipeRepresentation::~svtkRectilinearWipeRepresentation()
{
  if (this->RectilinearWipe)
  {
    this->RectilinearWipe->Delete();
  }
  if (this->ImageActor)
  {
    this->ImageActor->Delete();
  }

  this->Points->Delete();
  this->Lines->Delete();
  this->Wipe->Delete();
  this->WipeMapper->Delete();
  this->WipeActor->Delete();
  this->Property->Delete();
}

//-------------------------------------------------------------------------
int svtkRectilinearWipeRepresentation::ComputeInteractionState(int X, int Y, int svtkNotUsed(modify))
{
  this->InteractionState = svtkRectilinearWipeRepresentation::Outside;

  // Check if the widget is initialized, ie BuildRepresentation has been
  // invoked at least once
  if (this->ActiveParts != -1)
  {

    // Start by grabbing the five points that define the horizontal and vertical
    // panes, plus the center point
    double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
    double* p4 = pts + 3 * 4;
    double* p5 = pts + 3 * 5;
    double* p6 = pts + 3 * 6;
    double* p7 = pts + 3 * 7;
    double* p8 = pts + 3 * 8;

    svtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, p4[0], p4[1], p4[2], this->DP4);
    svtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, p5[0], p5[1], p5[2], this->DP5);
    svtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, p6[0], p6[1], p6[2], this->DP6);
    svtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, p7[0], p7[1], p7[2], this->DP7);
    svtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, p8[0], p8[1], p8[2], this->DP8);

    // Compare the distance between the current event position and the widget
    double xyz[3], t, closest[3];
    xyz[0] = X;
    xyz[1] = Y;
    xyz[2] = this->DP4[2] = this->DP5[2] = this->DP6[2] = this->DP7[2] = this->DP8[2] = 0.0;

    double tol = this->Tolerance * this->Tolerance;
    if ((this->ActiveParts & 16) && svtkMath::Distance2BetweenPoints(xyz, this->DP8) <= tol)
    {
      this->InteractionState = svtkRectilinearWipeRepresentation::MovingCenter;
    }
    else if ((this->ActiveParts & 1) &&
      svtkLine::DistanceToLine(xyz, this->DP8, this->DP4, t, closest) <= tol)
    {
      this->InteractionState = svtkRectilinearWipeRepresentation::MovingVPane;
    }
    else if ((this->ActiveParts & 2) &&
      svtkLine::DistanceToLine(xyz, this->DP8, this->DP5, t, closest) <= tol)
    {
      this->InteractionState = svtkRectilinearWipeRepresentation::MovingHPane;
    }
    else if ((this->ActiveParts & 4) &&
      svtkLine::DistanceToLine(xyz, this->DP8, this->DP6, t, closest) <= tol)
    {
      this->InteractionState = svtkRectilinearWipeRepresentation::MovingVPane;
    }
    else if ((this->ActiveParts & 8) &&
      svtkLine::DistanceToLine(xyz, this->DP8, this->DP7, t, closest) <= tol)
    {
      this->InteractionState = svtkRectilinearWipeRepresentation::MovingHPane;
    }
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void svtkRectilinearWipeRepresentation::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];

  if (!this->RectilinearWipe)
  {
    return;
  }
  int pos[2];
  this->RectilinearWipe->GetPosition(pos);
  this->StartWipePosition[0] = static_cast<double>(pos[0]);
  this->StartWipePosition[1] = static_cast<double>(pos[1]);
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void svtkRectilinearWipeRepresentation::WidgetInteraction(double newEventPos[2])
{
  int i;
  double v75[3], v46[3];
  for (i = 0; i < 3; i++)
  {
    v75[i] = this->DP5[i] - this->DP7[i];
    v46[i] = this->DP6[i] - this->DP4[i];
  }
  double l75 = svtkMath::Normalize(v75);
  double l46 = svtkMath::Normalize(v46);

  double xPixels = this->Dims[this->I] *
    (v75[0] * (newEventPos[0] - this->StartEventPosition[0]) +
      v75[1] * (newEventPos[1] - this->StartEventPosition[1])) /
    l75;
  double yPixels = this->Dims[this->J] *
    (v46[0] * (newEventPos[0] - this->StartEventPosition[0]) +
      v46[1] * (newEventPos[1] - this->StartEventPosition[1])) /
    l46;

  int newPosition[2];
  newPosition[0] = static_cast<int>(this->StartWipePosition[0]);
  newPosition[1] = static_cast<int>(this->StartWipePosition[1]);

  switch (this->InteractionState)
  {
    case MovingVPane:
      newPosition[0] += static_cast<int>(xPixels + 0.5);
      break;
    case MovingHPane:
      newPosition[1] += static_cast<int>(yPixels + 0.5);
      break;
    case MovingCenter:
      newPosition[0] += static_cast<int>(xPixels + 0.5);
      newPosition[1] += static_cast<int>(yPixels + 0.5);
  }

  newPosition[0] = (newPosition[0] < 0 ? 0 : newPosition[0]);
  newPosition[0] =
    (newPosition[0] >= this->Dims[this->I] ? this->Dims[this->I] - 1 : newPosition[0]);

  newPosition[1] = (newPosition[1] < 0 ? 0 : newPosition[1]);
  newPosition[1] =
    (newPosition[1] >= this->Dims[this->J] ? this->Dims[this->J] - 1 : newPosition[1]);

  this->RectilinearWipe->SetPosition(newPosition);

  // Rebuild the widget based on the change
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
void svtkRectilinearWipeRepresentation::BuildRepresentation()
{
  if (!this->RectilinearWipe || !this->ImageActor)
  {
    svtkWarningMacro(<< "Must define both image rectilinear wipe and image actor");
    return;
  }

  // get the necessary image information
  double bounds[6], o[3], sp[3];
  int pos[2];
  svtkImageData* image = this->ImageActor->GetInput();
  image->GetBounds(bounds);
  image->GetDimensions(this->Dims);
  image->GetOrigin(o);
  image->GetSpacing(sp);
  this->RectilinearWipe->GetPosition(pos);

  // define the geometry
  double s, t;
  double t0 = bounds[1] - bounds[0];
  double t1 = bounds[3] - bounds[2];
  double t2 = bounds[5] - bounds[4];
  int orthoAxis = (t0 < t1 ? (t0 < t2 ? 0 : 2) : (t1 < t2 ? 1 : 2));
  double p0[3], p1[3], p2[3], p3[3];

  if (orthoAxis == 0) // x-axis
  {
    this->I = 1;
    this->J = 2;
    s = static_cast<double>(pos[0] + 0.5) / (this->Dims[1] - 1);
    t = static_cast<double>(pos[1] + 0.5) / (this->Dims[2] - 1);
    p0[0] = bounds[0];
    p0[1] = bounds[2];
    p0[2] = bounds[4];
    p1[0] = bounds[0];
    p1[1] = bounds[3];
    p1[2] = bounds[4];
    p2[0] = bounds[0];
    p2[1] = bounds[3];
    p2[2] = bounds[5];
    p3[0] = bounds[0];
    p3[1] = bounds[2];
    p3[2] = bounds[5];
    this->Points->SetPoint(8, bounds[0], p0[1] + s * (p1[1] - p0[1]), p1[2] + t * (p2[2] - p1[2]));
  }
  else if (orthoAxis == 1) // y-axis
  {
    this->I = 0;
    this->J = 2;
    s = static_cast<double>(pos[0] + 0.5) / (this->Dims[0] - 1);
    t = static_cast<double>(pos[1] + 0.5) / (this->Dims[2] - 1);
    p0[0] = bounds[0];
    p0[1] = bounds[2];
    p0[2] = bounds[4];
    p1[0] = bounds[1];
    p1[1] = bounds[2];
    p1[2] = bounds[4];
    p2[0] = bounds[1];
    p2[1] = bounds[2];
    p2[2] = bounds[5];
    p3[0] = bounds[0];
    p3[1] = bounds[2];
    p3[2] = bounds[5];
    this->Points->SetPoint(8, p0[0] + s * (p1[0] - p0[0]), bounds[2], p1[2] + t * (p2[2] - p1[2]));
  }
  else // if( orthoAxis == 2 ) //z-axis
  {
    this->I = 0;
    this->J = 1;
    s = static_cast<double>(pos[0] + 0.5) / (this->Dims[0] - 1);
    t = static_cast<double>(pos[1] + 0.5) / (this->Dims[1] - 1);
    p0[0] = bounds[0];
    p0[1] = bounds[2];
    p0[2] = bounds[4];
    p1[0] = bounds[1];
    p1[1] = bounds[2];
    p1[2] = bounds[4];
    p2[0] = bounds[1];
    p2[1] = bounds[3];
    p2[2] = bounds[4];
    p3[0] = bounds[0];
    p3[1] = bounds[3];
    p3[2] = bounds[4];
    this->Points->SetPoint(8, p0[0] + s * (p1[0] - p0[0]), p1[1] + t * (p2[1] - p1[1]), bounds[4]);
  }

  // corners
  this->Points->SetPoint(0, p0);
  this->Points->SetPoint(1, p1);
  this->Points->SetPoint(2, p2);
  this->Points->SetPoint(3, p3);

  // mid-edge
  this->Points->SetPoint(
    4, p0[0] + s * (p1[0] - p0[0]), p0[1] + s * (p1[1] - p0[1]), p0[2] + s * (p1[2] - p0[2]));
  this->Points->SetPoint(
    5, p1[0] + t * (p2[0] - p1[0]), p1[1] + t * (p2[1] - p1[1]), p1[2] + t * (p2[2] - p1[2]));
  this->Points->SetPoint(
    6, p3[0] + s * (p2[0] - p3[0]), p3[1] + s * (p2[1] - p3[1]), p3[2] + s * (p2[2] - p3[2]));
  this->Points->SetPoint(
    7, p0[0] + t * (p3[0] - p0[0]), p0[1] + t * (p3[1] - p0[1]), p0[2] + t * (p3[2] - p0[2]));
  this->Points->Modified();

  // Define the lines
  this->Lines->Reset();
  this->Lines->InsertNextCell(5);
  this->Lines->InsertCellPoint(0);
  this->Lines->InsertCellPoint(1);
  this->Lines->InsertCellPoint(2);
  this->Lines->InsertCellPoint(3);
  this->Lines->InsertCellPoint(0);

  int wipe = this->RectilinearWipe->GetWipe();
  this->ActiveParts = 0;
  if (wipe == SVTK_WIPE_QUAD)
  {
    this->ActiveParts |= 1;  // activate center to bottom edge
    this->ActiveParts |= 2;  // activate center to right edge
    this->ActiveParts |= 4;  // activate center to top edge
    this->ActiveParts |= 8;  // activate center to left edge
    this->ActiveParts |= 16; // activate center point
    this->Lines->InsertNextCell(2);
    this->Lines->InsertCellPoint(4);
    this->Lines->InsertCellPoint(6);
    this->Lines->InsertNextCell(2);
    this->Lines->InsertCellPoint(5);
    this->Lines->InsertCellPoint(7);
  }
  else if (wipe == SVTK_WIPE_VERTICAL)
  {
    this->ActiveParts |= 2;
    this->ActiveParts |= 8;
    this->Lines->InsertNextCell(2);
    this->Lines->InsertCellPoint(5);
    this->Lines->InsertCellPoint(7);
  }
  else if (wipe == SVTK_WIPE_HORIZONTAL)
  {
    this->ActiveParts |= 1;
    this->ActiveParts |= 4;
    this->Lines->InsertNextCell(2);
    this->Lines->InsertCellPoint(4);
    this->Lines->InsertCellPoint(6);
  }
  else if (wipe == SVTK_WIPE_LOWER_LEFT)
  {
    this->ActiveParts |= 1;
    this->ActiveParts |= 8;
    this->ActiveParts |= 16;
    this->Lines->InsertNextCell(3);
    this->Lines->InsertCellPoint(4);
    this->Lines->InsertCellPoint(8);
    this->Lines->InsertCellPoint(7);
  }
  else if (wipe == SVTK_WIPE_LOWER_RIGHT)
  {
    this->ActiveParts |= 1;
    this->ActiveParts |= 2;
    this->ActiveParts |= 16;
    this->Lines->InsertNextCell(3);
    this->Lines->InsertCellPoint(4);
    this->Lines->InsertCellPoint(8);
    this->Lines->InsertCellPoint(5);
  }
  else if (wipe == SVTK_WIPE_UPPER_LEFT)
  {
    this->ActiveParts |= 4;
    this->ActiveParts |= 8;
    this->ActiveParts |= 16;
    this->Lines->InsertNextCell(3);
    this->Lines->InsertCellPoint(7);
    this->Lines->InsertCellPoint(8);
    this->Lines->InsertCellPoint(6);
  }
  else // if ( wipe == SVTK_WIPE_UPPER_RIGHT )
  {
    this->ActiveParts |= 2;
    this->ActiveParts |= 4;
    this->ActiveParts |= 16;
    this->Lines->InsertNextCell(3);
    this->Lines->InsertCellPoint(6);
    this->Lines->InsertCellPoint(8);
    this->Lines->InsertCellPoint(5);
  }
}

//----------------------------------------------------------------------
void svtkRectilinearWipeRepresentation::GetActors2D(svtkPropCollection* pc)
{
  this->WipeActor->GetActors2D(pc);
}

//----------------------------------------------------------------------
void svtkRectilinearWipeRepresentation::ReleaseGraphicsResources(svtkWindow* win)
{
  this->WipeActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int svtkRectilinearWipeRepresentation::RenderOverlay(svtkViewport* viewport)
{
  return this->WipeActor->RenderOverlay(viewport);
}

//-----------------------------------------------------------------------------
int svtkRectilinearWipeRepresentation::RenderOpaqueGeometry(svtkViewport* viewport)
{
  return this->WipeActor->RenderOpaqueGeometry(viewport);
}

//-----------------------------------------------------------------------------
int svtkRectilinearWipeRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  return this->WipeActor->RenderTranslucentPolygonalGeometry(viewport);
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkRectilinearWipeRepresentation::HasTranslucentPolygonalGeometry()
{
  return this->WipeActor->HasTranslucentPolygonalGeometry();
}

//-----------------------------------------------------------------------------
void svtkRectilinearWipeRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->ImageActor)
  {
    os << indent << "Image Actor: " << this->ImageActor << "\n";
  }
  else
  {
    os << indent << "Image Actor: (none)\n";
  }

  if (this->RectilinearWipe)
  {
    os << indent << "RectilinearWipe: " << this->RectilinearWipe << "\n";
  }
  else
  {
    os << indent << "Image RectilinearWipe: (none)\n";
  }

  if (this->Property)
  {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
