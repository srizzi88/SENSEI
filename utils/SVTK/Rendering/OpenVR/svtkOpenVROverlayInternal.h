/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkPVOpenVROverlayInternal_h
#define svtkPVOpenVROverlayInternal_h

#include "svtkInteractorStyle3D.h"
#include "svtkOpenVRCamera.h"
#include "svtkOpenVRRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkVectorOperators.h"

class svtkOpenVRCameraPose
{
public:
  double Position[3];
  double PhysicalViewUp[3];
  double PhysicalViewDirection[3];
  double ViewDirection[3];
  double Translation[3];
  double Distance;
  double MotionFactor = 1.0;
  bool Loaded = false;

  // return a vector based on in that is orthogonal to normal
  svtkVector3d SanitizeVector(svtkVector3d& in, svtkVector3d& normal)
  {
    svtkVector3d result;
    if (fabs(in.Dot(normal)) > 0.999) // some epsilon
    {
      if (fabs(normal[0]) < 0.1)
      {
        result.Set(1.0, 0.0, 0.0);
      }
      else
      {
        result.Set(0.0, 1.0, 0.0);
      }
    }
    else
    {
      result = in - (in.Dot(normal)) * normal;
      result.Normalize();
    }
    return result;
  }

  void Set(svtkOpenVRCamera* cam, svtkOpenVRRenderWindow* win)
  {
    win->GetPhysicalTranslation(this->Translation);
    win->GetPhysicalViewUp(this->PhysicalViewUp);
    this->Distance = win->GetPhysicalScale();
    svtkInteractorStyle3D* is =
      static_cast<svtkInteractorStyle3D*>(win->GetInteractor()->GetInteractorStyle());
    this->MotionFactor = is->GetDollyPhysicalSpeed();

    cam->GetPosition(this->Position);

    win->GetPhysicalViewDirection(this->PhysicalViewDirection);
    cam->GetDirectionOfProjection(this->ViewDirection);

    this->Loaded = true;
  }

  void Apply(svtkOpenVRCamera* cam, svtkOpenVRRenderWindow* win)
  {

    // s = saved values
    svtkVector3d svup(this->PhysicalViewUp);
    svtkVector3d svdir(this->ViewDirection);
    svtkVector3d strans(this->Translation);
    svtkVector3d spos(this->Position);
    double sdistance = this->Distance;

    // c = current values
    svtkVector3d cvup;
    win->GetPhysicalViewUp(cvup.GetData());
    svtkVector3d cpos;
    cam->GetPosition(cpos.GetData());
    svtkVector3d ctrans;
    win->GetPhysicalTranslation(ctrans.GetData());
    svtkVector3d cvdir;
    cam->GetDirectionOfProjection(cvdir.GetData());
    svtkVector3d civdir;
    win->GetPhysicalViewDirection(civdir.GetData());
    double cdistance = win->GetPhysicalScale();

    // n = new values
    svtkVector3d nvup = svup;
    win->SetPhysicalViewUp(nvup.GetData());

    // sanitize the svdir, must be orthogonal to nvup
    svdir = this->SanitizeVector(svdir, nvup);

    // make sure cvdir and civdir are orthogonal to our nvup
    cvdir = this->SanitizeVector(cvdir, nvup);
    civdir = this->SanitizeVector(civdir, nvup);
    svtkVector3d civright = civdir.Cross(nvup);

    // find the new initialvdir
    svtkVector3d nivdir;
    double theta = acos(svdir.Dot(cvdir));
    if (nvup.Dot(cvdir.Cross(svdir)) < 0.0)
    {
      theta = -theta;
    }
    // rotate civdir by theta
    nivdir = civdir * cos(theta) - civright * sin(theta);
    win->SetPhysicalViewDirection(nivdir.GetData());
    svtkVector3d nivright = nivdir.Cross(nvup);

    // adjust translation so that we are in the same spot
    // as when the camera was saved
    svtkVector3d ntrans;
    svtkVector3d cppwc;
    cppwc = cpos + ctrans;
    double x = cppwc.Dot(civdir) / cdistance;
    double y = cppwc.Dot(civright) / cdistance;

    ntrans = strans * nvup + nivdir * (x * sdistance - spos.Dot(nivdir)) +
      nivright * (y * sdistance - spos.Dot(nivright));

    win->SetPhysicalTranslation(ntrans.GetData());
    cam->SetPosition(cpos.GetData());

    // this really only sets the distance as the render loop
    // sets focal point and position every frame
    svtkVector3d nfp;
    nfp = cpos + nivdir * sdistance;
    cam->SetFocalPoint(nfp.GetData());
    win->SetPhysicalScale(sdistance);

#if 0
    win->SetPhysicalViewDirection(this->PhysicalViewDirection);
    cam->SetTranslation(this->Translation);
    cam->SetFocalPoint(this->FocalPoint);
    cam->SetPosition(
      this->FocalPoint[0] - this->PhysicalViewDirection[0]*this->Distance,
      this->FocalPoint[1] - this->PhysicalViewDirection[1]*this->Distance,
      this->FocalPoint[2] - this->PhysicalViewDirection[2]*this->Distance);
#endif

    win->SetPhysicalViewUp(this->PhysicalViewUp);
    svtkInteractorStyle3D* is =
      static_cast<svtkInteractorStyle3D*>(win->GetInteractor()->GetInteractorStyle());
    is->SetDollyPhysicalSpeed(this->MotionFactor);
  }
};

class svtkOpenVROverlaySpot
{
public:
  svtkOpenVROverlaySpot(int x1, int x2, int y1, int y2, svtkCommand* cb)
  {
    this->xmin = x1;
    this->xmax = x2;
    this->ymin = y1;
    this->ymax = y2;
    this->Callback = cb;
    cb->Register(nullptr);
    this->Active = false;
  }
  ~svtkOpenVROverlaySpot()
  {
    if (this->Callback)
    {
      this->Callback->Delete();
      this->Callback = nullptr;
    }
  }
  bool Active;
  int xmin;
  int xmax;
  int ymin;
  int ymax;
  svtkCommand* Callback;
  std::string Group;
  int GroupId;

  svtkOpenVROverlaySpot(const svtkOpenVROverlaySpot& in)
  {
    this->xmin = in.xmin;
    this->xmax = in.xmax;
    this->ymin = in.ymin;
    this->ymax = in.ymax;
    this->Callback = in.Callback;
    this->Callback->Register(0);
    this->Active = in.Active;
    this->Group = in.Group;
    this->GroupId = in.GroupId;
  }
  svtkOpenVROverlaySpot& operator=(const svtkOpenVROverlaySpot&) = delete;
};

#endif // svtkPVOpenVROverlayInternal_h

//****************************************************************************
// SVTK-HeaderTest-Exclude: svtkOpenVROverlayInternal.h
