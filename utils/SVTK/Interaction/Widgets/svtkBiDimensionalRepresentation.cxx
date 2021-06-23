/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBiDimensionalRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBiDimensionalRepresentation.h"
#include "svtkActor2D.h"
#include "svtkCellArray.h"
#include "svtkCommand.h"
#include "svtkCoordinate.h"
#include "svtkHandleRepresentation.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointHandleRepresentation2D.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"
#include "svtkWindow.h"

#include <sstream>

//----------------------------------------------------------------------
svtkBiDimensionalRepresentation::svtkBiDimensionalRepresentation()
{
  // By default, use one of these handles
  this->HandleRepresentation = svtkPointHandleRepresentation2D::New();
  this->Point1Representation = nullptr;
  this->Point2Representation = nullptr;
  this->Point3Representation = nullptr;
  this->Point4Representation = nullptr;
  this->InstantiateHandleRepresentation();

  this->Modifier = 0;
  this->Tolerance = 5;
  this->Placed = 0;

  this->Line1Visibility = 1;
  this->Line2Visibility = 1;

  this->LabelFormat = new char[6];
  snprintf(this->LabelFormat, 6, "%s", "%0.3g");

  this->ID = SVTK_ID_MAX;
  this->IDInitialized = 0;

  this->ShowLabelAboveWidget = 1;
}

//----------------------------------------------------------------------
svtkBiDimensionalRepresentation::~svtkBiDimensionalRepresentation()
{
  if (this->HandleRepresentation)
  {
    this->HandleRepresentation->Delete();
  }
  if (this->Point1Representation)
  {
    this->Point1Representation->Delete();
  }
  if (this->Point2Representation)
  {
    this->Point2Representation->Delete();
  }
  if (this->Point3Representation)
  {
    this->Point3Representation->Delete();
  }
  if (this->Point4Representation)
  {
    this->Point4Representation->Delete();
  }

  this->SetLabelFormat(nullptr);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation ::SetHandleRepresentation(svtkHandleRepresentation* handle)
{
  if (handle == nullptr || handle == this->HandleRepresentation)
  {
    return;
  }

  this->Modified();
  this->HandleRepresentation->Delete();
  this->HandleRepresentation = handle;
  this->HandleRepresentation->Register(this);

  this->Point1Representation->Delete();
  this->Point2Representation->Delete();
  this->Point3Representation->Delete();
  this->Point4Representation->Delete();

  this->Point1Representation = nullptr;
  this->Point2Representation = nullptr;
  this->Point3Representation = nullptr;
  this->Point4Representation = nullptr;

  this->InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::GetPoint1WorldPosition(double pos[3])
{
  this->Point1Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::GetPoint2WorldPosition(double pos[3])
{
  this->Point2Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::GetPoint3WorldPosition(double pos[3])
{
  this->Point3Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::GetPoint4WorldPosition(double pos[3])
{
  this->Point4Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetPoint1DisplayPosition(double x[3])
{
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetPoint2DisplayPosition(double x[3])
{
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetPoint3DisplayPosition(double x[3])
{
  this->Point3Representation->SetDisplayPosition(x);
  double p[3];
  this->Point3Representation->GetWorldPosition(p);
  this->Point3Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetPoint4DisplayPosition(double x[3])
{
  this->Point4Representation->SetDisplayPosition(x);
  double p[3];
  this->Point4Representation->GetWorldPosition(p);
  this->Point4Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetPoint1WorldPosition(double x[3])
{
  this->Point1Representation->SetWorldPosition(x);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetPoint2WorldPosition(double x[3])
{
  this->Point2Representation->SetWorldPosition(x);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetPoint3WorldPosition(double x[3])
{
  this->Point3Representation->SetWorldPosition(x);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetPoint4WorldPosition(double x[3])
{
  this->Point4Representation->SetWorldPosition(x);
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::GetPoint1DisplayPosition(double pos[3])
{
  this->Point1Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::GetPoint2DisplayPosition(double pos[3])
{
  this->Point2Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::GetPoint3DisplayPosition(double pos[3])
{
  this->Point3Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::GetPoint4DisplayPosition(double pos[3])
{
  this->Point4Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::InstantiateHandleRepresentation()
{
  if (!this->Point1Representation)
  {
    this->Point1Representation = this->HandleRepresentation->NewInstance();
    this->Point1Representation->ShallowCopy(this->HandleRepresentation);
  }

  if (!this->Point2Representation)
  {
    this->Point2Representation = this->HandleRepresentation->NewInstance();
    this->Point2Representation->ShallowCopy(this->HandleRepresentation);
  }

  if (!this->Point3Representation)
  {
    this->Point3Representation = this->HandleRepresentation->NewInstance();
    this->Point3Representation->ShallowCopy(this->HandleRepresentation);
  }

  if (!this->Point4Representation)
  {
    this->Point4Representation = this->HandleRepresentation->NewInstance();
    this->Point4Representation->ShallowCopy(this->HandleRepresentation);
  }
}

//----------------------------------------------------------------------
double svtkBiDimensionalRepresentation::GetLength1()
{
  double x1[3], x2[3];

  this->GetPoint1WorldPosition(x1);
  this->GetPoint2WorldPosition(x2);

  return sqrt(svtkMath::Distance2BetweenPoints(x1, x2));
}

//----------------------------------------------------------------------
double svtkBiDimensionalRepresentation::GetLength2()
{
  double x3[3], x4[3];

  this->GetPoint3WorldPosition(x3);
  this->GetPoint4WorldPosition(x4);

  return sqrt(svtkMath::Distance2BetweenPoints(x3, x4));
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::SetID(svtkIdType id)
{
  if (id == this->ID)
  {
    return;
  }

  this->ID = id;
  this->IDInitialized = 1;
  this->Modified();
}

//----------------------------------------------------------------------
void svtkBiDimensionalRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "Length1: " << this->GetLength1() << "\n";
  os << indent << "Length2: " << this->GetLength2() << "\n";

  os << indent << "Line1 Visibility: " << (this->Line1Visibility ? "On\n" : "Off\n");
  os << indent << "Line2 Visibility: " << (this->Line2Visibility ? "On\n" : "Off\n");

  os << indent << "Handle Representation: " << this->HandleRepresentation << "\n";

  os << indent << "ID: " << this->ID << "\n";

  double labelPosition[3] = { 0.0, 0.0, 0.0 };
  this->GetLabelPosition(labelPosition);
  os << indent << "Label Position: (" << labelPosition[0] << ", " << labelPosition[1] << ","
     << labelPosition[2] << ")\n";

  if (this->Renderer)
  {
    double worldLabelPosition[3] = { 0.0, 0.0, 0.0 };
    this->GetWorldLabelPosition(worldLabelPosition);
    os << indent << "World Label Position: (" << worldLabelPosition[0] << ", "
       << worldLabelPosition[1] << "," << worldLabelPosition[2] << ")\n";
  }

  os << indent << "Label Text: " << this->GetLabelText() << "\n";
  os << indent << "Label Format: ";
  if (this->LabelFormat)
  {
    os << this->LabelFormat << "\n";
  }
  else
  {
    os << "(null))\n";
  }

  os << indent << "Point1 Representation\n";
  this->Point1Representation->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point2 Representation\n";
  this->Point2Representation->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point3 Representation\n";
  this->Point3Representation->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point4 Representation\n";
  this->Point4Representation->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Show Label Above Widget: " << (this->ShowLabelAboveWidget ? "On\n" : "Off\n");
}
