/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompassRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkCompassRepresentation.h"

#include "svtkCellArray.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkUnsignedCharArray.h"
#include "svtkWindow.h"

#include <sstream>

svtkStandardNewMacro(svtkCompassRepresentation);

//----------------------------------------------------------------------
svtkCompassRepresentation::svtkCompassRepresentation()
{
  // The coordinates defining the compass
  this->Point1Coordinate = svtkCoordinate::New();
  this->Point1Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Point1Coordinate->SetValue(0.80, 0.80, 0.0);

  this->Point2Coordinate = svtkCoordinate::New();
  this->Point2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Point2Coordinate->SetValue(0.99, 0.99, 0.0);

  // Default configuration
  this->OuterRadius = 0.9;
  this->InnerRadius = 0.75;

  this->TiltRepresentation = svtkSmartPointer<svtkCenteredSliderRepresentation>::New();
  this->TiltRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToViewport();
  this->TiltRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToViewport();
  this->TiltRepresentation->SetMinimumValue(-15);
  this->TiltRepresentation->SetMaximumValue(15);
  this->TiltRepresentation->SetValue(0);
  this->TiltRepresentation->SetTitleText("tilt");

  this->DistanceRepresentation = svtkSmartPointer<svtkCenteredSliderRepresentation>::New();
  this->DistanceRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToViewport();
  this->DistanceRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToViewport();
  this->DistanceRepresentation->SetMinimumValue(0.8);
  this->DistanceRepresentation->SetMaximumValue(1.2);
  this->DistanceRepresentation->SetValue(1.0);
  this->DistanceRepresentation->SetTitleText("dist");

  // The points and the transformation for the points. There are a total of
  // 73 points two rings of 340 degrees in increments of 10 plus three extra
  // points
  this->XForm = svtkTransform::New();
  this->Points = svtkPoints::New();
  this->Points->SetNumberOfPoints(73);

  this->BuildRing();

  this->RingXForm = svtkTransformPolyDataFilter::New();
  this->RingXForm->SetInputData(this->Ring);
  this->RingXForm->SetTransform(this->XForm);

  this->RingMapper = svtkPolyDataMapper2D::New();
  this->RingMapper->SetInputConnection(this->RingXForm->GetOutputPort());

  this->RingProperty = svtkProperty2D::New();
  this->RingProperty->SetOpacity(0.5);

  this->RingActor = svtkActor2D::New();
  this->RingActor->SetMapper(this->RingMapper);
  this->RingActor->SetProperty(this->RingProperty);

  this->SelectedProperty = svtkProperty2D::New();
  this->SelectedProperty->SetOpacity(0.8);

  this->LabelProperty = svtkTextProperty::New();
  this->LabelProperty->SetFontFamilyToTimes();
  this->LabelProperty->SetJustificationToCentered();
  // this->LabelProperty->ShadowOn();
  this->LabelActor = svtkTextActor::New();
  this->LabelActor->SetTextProperty(this->LabelProperty);
  this->LabelActor->SetInput("N");
  this->LabelActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();

  this->StatusProperty = svtkTextProperty::New();
  this->StatusProperty->SetFontFamilyToArial();
  // this->StatusProperty->ShadowOn();
  this->StatusProperty->SetJustificationToCentered();
  this->StatusProperty->SetJustificationToRight();
  this->StatusProperty->SetVerticalJustificationToTop();
  this->StatusActor = svtkTextActor::New();
  this->StatusActor->SetTextProperty(this->StatusProperty);
  this->StatusActor->SetInput("0 Degrees");
  this->StatusActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();

  this->BuildBackdrop();

  this->Heading = 0;
  this->Tilt = 0;
  this->Distance = 100000;

  this->HighlightState = 0;
}

void svtkCompassRepresentation::BuildBackdrop()
{
  svtkPolyData* backdropPolyData = svtkPolyData::New();
  svtkPoints* pts = svtkPoints::New();
  pts->SetNumberOfPoints(4);
  pts->SetPoint(0, 0, 0, 0);
  pts->SetPoint(1, 1, 0, 0);
  pts->SetPoint(2, 1, 1, 0);
  pts->SetPoint(3, 0, 1, 0);
  backdropPolyData->SetPoints(pts);
  pts->Delete();

  svtkCellArray* backdrop = svtkCellArray::New();
  backdrop->InsertNextCell(4);
  backdrop->InsertCellPoint(0);
  backdrop->InsertCellPoint(1);
  backdrop->InsertCellPoint(2);
  backdrop->InsertCellPoint(3);
  backdropPolyData->SetPolys(backdrop);
  backdrop->Delete();

  svtkSmartPointer<svtkUnsignedCharArray> colors = svtkSmartPointer<svtkUnsignedCharArray>::New();
  colors->SetNumberOfComponents(4);
  colors->SetNumberOfTuples(4);
  unsigned char color[4];
  color[0] = 0;
  color[1] = 0;
  color[2] = 0;
  color[3] = 0;
  colors->SetTypedTuple(0, color);
  colors->SetTypedTuple(3, color);
  color[3] = 80;
  colors->SetTypedTuple(1, color);
  colors->SetTypedTuple(2, color);
  backdropPolyData->GetPointData()->SetScalars(colors);

  this->BackdropMapper = svtkPolyDataMapper2D::New();
  this->BackdropMapper->SetInputData(backdropPolyData);
  this->BackdropMapper->ScalarVisibilityOn();
  backdropPolyData->Delete();

  this->Backdrop = svtkActor2D::New();
  this->Backdrop->SetMapper(this->BackdropMapper);
  this->Backdrop->GetProperty()->SetColor(0, 0, 0);
  // this->Backdrop->GetProperty()->SetOpacity(0.2);
}

void svtkCompassRepresentation::BuildRing()
{
  // crate the polydata
  this->Ring = svtkPolyData::New();
  this->Ring->SetPoints(this->Points);

  // build the cells
  svtkCellArray* ringCells = svtkCellArray::New();
  for (int i = 0; i < 4; ++i)
  {
    ringCells->InsertNextCell(17);
    for (int j = 0; j < 8; ++j)
    {
      ringCells->InsertCellPoint(i * 9 + j);
    }
    for (int j = 0; j < 8; ++j)
    {
      ringCells->InsertCellPoint(i * 9 + 35 + 7 - j);
    }
    ringCells->InsertCellPoint(i * 9);
  }
  this->Ring->SetLines(ringCells);
  ringCells->Delete();

  // add some polys
  svtkCellArray* markCells = svtkCellArray::New();
  for (int i = 1; i < 4; ++i)
  {
    markCells->InsertNextCell(3);
    markCells->InsertCellPoint(i + 69);
    markCells->InsertCellPoint(i * 9 + 35);
    markCells->InsertCellPoint(i * 9 + 33);
  }
  this->Ring->SetPolys(markCells);
  markCells->Delete();

  // build the points
  for (int i = 0; i < 35; ++i)
  {
    this->Points->SetPoint(i, this->OuterRadius * cos(svtkMath::RadiansFromDegrees(10. * (i + 10))),
      this->OuterRadius * sin(svtkMath::RadiansFromDegrees(10. * (i + 10))), 0.0);
    this->Points->SetPoint(i + 35,
      this->InnerRadius * cos(svtkMath::RadiansFromDegrees(10. * (i + 10))),
      this->InnerRadius * sin(svtkMath::RadiansFromDegrees(10. * (i + 10))), 0.0);
  }
  // add the WSE points
  this->Points->SetPoint(70, -this->OuterRadius - 0.1, 0.0, 0.0);
  this->Points->SetPoint(71, 0.0, -this->OuterRadius - 0.1, 0.0);
  this->Points->SetPoint(72, this->OuterRadius + 0.1, 0.0, 0.0);
}

//----------------------------------------------------------------------
svtkCompassRepresentation::~svtkCompassRepresentation()
{
  this->Backdrop->Delete();
  this->BackdropMapper->Delete();

  this->Point1Coordinate->Delete();
  this->Point2Coordinate->Delete();

  this->XForm->Delete();
  this->Points->Delete();

  this->Ring->Delete();
  this->RingXForm->Delete();
  this->RingMapper->Delete();
  this->RingActor->Delete();
  this->RingProperty->Delete();

  this->SelectedProperty->Delete();

  this->LabelProperty->Delete();
  this->LabelActor->Delete();
  this->StatusProperty->Delete();
  this->StatusActor->Delete();
}

//----------------------------------------------------------------------
svtkCoordinate* svtkCompassRepresentation::GetPoint1Coordinate()
{
  return this->Point1Coordinate;
}

//----------------------------------------------------------------------
void svtkCompassRepresentation ::StartWidgetInteraction(double eventPos[2])
{
  this->ComputeInteractionState(static_cast<int>(eventPos[0]), static_cast<int>(eventPos[1]));
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::TiltWidgetInteraction(double eventPos[2])
{
  this->TiltRepresentation->WidgetInteraction(eventPos);
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::DistanceWidgetInteraction(double eventPos[2])
{
  this->DistanceRepresentation->WidgetInteraction(eventPos);
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::WidgetInteraction(double eventPos[2])
{
  // adjust the value
  int center[2];
  double rsize;
  this->GetCenterAndUnitRadius(center, rsize);

  svtkRenderWindowInteractor* rwi = this->Renderer->GetRenderWindow()->GetInteractor();

  // how far did we move?
  double mousePt[3];
  mousePt[2] = 0;
  mousePt[0] = rwi->GetLastEventPosition()[0] - center[0];
  mousePt[1] = rwi->GetLastEventPosition()[1] - center[1];
  svtkMath::Normalize(mousePt);
  double angle1 = atan2(mousePt[1], mousePt[0]);
  mousePt[0] = eventPos[0] - center[0];
  mousePt[1] = eventPos[1] - center[1];
  svtkMath::Normalize(mousePt);
  double angle2 = atan2(mousePt[1], mousePt[0]);
  angle2 = angle2 - angle1;

  this->Heading = this->Heading + angle2 * 0.5 / svtkMath::Pi();

  this->BuildRepresentation();
}

//----------------------------------------------------------------------
svtkCoordinate* svtkCompassRepresentation::GetPoint2Coordinate()
{
  return this->Point2Coordinate;
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::PlaceWidget(double* svtkNotUsed(bds[6]))
{
  // Position the handles at the end of the lines
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::Highlight(int highlight)
{
  if (highlight == this->HighlightState)
  {
    return;
  }
  this->HighlightState = highlight;
  if (highlight)
  {
    this->RingActor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->RingActor->SetProperty(this->RingProperty);
  }
  this->TiltRepresentation->Highlight(highlight);
  this->DistanceRepresentation->Highlight(highlight);
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::BuildRepresentation()
{
  if (this->GetMTime() <= this->BuildTime &&
    (!this->Renderer || !this->Renderer->GetSVTKWindow() ||
      this->Renderer->GetSVTKWindow()->GetMTime() <= this->BuildTime))
  {
    return;
  }

  const int* size = this->Renderer->GetSize();
  if (0 == size[0] || 0 == size[1])
  {
    // Renderer has no size yet: wait until the next
    // BuildRepresentation...
    return;
  }

  this->XForm->Identity();

  int center[2];
  double rsize;
  this->GetCenterAndUnitRadius(center, rsize);

  while (this->Heading < 0)
  {
    this->Heading += 1;
  }
  while (this->Heading > 1)
  {
    this->Heading -= 1;
  }

  double angle = this->Heading * 2.0 * svtkMath::Pi();

  this->XForm->Translate(center[0], center[1], 0.0);
  this->XForm->Scale(rsize, rsize, 1.0);
  this->XForm->RotateZ(svtkMath::DegreesFromRadians(angle));

  this->LabelActor->SetPosition(
    center[0] + rsize * cos(angle + svtkMath::Pi() / 2.0) * this->InnerRadius,
    center[1] + rsize * sin(angle + svtkMath::Pi() / 2.0) * this->InnerRadius);

  double fsize = 1.4 * rsize * this->InnerRadius * sin(svtkMath::RadiansFromDegrees(18.));

  this->LabelActor->SetOrientation(svtkMath::DegreesFromRadians(angle));
  this->LabelProperty->SetFontSize(static_cast<int>(fsize));

  if (rsize > 40)
  {
    std::ostringstream out;
    out.setf(ios::fixed);
    out.precision(0);

    out << "Distance: ";
    if (this->Distance > 10000)
    {
      out << this->Distance / 1000 << "km";
    }
    else
    {
      out << this->Distance << "m";
    }

    out << "\nTilt: " << this->Tilt;

    out << "\nHeading: " << svtkMath::DegreesFromRadians(angle);

    this->LabelProperty->SetFontSize(static_cast<int>(fsize * 0.8));
    this->StatusProperty->SetFontSize(static_cast<int>(fsize * 0.9));
    this->StatusActor->SetInput(out.str().c_str());

    this->StatusActor->SetPosition(center[0] - rsize * 2.0, center[1] + rsize);
  }
  else
  {
    this->StatusActor->SetInput("");
  }

  // adjust the slider as well
  this->TiltRepresentation->GetPoint1Coordinate()->SetValue(
    center[0] - rsize * 1.5, center[1] - rsize, 0.0);
  this->TiltRepresentation->GetPoint2Coordinate()->SetValue(
    center[0] - rsize * 1.2, center[1] + rsize, 0.0);
  this->TiltRepresentation->Modified();
  this->TiltRepresentation->BuildRepresentation();

  // adjust the slider as well
  this->DistanceRepresentation->GetPoint1Coordinate()->SetValue(
    center[0] - rsize * 1.9, center[1] - rsize, 0.0);
  this->DistanceRepresentation->GetPoint2Coordinate()->SetValue(
    center[0] - rsize * 1.6, center[1] + rsize, 0.0);
  this->DistanceRepresentation->Modified();
  this->DistanceRepresentation->BuildRepresentation();

  const int* renSize = this->Renderer->GetSize();
  svtkUnsignedCharArray* colors = svtkArrayDownCast<svtkUnsignedCharArray>(
    this->BackdropMapper->GetInput()->GetPointData()->GetScalars());
  unsigned char color[4];
  color[0] = 0;
  color[1] = 0;
  color[2] = 0;

  svtkPoints* pts = this->BackdropMapper->GetInput()->GetPoints();
  pts->SetPoint(1, renSize[0], center[1] - rsize * 1.1, 0);
  pts->SetPoint(2, renSize[0], renSize[1], 0);
  if (this->HighlightState)
  {
    pts->SetPoint(0, center[0] - rsize * 5.0, center[1] - rsize * 1.1, 0);
    pts->SetPoint(3, center[0] - rsize * 5.0, renSize[1], 0);
    color[3] = 80;
    colors->SetTypedTuple(1, color);
  }
  else
  {
    pts->SetPoint(0, center[0] - rsize * 3.0, center[1] - rsize * 1.1, 0);
    pts->SetPoint(3, center[0] - rsize * 3.0, renSize[1], 0);
    color[3] = 0;
    colors->SetTypedTuple(1, color);
  }
  pts->Modified();
  colors->Modified();

  this->BackdropMapper->GetInput()->Modified();
  this->BackdropMapper->Modified();
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::GetActors(svtkPropCollection* pc)
{
  pc->AddItem(this->Backdrop);
  pc->AddItem(this->RingActor);
  pc->AddItem(this->LabelActor);
  pc->AddItem(this->StatusActor);
  this->TiltRepresentation->GetActors(pc);
  this->DistanceRepresentation->GetActors(pc);
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  this->Backdrop->ReleaseGraphicsResources(w);
  this->RingActor->ReleaseGraphicsResources(w);
  this->LabelActor->ReleaseGraphicsResources(w);
  this->StatusActor->ReleaseGraphicsResources(w);
  this->TiltRepresentation->ReleaseGraphicsResources(w);
  this->DistanceRepresentation->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int svtkCompassRepresentation ::RenderOpaqueGeometry(svtkViewport* viewport)
{
  this->BuildRepresentation();
  int count = 0;
  count += this->Backdrop->RenderOpaqueGeometry(viewport);
  if (this->HighlightState)
  {
    if (strlen(this->StatusActor->GetInput()))
    {
      count += this->StatusActor->RenderOpaqueGeometry(viewport);
    }
  }
  count += this->RingActor->RenderOpaqueGeometry(viewport);
  count += this->LabelActor->RenderOpaqueGeometry(viewport);
  count += this->TiltRepresentation->RenderOpaqueGeometry(viewport);
  count += this->DistanceRepresentation->RenderOpaqueGeometry(viewport);
  return count;
}

//----------------------------------------------------------------------
int svtkCompassRepresentation::RenderOverlay(svtkViewport* viewport)
{
  this->BuildRepresentation();
  int count = 0;
  count += this->Backdrop->RenderOverlay(viewport);
  if (this->HighlightState)
  {
    if (strlen(this->StatusActor->GetInput()))
    {
      count += this->StatusActor->RenderOverlay(viewport);
    }
  }
  count += this->RingActor->RenderOverlay(viewport);
  count += this->LabelActor->RenderOverlay(viewport);
  count += this->TiltRepresentation->RenderOverlay(viewport);
  count += this->DistanceRepresentation->RenderOverlay(viewport);
  return count;
}

double svtkCompassRepresentation::GetHeading()
{
  return this->Heading;
}

void svtkCompassRepresentation::SetHeading(double v)
{
  this->Heading = v;
  if (this->Renderer)
  {
    this->BuildRepresentation();
  }
}

double svtkCompassRepresentation::GetTilt()
{
  return this->Tilt;
}

void svtkCompassRepresentation::SetTilt(double v)
{
  this->Tilt = v;
  if (this->Tilt > 90)
  {
    this->Tilt = 90;
  }
  if (this->Tilt < 0)
  {
    this->Tilt = 0;
  }
}

void svtkCompassRepresentation::EndTilt()
{
  this->TiltRepresentation->SetValue(0);
}

void svtkCompassRepresentation::UpdateTilt(double time)
{
  double val = this->TiltRepresentation->GetValue();
  this->SetTilt(this->Tilt + val * time);
}

double svtkCompassRepresentation::GetDistance()
{
  return this->Distance;
}

void svtkCompassRepresentation::SetDistance(double v)
{
  this->Distance = v;
  if (this->Distance < 5)
  {
    this->Distance = 5;
  }
}

void svtkCompassRepresentation::EndDistance()
{
  this->DistanceRepresentation->SetValue(1.0);
}

void svtkCompassRepresentation::UpdateDistance(double time)
{
  double val = this->DistanceRepresentation->GetValue();
  this->SetDistance(this->Distance * (1 + (1.0 / val - 1) * time));
}

void svtkCompassRepresentation::GetCenterAndUnitRadius(int center[2], double& radius)
{
  // we always fit in the bounding box, but we try to be smart :)
  // We stick to the upper right and as the size increases we
  // scale in a non-linear manner
  int* p1 = this->Point1Coordinate->GetComputedViewportValue(this->Renderer);
  int* p2 = this->Point2Coordinate->GetComputedViewportValue(this->Renderer);

  radius = abs(p1[0] - p2[0]);
  if (abs(p1[1] - p2[1]) < radius)
  {
    radius = abs(p1[1] - p2[1]);
  }
  radius /= 2;

  // scale the rsize between 100% and 50%
  double scale = 1.0 - (radius - 40) / (radius + 100.0) * 0.5;
  if (scale > 1.0)
  {
    scale = 1.0;
  }
  radius *= scale;

  // stick to the upper right
  center[0] = static_cast<int>(p2[0] - radius);
  center[1] = static_cast<int>(p2[1] - radius);

  if (this->HighlightState == 0)
  {
    // create a reduced size when not highlighted by applying the scale
    // again, only do it when there is a significant difference
    if (scale < 0.9)
    {
      radius = radius * scale * scale;
    }
  }
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "Label Text: " << (this->LabelActor->GetInput() ? this->LabelActor->GetInput() : "(none)")
     << "\n";

  os << indent << "Point1 Coordinate: " << this->Point1Coordinate << "\n";
  this->Point1Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point2 Coordinate: " << this->Point2Coordinate << "\n";
  this->Point2Coordinate->PrintSelf(os, indent.GetNextIndent());

  if (this->SelectedProperty)
  {
    os << indent << "SelectedProperty:\n";
    this->SelectedProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "SelectedProperty: (none)\n";
  }

  if (this->RingProperty)
  {
    os << indent << "RingProperty:\n";
    this->RingProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "RingProperty: (none)\n";
  }

  if (this->SelectedProperty)
  {
    os << indent << "SelectedProperty:\n";
    this->SelectedProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "SelectedProperty: (none)\n";
  }

  if (this->LabelProperty)
  {
    os << indent << "LabelProperty:\n";
    this->LabelProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "LabelProperty: (none)\n";
  }
}

//----------------------------------------------------------------------
int svtkCompassRepresentation::ComputeInteractionState(int x, int y, int modify)
{
  const int* size = this->Renderer->GetSize();
  if (0 == size[0] || 0 == size[1])
  {
    // Renderer has no size yet
    this->InteractionState = svtkCompassRepresentation::Outside;
    return this->InteractionState;
  }

  // is the pick on the ring?
  int center[2];
  double rsize;
  this->GetCenterAndUnitRadius(center, rsize);
  double radius = sqrt(
    static_cast<double>((x - center[0]) * (x - center[0]) + (y - center[1]) * (y - center[1])));

  if (radius < rsize * this->OuterRadius + 2 && radius > rsize * this->InnerRadius - 2)
  {
    this->InteractionState = svtkCompassRepresentation::Adjusting;
    return this->InteractionState;
  }

  // on tilt?
  int tiltState = this->TiltRepresentation->ComputeInteractionState(x, y, modify);
  if (tiltState != svtkCenteredSliderRepresentation::Outside)
  {
    switch (tiltState)
    {
      case svtkSliderRepresentation::LeftCap:
        this->InteractionState = svtkCompassRepresentation::TiltDown;
        break;
      case svtkSliderRepresentation::RightCap:
        this->InteractionState = svtkCompassRepresentation::TiltUp;
        break;
      case svtkSliderRepresentation::Slider:
        this->InteractionState = svtkCompassRepresentation::TiltAdjusting;
        break;
    }
    return this->InteractionState;
  }

  // on dist?
  int distanceState = this->DistanceRepresentation->ComputeInteractionState(x, y, modify);
  if (distanceState != svtkCenteredSliderRepresentation::Outside)
  {
    switch (distanceState)
    {
      case svtkSliderRepresentation::LeftCap:
        this->InteractionState = svtkCompassRepresentation::DistanceOut;
        break;
      case svtkSliderRepresentation::RightCap:
        this->InteractionState = svtkCompassRepresentation::DistanceIn;
        break;
      case svtkSliderRepresentation::Slider:
        this->InteractionState = svtkCompassRepresentation::DistanceAdjusting;
        break;
    }
    return this->InteractionState;
  }

  if (radius < rsize * 3.0)
  {
    this->InteractionState = svtkCompassRepresentation::Inside;
    return this->InteractionState;
  }

  this->InteractionState = svtkCompassRepresentation::Outside;
  return this->InteractionState;
}

//----------------------------------------------------------------------
void svtkCompassRepresentation::SetRenderer(svtkRenderer* ren)
{
  this->Superclass::SetRenderer(ren);
  this->TiltRepresentation->SetRenderer(ren);
  this->DistanceRepresentation->SetRenderer(ren);
}
