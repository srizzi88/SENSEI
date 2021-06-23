/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDistanceRepresentation2D.h"
#include "svtkAxisActor2D.h"
#include "svtkBox.h"
#include "svtkCoordinate.h"
#include "svtkInteractorObserver.h"
#include "svtkLineSource.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointHandleRepresentation2D.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"
#include "svtkWindow.h"

svtkStandardNewMacro(svtkDistanceRepresentation2D);

//----------------------------------------------------------------------
svtkDistanceRepresentation2D::svtkDistanceRepresentation2D()
{
  // By default, use one of these handles
  this->HandleRepresentation = svtkPointHandleRepresentation2D::New();

  this->AxisProperty = svtkProperty2D::New();
  this->AxisProperty->SetColor(0, 1, 0);

  this->AxisActor = svtkAxisActor2D::New();
  this->AxisActor->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->AxisActor->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->AxisActor->SetNumberOfLabels(5);
  this->AxisActor->LabelVisibilityOff();
  this->AxisActor->AdjustLabelsOff();
  this->AxisActor->SetProperty(this->AxisProperty);
  this->AxisActor->SetTitle("Distance");
  this->AxisActor->GetTitleTextProperty()->SetBold(1);
  this->AxisActor->GetTitleTextProperty()->SetItalic(1);
  this->AxisActor->GetTitleTextProperty()->SetShadow(1);
  this->AxisActor->GetTitleTextProperty()->SetFontFamilyToArial();

  this->Distance = 0.0;
}

//----------------------------------------------------------------------
svtkDistanceRepresentation2D::~svtkDistanceRepresentation2D()
{
  this->AxisProperty->Delete();
  this->AxisActor->Delete();
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::GetPoint1WorldPosition(double pos[3])
{
  this->Point1Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::GetPoint2WorldPosition(double pos[3])
{
  this->Point2Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
double* svtkDistanceRepresentation2D::GetPoint1WorldPosition()
{
  if (!this->Point1Representation)
  {
    static double temp[3] = { 0, 0, 0 };
    return temp;
  }
  return this->Point1Representation->GetWorldPosition();
}

//----------------------------------------------------------------------
double* svtkDistanceRepresentation2D::GetPoint2WorldPosition()
{
  if (!this->Point2Representation)
  {
    static double temp[3] = { 0, 0, 0 };
    return temp;
  }
  return this->Point2Representation->GetWorldPosition();
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::SetPoint1DisplayPosition(double x[3])
{
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::SetPoint2DisplayPosition(double x[3])
{
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::SetPoint1WorldPosition(double x[3])
{
  if (this->Point1Representation)
  {
    this->Point1Representation->SetWorldPosition(x);
    this->BuildRepresentation();
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::SetPoint2WorldPosition(double x[3])
{
  if (this->Point2Representation)
  {
    this->Point2Representation->SetWorldPosition(x);
    this->BuildRepresentation();
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::GetPoint1DisplayPosition(double pos[3])
{
  this->Point1Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::GetPoint2DisplayPosition(double pos[3])
{
  this->Point2Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
svtkAxisActor2D* svtkDistanceRepresentation2D::GetAxis()
{
  return this->AxisActor;
}

//----------------------------------------------------------------------
svtkProperty2D* svtkDistanceRepresentation2D::GetAxisProperty()
{
  return this->AxisActor->GetProperty();
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime || this->AxisActor->GetMTime() > this->BuildTime ||
    this->AxisActor->GetTitleTextProperty()->GetMTime() > this->BuildTime ||
    this->Point1Representation->GetMTime() > this->BuildTime ||
    this->Point2Representation->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {
    this->Superclass::BuildRepresentation();

    // Okay, compute the distance and set the label
    double p1[3], p2[3];
    this->Point1Representation->GetWorldPosition(p1);
    this->Point2Representation->GetWorldPosition(p2);
    this->Distance = sqrt(svtkMath::Distance2BetweenPoints(p1, p2));

    this->AxisActor->GetPoint1Coordinate()->SetValue(p1);
    this->AxisActor->GetPoint2Coordinate()->SetValue(p2);
    this->AxisActor->SetRulerMode(this->RulerMode);
    if (this->Scale != 0.0)
    {
      this->AxisActor->SetRulerDistance(this->RulerDistance / this->Scale);
    }
    this->AxisActor->SetNumberOfLabels(this->NumberOfRulerTicks);

    char string[512];
    snprintf(string, sizeof(string), this->LabelFormat, this->Distance * this->Scale);
    this->AxisActor->SetTitle(string);

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::ReleaseGraphicsResources(svtkWindow* w)
{
  this->AxisActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int svtkDistanceRepresentation2D::RenderOverlay(svtkViewport* v)
{
  this->BuildRepresentation();

  if (this->AxisActor->GetVisibility())
  {
    return this->AxisActor->RenderOverlay(v);
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------
int svtkDistanceRepresentation2D::RenderOpaqueGeometry(svtkViewport* v)
{
  this->BuildRepresentation();

  if (this->AxisActor->GetVisibility())
  {
    return this->AxisActor->RenderOpaqueGeometry(v);
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation2D::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
