/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCheckerboardRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCheckerboardRepresentation.h"
#include "svtkCommand.h"
#include "svtkImageActor.h"
#include "svtkImageCheckerboard.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkObjectFactory.h"
#include "svtkSliderRepresentation3D.h"

svtkStandardNewMacro(svtkCheckerboardRepresentation);

svtkCxxSetObjectMacro(svtkCheckerboardRepresentation, Checkerboard, svtkImageCheckerboard);
svtkCxxSetObjectMacro(svtkCheckerboardRepresentation, ImageActor, svtkImageActor);

svtkCxxSetObjectMacro(svtkCheckerboardRepresentation, TopRepresentation, svtkSliderRepresentation3D);
svtkCxxSetObjectMacro(svtkCheckerboardRepresentation, RightRepresentation, svtkSliderRepresentation3D);
svtkCxxSetObjectMacro(
  svtkCheckerboardRepresentation, BottomRepresentation, svtkSliderRepresentation3D);
svtkCxxSetObjectMacro(svtkCheckerboardRepresentation, LeftRepresentation, svtkSliderRepresentation3D);

//----------------------------------------------------------------------
svtkCheckerboardRepresentation::svtkCheckerboardRepresentation()
{
  this->Checkerboard = nullptr;
  this->ImageActor = nullptr;

  this->TopRepresentation = svtkSliderRepresentation3D::New();
  this->TopRepresentation->ShowSliderLabelOff();
  this->TopRepresentation->SetTitleText(nullptr);
  this->TopRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->TopRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->TopRepresentation->SetSliderLength(0.050);
  this->TopRepresentation->SetSliderWidth(0.025);
  this->TopRepresentation->SetTubeWidth(0.015);
  this->TopRepresentation->SetEndCapLength(0.0);
  this->TopRepresentation->SetMinimumValue(1);
  this->TopRepresentation->SetMaximumValue(10);
  this->TopRepresentation->SetSliderShapeToCylinder();

  this->RightRepresentation = svtkSliderRepresentation3D::New();
  this->RightRepresentation->ShowSliderLabelOff();
  this->RightRepresentation->SetTitleText(nullptr);
  this->RightRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->RightRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->RightRepresentation->SetSliderLength(0.050);
  this->RightRepresentation->SetSliderWidth(0.025);
  this->RightRepresentation->SetTubeWidth(0.015);
  this->RightRepresentation->SetEndCapLength(0.0);
  this->RightRepresentation->SetMinimumValue(1);
  this->RightRepresentation->SetMaximumValue(10);
  this->RightRepresentation->SetSliderShapeToCylinder();

  this->BottomRepresentation = svtkSliderRepresentation3D::New();
  this->BottomRepresentation->ShowSliderLabelOff();
  this->BottomRepresentation->SetTitleText(nullptr);
  this->BottomRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->BottomRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->BottomRepresentation->SetSliderLength(0.050);
  this->BottomRepresentation->SetSliderWidth(0.025);
  this->BottomRepresentation->SetTubeWidth(0.015);
  this->BottomRepresentation->SetEndCapLength(0.0);
  this->BottomRepresentation->SetMinimumValue(1);
  this->BottomRepresentation->SetMaximumValue(10);
  this->BottomRepresentation->SetSliderShapeToCylinder();

  this->LeftRepresentation = svtkSliderRepresentation3D::New();
  this->LeftRepresentation->ShowSliderLabelOff();
  this->LeftRepresentation->SetTitleText(nullptr);
  this->LeftRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->LeftRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->LeftRepresentation->SetSliderLength(0.050);
  this->LeftRepresentation->SetSliderWidth(0.025);
  this->LeftRepresentation->SetTubeWidth(0.015);
  this->LeftRepresentation->SetEndCapLength(0.0);
  this->LeftRepresentation->SetMinimumValue(1);
  this->LeftRepresentation->SetMaximumValue(10);
  this->LeftRepresentation->SetSliderShapeToCylinder();

  this->CornerOffset = 0.00;
  this->OrthoAxis = 2;
}

//----------------------------------------------------------------------
svtkCheckerboardRepresentation::~svtkCheckerboardRepresentation()
{
  if (this->Checkerboard)
  {
    this->Checkerboard->Delete();
  }
  if (this->ImageActor)
  {
    this->ImageActor->Delete();
  }

  this->TopRepresentation->Delete();
  this->RightRepresentation->Delete();
  this->BottomRepresentation->Delete();
  this->LeftRepresentation->Delete();
}

//----------------------------------------------------------------------
void svtkCheckerboardRepresentation::SliderValueChanged(int sliderNum)
{
  int* numDivisions = this->Checkerboard->GetNumberOfDivisions();
  int value;
  int div[] = { 1, 1, 1 };

  if (sliderNum == svtkCheckerboardRepresentation::TopSlider)
  {
    value = static_cast<int>(this->TopRepresentation->GetValue());
    this->BottomRepresentation->SetValue(this->TopRepresentation->GetValue());
    switch (this->OrthoAxis)
    {
      case 0:
        div[1] = value;
        div[2] = numDivisions[2];
        break;
      case 1:
        div[0] = value;
        div[2] = numDivisions[2];
        break;
      case 2:
        div[0] = value;
        div[1] = numDivisions[1];
        break;
    }

    this->Checkerboard->SetNumberOfDivisions(div);
  }
  else if (sliderNum == svtkCheckerboardRepresentation::RightSlider)
  {
    value = static_cast<int>(this->RightRepresentation->GetValue());
    this->LeftRepresentation->SetValue(this->RightRepresentation->GetValue());
    switch (this->OrthoAxis)
    {
      case 0:
        div[1] = numDivisions[1];
        div[2] = value;
        break;
      case 1:
        div[0] = numDivisions[0];
        div[2] = value;
        break;
      case 2:
        div[0] = numDivisions[0];
        div[1] = value;
        break;
    }

    this->Checkerboard->SetNumberOfDivisions(div);
  }
  else if (sliderNum == svtkCheckerboardRepresentation::BottomSlider)
  {
    value = static_cast<int>(this->BottomRepresentation->GetValue());
    this->TopRepresentation->SetValue(this->BottomRepresentation->GetValue());
    switch (this->OrthoAxis)
    {
      case 0:
        div[1] = value;
        div[2] = numDivisions[2];
        break;
      case 1:
        div[0] = value;
        div[2] = numDivisions[2];
        break;
      case 2:
        div[0] = value;
        div[1] = numDivisions[1];
        break;
    }

    this->Checkerboard->SetNumberOfDivisions(div);
  }
  else if (sliderNum == svtkCheckerboardRepresentation::LeftSlider)
  {
    value = static_cast<int>(this->LeftRepresentation->GetValue());
    this->RightRepresentation->SetValue(this->LeftRepresentation->GetValue());
    switch (this->OrthoAxis)
    {
      case 0:
        div[1] = numDivisions[1];
        div[2] = value;
        break;
      case 1:
        div[0] = numDivisions[0];
        div[2] = value;
        break;
      case 2:
        div[0] = numDivisions[0];
        div[1] = value;
        break;
    }

    this->Checkerboard->SetNumberOfDivisions(div);
  }
}

//----------------------------------------------------------------------
void svtkCheckerboardRepresentation::BuildRepresentation()
{
  // Make sure that the checkerboard is up to date
  if (!this->Checkerboard || !this->ImageActor)
  {
    svtkErrorMacro("requires a checkerboard and image actor");
    return;
  }

  double bounds[6];
  svtkImageData* image = this->ImageActor->GetInput();
  this->ImageActor->GetMapper()->GetInputAlgorithm()->Update();
  image->GetBounds(bounds);
  if (image->GetDataDimension() != 2)
  {
    svtkErrorMacro(<< " requires a 2D image");
    return;
  }
  double t0 = bounds[1] - bounds[0];
  double t1 = bounds[3] - bounds[2];
  double t2 = bounds[5] - bounds[4];
  this->OrthoAxis = (t0 < t1 ? (t0 < t2 ? 0 : 2) : (t1 < t2 ? 1 : 2));
  double o0 = t0 * this->CornerOffset;
  double o1 = t1 * this->CornerOffset;
  double o2 = t2 * this->CornerOffset;

  // Set up the initial values in the slider widgets
  int* numDivisions = this->Checkerboard->GetNumberOfDivisions();

  if (this->OrthoAxis == 0) // x-axis
  {
    // point1 and point2 are switched for top and bottom in case a
    // user wants to see the slider label positions as text, and
    // rotation of the text about the slider's local x-axis must be
    // set correctly.  Similar logic applies to X-Z plane case.

    this->TopRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[2] + o1, bounds[5]);
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[3] - o1, bounds[5]);
    this->TopRepresentation->SetValue(numDivisions[1]);
    this->TopRepresentation->SetRotation(90.0);

    this->RightRepresentation->GetPoint1Coordinate()->SetValue(
      bounds[0], bounds[3], bounds[4] + o2);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(
      bounds[0], bounds[3], bounds[5] - o2);
    this->RightRepresentation->SetValue(numDivisions[2]);
    this->RightRepresentation->SetRotation(0.0);

    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(
      bounds[0], bounds[2] + o1, bounds[4]);
    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(
      bounds[0], bounds[3] - o1, bounds[4]);
    this->BottomRepresentation->SetValue(numDivisions[1]);
    this->BottomRepresentation->SetRotation(90.0);

    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[2], bounds[4] + o2);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[2], bounds[5] - o2);
    this->LeftRepresentation->SetValue(numDivisions[2]);
    this->LeftRepresentation->SetRotation(0.0);
  }
  else if (this->OrthoAxis == 1) // y-axis
  {
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(bounds[0] + o0, bounds[2], bounds[5]);
    this->TopRepresentation->GetPoint2Coordinate()->SetValue(bounds[1] - o0, bounds[2], bounds[5]);
    this->TopRepresentation->SetValue(numDivisions[0]);
    this->TopRepresentation->SetRotation(90.0);

    this->RightRepresentation->GetPoint1Coordinate()->SetValue(
      bounds[1], bounds[2], bounds[4] + o2);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(
      bounds[1], bounds[2], bounds[5] - o2);
    this->RightRepresentation->SetValue(numDivisions[2]);
    this->RightRepresentation->SetRotation(90.0);

    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(
      bounds[0] + o0, bounds[2], bounds[4]);
    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(
      bounds[1] - o0, bounds[2], bounds[4]);
    this->BottomRepresentation->SetValue(numDivisions[0]);
    this->BottomRepresentation->SetRotation(90.0);

    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[2], bounds[4] + o2);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[2], bounds[5] - o2);
    this->LeftRepresentation->SetValue(numDivisions[2]);
    this->LeftRepresentation->SetRotation(90.0);
  }
  else // if( orthoAxis == 2 ) //z-axis
  {
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(bounds[0] + o0, bounds[3], bounds[4]);
    this->TopRepresentation->GetPoint2Coordinate()->SetValue(bounds[1] - o0, bounds[3], bounds[4]);
    this->TopRepresentation->SetValue(numDivisions[0]);
    this->TopRepresentation->SetRotation(0.0);

    this->RightRepresentation->GetPoint1Coordinate()->SetValue(
      bounds[1], bounds[2] + o1, bounds[4]);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(
      bounds[1], bounds[3] - o1, bounds[4]);
    this->RightRepresentation->SetValue(numDivisions[1]);
    this->RightRepresentation->SetRotation(0.0);

    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(
      bounds[0] + o0, bounds[2], bounds[4]);
    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(
      bounds[1] - o0, bounds[2], bounds[4]);
    this->BottomRepresentation->SetValue(numDivisions[0]);
    this->BottomRepresentation->SetRotation(0.0);

    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[2] + o1, bounds[4]);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[3] - o1, bounds[4]);
    this->LeftRepresentation->SetValue(numDivisions[1]);
    this->LeftRepresentation->SetRotation(0.0);
  }

  this->TopRepresentation->BuildRepresentation();
  this->RightRepresentation->BuildRepresentation();
  this->BottomRepresentation->BuildRepresentation();
  this->LeftRepresentation->BuildRepresentation();
}

//----------------------------------------------------------------------
void svtkCheckerboardRepresentation::GetActors(svtkPropCollection* pc)
{
  this->TopRepresentation->GetActors(pc);
  this->RightRepresentation->GetActors(pc);
  this->BottomRepresentation->GetActors(pc);
  this->LeftRepresentation->GetActors(pc);
}

//----------------------------------------------------------------------
void svtkCheckerboardRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  this->TopRepresentation->ReleaseGraphicsResources(w);
  this->RightRepresentation->ReleaseGraphicsResources(w);
  this->BottomRepresentation->ReleaseGraphicsResources(w);
  this->LeftRepresentation->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int svtkCheckerboardRepresentation::RenderOverlay(svtkViewport* v)
{
  int count = this->TopRepresentation->RenderOverlay(v);
  count += this->RightRepresentation->RenderOverlay(v);
  count += this->BottomRepresentation->RenderOverlay(v);
  count += this->LeftRepresentation->RenderOverlay(v);
  return count;
}

//----------------------------------------------------------------------
int svtkCheckerboardRepresentation::RenderOpaqueGeometry(svtkViewport* v)
{
  int count = this->TopRepresentation->RenderOpaqueGeometry(v);
  count += this->RightRepresentation->RenderOpaqueGeometry(v);
  count += this->BottomRepresentation->RenderOpaqueGeometry(v);
  count += this->LeftRepresentation->RenderOpaqueGeometry(v);
  return count;
}

//-----------------------------------------------------------------------------
int svtkCheckerboardRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* v)
{
  int count = this->TopRepresentation->RenderTranslucentPolygonalGeometry(v);
  count += this->RightRepresentation->RenderTranslucentPolygonalGeometry(v);
  count += this->BottomRepresentation->RenderTranslucentPolygonalGeometry(v);
  count += this->LeftRepresentation->RenderTranslucentPolygonalGeometry(v);
  return count;
}
//-----------------------------------------------------------------------------
svtkTypeBool svtkCheckerboardRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->TopRepresentation->HasTranslucentPolygonalGeometry();
  result |= this->RightRepresentation->HasTranslucentPolygonalGeometry();
  result |= this->BottomRepresentation->HasTranslucentPolygonalGeometry();
  result |= this->LeftRepresentation->HasTranslucentPolygonalGeometry();
  return result;
}

//----------------------------------------------------------------------
void svtkCheckerboardRepresentation::PrintSelf(ostream& os, svtkIndent indent)
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

  if (this->Checkerboard)
  {
    os << indent << "Checkerboard: " << this->Checkerboard << "\n";
  }
  else
  {
    os << indent << "Image Checkerboard: (none)\n";
  }

  os << indent << "Corner Offset: " << this->CornerOffset << "\n";

  os << indent << "Top Representation\n";
  this->TopRepresentation->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Bottom Representation\n";
  this->BottomRepresentation->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Right Representation\n";
  this->RightRepresentation->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Left Representation\n";
  this->LeftRepresentation->PrintSelf(os, indent.GetNextIndent());
}
