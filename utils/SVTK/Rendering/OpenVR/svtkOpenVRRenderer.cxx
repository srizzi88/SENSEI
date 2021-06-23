/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRRenderer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

Parts Copyright Valve Coproration from hellovr_opengl_main.cpp
under their BSD license found here:
https://github.com/ValveSoftware/openvr/blob/master/LICENSE

=========================================================================*/
#include "svtkOpenVRRenderer.h"
#include "svtkOpenVRCamera.h"

#include "svtkObjectFactory.h"

#include "svtkActor.h"
#include "svtkImageCanvasSource2D.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkOpenVRRenderWindow.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkTexture.h"

svtkStandardNewMacro(svtkOpenVRRenderer);

svtkOpenVRRenderer::svtkOpenVRRenderer()
{
  // better default
  this->ClippingRangeExpansion = 0.05;

  this->FloorActor = svtkActor::New();
  this->FloorActor->PickableOff();

  svtkNew<svtkPolyDataMapper> pdm;
  this->FloorActor->SetMapper(pdm);
  svtkNew<svtkPlaneSource> plane;
  pdm->SetInputConnection(plane->GetOutputPort());
  plane->SetOrigin(-5.0, 0.0, -5.0);
  plane->SetPoint1(5.0, 0.0, -5.0);
  plane->SetPoint2(-5.0, 0.0, 5.0);

  svtkNew<svtkTransform> tf;
  tf->Identity();
  this->FloorActor->SetUserTransform(tf);

  svtkNew<svtkTexture> texture;
  this->FloorActor->SetTexture(texture);

  // build a grid fading off in the distance
  svtkNew<svtkImageCanvasSource2D> grid;
  grid->SetScalarTypeToUnsignedChar();
  grid->SetNumberOfScalarComponents(4);
  grid->SetExtent(0, 511, 0, 511, 0, 0);
  int divisions = 16;
  int divSize = 512 / divisions;
  double alpha = 1.0;
  for (int i = 0; i < divisions; i++)
  {
    for (int j = 0; j < divisions; j++)
    {
      grid->SetDrawColor(255, 255, 255, 255 * alpha);
      grid->FillBox(i * divSize, (i + 1) * divSize - 1, j * divSize, (j + 1) * divSize - 1);
      grid->SetDrawColor(230, 230, 230, 255 * alpha);
      grid->DrawSegment(i * divSize, j * divSize, (i + 1) * divSize - 1, j * divSize);
      grid->DrawSegment(i * divSize, j * divSize, i * divSize, (j + 1) * divSize - 1);
    }
  }

  texture->SetInputConnection(grid->GetOutputPort());

  this->FloorActor->SetUseBounds(false);

  this->ShowFloor = false;
}

svtkOpenVRRenderer::~svtkOpenVRRenderer()
{
  this->FloorActor->Delete();
  this->FloorActor = 0;
}

//----------------------------------------------------------------------------
svtkCamera* svtkOpenVRRenderer::MakeCamera()
{
  svtkCamera* cam = svtkOpenVRCamera::New();
  this->InvokeEvent(svtkCommand::CreateCameraEvent, cam);
  return cam;
}

// adjust the floor if we need to
void svtkOpenVRRenderer::DeviceRender()
{
  if (this->ShowFloor)
  {
    svtkOpenVRRenderWindow* win = static_cast<svtkOpenVRRenderWindow*>(this->GetRenderWindow());

    double physicalScale = win->GetPhysicalScale();

    double trans[3];
    win->GetPhysicalTranslation(trans);

    double* vup = win->GetPhysicalViewUp();
    double* dop = win->GetPhysicalViewDirection();
    double vr[3];
    svtkMath::Cross(dop, vup, vr);
    double rot[16] = { vr[0], vup[0], -dop[0], 0.0, vr[1], vup[1], -dop[1], 0.0, vr[2], vup[2],
      -dop[2], 0.0, 0.0, 0.0, 0.0, 1.0 };

    static_cast<svtkTransform*>(this->FloorActor->GetUserTransform())->Identity();
    static_cast<svtkTransform*>(this->FloorActor->GetUserTransform())
      ->Translate(-trans[0], -trans[1], -trans[2]);
    static_cast<svtkTransform*>(this->FloorActor->GetUserTransform())
      ->Scale(physicalScale, physicalScale, physicalScale);
    static_cast<svtkTransform*>(this->FloorActor->GetUserTransform())->Concatenate(rot);
  }
  this->Superclass::DeviceRender();
}

void svtkOpenVRRenderer::SetShowFloor(bool value)
{
  if (this->ShowFloor == value)
  {
    return;
  }

  this->ShowFloor = value;

  if (this->ShowFloor)
  {
    this->AddActor(this->FloorActor);
  }
  else
  {
    this->RemoveActor(this->FloorActor);
  }
}

// Automatically set up the camera based on the visible actors.
// The camera will reposition itself to view the center point of the actors,
// and move along its initial view plane normal (i.e., vector defined from
// camera position to focal point) so that all of the actors can be seen.
void svtkOpenVRRenderer::ResetCamera()
{
  this->Superclass::ResetCamera();
}

// Automatically set up the camera based on a specified bounding box
// (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
// that its focal point is the center of the bounding box, and adjust its
// distance and position to preserve its initial view plane normal
// (i.e., vector defined from camera position to focal point). Note: if
// the view plane is parallel to the view up axis, the view up axis will
// be reset to one of the three coordinate axes.
void svtkOpenVRRenderer::ResetCamera(double bounds[6])
{
  double center[3];
  double distance;
  double vn[3], *vup;

  this->GetActiveCamera();
  if (this->ActiveCamera != nullptr)
  {
    this->ActiveCamera->GetViewPlaneNormal(vn);
  }
  else
  {
    svtkErrorMacro(<< "Trying to reset non-existent camera");
    return;
  }

  // Reset the perspective zoom factors, otherwise subsequent zooms will cause
  // the view angle to become very small and cause bad depth sorting.
  this->ActiveCamera->SetViewAngle(110.0);

  this->ExpandBounds(bounds, this->ActiveCamera->GetModelTransformMatrix());

  center[0] = (bounds[0] + bounds[1]) / 2.0;
  center[1] = (bounds[2] + bounds[3]) / 2.0;
  center[2] = (bounds[4] + bounds[5]) / 2.0;

  double w1 = bounds[1] - bounds[0];
  double w2 = bounds[3] - bounds[2];
  double w3 = bounds[5] - bounds[4];
  w1 *= w1;
  w2 *= w2;
  w3 *= w3;
  double radius = w1 + w2 + w3;

  // If we have just a single point, pick a radius of 1.0
  radius = (radius == 0) ? (1.0) : (radius);

  // compute the radius of the enclosing sphere
  radius = sqrt(radius) * 0.5;

  // default so that the bounding sphere fits within the view fustrum

  // compute the distance from the intersection of the view frustum with the
  // bounding sphere. Basically in 2D draw a circle representing the bounding
  // sphere in 2D then draw a horizontal line going out from the center of
  // the circle. That is the camera view. Then draw a line from the camera
  // position to the point where it intersects the circle. (it will be tangent
  // to the circle at this point, this is important, only go to the tangent
  // point, do not draw all the way to the view plane). Then draw the radius
  // from the tangent point to the center of the circle. You will note that
  // this forms a right triangle with one side being the radius, another being
  // the target distance for the camera, then just find the target dist using
  // a sin.
  double angle = svtkMath::RadiansFromDegrees(this->ActiveCamera->GetViewAngle());

  this->ComputeAspect();
  double aspect[2];
  this->GetAspect(aspect);

  if (aspect[0] >= 1.0) // horizontal window, deal with vertical angle|scale
  {
    if (this->ActiveCamera->GetUseHorizontalViewAngle())
    {
      angle = 2.0 * atan(tan(angle * 0.5) / aspect[0]);
    }
  }
  else // vertical window, deal with horizontal angle|scale
  {
    if (!this->ActiveCamera->GetUseHorizontalViewAngle())
    {
      angle = 2.0 * atan(tan(angle * 0.5) * aspect[0]);
    }
  }
  distance = radius / sin(angle * 0.5);

  // check view-up vector against view plane normal
  vup = this->ActiveCamera->GetViewUp();
  if (fabs(svtkMath::Dot(vup, vn)) > 0.999)
  {
    svtkWarningMacro(<< "Resetting view-up since view plane normal is parallel");
    this->ActiveCamera->SetViewUp(-vup[2], vup[0], vup[1]);
  }

  // update the camera
  this->ActiveCamera->SetFocalPoint(center[0], center[1], center[2]);
  this->ActiveCamera->SetPosition(
    center[0] + distance * vn[0], center[1] + distance * vn[1], center[2] + distance * vn[2]);

  // now set the cameras shift and scale to the HMD space
  // since the vive is always in meters (or something like that)
  // we use a shift scale to map view space into hmd view space
  // that way the solar system can be modelled in its units
  // while the shift scale maps it into meters.  This can also
  // be done in the actors but then it requires every actor
  // to be adjusted.  It cannot be done with the camera model
  // matrix as that is broken.
  // The +distance in the Y translation is because we want
  // the center of the world to be 1 meter up
  svtkOpenVRRenderWindow* win = static_cast<svtkOpenVRRenderWindow*>(this->GetRenderWindow());
  win->SetPhysicalTranslation(-center[0], -center[1] + distance, -center[2]);
  win->SetPhysicalScale(distance);
}

// Alternative version of ResetCamera(bounds[6]);
void svtkOpenVRRenderer::ResetCamera(
  double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
  double bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->ResetCamera(bounds);
}

// Reset the camera clipping range to include this entire bounding box
void svtkOpenVRRenderer::ResetCameraClippingRange(double bounds[6])
{
  double range[2];
  int i, j, k;

  // Don't reset the clipping range when we don't have any 3D visible props
  if (!svtkMath::AreBoundsInitialized(bounds))
  {
    return;
  }

  this->GetActiveCameraAndResetIfCreated();
  if (this->ActiveCamera == nullptr)
  {
    svtkErrorMacro(<< "Trying to reset clipping range of non-existent camera");
    return;
  }

  this->ExpandBounds(bounds, this->ActiveCamera->GetModelTransformMatrix());

  double trans[3];
  svtkOpenVRRenderWindow* win = static_cast<svtkOpenVRRenderWindow*>(this->GetRenderWindow());
  win->GetPhysicalTranslation(trans);
  double physicalScale = win->GetPhysicalScale();

  range[0] = 0.2; // 20 cm in front of HMD
  range[1] = 0.0;

  // Find the farthest bounding box vertex
  for (k = 0; k < 2; k++)
  {
    for (j = 0; j < 2; j++)
    {
      for (i = 0; i < 2; i++)
      {
        double fard = sqrt((bounds[i] - trans[0]) * (bounds[i] - trans[0]) +
          (bounds[2 + j] - trans[1]) * (bounds[2 + j] - trans[1]) +
          (bounds[4 + k] - trans[2]) * (bounds[4 + k] - trans[2]));
        range[1] = (fard > range[1]) ? fard : range[1];
      }
    }
  }

  range[1] /= physicalScale; // convert to physical scale
  range[1] += 3.0;           // add 3 meters for room to walk around

  // to see transmitters make sure far is at least 10 meters
  if (range[1] < 10.0)
  {
    range[1] = 10.0;
  }

  this->ActiveCamera->SetClippingRange(range[0] * physicalScale, range[1] * physicalScale);
}

void svtkOpenVRRenderer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
