/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSplineRepresentation.h"

#include "svtkActor.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkBoundingBox.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellPicker.h"
#include "svtkConeSource.h"
#include "svtkDoubleArray.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkParametricFunctionSource.h"
#include "svtkParametricSpline.h"
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
#include "svtkVector.h"
#include "svtkVectorOperators.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkSplineRepresentation);

//----------------------------------------------------------------------------
svtkSplineRepresentation::svtkSplineRepresentation()
{
  // Build the representation of the widget

  // Create the handles along a straight line within the bounds of a unit cube
  double x0 = -0.5;
  double x1 = 0.5;
  double y0 = -0.5;
  double y1 = 0.5;
  double z0 = -0.5;
  double z1 = 0.5;

  svtkPoints* points = svtkPoints::New(SVTK_DOUBLE);
  points->SetNumberOfPoints(this->NumberOfHandles);

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    double u = i / (this->NumberOfHandles - 1.0);
    double x = (1.0 - u) * x0 + u * x1;
    double y = (1.0 - u) * y0 + u * y1;
    double z = (1.0 - u) * z0 + u * z1;
    points->SetPoint(i, x, y, z);
    this->HandleGeometry[i]->SetCenter(x, y, z);
  }

  // svtkParametric spline acts as the interpolating engine
  this->ParametricSpline = svtkParametricSpline::New();
  this->ParametricSpline->Register(this);
  this->ParametricSpline->SetPoints(points);
  points->Delete();
  this->ParametricSpline->Delete();

  // Define the points and line segments representing the spline
  this->Resolution = 499;

  this->ParametricFunctionSource = svtkParametricFunctionSource::New();
  this->ParametricFunctionSource->SetParametricFunction(this->ParametricSpline);
  this->ParametricFunctionSource->SetScalarModeToNone();
  this->ParametricFunctionSource->GenerateTextureCoordinatesOff();
  this->ParametricFunctionSource->SetUResolution(this->Resolution);
  this->ParametricFunctionSource->Update();

  svtkPolyDataMapper* lineMapper = svtkPolyDataMapper::New();
  lineMapper->SetInputConnection(this->ParametricFunctionSource->GetOutputPort());
  lineMapper->SetResolveCoincidentTopologyToPolygonOffset();

  this->LineActor->SetMapper(lineMapper);
  lineMapper->Delete();
}

//----------------------------------------------------------------------------
svtkSplineRepresentation::~svtkSplineRepresentation()
{
  if (this->ParametricSpline)
  {
    this->ParametricSpline->UnRegister(this);
  }

  this->ParametricFunctionSource->Delete();
}

//----------------------------------------------------------------------------
void svtkSplineRepresentation::SetParametricSpline(svtkParametricSpline* spline)
{
  if (this->ParametricSpline != spline)
  {
    // to avoid destructor recursion
    svtkParametricSpline* temp = this->ParametricSpline;
    this->ParametricSpline = spline;
    if (temp != nullptr)
    {
      temp->UnRegister(this);
    }
    if (this->ParametricSpline != nullptr)
    {
      this->ParametricSpline->Register(this);
      this->ParametricFunctionSource->SetParametricFunction(this->ParametricSpline);
    }
  }
}

//----------------------------------------------------------------------------
svtkDoubleArray* svtkSplineRepresentation::GetHandlePositions()
{
  return svtkArrayDownCast<svtkDoubleArray>(this->ParametricSpline->GetPoints()->GetData());
}

//----------------------------------------------------------------------------
void svtkSplineRepresentation::BuildRepresentation()
{
  this->ValidPick = 1;
  // TODO: Avoid unnecessary rebuilds.
  // Handles have changed position, re-compute the spline coeffs
  svtkPoints* points = this->ParametricSpline->GetPoints();
  if (points->GetNumberOfPoints() != this->NumberOfHandles)
  {
    points->SetNumberOfPoints(this->NumberOfHandles);
  }

  svtkBoundingBox bbox;
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    double pt[3];
    this->HandleGeometry[i]->GetCenter(pt);
    points->SetPoint(i, pt);
    bbox.AddPoint(pt);
  }
  this->ParametricSpline->SetClosed(this->Closed);
  this->ParametricSpline->Modified();

  // Update end arrow direction
  if (this->DirectionalLine && this->NumberOfHandles >= 2)
  {
    svtkVector3d pt1, pt2;
    svtkIdType npts = this->ParametricFunctionSource->GetOutput()->GetNumberOfPoints();
    this->ParametricFunctionSource->GetOutput()->GetPoint(npts - 1, pt1.GetData());
    this->ParametricFunctionSource->GetOutput()->GetPoint(npts - 2, pt2.GetData());
    pt1 = pt1 - pt2;
    this->HandleGeometry[this->NumberOfHandles - 1]->SetDirection(pt1.GetData());
  }

  double bounds[6];
  bbox.GetBounds(bounds);
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
  this->SizeHandles();
}

//----------------------------------------------------------------------------
void svtkSplineRepresentation::SetNumberOfHandles(int npts)
{
  if (this->NumberOfHandles == npts)
  {
    return;
  }
  if (npts < 1)
  {
    svtkGenericWarningMacro(<< "svtkSplineRepresentation: minimum of 1 points required.");
    return;
  }

  // Ensure that no handle is current
  this->HighlightHandle(nullptr);

  double radius = this->HandleGeometry[0]->GetRadius();
  this->Initialize();

  this->NumberOfHandles = npts;

  // Create the handles
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
    this->Handle[i]->SetProperty(this->HandleProperty);
    double u[3], pt[3];
    u[0] = i / (this->NumberOfHandles - 1.0);
    this->ParametricSpline->Evaluate(u, pt, nullptr);
    this->HandleGeometry[i]->SetCenter(pt);
    this->HandleGeometry[i]->SetRadius(radius);
    this->HandlePicker->AddPickList(this->Handle[i]);
  }

  if (this->DirectionalLine && this->NumberOfHandles >= 2)
  {
    this->HandleGeometry[this->NumberOfHandles - 1]->SetUseSphere(false);
  }

  if (this->CurrentHandleIndex >= 0 && this->CurrentHandleIndex < this->NumberOfHandles)
  {
    this->CurrentHandleIndex = this->HighlightHandle(this->Handle[this->CurrentHandleIndex]);
  }
  else
  {
    this->CurrentHandleIndex = this->HighlightHandle(nullptr);
  }

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void svtkSplineRepresentation::SetResolution(int resolution)
{
  if (this->Resolution == resolution || resolution < (this->NumberOfHandles - 1))
  {
    return;
  }

  this->Resolution = resolution;
  this->ParametricFunctionSource->SetUResolution(this->Resolution);
  this->ParametricFunctionSource->Modified();
}

//----------------------------------------------------------------------------
void svtkSplineRepresentation::GetPolyData(svtkPolyData* pd)
{
  this->ParametricFunctionSource->Update();
  pd->ShallowCopy(this->ParametricFunctionSource->GetOutput());
}

//----------------------------------------------------------------------------
double svtkSplineRepresentation::GetSummedLength()
{
  svtkPoints* points = this->ParametricFunctionSource->GetOutput()->GetPoints();
  int npts = points->GetNumberOfPoints();

  if (npts < 2)
  {
    return 0.0;
  }

  double a[3];
  double b[3];
  double sum = 0.0;
  int i = 0;
  points->GetPoint(i, a);
  int imax = (npts % 2 == 0) ? npts - 2 : npts - 1;

  while (i < imax)
  {
    points->GetPoint(i + 1, b);
    sum += sqrt(svtkMath::Distance2BetweenPoints(a, b));
    i = i + 2;
    points->GetPoint(i, a);
    sum = sum + sqrt(svtkMath::Distance2BetweenPoints(a, b));
  }

  if (npts % 2 == 0)
  {
    points->GetPoint(i + 1, b);
    sum += sqrt(svtkMath::Distance2BetweenPoints(a, b));
  }

  return sum;
}

//----------------------------------------------------------------------------
int svtkSplineRepresentation::InsertHandleOnLine(double* pos)
{
  if (this->NumberOfHandles < 2)
  {
    return -1;
  }

  svtkIdType id = this->LinePicker->GetCellId();
  if (id == -1)
  {
    return -1;
  }

  svtkIdType subid = this->LinePicker->GetSubId();

  svtkPoints* newpoints = svtkPoints::New(SVTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles + 1);

  int istart = svtkMath::Floor(
    subid * (this->NumberOfHandles + this->Closed - 1.0) / static_cast<double>(this->Resolution));
  int istop = istart + 1;
  int count = 0;
  for (int i = 0; i <= istart; ++i)
  {
    newpoints->SetPoint(count++, this->HandleGeometry[i]->GetCenter());
  }

  const int insert_index = count;
  newpoints->SetPoint(count++, pos);

  for (int i = istop; i < this->NumberOfHandles; ++i)
  {
    newpoints->SetPoint(count++, this->HandleGeometry[i]->GetCenter());
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();

  return insert_index;
}

//----------------------------------------------------------------------------
void svtkSplineRepresentation::InitializeHandles(svtkPoints* points)
{
  if (!points)
  {
    return;
  }

  int npts = points->GetNumberOfPoints();
  if (npts < 2)
  {
    return;
  }

  double p0[3];
  double p1[3];

  points->GetPoint(0, p0);
  points->GetPoint(npts - 1, p1);

  if (svtkMath::Distance2BetweenPoints(p0, p1) == 0.0)
  {
    --npts;
    this->Closed = 1;
    this->ParametricSpline->ClosedOn();
  }

  this->SetNumberOfHandles(npts);
  for (int i = 0; i < npts; ++i)
  {
    this->SetHandlePosition(i, points->GetPoint(i));
  }
}

//----------------------------------------------------------------------------
void svtkSplineRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->ParametricSpline)
  {
    os << indent << "ParametricSpline: " << this->ParametricSpline << "\n";
  }
  else
  {
    os << indent << "ParametricSpline: (none)\n";
  }
}
