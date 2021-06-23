/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAngleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAngleRepresentation.h"
#include "svtkActor2D.h"
#include "svtkCoordinate.h"
#include "svtkHandleRepresentation.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"
#include "svtkWindow.h"

svtkCxxSetObjectMacro(svtkAngleRepresentation, HandleRepresentation, svtkHandleRepresentation);

//----------------------------------------------------------------------
svtkAngleRepresentation::svtkAngleRepresentation()
{
  this->HandleRepresentation = nullptr;
  this->Point1Representation = nullptr;
  this->CenterRepresentation = nullptr;
  this->Point2Representation = nullptr;

  this->Tolerance = 5;
  this->Placed = 0;

  this->Ray1Visibility = 1;
  this->Ray2Visibility = 1;
  this->ArcVisibility = 1;

  this->LabelFormat = new char[8];
  snprintf(this->LabelFormat, 8, "%s", "%-#6.3g");
}

//----------------------------------------------------------------------
svtkAngleRepresentation::~svtkAngleRepresentation()
{
  if (this->HandleRepresentation)
  {
    this->HandleRepresentation->Delete();
  }
  if (this->Point1Representation)
  {
    this->Point1Representation->Delete();
  }
  if (this->CenterRepresentation)
  {
    this->CenterRepresentation->Delete();
  }
  if (this->Point2Representation)
  {
    this->Point2Representation->Delete();
  }

  delete[] this->LabelFormat;
  this->LabelFormat = nullptr;
}

//----------------------------------------------------------------------
void svtkAngleRepresentation::InstantiateHandleRepresentation()
{
  if (!this->Point1Representation)
  {
    this->Point1Representation = this->HandleRepresentation->NewInstance();
    this->Point1Representation->ShallowCopy(this->HandleRepresentation);
  }

  if (!this->CenterRepresentation)
  {
    this->CenterRepresentation = this->HandleRepresentation->NewInstance();
    this->CenterRepresentation->ShallowCopy(this->HandleRepresentation);
  }

  if (!this->Point2Representation)
  {
    this->Point2Representation = this->HandleRepresentation->NewInstance();
    this->Point2Representation->ShallowCopy(this->HandleRepresentation);
  }
}

//----------------------------------------------------------------------
int svtkAngleRepresentation::ComputeInteractionState(
  int svtkNotUsed(X), int svtkNotUsed(Y), int svtkNotUsed(modify))
{
  if (this->Point1Representation == nullptr || this->CenterRepresentation == nullptr ||
    this->Point2Representation == nullptr)
  {
    this->InteractionState = svtkAngleRepresentation::Outside;
    return this->InteractionState;
  }

  int p1State = this->Point1Representation->GetInteractionState();
  int cState = this->CenterRepresentation->GetInteractionState();
  int p2State = this->Point2Representation->GetInteractionState();
  if (p1State == svtkHandleRepresentation::Nearby)
  {
    this->InteractionState = svtkAngleRepresentation::NearP1;
  }
  else if (cState == svtkHandleRepresentation::Nearby)
  {
    this->InteractionState = svtkAngleRepresentation::NearCenter;
  }
  else if (p2State == svtkHandleRepresentation::Nearby)
  {
    this->InteractionState = svtkAngleRepresentation::NearP2;
  }
  else
  {
    this->InteractionState = svtkAngleRepresentation::Outside;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void svtkAngleRepresentation::StartWidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint1DisplayPosition(pos);
  this->SetCenterDisplayPosition(pos);
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation::CenterWidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetCenterDisplayPosition(pos);
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation::WidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation::BuildRepresentation()
{
  // Make sure that tolerance is consistent between handles and this representation
  this->Point1Representation->SetTolerance(this->Tolerance);
  this->CenterRepresentation->SetTolerance(this->Tolerance);
  this->Point2Representation->SetTolerance(this->Tolerance);
}

//----------------------------------------------------------------------
void svtkAngleRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Angle: " << this->GetAngle() << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Ray1 Visibility: " << (this->Ray1Visibility ? "On\n" : "Off\n");
  os << indent << "Ray2 Visibility: " << (this->Ray2Visibility ? "On\n" : "Off\n");
  os << indent << "Arc Visibility: " << (this->ArcVisibility ? "On\n" : "Off\n");
  os << indent << "Handle Representation: " << this->HandleRepresentation << "\n";

  os << indent << "Label Format: ";
  if (this->LabelFormat)
  {
    os << this->LabelFormat << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Point1 Representation: ";
  if (this->Point1Representation)
  {
    this->Point1Representation->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Center Representation: ";
  if (this->CenterRepresentation)
  {
    this->CenterRepresentation->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Point2 Representation: ";
  if (this->Point2Representation)
  {
    this->Point2Representation->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
