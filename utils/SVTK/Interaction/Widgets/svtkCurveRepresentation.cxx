/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCurveRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCurveRepresentation.h"

#include "svtkActor.h"
#include "svtkAssemblyPath.h"
#include "svtkBoundingBox.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellPicker.h"
#include "svtkConeSource.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPlaneSource.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTransform.h"

#include <algorithm>
#include <assert.h>
#include <iterator>

//----------------------------------------------------------------------------
svtkCurveRepresentation::svtkCurveRepresentation()
{
  this->LastEventPosition[0] = SVTK_DOUBLE_MAX;
  this->LastEventPosition[1] = SVTK_DOUBLE_MAX;
  this->LastEventPosition[2] = SVTK_DOUBLE_MAX;

  this->Bounds[0] = SVTK_DOUBLE_MAX;
  this->Bounds[1] = -SVTK_DOUBLE_MAX;
  this->Bounds[2] = SVTK_DOUBLE_MAX;
  this->Bounds[3] = -SVTK_DOUBLE_MAX;
  this->Bounds[4] = SVTK_DOUBLE_MAX;
  this->Bounds[5] = -SVTK_DOUBLE_MAX;

  this->HandleSize = 5.0;

  this->InteractionState = svtkCurveRepresentation::Outside;
  this->ProjectToPlane = 0;   // default off
  this->ProjectionNormal = 0; // default YZ not used
  this->ProjectionPosition = 0.0;
  this->PlaneSource = nullptr;
  this->Closed = 0;

  // Build the representation of the widget

  this->DirectionalLine = false;

  // Create the handles along a straight line within the bounds of a unit cube
  this->NumberOfHandles = 5;
  this->Handle = new svtkActor*[this->NumberOfHandles];
  this->HandleGeometry = new HandleSource*[this->NumberOfHandles];

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i] = HandleSource::New();
    svtkPolyDataMapper* handleMapper = svtkPolyDataMapper::New();
    handleMapper->SetInputConnection(this->HandleGeometry[i]->GetOutputPort());
    this->Handle[i] = svtkActor::New();
    this->Handle[i]->SetMapper(handleMapper);
    handleMapper->Delete();
  }

  this->LineActor = svtkActor::New();

  // Default bounds to get started
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };

  // Initial creation of the widget, serves to initialize it
  this->PlaceFactor = 1.0;
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = svtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.005);

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandlePicker->AddPickList(this->Handle[i]);
  }
  this->HandlePicker->PickFromListOn();

  this->LinePicker = svtkCellPicker::New();
  this->LinePicker->SetTolerance(0.01);
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->PickFromListOn();

  this->LastPickPosition[0] = SVTK_DOUBLE_MAX;
  this->LastPickPosition[1] = SVTK_DOUBLE_MAX;
  this->LastPickPosition[2] = SVTK_DOUBLE_MAX;

  this->CurrentHandle = nullptr;
  this->CurrentHandleIndex = -1;
  this->FirstSelected = true;

  this->Transform = svtkTransform::New();

  // Set up the initial properties
  this->HandleProperty = nullptr;
  this->SelectedHandleProperty = nullptr;
  this->LineProperty = nullptr;
  this->SelectedLineProperty = nullptr;
  this->CreateDefaultProperties();

  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;

  this->TranslationAxis = Axis::NONE;
}

//----------------------------------------------------------------------------
svtkCurveRepresentation::~svtkCurveRepresentation()
{
  this->LineActor->Delete();

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->Delete();
    this->Handle[i]->Delete();
  }
  delete[] this->Handle;
  delete[] this->HandleGeometry;

  this->HandlePicker->Delete();
  this->LinePicker->Delete();

  if (this->HandleProperty)
  {
    this->HandleProperty->Delete();
  }
  if (this->SelectedHandleProperty)
  {
    this->SelectedHandleProperty->Delete();
  }
  if (this->LineProperty)
  {
    this->LineProperty->Delete();
  }
  if (this->SelectedLineProperty)
  {
    this->SelectedLineProperty->Delete();
  }

  this->Transform->Delete();
}

//----------------------------------------------------------------------
void svtkCurveRepresentation::SetDirectionalLine(bool val)
{
  if (this->DirectionalLine == val)
  {
    return;
  }

  this->DirectionalLine = val;
  this->Modified();

  if (this->NumberOfHandles < 2)
  {
    return;
  }

  if (this->DirectionalLine)
  {
    this->HandleGeometry[this->NumberOfHandles - 1]->SetUseSphere(false);
  }
  else
  {
    this->HandleGeometry[this->NumberOfHandles - 1]->SetUseSphere(true);
  }
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::SetClosed(svtkTypeBool closed)
{
  if (this->Closed == closed)
  {
    return;
  }
  this->Closed = closed;

  this->BuildRepresentation();
}

//----------------------------------------------------------------------
void svtkCurveRepresentation::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->HandlePicker, this);
  pm->AddPicker(this->LinePicker, this);
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::SetHandlePosition(int handle, double x, double y, double z)
{
  if (handle < 0 || handle >= this->NumberOfHandles)
  {
    svtkErrorMacro(<< "svtkCurveRepresentation: handle index out of range.");
    return;
  }
  this->HandleGeometry[handle]->SetCenter(x, y, z);
  this->HandleGeometry[handle]->Update();
  if (this->ProjectToPlane)
  {
    this->ProjectPointsToPlane();
  }
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::SetHandlePosition(int handle, double xyz[3])
{
  this->SetHandlePosition(handle, xyz[0], xyz[1], xyz[2]);
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::GetHandlePosition(int handle, double xyz[3])
{
  if (handle < 0 || handle >= this->NumberOfHandles)
  {
    svtkErrorMacro(<< "svtkCurveRepresentation: handle index out of range.");
    return;
  }
  this->HandleGeometry[handle]->GetCenter(xyz);
}

//----------------------------------------------------------------------------
double* svtkCurveRepresentation::GetHandlePosition(int handle)
{
  if (handle < 0 || handle >= this->NumberOfHandles)
  {
    svtkErrorMacro(<< "svtkCurveRepresentation: handle index out of range.");
    return nullptr;
  }

  return this->HandleGeometry[handle]->GetCenter();
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::ProjectPointsToPlane()
{
  if (this->ProjectionNormal == SVTK_PROJECTION_OBLIQUE)
  {
    if (this->PlaneSource != nullptr)
    {
      this->ProjectPointsToObliquePlane();
    }
    else
    {
      svtkGenericWarningMacro(<< "Set the plane source for oblique projections...");
    }
  }
  else
  {
    this->ProjectPointsToOrthoPlane();
  }
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::ProjectPointsToObliquePlane()
{
  double o[3];
  double u[3];
  double v[3];

  this->PlaneSource->GetPoint1(u);
  this->PlaneSource->GetPoint2(v);
  this->PlaneSource->GetOrigin(o);

  int i;
  for (i = 0; i < 3; ++i)
  {
    u[i] = u[i] - o[i];
    v[i] = v[i] - o[i];
  }
  svtkMath::Normalize(u);
  svtkMath::Normalize(v);

  double o_dot_u = svtkMath::Dot(o, u);
  double o_dot_v = svtkMath::Dot(o, v);
  double fac1;
  double fac2;
  double ctr[3];
  for (i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    fac1 = svtkMath::Dot(ctr, u) - o_dot_u;
    fac2 = svtkMath::Dot(ctr, v) - o_dot_v;
    ctr[0] = o[0] + fac1 * u[0] + fac2 * v[0];
    ctr[1] = o[1] + fac1 * u[1] + fac2 * v[1];
    ctr[2] = o[2] + fac1 * u[2] + fac2 * v[2];
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::ProjectPointsToOrthoPlane()
{
  double ctr[3];
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    ctr[this->ProjectionNormal] = this->ProjectionPosition;
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
int svtkCurveRepresentation::GetHandleIndex(svtkProp* prop)
{
  auto iter =
    std::find(this->Handle, this->Handle + this->NumberOfHandles, static_cast<svtkActor*>(prop));
  return (iter != this->Handle + NumberOfHandles)
    ? static_cast<int>(std::distance(this->Handle, iter))
    : -1;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::SetCurrentHandleIndex(int index)
{
  if (index < -1 || index >= this->NumberOfHandles)
  {
    index = -1;
  }

  if (index != this->CurrentHandleIndex)
  {
    this->CurrentHandleIndex = index;
    this->HighlightHandle(index == -1 ? nullptr : this->Handle[index]);
  }
}

//----------------------------------------------------------------------------
int svtkCurveRepresentation::HighlightHandle(svtkProp* prop)
{
  // First unhighlight anything picked
  if (this->CurrentHandle)
  {
    this->CurrentHandle->SetProperty(this->HandleProperty);
  }

  this->CurrentHandle = static_cast<svtkActor*>(prop);

  if (this->CurrentHandle)
  {
    this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
    return this->GetHandleIndex(prop);
  }
  return -1;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::HighlightLine(int highlight)
{
  if (highlight)
  {
    this->LineActor->SetProperty(this->SelectedLineProperty);
  }
  else
  {
    this->LineActor->SetProperty(this->LineProperty);
  }
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::MovePoint(double* p1, double* p2)
{
  if (this->CurrentHandleIndex < 0 || this->CurrentHandleIndex >= this->NumberOfHandles)
  {
    svtkGenericWarningMacro(<< "Poly line handle index out of range.");
    return;
  }

  // Get the motion vector
  double v[3] = { 0, 0, 0 };
  // Move the center of the handle along the motion vector
  if (this->TranslationAxis == Axis::NONE)
  {
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
  }
  // Translation restriction handling
  else
  {
    v[this->TranslationAxis] = p2[this->TranslationAxis] - p1[this->TranslationAxis];
  }

  double* ctr = this->HandleGeometry[this->CurrentHandleIndex]->GetCenter();

  double newCtr[3];
  newCtr[0] = ctr[0] + v[0];
  newCtr[1] = ctr[1] + v[1];
  newCtr[2] = ctr[2] + v[2];
  this->HandleGeometry[this->CurrentHandleIndex]->SetCenter(newCtr);
  this->HandleGeometry[this->CurrentHandleIndex]->Update();
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::Translate(double* p1, double* p2)
{
  // Get the motion vector
  double v[3] = { 0, 0, 0 };
  // Move the center of the handle along the motion vector
  if (this->TranslationAxis == Axis::NONE)
  {
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
  }
  // Translation restriction handling
  else
  {
    // this->TranslationAxis in [0,2]
    assert(this->TranslationAxis > -1 && this->TranslationAxis < 3 &&
      "this->TranslationAxis shoud be in [0,2]");
    v[this->TranslationAxis] = p2[this->TranslationAxis] - p1[this->TranslationAxis];
  }

  double newCtr[3];
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    double* ctr = this->HandleGeometry[i]->GetCenter();
    for (int j = 0; j < 3; ++j)
    {
      newCtr[j] = ctr[j] + v[j];
    }
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::Scale(double* p1, double* p2, int svtkNotUsed(X), int Y)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double center[3] = { 0.0, 0.0, 0.0 };
  double avgdist = 0.0;
  double* prevctr = this->HandleGeometry[0]->GetCenter();
  double* ctr;

  center[0] += prevctr[0];
  center[1] += prevctr[1];
  center[2] += prevctr[2];

  int i;
  for (i = 1; i < this->NumberOfHandles; ++i)
  {
    ctr = this->HandleGeometry[i]->GetCenter();
    center[0] += ctr[0];
    center[1] += ctr[1];
    center[2] += ctr[2];
    avgdist += sqrt(svtkMath::Distance2BetweenPoints(ctr, prevctr));
    prevctr = ctr;
  }

  avgdist /= this->NumberOfHandles;

  center[0] /= this->NumberOfHandles;
  center[1] /= this->NumberOfHandles;
  center[2] /= this->NumberOfHandles;

  // Compute the scale factor
  double sf = svtkMath::Norm(v) / avgdist;
  if (Y > this->LastEventPosition[1])
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  // Move the handle points
  double newCtr[3];
  for (i = 0; i < this->NumberOfHandles; ++i)
  {
    ctr = this->HandleGeometry[i]->GetCenter();
    for (int j = 0; j < 3; ++j)
    {
      newCtr[j] = sf * (ctr[j] - center[j]) + center[j];
    }
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::Spin(double* p1, double* p2, double* vpn)
{
  // Mouse motion vector in world space
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Axis of rotation
  double axis[3] = { 0.0, 0.0, 0.0 };

  if (this->ProjectToPlane)
  {
    if (this->ProjectionNormal == SVTK_PROJECTION_OBLIQUE)
    {
      if (this->PlaneSource != nullptr)
      {
        double* normal = this->PlaneSource->GetNormal();
        axis[0] = normal[0];
        axis[1] = normal[1];
        axis[2] = normal[2];
        svtkMath::Normalize(axis);
      }
      else
      {
        axis[0] = 1.;
      }
    }
    else
    {
      axis[this->ProjectionNormal] = 1.;
    }
  }
  else
  {
    // Create axis of rotation and angle of rotation
    svtkMath::Cross(vpn, v, axis);
    if (svtkMath::Normalize(axis) == 0.0)
    {
      return;
    }
  }

  // Radius vector (from mean center to cursor position)
  double rv[3] = { p2[0] - this->Centroid[0], p2[1] - this->Centroid[1],
    p2[2] - this->Centroid[2] };

  // Distance between center and cursor location
  double rs = svtkMath::Normalize(rv);

  // Spin direction
  double ax_cross_rv[3];
  svtkMath::Cross(axis, rv, ax_cross_rv);

  // Spin angle
  double theta = 360.0 * svtkMath::Dot(v, ax_cross_rv) / rs;

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(this->Centroid[0], this->Centroid[1], this->Centroid[2]);
  this->Transform->RotateWXYZ(theta, axis);
  this->Transform->Translate(-this->Centroid[0], -this->Centroid[1], -this->Centroid[2]);

  // Set the handle points
  double newCtr[3];
  double ctr[3];
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Transform->TransformPoint(ctr, newCtr);
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::CreateDefaultProperties()
{
  this->HandleProperty = svtkProperty::New();
  this->HandleProperty->SetColor(1, 1, 1);

  this->SelectedHandleProperty = svtkProperty::New();
  this->SelectedHandleProperty->SetColor(1, 0, 0);

  this->LineProperty = svtkProperty::New();
  this->LineProperty->SetRepresentationToWireframe();
  this->LineProperty->SetAmbient(1.0);
  this->LineProperty->SetColor(1.0, 1.0, 0.0);
  this->LineProperty->SetLineWidth(2.0);

  this->SelectedLineProperty = svtkProperty::New();
  this->SelectedLineProperty->SetRepresentationToWireframe();
  this->SelectedLineProperty->SetAmbient(1.0);
  this->SelectedLineProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedLineProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::SetProjectionPosition(double position)
{
  this->ProjectionPosition = position;
  if (this->ProjectToPlane)
  {
    this->ProjectPointsToPlane();
  }
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::SetPlaneSource(svtkPlaneSource* plane)
{
  if (this->PlaneSource == plane)
  {
    return;
  }
  this->PlaneSource = plane;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::Initialize()
{
  int i;
  for (i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandlePicker->DeletePickList(this->Handle[i]);
    this->HandleGeometry[i]->Delete();
    this->Handle[i]->Delete();
  }

  this->NumberOfHandles = 0;

  delete[] this->Handle;
  delete[] this->HandleGeometry;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::SizeHandles()
{
  if (this->NumberOfHandles > 0)
  {
    double radius = this->SizeHandlesInPixels(1.5, this->HandleGeometry[0]->GetCenter());
    for (int i = 0; i < this->NumberOfHandles; ++i)
    {
      this->HandleGeometry[i]->SetRadius(radius);
    }
  }
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::CalculateCentroid()
{
  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;

  double ctr[3];
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Centroid[0] += ctr[0];
    this->Centroid[1] += ctr[1];
    this->Centroid[2] += ctr[2];
  }

  this->Centroid[0] /= this->NumberOfHandles;
  this->Centroid[1] /= this->NumberOfHandles;
  this->Centroid[2] /= this->NumberOfHandles;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::EraseHandle(const int& index)
{
  if (this->NumberOfHandles < 3 || index < 0 || index >= this->NumberOfHandles)
  {
    return;
  }

  svtkPoints* newpoints = svtkPoints::New(SVTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles - 1);
  int count = 0;
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    if (i != index)
    {
      newpoints->SetPoint(count++, this->HandleGeometry[i]->GetCenter());
    }
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

//----------------------------------------------------------------------------
svtkTypeBool svtkCurveRepresentation::IsClosed()
{
  if (this->NumberOfHandles < 3 || !this->Closed)
  {
    return 0;
  }

  svtkPolyData* lineData = svtkPolyData::New();
  this->GetPolyData(lineData);
  if (!lineData || !(lineData->GetPoints()))
  {
    svtkErrorMacro(<< "No line data to query geometric closure");
    return 0;
  }

  svtkPoints* points = lineData->GetPoints();
  int numPoints = points->GetNumberOfPoints();

  if (numPoints < 3)
  {
    return 0;
  }

  int numEntries =
    lineData->GetLines()->GetNumberOfConnectivityIds() + lineData->GetLines()->GetNumberOfCells();

  double p0[3];
  double p1[3];

  points->GetPoint(0, p0);
  points->GetPoint(numPoints - 1, p1);
  int minusNth = (p0[0] == p1[0] && p0[1] == p1[1] && p0[2] == p1[2]) ? 1 : 0;
  int result;
  if (minusNth) // definitely closed
  {
    result = 1;
  }
  else // not physically closed, check connectivity
  {
    result = ((numEntries - numPoints) == 2) ? 1 : 0;
  }

  return result;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::ReleaseGraphicsResources(svtkWindow* win)
{
  this->LineActor->ReleaseGraphicsResources(win);
  for (int cc = 0; cc < this->NumberOfHandles; cc++)
  {
    this->Handle[cc]->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
int svtkCurveRepresentation::RenderOpaqueGeometry(svtkViewport* win)
{
  this->BuildRepresentation();

  int count = 0;
  count += this->LineActor->RenderOpaqueGeometry(win);
  for (int cc = 0; cc < this->NumberOfHandles; cc++)
  {
    count += this->Handle[cc]->RenderOpaqueGeometry(win);
  }
  return count;
}

//----------------------------------------------------------------------------
int svtkCurveRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* win)
{
  int count = 0;
  count += this->LineActor->RenderTranslucentPolygonalGeometry(win);
  for (int cc = 0; cc < this->NumberOfHandles; cc++)
  {
    count += this->Handle[cc]->RenderTranslucentPolygonalGeometry(win);
  }
  return count;
}

//----------------------------------------------------------------------------
int svtkCurveRepresentation::RenderOverlay(svtkViewport* win)
{
  int count = 0;
  count += this->LineActor->RenderOverlay(win);
  for (int cc = 0; cc < this->NumberOfHandles; cc++)
  {
    count += this->Handle[cc]->RenderOverlay(win);
  }
  return count;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkCurveRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();
  int count = 0;
  count |= this->LineActor->HasTranslucentPolygonalGeometry();
  for (int cc = 0; cc < this->NumberOfHandles; cc++)
  {
    count |= this->Handle[cc]->HasTranslucentPolygonalGeometry();
  }
  return count;
}

//----------------------------------------------------------------------------
int svtkCurveRepresentation::ComputeInteractionState(int X, int Y, int svtkNotUsed(modify))
{
  this->InteractionState = svtkCurveRepresentation::Outside;
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    return this->InteractionState;
  }

  // Try and pick a handle first. This allows the picking of the handle even
  // if it is "behind" the poly line.
  int handlePicked = 0;

  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  // always get pick position
  this->HandlePicker->GetPickPosition(this->LastPickPosition);

  if (path != nullptr)
  {
    this->ValidPick = 1;
    this->InteractionState = svtkCurveRepresentation::OnHandle;
    this->SetCurrentHandleIndex(this->GetHandleIndex(path->GetFirstNode()->GetViewProp()));
    handlePicked = 1;
    this->FirstSelected = (this->CurrentHandleIndex == 0);
  }
  else
  {
    this->SetCurrentHandleIndex(-1);
  }

  if (!handlePicked)
  {
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

    if (path != nullptr)
    {
      this->ValidPick = 1;
      this->LinePicker->GetPickPosition(this->LastPickPosition);
      this->HighlightLine(1);
      this->InteractionState = svtkCurveRepresentation::OnLine;
    }
    else
    {
      this->HighlightLine(0);
    }
  }
  else
  {
    this->HighlightLine(0);
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  this->ComputeInteractionState(static_cast<int>(e[0]), static_cast<int>(e[1]), 0);
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::WidgetInteraction(double e[2])
{
  // Convert events to appropriate coordinate systems
  svtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  // Compute the two points defining the motion vector
  svtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, this->LastPickPosition[0],
    this->LastPickPosition[1], this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  svtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
  svtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == svtkCurveRepresentation::Moving)
  {
    if (this->CurrentHandleIndex != -1)
    {
      this->MovePoint(prevPickPoint, pickPoint);
    }
    else
    {
      this->Translate(prevPickPoint, pickPoint);
    }
  }
  else if (this->InteractionState == svtkCurveRepresentation::Scaling)
  {
    this->Scale(prevPickPoint, pickPoint, static_cast<int>(e[0]), static_cast<int>(e[1]));
  }
  else if (this->InteractionState == svtkCurveRepresentation::Spinning)
  {
    camera->GetViewPlaneNormal(vpn);
    this->Spin(prevPickPoint, pickPoint, vpn);
  }

  if (this->ProjectToPlane)
  {
    this->ProjectPointsToPlane();
  }

  this->BuildRepresentation();

  // Store the position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::PushHandle(double* pos)
{
  svtkPoints* newpoints = svtkPoints::New(SVTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles + 1);

  if (this->FirstSelected)
  {
    // pushing front
    newpoints->SetPoint(0, pos);
    for (int i = 0; i < this->NumberOfHandles; ++i)
    {
      newpoints->SetPoint(i + 1, this->HandleGeometry[i]->GetCenter());
    }
  }
  else
  {
    // pushing back
    newpoints->SetPoint(this->NumberOfHandles, pos);
    for (int i = 0; i < this->NumberOfHandles; ++i)
    {
      newpoints->SetPoint(i, this->HandleGeometry[i]->GetCenter());
    }
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::EndWidgetInteraction(double[2])
{
  switch (this->InteractionState)
  {
    case svtkCurveRepresentation::Pushing:
      this->PushHandle(this->LastPickPosition);
      break;

    case svtkCurveRepresentation::Inserting:
      this->SetCurrentHandleIndex(this->InsertHandleOnLine(this->LastPickPosition));
      break;

    case svtkCurveRepresentation::Erasing:
      if (this->CurrentHandleIndex)
      {
        int index = this->CurrentHandleIndex;
        this->SetCurrentHandleIndex(-1);
        this->EraseHandle(index);
      }
  }

  this->HighlightLine(0);
  this->InteractionState = svtkCurveRepresentation::Outside;
}

//----------------------------------------------------------------------------
double* svtkCurveRepresentation::GetBounds()
{
  this->BuildRepresentation();

  svtkBoundingBox bbox;
  bbox.AddBounds(this->LineActor->GetBounds());
  for (int cc = 0; cc < this->NumberOfHandles; cc++)
  {
    bbox.AddBounds(this->HandleGeometry[cc]->GetOutput()->GetBounds());
  }
  bbox.GetBounds(this->Bounds);
  return this->Bounds;
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::SetLineColor(double r, double g, double b)
{
  this->GetLineProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void svtkCurveRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->HandleProperty)
  {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
  }
  else
  {
    os << indent << "Handle Property: (none)\n";
  }
  if (this->SelectedHandleProperty)
  {
    os << indent << "Selected Handle Property: " << this->SelectedHandleProperty << "\n";
  }
  else
  {
    os << indent << "Selected Handle Property: (none)\n";
  }
  if (this->LineProperty)
  {
    os << indent << "Line Property: " << this->LineProperty << "\n";
  }
  else
  {
    os << indent << "Line Property: (none)\n";
  }
  if (this->SelectedLineProperty)
  {
    os << indent << "Selected Line Property: " << this->SelectedLineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Line Property: (none)\n";
  }

  os << indent << "Project To Plane: " << (this->ProjectToPlane ? "On" : "Off") << "\n";
  os << indent << "Projection Normal: " << this->ProjectionNormal << "\n";
  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";
  os << indent << "Number Of Handles: " << this->NumberOfHandles << "\n";
  os << indent << "Closed: " << (this->Closed ? "On" : "Off") << "\n";
  os << indent << "InteractionState: " << this->InteractionState << endl;
}

svtkStandardNewMacro(svtkCurveRepresentation::HandleSource);

//----------------------------------------------------------------------------
svtkCurveRepresentation::HandleSource::HandleSource()
  : UseSphere(true)
  , Radius(0.5)
{
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->Direction[0] = 1.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 0.0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int svtkCurveRepresentation::HandleSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  auto output = svtkPolyData::GetData(outputVector);
  if (this->UseSphere)
  {
    svtkNew<svtkSphereSource> sphere;
    sphere->SetRadius(this->Radius);
    sphere->SetCenter(this->Center);
    sphere->SetThetaResolution(16);
    sphere->SetPhiResolution(8);
    sphere->Update();
    output->ShallowCopy(sphere->GetOutput(0));
  }
  else
  {
    svtkNew<svtkConeSource> cone;
    cone->SetRadius(this->Radius);
    cone->SetCenter(this->Center);
    cone->SetHeight(2.8 * this->Radius);
    cone->SetResolution(16);
    cone->SetDirection(this->Direction);
    cone->Update();
    output->ShallowCopy(cone->GetOutput(0));
  }
  return 1;
}
