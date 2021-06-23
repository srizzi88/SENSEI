/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliderRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSliderRepresentation3D.h"
#include "svtkActor.h"
#include "svtkAssembly.h"
#include "svtkAssemblyPath.h"
#include "svtkBox.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellPicker.h"
#include "svtkCommand.h"
#include "svtkCylinder.h"
#include "svtkCylinderSource.h"
#include "svtkEvent.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkVectorText.h"
#include "svtkWindow.h"

svtkStandardNewMacro(svtkSliderRepresentation3D);

//----------------------------------------------------------------------
svtkSliderRepresentation3D::svtkSliderRepresentation3D()
{
  this->SliderShape = svtkSliderRepresentation3D::SphereShape;
  this->SliderLength = 0.05;
  this->SliderWidth = 0.05;
  this->EndCapLength = 0.025;
  this->TubeWidth = 0.025;
  this->Rotation = 0.0;

  // The cylinder used to model various parts of the widget
  // (caps, tube, and possible the slider)
  this->CylinderSource = svtkCylinderSource::New();
  this->CylinderSource->SetResolution(16);
  this->CylinderSource->SetCenter(0.0, 0.0, 0.0);
  this->CylinderSource->SetRadius(0.5);
  this->CylinderSource->SetHeight(1.0);

  svtkTransform* xform = svtkTransform::New();
  xform->RotateZ(90.0);
  this->Cylinder = svtkTransformPolyDataFilter::New(); // align the axis along the x-axis
  this->Cylinder->SetInputConnection(this->CylinderSource->GetOutputPort());
  this->Cylinder->SetTransform(xform);
  xform->Delete();

  // The tube (the slider moves along the tube)
  this->TubeMapper = svtkPolyDataMapper::New();
  this->TubeMapper->SetInputConnection(this->CylinderSource->GetOutputPort());

  this->TubeProperty = svtkProperty::New();
  this->TubeProperty->SetColor(1, 1, 1);

  this->TubeActor = svtkActor::New();
  this->TubeActor->SetMapper(this->TubeMapper);
  this->TubeActor->SetProperty(this->TubeProperty);
  this->TubeActor->RotateZ(90.0);

  // The slider (either sphere or cylinder)
  this->SliderSource = svtkSphereSource::New();
  this->SliderSource->SetPhiResolution(8);
  this->SliderSource->SetThetaResolution(16);
  this->SliderSource->SetRadius(0.5);

  this->SliderMapper = svtkPolyDataMapper::New();
  this->SliderMapper->SetInputConnection(this->SliderSource->GetOutputPort());

  this->SliderProperty = svtkProperty::New();
  this->SliderProperty->SetColor(0.2000, 0.6300, 0.7900); // peacock
  this->SliderProperty->SetSpecularColor(1, 1, 1);
  this->SliderProperty->SetSpecular(0.3);
  this->SliderProperty->SetSpecularPower(20);
  this->SliderProperty->SetAmbient(0.2);
  this->SliderProperty->SetDiffuse(0.8);

  this->SliderActor = svtkActor::New();
  this->SliderActor->SetMapper(this->SliderMapper);
  this->SliderActor->SetProperty(this->SliderProperty);

  // Position the sphere (slider) and compute some intermediate information
  this->SP1[0] = -0.5 + this->EndCapLength + this->SliderLength / 2.0;
  this->SP1[1] = 0.0;
  this->SP1[2] = 0.0;
  this->SP2[0] = -0.5 + (1.0 - this->EndCapLength) - this->SliderLength / 2.0;
  this->SP2[1] = 0.0;
  this->SP2[2] = 0.0;

  this->SelectedProperty = svtkProperty::New();
  this->SelectedProperty->SetColor(1.0000, 0.4118, 0.7059); // hot pink
  this->SelectedProperty->SetSpecularColor(1, 1, 1);
  this->SelectedProperty->SetSpecular(0.3);
  this->SelectedProperty->SetSpecularPower(20);
  this->SelectedProperty->SetAmbient(0.2);
  this->SelectedProperty->SetDiffuse(0.8);

  // The left cap
  this->LeftCapMapper = svtkPolyDataMapper::New();
  this->LeftCapMapper->SetInputConnection(this->Cylinder->GetOutputPort());

  this->CapProperty = svtkProperty::New();
  this->CapProperty->SetColor(1, 1, 1);
  this->CapProperty->SetSpecularColor(1, 1, 1);
  this->CapProperty->SetSpecular(0.3);
  this->CapProperty->SetSpecularPower(20);
  this->CapProperty->SetAmbient(0.2);
  this->CapProperty->SetDiffuse(0.8);

  this->LeftCapActor = svtkActor::New();
  this->LeftCapActor->SetMapper(this->LeftCapMapper);
  this->LeftCapActor->SetProperty(this->CapProperty);

  // The right cap
  this->RightCapMapper = svtkPolyDataMapper::New();
  this->RightCapMapper->SetInputConnection(this->Cylinder->GetOutputPort());

  this->RightCapActor = svtkActor::New();
  this->RightCapActor->SetMapper(this->RightCapMapper);
  this->RightCapActor->SetProperty(this->CapProperty);

  this->Point1Coordinate = svtkCoordinate::New();
  this->Point1Coordinate->SetCoordinateSystemToWorld();
  this->Point1Coordinate->SetValue(-1.0, 0.0, 0.0);

  this->Point2Coordinate = svtkCoordinate::New();
  this->Point2Coordinate->SetCoordinateSystemToWorld();
  this->Point2Coordinate->SetValue(1.0, 0.0, 0.0);

  // Labels and text
  this->ShowSliderLabel = 1;
  this->LabelHeight = 0.05;
  this->LabelText = svtkVectorText::New();
  this->LabelText->SetText("");
  this->LabelMapper = svtkPolyDataMapper::New();
  this->LabelMapper->SetInputConnection(this->LabelText->GetOutputPort());
  this->LabelActor = svtkActor::New();
  this->LabelActor->SetMapper(this->LabelMapper);
  this->LabelActor->PickableOff();

  this->TitleText = svtkVectorText::New();
  this->TitleText->SetText("");
  this->TitleHeight = 0.15;
  this->TitleMapper = svtkPolyDataMapper::New();
  this->TitleMapper->SetInputConnection(this->TitleText->GetOutputPort());
  this->TitleActor = svtkActor::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->PickableOff();

  // Finally, the assembly that holds everything together
  this->WidgetAssembly = svtkAssembly::New();
  this->WidgetAssembly->AddPart(this->TubeActor);
  this->WidgetAssembly->AddPart(this->SliderActor);
  this->WidgetAssembly->AddPart(this->LeftCapActor);
  this->WidgetAssembly->AddPart(this->RightCapActor);
  this->WidgetAssembly->AddPart(this->LabelActor);
  this->WidgetAssembly->AddPart(this->TitleActor);

  // Manage the picking stuff
  this->Picker = svtkCellPicker::New();
  this->Picker->SetTolerance(0.001);
  this->Picker->AddPickList(this->WidgetAssembly);
  this->Picker->PickFromListOn();

  this->Matrix = svtkMatrix4x4::New();
  this->Transform = svtkTransform::New();
}

//----------------------------------------------------------------------
svtkSliderRepresentation3D::~svtkSliderRepresentation3D()
{
  this->WidgetAssembly->Delete();

  this->CylinderSource->Delete();
  this->Cylinder->Delete();

  this->TubeMapper->Delete();
  this->TubeActor->Delete();
  this->TubeProperty->Delete();

  this->SliderSource->Delete();
  this->SliderMapper->Delete();
  this->SliderActor->Delete();

  this->SliderProperty->Delete();
  this->SelectedProperty->Delete();

  this->LeftCapMapper->Delete();
  this->LeftCapActor->Delete();
  this->CapProperty->Delete();

  this->RightCapMapper->Delete();
  this->RightCapActor->Delete();

  this->Point1Coordinate->Delete();
  this->Point2Coordinate->Delete();

  this->Picker->Delete();

  this->LabelText->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();
  this->TitleText->Delete();
  this->TitleMapper->Delete();
  this->TitleActor->Delete();

  this->Matrix->Delete();
  this->Transform->Delete();
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::SetTitleText(const char* label)
{
  this->TitleText->SetText(label);
  if (this->TitleText->GetMTime() > this->GetMTime())
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------
const char* svtkSliderRepresentation3D::GetTitleText()
{
  return this->TitleText->GetText();
}

//----------------------------------------------------------------------
svtkCoordinate* svtkSliderRepresentation3D::GetPoint1Coordinate()
{
  return this->Point1Coordinate;
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::StartWidgetInteraction(double eventPos[2])
{
  svtkAssemblyPath* path = this->GetAssemblyPath(eventPos[0], eventPos[1], 0., this->Picker);

  if (path != nullptr)
  {
    svtkActor* prop = static_cast<svtkActor*>(path->GetLastNode()->GetViewProp());

    if (prop == this->SliderActor)
    {
      this->InteractionState = svtkSliderRepresentation::Slider;
      this->PickedT = this->CurrentT;
    }
    else
    {
      if (prop == this->TubeActor)
      {
        this->InteractionState = svtkSliderRepresentation::Tube;
        this->PickedT = this->ComputePickPosition(eventPos);
      }
      else if (prop == this->LeftCapActor)
      {
        this->InteractionState = svtkSliderRepresentation::LeftCap;
        this->PickedT = 0.0;
      }
      else if (prop == this->RightCapActor)
      {
        this->InteractionState = svtkSliderRepresentation::RightCap;
        this->PickedT = 1.0;
      }
    }
  }
  else
  {
    this->InteractionState = svtkSliderRepresentation::Outside;
  }
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::WidgetInteraction(double eventPos[2])
{
  double t = this->ComputePickPosition(eventPos);
  this->SetValue(this->MinimumValue + t * (this->MaximumValue - this->MinimumValue));
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
svtkCoordinate* svtkSliderRepresentation3D::GetPoint2Coordinate()
{
  return this->Point2Coordinate;
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  double placeFactor = this->PlaceFactor;
  this->PlaceFactor = 1.0;
  this->AdjustBounds(bds, bounds, center);
  this->PlaceFactor = placeFactor;

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  // When PlaceWidget is invoked, the widget orientation is preserved, but it
  // is allowed to translate and scale. This means it is centered in the
  // bounding box, and the representation scales itself to intersect the sides
  // of the bounding box. Thus we have to determine where Point1 and Point2
  // intersect the bounding box.
  double *p1, *p2, r[3], o[3], t, placedP1[3], placedP2[3];
  if (this->Renderer)
  {
    p1 = this->Point1Coordinate->GetComputedWorldValue(this->Renderer);
    p2 = this->Point2Coordinate->GetComputedWorldValue(this->Renderer);
  }
  else
  {
    p1 = this->Point1Coordinate->GetValue();
    p2 = this->Point2Coordinate->GetValue();
  }

  // Okay, this looks really weird, we are shooting rays from OUTSIDE
  // the bounding box back towards it. This is because the IntersectBox()
  // method computes intersections only if the ray originates outside the
  // bounding box.
  r[0] = this->InitialLength * (p1[0] - p2[0]);
  r[1] = this->InitialLength * (p1[1] - p2[1]);
  r[2] = this->InitialLength * (p1[2] - p2[2]);
  o[0] = center[0] - r[0];
  o[1] = center[1] - r[1];
  o[2] = center[2] - r[2];
  svtkBox::IntersectBox(bounds, o, r, placedP1, t);
  this->Point1Coordinate->SetCoordinateSystemToWorld();
  this->Point1Coordinate->SetValue(placedP1);

  r[0] = this->InitialLength * (p2[0] - p1[0]);
  r[1] = this->InitialLength * (p2[1] - p1[1]);
  r[2] = this->InitialLength * (p2[2] - p1[2]);
  o[0] = center[0] - r[0];
  o[1] = center[1] - r[1];
  o[2] = center[2] - r[2];
  svtkBox::IntersectBox(bounds, o, r, placedP2, t);
  this->Point2Coordinate->SetCoordinateSystemToWorld();
  this->Point2Coordinate->SetValue(placedP2);

  // Position the handles at the end of the lines
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
double svtkSliderRepresentation3D::ComputePickPosition(double eventPos[2])
{
  // Transform current pick ray into canonical (untransformed)
  // widget coordinates. This requires a camera.
  svtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return 0.0;
  }

  // The pick ray is defined by the camera position and the (X,Y)
  // pick position in the renderer. The depth of the (X,Y) pick is
  // the back clipping plane.
  double cameraWorldPosition[4], cameraPosition[4];
  camera->GetPosition(cameraWorldPosition);
  cameraWorldPosition[3] = 1.0;
  this->Transform->TransformPoint(cameraWorldPosition, cameraPosition);

  double rayEndPoint[4], rayPosition[4];
  svtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, eventPos[0], eventPos[1], 1.0, rayEndPoint);
  this->Transform->TransformPoint(rayEndPoint, rayPosition);

  // Now intersect the two lines and compute the pick position
  // along the slider.
  double u, v;
  svtkLine::Intersection(this->SP1, this->SP2, cameraPosition, rayPosition, u, v);

  return u;
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::Highlight(int highlight)
{
  if (highlight)
  {
    this->SliderActor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->SliderActor->SetProperty(this->SliderProperty);
  }
}

//----------------------------------------------------------------------
// Description:
// Override GetMTime to include point coordinates
svtkMTimeType svtkSliderRepresentation3D::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType p1Time, p2Time;

  p1Time = this->Point1Coordinate->GetMTime();
  mTime = (p1Time > mTime ? p1Time : mTime);
  p2Time = this->Point2Coordinate->GetMTime();
  mTime = (p2Time > mTime ? p2Time : mTime);

  return mTime;
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {
    double sx, sy;
    double t = (this->Value - this->MinimumValue) / (this->MaximumValue - this->MinimumValue);

    // Setup the geometry of the widget (canonical -0.5 to 0.5 along the x-axis)
    this->SP1[0] = -0.5 + this->EndCapLength + this->SliderLength / 2.0;
    this->SP1[1] = 0.0;
    this->SP1[2] = 0.0;
    this->SP2[0] = -0.5 + (1.0 - this->EndCapLength) - this->SliderLength / 2.0;
    this->SP2[1] = 0.0;
    this->SP2[2] = 0.0;

    // The end points P1,P2 are assumed set by PlaceWidget() or other
    double* p1 = this->Point1Coordinate->GetComputedWorldValue(this->Renderer);
    double* p2 = this->Point2Coordinate->GetComputedWorldValue(this->Renderer);
    this->Length = sqrt(svtkMath::Distance2BetweenPoints(p1, p2));
    if (this->Length <= 0.0)
    {
      this->Length = 1.0;
    }

    // Update the canonical shape of the widget
    if (this->SliderShape == svtkSliderRepresentation3D::SphereShape)
    {
      this->SliderMapper->SetInputConnection(this->SliderSource->GetOutputPort());
    }
    else
    {
      this->SliderMapper->SetInputConnection(this->Cylinder->GetOutputPort());
    }

    this->TubeActor->SetScale(this->TubeWidth, 1.0 - (2.0 * this->EndCapLength), this->TubeWidth);
    this->LeftCapActor->SetPosition(-0.5 + (this->EndCapLength / 2.0), 0, 0);
    this->LeftCapActor->SetScale(this->EndCapWidth, this->EndCapLength, this->EndCapWidth);
    this->RightCapActor->SetPosition(0.5 - (this->EndCapLength / 2.0), 0, 0);
    this->RightCapActor->SetScale(this->EndCapWidth, this->EndCapLength, this->EndCapWidth);
    if (this->EndCapLength <= 0.0)
    {
      this->RightCapActor->VisibilityOff();
      this->LeftCapActor->VisibilityOff();
    }
    else
    {
      this->RightCapActor->VisibilityOn();
      this->LeftCapActor->VisibilityOn();
    }

    // Position the slider (sphere)
    //
    double p[3];
    p[0] = this->SP1[0] + t * (this->SP2[0] - this->SP1[0]);
    p[1] = this->SP1[1] + t * (this->SP2[1] - this->SP1[1]);
    p[2] = this->SP1[2] + t * (this->SP2[2] - this->SP1[2]);
    this->SliderActor->SetPosition(p);
    this->SliderActor->SetScale(this->SliderLength, this->SliderWidth, this->SliderWidth);

    // Here we position the title and the slider label. Of course this is a
    // function of the text strings that ave been supplied.
    // Place the title
    if (this->TitleText->GetText() == nullptr || *(this->TitleText->GetText()) == '\0')
    {
      this->TitleActor->VisibilityOff();
    }
    else
    {
      double bounds[6];
      this->TitleActor->VisibilityOn();
      this->TitleText->Update();
      this->TitleText->GetOutput()->GetBounds(bounds);

      sy = this->TitleHeight / (bounds[3] - bounds[2]);
      sx = sy;

      // Compute translation: first, where the current center is
      // (We want to perform scaling and rotation around origin.)
      double c1[3], c2[3];
      c1[0] = (bounds[1] + bounds[0]) / 2.0;
      c1[1] = (bounds[3] + bounds[2]) / 2.0;
      c1[2] = (bounds[5] + bounds[4]) / 2.0;

      // Where we want the center to be
      c2[0] = (this->SP1[0] + this->SP2[0]) / 2.0;
      c2[1] = (this->SP1[1] + this->SP2[1]) / 2.0 - 2.0 * sy;
      c2[2] = (this->SP1[2] + this->SP2[2]) / 2.0;

      // Transform the text
      this->TitleActor->SetOrigin(c1[0], c1[1], c1[2]);
      this->TitleActor->SetScale(sx, sy, 1.0);
      this->TitleActor->SetPosition(c2[0] - c1[0], c2[1] - c1[1], c2[2] - c1[2]);
    }

    // Place the slider label
    if (!this->ShowSliderLabel)
    {
      this->LabelActor->VisibilityOff();
    }
    else
    {
      char label[256];
      snprintf(label, sizeof(label), this->LabelFormat, this->Value);
      double bounds[6];
      this->LabelActor->VisibilityOn();
      this->LabelText->SetText(label);
      this->LabelText->Update();
      this->LabelText->GetOutput()->GetBounds(bounds);

      // scaling
      sy = this->LabelHeight / (bounds[3] - bounds[2]);
      sx = sy;

      // Compute translation: first, where the current center is
      // (We want to perform scaling and rotation around origin.)
      double c1[3], c2[3];
      c1[0] = (bounds[1] + bounds[0]) / 2.0;
      c1[1] = (bounds[3] + bounds[2]) / 2.0;
      c1[2] = (bounds[5] + bounds[4]) / 2.0;

      // Where we want the center to be
      c2[0] = this->SP1[0] + t * (this->SP2[0] - this->SP1[0]);
      c2[1] = this->SP1[1] + t * (this->SP2[1] - this->SP1[1]) + 2.0 * sy;
      c2[2] = this->SP1[2] + t * (this->SP2[2] - this->SP1[2]);

      // Actor the text
      this->LabelActor->SetOrigin(c1[0], c1[1], c1[2]);
      this->LabelActor->SetScale(sx, sy, 1.0);
      this->LabelActor->SetPosition(c2[0] - c1[0], c2[1] - c1[1], c2[2] - c1[2]);
    }

    // Compute the rotation of the widget. Note that the widget as constructed
    // is oriented in the x-direction. Here we rotate the whole assembly.
    double x[3], v[3], axis[3];
    x[0] = 1.0;
    x[1] = 0.0;
    x[2] = 0.0;
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
    svtkMath::Normalize(v);
    svtkMath::Cross(v, x, axis);
    double theta, axisLen = svtkMath::Norm(axis);
    if (axisLen != 0.0)
    {
      theta = svtkMath::DegreesFromRadians(asin(axisLen));
    }
    else
    {
      theta = 0.0;
      axis[0] = 0.0;
      axis[1] = 1.0;
      axis[2] = 0.0;
    }
    this->WidgetAssembly->SetOrientation(0.0, 0.0, 0.0);
    this->WidgetAssembly->RotateX(this->Rotation);
    this->WidgetAssembly->RotateWXYZ(theta, axis[0], axis[1], axis[2]);
    this->WidgetAssembly->SetScale(this->Length, this->Length, this->Length);
    p[0] = (p1[0] + p2[0]) / 2.0;
    p[1] = (p1[1] + p2[1]) / 2.0;
    p[2] = (p1[2] + p2[2]) / 2.0;
    this->WidgetAssembly->SetPosition(p);

    // A final task: get the transformation matrix for the "tube"
    this->Transform->Pop();
    this->WidgetAssembly->GetMatrix(this->Matrix);
    this->Transform->SetMatrix(this->Matrix);
    this->Transform->Push();
    this->Transform->Inverse();

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::GetActors(svtkPropCollection* pc)
{
  pc->AddItem(this->WidgetAssembly);
}

//----------------------------------------------------------------------
double* svtkSliderRepresentation3D::GetBounds()
{
  this->BuildRepresentation();
  return this->WidgetAssembly->GetBounds();
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::ReleaseGraphicsResources(svtkWindow* w)
{
  this->WidgetAssembly->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int svtkSliderRepresentation3D::RenderOpaqueGeometry(svtkViewport* viewport)
{
  this->BuildRepresentation();
  return this->WidgetAssembly->RenderOpaqueGeometry(viewport);
}

//-----------------------------------------------------------------------------
int svtkSliderRepresentation3D::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  this->BuildRepresentation();
  return this->WidgetAssembly->RenderTranslucentPolygonalGeometry(viewport);
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkSliderRepresentation3D::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();
  return this->WidgetAssembly->HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::SetPoint1InWorldCoordinates(double x, double y, double z)
{
  this->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->GetPoint1Coordinate()->SetValue(x, y, z);
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::SetPoint2InWorldCoordinates(double x, double y, double z)
{
  this->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->GetPoint2Coordinate()->SetValue(x, y, z);
}

//----------------------------------------------------------------------
void svtkSliderRepresentation3D::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Rotation: " << this->Rotation << "\n";
  os << indent
     << "Label Text: " << (this->LabelText->GetText() ? this->LabelText->GetText() : "(none)")
     << "\n";
  os << indent
     << "Title Text: " << (this->TitleText->GetText() ? this->TitleText->GetText() : "(none)")
     << "\n";

  os << indent << "Point1 Coordinate: " << this->Point1Coordinate << "\n";
  this->Point1Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point2 Coordinate: " << this->Point2Coordinate << "\n";
  this->Point2Coordinate->PrintSelf(os, indent.GetNextIndent());

  if (this->SliderProperty)
  {
    os << indent << "Slider Property:\n";
    this->SliderProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Slider Property: (none)\n";
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

  if (this->TubeProperty)
  {
    os << indent << "TubeProperty:\n";
    this->TubeProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "TubeProperty: (none)\n";
  }

  if (this->CapProperty)
  {
    os << indent << "CapProperty:\n";
    this->CapProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "CapProperty: (none)\n";
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

  if (this->SliderShape == svtkSliderRepresentation3D::SphereShape)
  {
    os << indent << "Slider Shape: Sphere\n";
  }
  else
  {
    os << indent << "Slider Shape: Cylinder\n";
  }
}
