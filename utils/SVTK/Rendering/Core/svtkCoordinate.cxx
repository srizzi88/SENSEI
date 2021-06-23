/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCoordinate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCoordinate.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkViewport.h"

svtkStandardNewMacro(svtkCoordinate);

svtkCxxSetObjectMacro(svtkCoordinate, ReferenceCoordinate, svtkCoordinate);

//----------------------------------------------------------------------------
// Creates an Coordinate with the following defaults:
// value of  0, 0, 0 in world coordinates
svtkCoordinate::svtkCoordinate()
{
  this->CoordinateSystem = SVTK_WORLD;
  this->Value[0] = 0.0;
  this->Value[1] = 0.0;
  this->Value[2] = 0.0;
  this->Viewport = nullptr;
  this->ReferenceCoordinate = nullptr;
  this->Computing = 0;
}

//----------------------------------------------------------------------------
// Destroy a Coordinate.
svtkCoordinate::~svtkCoordinate()
{
  // To get rid of references (Reference counting).
  this->SetReferenceCoordinate(nullptr);
}

//----------------------------------------------------------------------------
// Set the viewport. This is a raw pointer, not a weak pointer or a reference
// counted object to avoid cycle reference loop between rendering classes
// and filter classes.
void svtkCoordinate::SetViewport(svtkViewport* viewport)
{
  if (this->Viewport != viewport)
  {
    this->Viewport = viewport;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkCoordinate::GetCoordinateSystemAsString()
{
  switch (this->CoordinateSystem)
  {
    case SVTK_DISPLAY:
      return "Display";
    case SVTK_NORMALIZED_DISPLAY:
      return "Normalized Display";
    case SVTK_VIEWPORT:
      return "Viewport";
    case SVTK_NORMALIZED_VIEWPORT:
      return "Normalized Viewport";
    case SVTK_VIEW:
      return "View";
    case SVTK_POSE:
      return "Pose";
    case SVTK_WORLD:
      return "World";
    case SVTK_USERDEFINED:
      return "User Defined";
    default:
      return "UNKNOWN!";
  }
}

//----------------------------------------------------------------------------
void svtkCoordinate::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Coordinate System: " << this->GetCoordinateSystemAsString() << "\n";
  os << indent << "Value: (" << this->Value[0] << "," << this->Value[1] << "," << this->Value[2]
     << ")\n";
  if (this->ReferenceCoordinate)
  {
    os << indent << "ReferenceCoordinate: " << this->ReferenceCoordinate << "\n";
  }
  else
  {
    os << indent << "ReferenceCoordinate: (none)\n";
  }
  if (this->Viewport)
  {
    os << indent << "Viewport: " << this->Viewport << "\n";
  }
  else
  {
    os << indent << "Viewport: (none)\n";
  }
}

//----------------------------------------------------------------------------
double* svtkCoordinate::GetComputedWorldValue(svtkViewport* viewport)
{
  double* val = this->ComputedWorldValue;

  // prevent infinite loops
  if (this->Computing)
  {
    return val;
  }
  this->Computing = 1;

  val[0] = this->Value[0];
  val[1] = this->Value[1];
  val[2] = this->Value[2];

  // use our viewport if set
  if (this->Viewport)
  {
    viewport = this->Viewport;
  }

  // if viewport is nullptr then we can only do minimal calculations
  if (!viewport)
  {
    if (this->CoordinateSystem == SVTK_WORLD)
    {
      if (this->ReferenceCoordinate)
      {
        double* refValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
        val[0] += refValue[0];
        val[1] += refValue[1];
        val[2] += refValue[2];
      }
      this->Computing = 0;
    }
    else
    {
      svtkErrorMacro(
        "Attempt to compute world coordinates from another coordinate system without a viewport");
    }
    return val;
  }

  if (this->ReferenceCoordinate && this->CoordinateSystem != SVTK_WORLD)
  {
    double refValue[3];
    double* fval;

    fval = this->ReferenceCoordinate->GetComputedDoubleDisplayValue(viewport);
    refValue[0] = fval[0];
    refValue[1] = fval[1];
    refValue[2] = 0.0;

    // convert to current coordinate system
    switch (this->CoordinateSystem)
    {
      case SVTK_NORMALIZED_DISPLAY:
        viewport->DisplayToNormalizedDisplay(refValue[0], refValue[1]);
        break;
      case SVTK_VIEWPORT:
        viewport->DisplayToNormalizedDisplay(refValue[0], refValue[1]);
        viewport->NormalizedDisplayToViewport(refValue[0], refValue[1]);
        break;
      case SVTK_NORMALIZED_VIEWPORT:
        viewport->DisplayToNormalizedDisplay(refValue[0], refValue[1]);
        viewport->NormalizedDisplayToViewport(refValue[0], refValue[1]);
        viewport->ViewportToNormalizedViewport(refValue[0], refValue[1]);
        break;
      case SVTK_VIEW:
        viewport->DisplayToNormalizedDisplay(refValue[0], refValue[1]);
        viewport->NormalizedDisplayToViewport(refValue[0], refValue[1]);
        viewport->ViewportToNormalizedViewport(refValue[0], refValue[1]);
        viewport->NormalizedViewportToView(refValue[0], refValue[1], refValue[2]);
        break;
      case SVTK_POSE:
        viewport->DisplayToNormalizedDisplay(refValue[0], refValue[1]);
        viewport->NormalizedDisplayToViewport(refValue[0], refValue[1]);
        viewport->ViewportToNormalizedViewport(refValue[0], refValue[1]);
        viewport->NormalizedViewportToView(refValue[0], refValue[1], refValue[2]);
        viewport->ViewToPose(refValue[0], refValue[1], refValue[2]);
        break;
    }

    // add to current value
    val[0] += refValue[0];
    val[1] += refValue[1];
    val[2] += refValue[2];
  }

  // compute our WC
  switch (this->CoordinateSystem)
  {
    case SVTK_DISPLAY:
      viewport->DisplayToNormalizedDisplay(val[0], val[1]);
      SVTK_FALLTHROUGH;
    case SVTK_NORMALIZED_DISPLAY:
      viewport->NormalizedDisplayToViewport(val[0], val[1]);
      SVTK_FALLTHROUGH;
    case SVTK_VIEWPORT:
      viewport->ViewportToNormalizedViewport(val[0], val[1]);
      SVTK_FALLTHROUGH;
    case SVTK_NORMALIZED_VIEWPORT:
      viewport->NormalizedViewportToView(val[0], val[1], val[2]);
      SVTK_FALLTHROUGH;
    case SVTK_VIEW:
      viewport->ViewToPose(val[0], val[1], val[2]);
      SVTK_FALLTHROUGH;
    case SVTK_POSE:
      viewport->PoseToWorld(val[0], val[1], val[2]);
      break;
  }

  if (this->ReferenceCoordinate && this->CoordinateSystem == SVTK_WORLD)
  {
    double* refValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
    val[0] += refValue[0];
    val[1] += refValue[1];
    val[2] += refValue[2];
  }

  this->Computing = 0;
  svtkDebugMacro("Returning WorldValue of : " << this->ComputedWorldValue[0] << " , "
                                             << this->ComputedWorldValue[1] << " , "
                                             << this->ComputedWorldValue[2]);
  return val;
}

//----------------------------------------------------------------------------
double* svtkCoordinate::GetComputedDoubleViewportValue(svtkViewport* viewport)
{
  // use our viewport if set
  if (this->Viewport)
  {
    viewport = this->Viewport;
  }

  double* d = this->GetComputedDoubleDisplayValue(viewport);

  if (!viewport)
  {
    svtkDebugMacro("Attempt to convert to compute viewport coordinates without a viewport, results "
                  "may not be valid");
    return d;
  }

  double f[2];
  f[0] = d[0];
  f[1] = d[1];

  viewport->DisplayToNormalizedDisplay(f[0], f[1]);
  viewport->NormalizedDisplayToViewport(f[0], f[1]);

  this->ComputedDoubleViewportValue[0] = f[0];
  this->ComputedDoubleViewportValue[1] = f[1];

  return this->ComputedDoubleViewportValue;
}

//----------------------------------------------------------------------------
int* svtkCoordinate::GetComputedViewportValue(svtkViewport* viewport)
{
  double* f = this->GetComputedDoubleViewportValue(viewport);

  this->ComputedViewportValue[0] = static_cast<int>(std::round(f[0]));
  this->ComputedViewportValue[1] = static_cast<int>(std::round(f[1]));

  return this->ComputedViewportValue;
}

//----------------------------------------------------------------------------
int* svtkCoordinate::GetComputedLocalDisplayValue(svtkViewport* viewport)
{
  double a[2];

  // use our viewport if set
  if (this->Viewport)
  {
    viewport = this->Viewport;
  }
  this->GetComputedDisplayValue(viewport);

  if (!viewport)
  {
    svtkErrorMacro("Attempt to convert to local display coordinates without a viewport");
    return this->ComputedDisplayValue;
  }

  a[0] = static_cast<double>(this->ComputedDisplayValue[0]);
  a[1] = static_cast<double>(this->ComputedDisplayValue[1]);

  viewport->DisplayToLocalDisplay(a[0], a[1]);

  this->ComputedDisplayValue[0] = static_cast<int>(std::round(a[0]));
  this->ComputedDisplayValue[1] = static_cast<int>(std::round(a[1]));

  svtkDebugMacro("Returning LocalDisplayValue of : " << this->ComputedDisplayValue[0] << " , "
                                                    << this->ComputedDisplayValue[1]);

  return this->ComputedDisplayValue;
}

//----------------------------------------------------------------------------
double* svtkCoordinate::GetComputedDoubleDisplayValue(svtkViewport* viewport)
{
  double val[3];

  // prevent infinite loops
  if (this->Computing)
  {
    return this->ComputedDoubleDisplayValue;
  }
  this->Computing = 1;

  val[0] = this->Value[0];
  val[1] = this->Value[1];
  val[2] = this->Value[2];

  // use our viewport if set
  if (this->Viewport)
  {
    viewport = this->Viewport;
  }

  // if viewport is nullptr, there is very little we can do
  if (viewport == nullptr)
  {
    // for DISPLAY and VIEWPORT just use the value
    if (this->CoordinateSystem == SVTK_DISPLAY)
    {
      this->ComputedDoubleDisplayValue[0] = val[0];
      this->ComputedDoubleDisplayValue[1] = val[1];
      if (this->ReferenceCoordinate)
      {
        double* refValue = this->ReferenceCoordinate->GetComputedDoubleDisplayValue(viewport);
        this->ComputedDoubleDisplayValue[0] += refValue[0];
        this->ComputedDoubleDisplayValue[1] += refValue[1];
      }
    }
    else
    {
      this->ComputedDoubleDisplayValue[0] = static_cast<double>(SVTK_INT_MAX);
      this->ComputedDoubleDisplayValue[1] = static_cast<double>(SVTK_INT_MAX);

      svtkErrorMacro("Request for coordinate transformation without required viewport");
    }
    return this->ComputedDoubleDisplayValue;
  }

  // compute our DC
  switch (this->CoordinateSystem)
  {
    case SVTK_WORLD:
      if (this->ReferenceCoordinate)
      {
        double* refValue = this->ReferenceCoordinate->GetComputedWorldValue(viewport);
        val[0] += refValue[0];
        val[1] += refValue[1];
        val[2] += refValue[2];
      }
      viewport->WorldToPose(val[0], val[1], val[2]);
      SVTK_FALLTHROUGH;
    case SVTK_POSE:
      viewport->PoseToView(val[0], val[1], val[2]);
      SVTK_FALLTHROUGH;
    case SVTK_VIEW:
      viewport->ViewToNormalizedViewport(val[0], val[1], val[2]);
      SVTK_FALLTHROUGH;
    case SVTK_NORMALIZED_VIEWPORT:
      viewport->NormalizedViewportToViewport(val[0], val[1]);
      SVTK_FALLTHROUGH;
    case SVTK_VIEWPORT:
      if ((this->CoordinateSystem == SVTK_NORMALIZED_VIEWPORT ||
            this->CoordinateSystem == SVTK_VIEWPORT) &&
        this->ReferenceCoordinate)
      {
        double* refValue = this->ReferenceCoordinate->GetComputedDoubleViewportValue(viewport);
        val[0] += refValue[0];
        val[1] += refValue[1];
      }
      viewport->ViewportToNormalizedDisplay(val[0], val[1]);
      SVTK_FALLTHROUGH;
    case SVTK_NORMALIZED_DISPLAY:
      viewport->NormalizedDisplayToDisplay(val[0], val[1]);
      break;
    case SVTK_USERDEFINED:
      this->GetComputedUserDefinedValue(viewport);
      val[0] = this->ComputedUserDefinedValue[0];
      val[1] = this->ComputedUserDefinedValue[1];
      val[2] = this->ComputedUserDefinedValue[2];
      break;
  }

  // if we have a reference coordinate and we haven't handled it yet
  if (this->ReferenceCoordinate &&
    (this->CoordinateSystem == SVTK_DISPLAY || this->CoordinateSystem == SVTK_NORMALIZED_DISPLAY))
  {
    double* refValue = this->ReferenceCoordinate->GetComputedDoubleDisplayValue(viewport);
    val[0] += refValue[0];
    val[1] += refValue[1];
  }
  this->ComputedDoubleDisplayValue[0] = val[0];
  this->ComputedDoubleDisplayValue[1] = val[1];

  this->Computing = 0;
  return this->ComputedDoubleDisplayValue;
}

//----------------------------------------------------------------------------
int* svtkCoordinate::GetComputedDisplayValue(svtkViewport* viewport)
{
  double* val = this->GetComputedDoubleDisplayValue(viewport);

  this->ComputedDisplayValue[0] = static_cast<int>(val[0]);
  this->ComputedDisplayValue[1] = static_cast<int>(val[1]);

  svtkDebugMacro("Returning DisplayValue of : " << this->ComputedDisplayValue[0] << " , "
                                               << this->ComputedDisplayValue[1]);
  return this->ComputedDisplayValue;
}

//----------------------------------------------------------------------------
double* svtkCoordinate::GetComputedValue(svtkViewport* viewport)
{
  // use our viewport if set
  if (this->Viewport)
  {
    viewport = this->Viewport;
  }

  switch (this->CoordinateSystem)
  {
    case SVTK_WORLD:
    case SVTK_POSE:
      return this->GetComputedWorldValue(viewport);
    case SVTK_VIEW:
    case SVTK_NORMALIZED_VIEWPORT:
    case SVTK_VIEWPORT:
    {
      // result stored in computed world value due to double
      // but is really a viewport value
      int* v = this->GetComputedViewportValue(viewport);
      this->ComputedWorldValue[0] = v[0];
      this->ComputedWorldValue[1] = v[1];
      break;
    }
    case SVTK_NORMALIZED_DISPLAY:
    case SVTK_DISPLAY:
    {
      // result stored in computed world value due to double
      // but is really a display value
      int* d = this->GetComputedDisplayValue(viewport);
      this->ComputedWorldValue[0] = d[0];
      this->ComputedWorldValue[1] = d[1];
      break;
    }
  }

  return this->ComputedWorldValue;
}
