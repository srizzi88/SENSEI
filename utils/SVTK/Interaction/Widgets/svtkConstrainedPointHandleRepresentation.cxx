/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConstrainedPointHandleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkConstrainedPointHandleRepresentation.h"
#include "svtkActor.h"
#include "svtkAssemblyPath.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkCleanPolyData.h"
#include "svtkCoordinate.h"
#include "svtkCursor2D.h"
#include "svtkCylinderSource.h"
#include "svtkDoubleArray.h"
#include "svtkGlyph3D.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPlaneCollection.h"
#include "svtkPlanes.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"

svtkStandardNewMacro(svtkConstrainedPointHandleRepresentation);

svtkCxxSetObjectMacro(svtkConstrainedPointHandleRepresentation, ObliquePlane, svtkPlane);
svtkCxxSetObjectMacro(svtkConstrainedPointHandleRepresentation, BoundingPlanes, svtkPlaneCollection);

//----------------------------------------------------------------------
svtkConstrainedPointHandleRepresentation::svtkConstrainedPointHandleRepresentation()
{
  // Initialize state
  this->InteractionState = svtkHandleRepresentation::Outside;

  this->ProjectionPosition = 0;
  this->ObliquePlane = nullptr;
  this->ProjectionNormal = svtkConstrainedPointHandleRepresentation::ZAxis;

  this->CursorShape = nullptr;
  this->ActiveCursorShape = nullptr;

  // Represent the position of the cursor
  this->FocalPoint = svtkPoints::New();
  this->FocalPoint->SetNumberOfPoints(1);
  this->FocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  svtkSmartPointer<svtkDoubleArray> normals = svtkSmartPointer<svtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(1);

  double normal[3];
  this->GetProjectionNormal(normal);
  normals->SetTuple(0, normal);

  this->FocalData = svtkPolyData::New();
  this->FocalData->SetPoints(this->FocalPoint);
  this->FocalData->GetPointData()->SetNormals(normals);

  this->Glypher = svtkGlyph3D::New();
  this->Glypher->SetInputData(this->FocalData);
  this->Glypher->SetVectorModeToUseNormal();
  this->Glypher->OrientOn();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  // The transformation of the cursor will be done via svtkGlyph3D
  // By default a svtkCursor2D will be used to define the cursor shape
  svtkSmartPointer<svtkCursor2D> cursor2D = svtkSmartPointer<svtkCursor2D>::New();
  cursor2D->AllOff();
  cursor2D->PointOn();
  cursor2D->Update();
  this->SetCursorShape(cursor2D->GetOutput());

  svtkSmartPointer<svtkCylinderSource> cylinder = svtkSmartPointer<svtkCylinderSource>::New();
  cylinder->SetResolution(64);
  cylinder->SetRadius(1.0);
  cylinder->SetHeight(0.0);
  cylinder->CappingOff();
  cylinder->SetCenter(0, 0, 0);

  svtkSmartPointer<svtkCleanPolyData> clean = svtkSmartPointer<svtkCleanPolyData>::New();
  clean->PointMergingOn();
  clean->CreateDefaultLocator();
  clean->SetInputConnection(0, cylinder->GetOutputPort(0));

  svtkSmartPointer<svtkTransform> t = svtkSmartPointer<svtkTransform>::New();
  t->RotateZ(90.0);

  svtkSmartPointer<svtkTransformPolyDataFilter> tpd =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  tpd->SetInputConnection(0, clean->GetOutputPort(0));
  tpd->SetTransform(t);
  tpd->Update();

  this->SetActiveCursorShape(tpd->GetOutput());

  this->Mapper = svtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());
  this->Mapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->Mapper->ScalarVisibilityOff();

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = svtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  this->InteractionOffset[0] = 0.0;
  this->InteractionOffset[1] = 0.0;

  this->BoundingPlanes = nullptr;
}

//----------------------------------------------------------------------
svtkConstrainedPointHandleRepresentation::~svtkConstrainedPointHandleRepresentation()
{
  this->FocalPoint->Delete();
  this->FocalData->Delete();

  this->SetCursorShape(nullptr);
  this->SetActiveCursorShape(nullptr);

  this->RemoveAllBoundingPlanes();

  this->Glypher->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();

  this->Property->Delete();
  this->SelectedProperty->Delete();
  this->ActiveProperty->Delete();

  if (this->ObliquePlane)
  {
    this->ObliquePlane->UnRegister(this);
    this->ObliquePlane = nullptr;
  }

  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->UnRegister(this);
  }
}

//----------------------------------------------------------------------
int svtkConstrainedPointHandleRepresentation::CheckConstraint(
  svtkRenderer* renderer, double eventPos[2])
{
  double worldPos[3];
  double tolerance = 0.0;
  return this->GetIntersectionPosition(eventPos, worldPos, tolerance, renderer);
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::SetProjectionPosition(double position)
{
  if (this->ProjectionPosition != position)
  {
    this->ProjectionPosition = position;
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::SetCursorShape(svtkPolyData* shape)
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
      this->Glypher->SetSourceData(this->CursorShape);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
svtkPolyData* svtkConstrainedPointHandleRepresentation::GetCursorShape()
{
  return this->CursorShape;
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::SetActiveCursorShape(svtkPolyData* shape)
{
  if (shape != this->ActiveCursorShape)
  {
    if (this->ActiveCursorShape)
    {
      this->ActiveCursorShape->Delete();
    }
    this->ActiveCursorShape = shape;
    if (this->CursorShape)
    {
      this->ActiveCursorShape->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
svtkPolyData* svtkConstrainedPointHandleRepresentation::GetActiveCursorShape()
{
  return this->ActiveCursorShape;
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::AddBoundingPlane(svtkPlane* plane)
{
  if (this->BoundingPlanes == nullptr)
  {
    this->BoundingPlanes = svtkPlaneCollection::New();
    this->BoundingPlanes->Register(this);
    this->BoundingPlanes->Delete();
  }

  this->BoundingPlanes->AddItem(plane);
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::RemoveBoundingPlane(svtkPlane* plane)
{
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->RemoveItem(plane);
  }
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::RemoveAllBoundingPlanes()
{
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->RemoveAllItems();
    this->BoundingPlanes->Delete();
    this->BoundingPlanes = nullptr;
  }
}
//----------------------------------------------------------------------

void svtkConstrainedPointHandleRepresentation::SetBoundingPlanes(svtkPlanes* planes)
{
  if (!planes)
  {
    return;
  }

  svtkPlane* plane;
  int numPlanes = planes->GetNumberOfPlanes();

  this->RemoveAllBoundingPlanes();
  for (int i = 0; i < numPlanes; i++)
  {
    plane = svtkPlane::New();
    planes->GetPlane(i, plane);
    this->AddBoundingPlane(plane);
    plane->Delete();
  }
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::SetRenderer(svtkRenderer* ren)
{
  this->WorldPosition->SetViewport(ren);
  this->Superclass::SetRenderer(ren);
}

//-------------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::SetPosition(double x, double y, double z)
{
  this->WorldPosition->SetValue(x, y, z);
  this->FocalPoint->SetPoint(0, x, y, z);
  this->FocalPoint->Modified();
}

//-------------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::SetDisplayPosition(double eventPos[3])
{
  double worldPos[3];
  this->DisplayPosition->SetValue(eventPos);
  if (this->Renderer)
  {
    if (this->GetIntersectionPosition(eventPos, worldPos))
    {
      this->SetPosition(worldPos);
    }
  }
  this->DisplayPositionTime.Modified();
}

//-------------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::SetPosition(double xyz[3])
{
  this->SetPosition(xyz[0], xyz[1], xyz[2]);
}

//-------------------------------------------------------------------------
double* svtkConstrainedPointHandleRepresentation::GetPosition()
{
  return this->FocalPoint->GetPoint(0);
}

//-------------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::GetPosition(double xyz[3])
{
  this->FocalPoint->GetPoint(0, xyz);
}

//-------------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::ShallowCopy(svtkProp* prop)
{
  svtkConstrainedPointHandleRepresentation* rep =
    svtkConstrainedPointHandleRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->Property->DeepCopy(rep->GetProperty());
    this->SelectedProperty->DeepCopy(rep->GetSelectedProperty());
    this->ActiveProperty->DeepCopy(rep->GetActiveProperty());
    this->ProjectionNormal = rep->GetProjectionNormal();
    this->ProjectionPosition = rep->GetProjectionPosition();

    this->SetObliquePlane(rep->GetObliquePlane());
    this->SetBoundingPlanes(rep->GetBoundingPlanes());
  }
  this->Superclass::ShallowCopy(prop);
}
//-------------------------------------------------------------------------
int svtkConstrainedPointHandleRepresentation::ComputeInteractionState(
  int X, int Y, int svtkNotUsed(modify))
{

  double pos[4], xyz[3];
  this->FocalPoint->GetPoint(0, pos);
  pos[3] = 1.0;
  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  xyz[2] = pos[2];

  this->VisibilityOn();
  double tol2 = this->Tolerance * this->Tolerance;
  if (svtkMath::Distance2BetweenPoints(xyz, pos) <= tol2)
  {
    this->InteractionState = svtkHandleRepresentation::Nearby;
    this->Glypher->SetSourceData(this->ActiveCursorShape);
    this->Actor->SetProperty(this->ActiveProperty);
    if (!this->ActiveCursorShape)
    {
      this->VisibilityOff();
    }
  }
  else
  {
    this->InteractionState = svtkHandleRepresentation::Outside;
    this->Glypher->SetSourceData(this->CursorShape);
    this->Actor->SetProperty(this->Property);
    if (!this->CursorShape)
    {
      this->VisibilityOff();
    }
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void svtkConstrainedPointHandleRepresentation::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  // How far is this in pixels from the position of this widget?
  // Maintain this during interaction such as translating (don't
  // force center of widget to snap to mouse position)

  // convert position to display coordinates
  double pos[3];
  this->GetDisplayPosition(pos);

  this->InteractionOffset[0] = pos[0] - startEventPos[0];
  this->InteractionOffset[1] = pos[1] - startEventPos[1];
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void svtkConstrainedPointHandleRepresentation::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if (this->InteractionState == svtkHandleRepresentation::Selecting ||
    this->InteractionState == svtkHandleRepresentation::Translating)
  {
    this->Translate(eventPos);
  }

  else if (this->InteractionState == svtkHandleRepresentation::Scaling)
  {
    this->Scale(eventPos);
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
}

//----------------------------------------------------------------------
// Translate everything
void svtkConstrainedPointHandleRepresentation::Translate(const double* eventPos)
{
  double worldPos[3], prevWorldPos[3];

  if (this->GetIntersectionPosition(eventPos, worldPos))
  {
    this->GetWorldPosition(prevWorldPos);
    Superclass::Translate(prevWorldPos, worldPos);
  }
  else
  {
    // I really want to track the closest point here,
    // but I am postponing this at the moment....
  }
}

//----------------------------------------------------------------------
int svtkConstrainedPointHandleRepresentation::GetIntersectionPosition(
  const double eventPos[2], double worldPos[3], double tolerance, svtkRenderer* renderer)
{
  double nearWorldPoint[4];
  double farWorldPoint[4];
  double tmp[3];

  tmp[0] = eventPos[0] + this->InteractionOffset[0];
  tmp[1] = eventPos[1] + this->InteractionOffset[1];
  tmp[2] = 0.0; // near plane
  if (renderer == nullptr)
  {
    renderer = this->Renderer;
  }

  renderer->SetDisplayPoint(tmp);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(nearWorldPoint);

  tmp[2] = 1.0; // far plane
  renderer->SetDisplayPoint(tmp);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(farWorldPoint);

  double normal[3];
  double origin[3];

  this->GetProjectionNormal(normal);
  this->GetProjectionOrigin(origin);

  svtkSmartPointer<svtkCellPicker> picker = svtkSmartPointer<svtkCellPicker>::New();

  picker->Pick(eventPos[0], eventPos[1], 0, renderer);

  svtkAssemblyPath* path = picker->GetPath();

  if (path == nullptr)
  {
    return 0;
  }
  double pickPos[3];
  picker->GetPickPosition(pickPos);
  if (this->BoundingPlanes)
  {
    svtkPlane* p;
    this->BoundingPlanes->InitTraversal();
    while ((p = this->BoundingPlanes->GetNextItem()))
    {
      double v = p->EvaluateFunction(pickPos);
      if (v < tolerance)
      {
        return 0;
      }
    }
  }

  worldPos[0] = pickPos[0];
  worldPos[1] = pickPos[1];
  worldPos[2] = pickPos[2];

  return 1;
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::GetProjectionNormal(double normal[3])
{
  switch (this->ProjectionNormal)
  {
    case svtkConstrainedPointHandleRepresentation::XAxis:
      normal[0] = 1.0;
      normal[1] = 0.0;
      normal[2] = 0.0;
      break;
    case svtkConstrainedPointHandleRepresentation::YAxis:
      normal[0] = 0.0;
      normal[1] = 1.0;
      normal[2] = 0.0;
      break;
    case svtkConstrainedPointHandleRepresentation::ZAxis:
      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = 1.0;
      break;
    case svtkConstrainedPointHandleRepresentation::Oblique:
      this->ObliquePlane->GetNormal(normal);
      break;
  }
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::GetProjectionOrigin(double origin[3])
{
  switch (this->ProjectionNormal)
  {
    case svtkConstrainedPointHandleRepresentation::XAxis:
      origin[0] = this->ProjectionPosition;
      origin[1] = 0.0;
      origin[2] = 0.0;
      break;
    case svtkConstrainedPointHandleRepresentation::YAxis:
      origin[0] = 0.0;
      origin[1] = this->ProjectionPosition;
      origin[2] = 0.0;
      break;
    case svtkConstrainedPointHandleRepresentation::ZAxis:
      origin[0] = 0.0;
      origin[1] = 0.0;
      origin[2] = this->ProjectionPosition;
      break;
    case svtkConstrainedPointHandleRepresentation::Oblique:
      this->ObliquePlane->GetOrigin(origin);
      break;
  }
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::Scale(const double* eventPos)
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
void svtkConstrainedPointHandleRepresentation::Highlight(int highlight)
{
  if (highlight)
  {
    this->Actor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->Actor->SetProperty(this->ActiveProperty);
  }
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::CreateDefaultProperties()
{
  this->Property = svtkProperty::New();
  this->Property->SetColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(0.5);
  this->Property->SetPointSize(3);

  this->SelectedProperty = svtkProperty::New();
  this->SelectedProperty->SetColor(0.0, 1.0, 1.0);
  this->SelectedProperty->SetRepresentationToWireframe();
  this->SelectedProperty->SetAmbient(1.0);
  this->SelectedProperty->SetDiffuse(0.0);
  this->SelectedProperty->SetSpecular(0.0);
  this->SelectedProperty->SetLineWidth(2.0);

  this->ActiveProperty = svtkProperty::New();
  this->ActiveProperty->SetColor(0.0, 1.0, 0.0);
  this->ActiveProperty->SetRepresentationToWireframe();
  this->ActiveProperty->SetAmbient(1.0);
  this->ActiveProperty->SetDiffuse(0.0);
  this->ActiveProperty->SetSpecular(0.0);
  this->ActiveProperty->SetLineWidth(1.0);
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::BuildRepresentation()
{
  double normal[3];
  this->GetProjectionNormal(normal);
  this->FocalData->GetPointData()->GetNormals()->SetTuple(0, normal);

  double* pos = this->WorldPosition->GetValue();
  this->FocalPoint->SetPoint(0, pos[0], pos[1], pos[2]);
  this->FocalPoint->Modified();
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::GetActors(svtkPropCollection* pc)
{
  this->Actor->GetActors(pc);
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int svtkConstrainedPointHandleRepresentation::RenderOverlay(svtkViewport* viewport)
{
  return this->Actor->RenderOverlay(viewport);
}

//----------------------------------------------------------------------
int svtkConstrainedPointHandleRepresentation::RenderOpaqueGeometry(svtkViewport* viewport)
{
  return this->Actor->RenderOpaqueGeometry(viewport);
}

//-----------------------------------------------------------------------------
int svtkConstrainedPointHandleRepresentation::RenderTranslucentPolygonalGeometry(
  svtkViewport* viewport)
{
  return this->Actor->RenderTranslucentPolygonalGeometry(viewport);
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkConstrainedPointHandleRepresentation::HasTranslucentPolygonalGeometry()
{
  return this->Actor->HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------
void svtkConstrainedPointHandleRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Projection Normal: ";
  if (this->ProjectionNormal == svtkConstrainedPointHandleRepresentation::XAxis)
  {
    os << "XAxis\n";
  }
  else if (this->ProjectionNormal == svtkConstrainedPointHandleRepresentation::YAxis)
  {
    os << "YAxis\n";
  }
  else if (this->ProjectionNormal == svtkConstrainedPointHandleRepresentation::ZAxis)
  {
    os << "ZAxis\n";
  }
  else // if ( this->ProjectionNormal == svtkConstrainedPointHandleRepresentation::Oblique )
  {
    os << "Oblique\n";
  }

  os << indent << "Active Property: ";
  this->ActiveProperty->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";

  os << indent << "Property: ";
  this->Property->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Selected Property: ";
  this->SelectedProperty->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Oblique Plane: ";
  if (this->ObliquePlane)
  {
    this->ObliquePlane->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Bounding Planes: ";
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
