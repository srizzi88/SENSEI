/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxesTransformRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAxesTransformRepresentation.h"
#include "svtkActor.h"
#include "svtkBox.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCoordinate.h"
#include "svtkCylinderSource.h"
#include "svtkDoubleArray.h"
#include "svtkFollower.h"
#include "svtkGlyph3D.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkVectorText.h"
#include "svtkWindow.h"

svtkStandardNewMacro(svtkAxesTransformRepresentation);

//----------------------------------------------------------------------
svtkAxesTransformRepresentation::svtkAxesTransformRepresentation()
{
  // By default, use one of these handles
  this->OriginRepresentation = svtkPointHandleRepresentation3D::New();
  this->SelectionRepresentation = svtkPointHandleRepresentation3D::New();

  // The line
  this->LinePoints = svtkPoints::New();
  this->LinePoints->SetDataTypeToDouble();
  this->LinePoints->SetNumberOfPoints(2);
  this->LinePolyData = svtkPolyData::New();
  this->LinePolyData->SetPoints(this->LinePoints);
  svtkSmartPointer<svtkCellArray> line = svtkSmartPointer<svtkCellArray>::New();
  line->InsertNextCell(2);
  line->InsertCellPoint(0);
  line->InsertCellPoint(1);
  this->LinePolyData->SetLines(line);
  this->LineMapper = svtkPolyDataMapper::New();
  this->LineMapper->SetInputData(this->LinePolyData);
  this->LineActor = svtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  // The label
  this->LabelText = svtkVectorText::New();
  this->LabelMapper = svtkPolyDataMapper::New();
  this->LabelMapper->SetInputConnection(this->LabelText->GetOutputPort());
  this->LabelActor = svtkFollower::New();
  this->LabelActor->SetMapper(this->LabelMapper);

  // The tick marks
  this->GlyphPoints = svtkPoints::New();
  this->GlyphPoints->SetDataTypeToDouble();
  this->GlyphVectors = svtkDoubleArray::New();
  this->GlyphVectors->SetNumberOfComponents(3);
  this->GlyphPolyData = svtkPolyData::New();
  this->GlyphPolyData->SetPoints(this->GlyphPoints);
  this->GlyphPolyData->GetPointData()->SetVectors(this->GlyphVectors);
  this->GlyphCylinder = svtkCylinderSource::New();
  this->GlyphCylinder->SetRadius(0.5);
  this->GlyphCylinder->SetHeight(0.1);
  this->GlyphCylinder->SetResolution(12);
  svtkSmartPointer<svtkTransform> xform = svtkSmartPointer<svtkTransform>::New();
  this->GlyphXForm = svtkTransformPolyDataFilter::New();
  this->GlyphXForm->SetInputConnection(this->GlyphCylinder->GetOutputPort());
  this->GlyphXForm->SetTransform(xform);
  xform->RotateZ(90);
  this->Glyph3D = svtkGlyph3D::New();
  this->Glyph3D->SetInputData(this->GlyphPolyData);
  this->Glyph3D->SetSourceConnection(this->GlyphXForm->GetOutputPort());
  this->Glyph3D->SetScaleModeToDataScalingOff();
  this->GlyphMapper = svtkPolyDataMapper::New();
  this->GlyphMapper->SetInputConnection(this->Glyph3D->GetOutputPort());
  this->GlyphActor = svtkActor::New();
  this->GlyphActor->SetMapper(this->GlyphMapper);

  // The bounding box
  this->BoundingBox = svtkBox::New();

  this->LabelFormat = nullptr;

  this->Tolerance = 1;

  this->InteractionState = Outside;
}

//----------------------------------------------------------------------
svtkAxesTransformRepresentation::~svtkAxesTransformRepresentation()
{
  this->OriginRepresentation->Delete();
  this->SelectionRepresentation->Delete();

  this->LinePoints->Delete();
  this->LinePolyData->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();

  this->LabelText->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();

  delete[] this->LabelFormat;
  this->LabelFormat = nullptr;

  this->GlyphPoints->Delete();
  this->GlyphVectors->Delete();
  this->GlyphPolyData->Delete();
  this->GlyphCylinder->Delete();
  this->GlyphXForm->Delete();
  this->Glyph3D->Delete();
  this->GlyphMapper->Delete();
  this->GlyphActor->Delete();

  this->BoundingBox->Delete();
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::GetOriginWorldPosition(double pos[3])
{
  this->OriginRepresentation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
double* svtkAxesTransformRepresentation::GetOriginWorldPosition()
{
  if (!this->OriginRepresentation)
  {
    static double temp[3] = { 0, 0, 0 };
    return temp;
  }
  return this->OriginRepresentation->GetWorldPosition();
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::SetOriginDisplayPosition(double x[3])
{
  this->OriginRepresentation->SetDisplayPosition(x);
  double p[3];
  this->OriginRepresentation->GetWorldPosition(p);
  this->OriginRepresentation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::SetOriginWorldPosition(double x[3])
{
  if (this->OriginRepresentation)
  {
    this->OriginRepresentation->SetWorldPosition(x);
  }
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::GetOriginDisplayPosition(double pos[3])
{
  this->OriginRepresentation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
double* svtkAxesTransformRepresentation::GetBounds()
{
  this->BuildRepresentation();

  this->BoundingBox->SetBounds(this->OriginRepresentation->GetBounds());
  this->BoundingBox->AddBounds(this->SelectionRepresentation->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor->GetBounds());

  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  // Get the coordinates of the three handles
  //   this->OriginRepresentation->GetWorldPosition(this->StartP1);
  //   this->SelectionRepresentation->GetWorldPosition(this->StartP2);
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::WidgetInteraction(double e[2])
{

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
int svtkAxesTransformRepresentation::ComputeInteractionState(int X, int Y, int svtkNotUsed(modify))
{
  // Check if we are on the origin. Use the handle to determine this.
  int p1State = this->OriginRepresentation->ComputeInteractionState(X, Y, 0);

  if (p1State == svtkHandleRepresentation::Nearby)
  {
    this->InteractionState = svtkAxesTransformRepresentation::OnOrigin;
  }
  else
  {
    this->InteractionState = svtkAxesTransformRepresentation::Outside;
  }

  // Okay if we're near a handle return, otherwise test the line
  if (this->InteractionState != svtkAxesTransformRepresentation::Outside)
  {
    return this->InteractionState;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    this->OriginRepresentation->GetMTime() > this->BuildTime ||
    this->SelectionRepresentation->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  this->LineActor->ReleaseGraphicsResources(w);
  this->LabelActor->ReleaseGraphicsResources(w);
  this->GlyphActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int svtkAxesTransformRepresentation::RenderOpaqueGeometry(svtkViewport* v)
{
  this->BuildRepresentation();

  this->LineActor->RenderOpaqueGeometry(v);
  this->LabelActor->RenderOpaqueGeometry(v);
  this->GlyphActor->RenderOpaqueGeometry(v);

  return 3;
}

//----------------------------------------------------------------------
int svtkAxesTransformRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* v)
{
  this->BuildRepresentation();

  this->LineActor->RenderTranslucentPolygonalGeometry(v);
  this->LabelActor->RenderTranslucentPolygonalGeometry(v);
  this->GlyphActor->RenderTranslucentPolygonalGeometry(v);

  return 3;
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::SetLabelScale(double scale[3])
{
  this->LabelActor->SetScale(scale);
}

//----------------------------------------------------------------------
double* svtkAxesTransformRepresentation::GetLabelScale()
{
  return this->LabelActor->GetScale();
}

//----------------------------------------------------------------------------
svtkProperty* svtkAxesTransformRepresentation::GetLabelProperty()
{
  return this->LabelActor->GetProperty();
}

//----------------------------------------------------------------------
void svtkAxesTransformRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "Label Format: ";
  if (this->LabelFormat)
  {
    os << this->LabelFormat << endl;
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "InteractionState: " << this->InteractionState << endl;

  os << indent << "Origin Representation: ";
  if (this->OriginRepresentation)
  {
    this->OriginRepresentation->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Selection Representation: " << endl;
  if (this->SelectionRepresentation)
  {
    this->SelectionRepresentation->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  this->Superclass::PrintSelf(os, indent);
}
