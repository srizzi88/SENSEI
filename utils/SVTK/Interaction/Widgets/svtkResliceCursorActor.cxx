/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkResliceCursorActor.h"

#include "svtkActor.h"
#include "svtkBoundingBox.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkResliceCursor.h"
#include "svtkResliceCursorPolyDataAlgorithm.h"
#include "svtkViewport.h"

svtkStandardNewMacro(svtkResliceCursorActor);

// ----------------------------------------------------------------------------
svtkResliceCursorActor::svtkResliceCursorActor()
{
  this->CursorAlgorithm = svtkResliceCursorPolyDataAlgorithm::New();

  for (int i = 0; i < 3; i++)
  {
    this->CursorCenterlineMapper[i] = svtkPolyDataMapper::New();
    this->CursorCenterlineActor[i] = svtkActor::New();
    this->CursorThickSlabMapper[i] = svtkPolyDataMapper::New();
    this->CursorThickSlabActor[i] = svtkActor::New();
    this->CursorCenterlineMapper[i]->ScalarVisibilityOff();
    this->CursorThickSlabMapper[i]->ScalarVisibilityOff();

    this->CursorCenterlineActor[i]->SetMapper(this->CursorCenterlineMapper[i]);
    this->CursorThickSlabActor[i]->SetMapper(this->CursorThickSlabMapper[i]);
    this->CenterlineProperty[i] = svtkProperty::New();
    this->ThickSlabProperty[i] = svtkProperty::New();

    this->CursorCenterlineActor[i]->SetProperty(this->CenterlineProperty[i]);
    this->CursorThickSlabActor[i]->SetProperty(this->ThickSlabProperty[i]);
  }

  this->CenterlineProperty[0]->SetColor(1, 0, 0);
  this->CenterlineProperty[1]->SetColor(0, 1, 0);
  this->CenterlineProperty[2]->SetColor(0, 0, 1);
  this->ThickSlabProperty[0]->SetColor(1, 0.6, 0.6);
  this->ThickSlabProperty[1]->SetColor(0.6, 1, 0.6);
  this->ThickSlabProperty[2]->SetColor(0.6, 0.6, 1);

  this->CenterlineProperty[0]->SetEdgeColor(1, 0, 0);
  this->CenterlineProperty[1]->SetEdgeColor(0, 1, 0);
  this->CenterlineProperty[2]->SetEdgeColor(0, 0, 1);
  this->ThickSlabProperty[0]->SetEdgeColor(1, 0.6, 0.6);
  this->ThickSlabProperty[1]->SetEdgeColor(0.6, 1, 0.6);
  this->ThickSlabProperty[2]->SetEdgeColor(0.6, 0.6, 1);

  this->CenterlineProperty[0]->SetEdgeVisibility(1);
  this->CenterlineProperty[1]->SetEdgeVisibility(1);
  this->CenterlineProperty[2]->SetEdgeVisibility(1);
  this->ThickSlabProperty[0]->SetEdgeVisibility(1);
  this->ThickSlabProperty[1]->SetEdgeVisibility(1);
  this->ThickSlabProperty[2]->SetEdgeVisibility(1);
}

// ----------------------------------------------------------------------------
svtkResliceCursorActor::~svtkResliceCursorActor()
{
  for (int i = 0; i < 3; i++)
  {
    this->CursorCenterlineMapper[i]->Delete();
    this->CursorCenterlineActor[i]->Delete();
    this->CursorThickSlabMapper[i]->Delete();
    this->CursorThickSlabActor[i]->Delete();
    this->CenterlineProperty[i]->Delete();
    this->ThickSlabProperty[i]->Delete();
  }
  this->CursorAlgorithm->Delete();
}

// ----------------------------------------------------------------------------
// Description:
// Support the standard render methods.
int svtkResliceCursorActor::RenderOpaqueGeometry(svtkViewport* viewport)
{
  int result = 0;

  if (this->CursorAlgorithm->GetResliceCursor())
  {
    this->UpdateViewProps(viewport);

    for (int i = 0; i < 3; i++)
    {
      if (this->CursorCenterlineActor[i]->GetVisibility())
      {
        result += this->CursorCenterlineActor[i]->RenderOpaqueGeometry(viewport);
      }
      if (this->CursorThickSlabActor[i]->GetVisibility())
      {
        result += this->CursorThickSlabActor[i]->RenderOpaqueGeometry(viewport);
      }
    }
  }

  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry? No.
svtkTypeBool svtkResliceCursorActor::HasTranslucentPolygonalGeometry()
{
  return false;
}

//-----------------------------------------------------------------------------
void svtkResliceCursorActor::ReleaseGraphicsResources(svtkWindow* window)
{
  for (int i = 0; i < 3; i++)
  {
    this->CursorCenterlineActor[i]->ReleaseGraphicsResources(window);
    this->CursorThickSlabActor[i]->ReleaseGraphicsResources(window);
  }
}

//-------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkResliceCursorActor::GetBounds()
{
  // we cannot initialize the Bounds the same way svtkBoundingBox does because
  // svtkProp3D::GetLength() does not check if the Bounds are initialized or
  // not and makes a call to sqrt(). This call to sqrt with invalid values
  // would raise a floating-point overflow exception (notably on BCC).
  // As svtkMath::UninitializeBounds initialized finite unvalid bounds, it
  // passes silently and GetLength() returns 0.
  svtkMath::UninitializeBounds(this->Bounds);

  this->UpdateViewProps();

  svtkBoundingBox* bb = new svtkBoundingBox();

  double bounds[6];
  for (int i = 0; i < 3; i++)
  {
    if (this->CursorCenterlineActor[i]->GetVisibility() &&
      this->CursorCenterlineActor[i]->GetUseBounds())
    {
      this->CursorCenterlineActor[i]->GetBounds(bounds);
      bb->AddBounds(bounds);
    }
    if (this->CursorThickSlabActor[i]->GetVisibility() &&
      this->CursorThickSlabActor[i]->GetUseBounds())
    {
      this->CursorThickSlabActor[i]->GetBounds(bounds);
      bb->AddBounds(bounds);
    }
  }

  bb->GetBounds(this->Bounds);

  delete bb;
  return this->Bounds;
}

//-------------------------------------------------------------------------
svtkMTimeType svtkResliceCursorActor::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->CursorAlgorithm)
  {
    svtkMTimeType time;
    time = this->CursorAlgorithm->GetMTime();
    if (time > mTime)
    {
      mTime = time;
    }
  }

  return mTime;
}

// ----------------------------------------------------------------------------
svtkProperty* svtkResliceCursorActor::GetCenterlineProperty(int i)
{
  return this->CenterlineProperty[i];
}

// ----------------------------------------------------------------------------
svtkProperty* svtkResliceCursorActor::GetThickSlabProperty(int i)
{
  return this->ThickSlabProperty[i];
}

// ----------------------------------------------------------------------------
void svtkResliceCursorActor::UpdateHoleSize(svtkViewport* v)
{
  svtkResliceCursor* r = this->CursorAlgorithm->GetResliceCursor();

  if (r->GetHoleWidthInPixels() && r->GetHole() && v)
  {

    // Get the reslice center in display coords.

    double dCenter[4], wCenter[4], wCenterHoleWidthAway[4];
    r->GetCenter(wCenter);
    wCenter[3] = 1.0;
    v->SetWorldPoint(wCenter);
    v->WorldToDisplay();
    v->GetDisplayPoint(dCenter);

    // Get the world position of a point "holeWidth pixels" away from the
    // reslice center

    dCenter[0] += (r->GetHoleWidthInPixels() / 2.0);
    v->SetDisplayPoint(dCenter);
    v->DisplayToWorld();
    v->GetWorldPoint(wCenterHoleWidthAway);

    const double holeWidth =
      2.0 * sqrt(svtkMath::Distance2BetweenPoints(wCenter, wCenterHoleWidthAway));
    r->SetHoleWidth(holeWidth);

    // MTime checks ensure that this will update only if the hole width
    // has actually changed.
    this->CursorAlgorithm->Update();
  }
}

// ----------------------------------------------------------------------------
void svtkResliceCursorActor::UpdateViewProps(svtkViewport* v)
{
  if (this->CursorAlgorithm->GetResliceCursor() == nullptr)
  {
    svtkDebugMacro(<< "no cursor to represent.");
    return;
  }

  this->CursorAlgorithm->Update();

  // Update the cursor to reflect a constant hole size in pixels, if necessary

  this->UpdateHoleSize(v);

  // Rebuild cursor to have the right hole with if necessary

  const int axisNormal = this->CursorAlgorithm->GetReslicePlaneNormal();
  const int axis1 = this->CursorAlgorithm->GetPlaneAxis1();
  const int axis2 = this->CursorAlgorithm->GetPlaneAxis2();

  this->CursorCenterlineMapper[axis1]->SetInputConnection(this->CursorAlgorithm->GetOutputPort(0));
  this->CursorCenterlineMapper[axis2]->SetInputConnection(this->CursorAlgorithm->GetOutputPort(1));

  const bool thickMode = this->CursorAlgorithm->GetResliceCursor()->GetThickMode() ? true : false;

  if (thickMode)
  {
    this->CursorThickSlabMapper[axis1]->SetInputConnection(this->CursorAlgorithm->GetOutputPort(2));
    this->CursorThickSlabMapper[axis2]->SetInputConnection(this->CursorAlgorithm->GetOutputPort(3));

    this->CursorThickSlabActor[axis1]->SetVisibility(1);
    this->CursorThickSlabActor[axis2]->SetVisibility(1);
  }

  this->CursorThickSlabActor[axis1]->SetVisibility(thickMode);
  this->CursorThickSlabActor[axis2]->SetVisibility(thickMode);
  this->CursorThickSlabActor[axisNormal]->SetVisibility(0);
  this->CursorCenterlineActor[axis1]->SetVisibility(1);
  this->CursorCenterlineActor[axis2]->SetVisibility(1);
  this->CursorCenterlineActor[axisNormal]->SetVisibility(0);

  this->CursorThickSlabActor[axis1]->GetProperty()->SetEdgeVisibility(thickMode);
  this->CursorThickSlabActor[axis2]->GetProperty()->SetEdgeVisibility(thickMode);
  this->CursorThickSlabActor[axisNormal]->GetProperty()->SetEdgeVisibility(0);
  this->CursorCenterlineActor[axis1]->GetProperty()->SetEdgeVisibility(1);
  this->CursorCenterlineActor[axis2]->GetProperty()->SetEdgeVisibility(1);
  this->CursorCenterlineActor[axisNormal]->GetProperty()->SetEdgeVisibility(0);
}

//----------------------------------------------------------------------
void svtkResliceCursorActor::SetUserMatrix(svtkMatrix4x4* m)
{
  this->CursorThickSlabActor[0]->SetUserMatrix(m);
  this->CursorThickSlabActor[1]->SetUserMatrix(m);
  this->CursorThickSlabActor[2]->SetUserMatrix(m);
  this->CursorCenterlineActor[0]->SetUserMatrix(m);
  this->CursorCenterlineActor[1]->SetUserMatrix(m);
  this->CursorCenterlineActor[2]->SetUserMatrix(m);

  this->Superclass::SetUserMatrix(m);
}

//-------------------------------------------------------------------------
svtkActor* svtkResliceCursorActor::GetCenterlineActor(int axis)
{
  return this->CursorCenterlineActor[axis];
}

//----------------------------------------------------------------------
// Prints an object if it exists.
#define svtkPrintMemberObjectMacro(obj, os, indent)                                                 \
  os << (indent) << #obj << ": ";                                                                  \
  if (this->obj)                                                                                   \
  {                                                                                                \
    os << this->obj << "\n";                                                                       \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    os << "(null)\n";                                                                              \
  }

//-------------------------------------------------------------------------
void svtkResliceCursorActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  svtkPrintMemberObjectMacro(CursorCenterlineActor[0], os, indent);
  svtkPrintMemberObjectMacro(CursorCenterlineActor[1], os, indent);
  svtkPrintMemberObjectMacro(CursorCenterlineActor[2], os, indent);
  svtkPrintMemberObjectMacro(CursorAlgorithm, os, indent);

  // this->CursorCenterlineMapper[3];
  // this->CursorCenterlineActor[3];
  // this->CursorThickSlabMapper[3];
  // this->CursorThickSlabActor[3];
  // this->CenterlineProperty[3];
  // this->ThickSlabProperty[3];
  // this->CursorAlgorithm;
}
