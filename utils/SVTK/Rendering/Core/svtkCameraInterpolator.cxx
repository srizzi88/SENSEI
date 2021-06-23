/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCameraInterpolator.h"
#include "svtkCamera.h"
#include "svtkObjectFactory.h"
#include "svtkTransform.h"
#include "svtkTupleInterpolator.h"
#include <list>

svtkStandardNewMacro(svtkCameraInterpolator);

// PIMPL STL encapsulation for list of cameras. This just keeps track of all
// the data the user specifies, which is later dumped into the interpolators.
struct svtkICamera
{
  double Time;   // Parameter t
  double P[3];   // Position
  double FP[3];  // Focal point
  double VUP[3]; // ViewUp
  double CR[2];  // Clipping range
  double VA[1];  // View angle
  double PS[1];  // Parallel scale

  svtkICamera()
  {
    this->Time = 0.0;
    this->P[0] = this->P[1] = this->P[2] = 0.0;
    this->FP[0] = this->FP[1] = this->FP[2] = 0.0;
    this->VUP[0] = this->VUP[1] = this->VUP[2] = 0.0;
    this->CR[0] = 1.0;
    this->CR[0] = 1000.0;
    this->VA[0] = 30.0;
    this->PS[0] = 1.0;
  }
  svtkICamera(double t, svtkCamera* camera)
  {
    this->Time = t;
    if (camera)
    {
      camera->GetPosition(this->P);
      camera->GetFocalPoint(this->FP);
      camera->GetViewUp(this->VUP);
      camera->GetClippingRange(this->CR);
      this->VA[0] = camera->GetViewAngle();
      this->PS[0] = camera->GetParallelScale();
    }
    else
    {
      this->P[0] = this->P[1] = this->P[2] = 0.0;
      this->FP[0] = this->FP[1] = this->FP[2] = 0.0;
      this->VUP[0] = this->VUP[1] = this->VUP[2] = 0.0;
      this->CR[0] = 1.0;
      this->CR[0] = 1000.0;
      this->VA[0] = 30.0;
      this->PS[0] = 1.0;
    }
  }
};

// The list is arranged in increasing order in T
class svtkCameraList : public std::list<svtkICamera>
{
};
typedef svtkCameraList::iterator CameraListIterator;

//----------------------------------------------------------------------------
svtkCameraInterpolator::svtkCameraInterpolator()
{
  // Set up the interpolation
  this->InterpolationType = INTERPOLATION_TYPE_SPLINE;

  // Spline interpolation
  this->PositionInterpolator = svtkTupleInterpolator::New();
  this->FocalPointInterpolator = svtkTupleInterpolator::New();
  this->ViewUpInterpolator = svtkTupleInterpolator::New();
  this->ViewAngleInterpolator = svtkTupleInterpolator::New();
  this->ParallelScaleInterpolator = svtkTupleInterpolator::New();
  this->ClippingRangeInterpolator = svtkTupleInterpolator::New();

  // Track the important camera parameters
  this->CameraList = new svtkCameraList;
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
svtkCameraInterpolator::~svtkCameraInterpolator()
{
  delete this->CameraList;

  this->SetPositionInterpolator(nullptr);
  this->SetFocalPointInterpolator(nullptr);
  this->SetViewUpInterpolator(nullptr);
  this->SetViewAngleInterpolator(nullptr);
  this->SetParallelScaleInterpolator(nullptr);
  this->SetClippingRangeInterpolator(nullptr);
}

//----------------------------------------------------------------------------
svtkMTimeType svtkCameraInterpolator::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType posMTime, fpMTime, vupMTime, vaMTime, psMTime, crMTime;

  if (this->PositionInterpolator)
  {
    posMTime = this->PositionInterpolator->GetMTime();
    mTime = (posMTime > mTime ? posMTime : mTime);
  }
  if (this->FocalPointInterpolator)
  {
    fpMTime = this->FocalPointInterpolator->GetMTime();
    mTime = (fpMTime > mTime ? fpMTime : mTime);
  }
  if (this->ViewUpInterpolator)
  {
    vupMTime = this->ViewUpInterpolator->GetMTime();
    mTime = (vupMTime > mTime ? vupMTime : mTime);
  }
  if (this->ViewAngleInterpolator)
  {
    vaMTime = this->ViewAngleInterpolator->GetMTime();
    mTime = (vaMTime > mTime ? vaMTime : mTime);
  }
  if (this->ParallelScaleInterpolator)
  {
    psMTime = this->ParallelScaleInterpolator->GetMTime();
    mTime = (psMTime > mTime ? psMTime : mTime);
  }
  if (this->ClippingRangeInterpolator)
  {
    crMTime = this->ClippingRangeInterpolator->GetMTime();
    mTime = (crMTime > mTime ? crMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int svtkCameraInterpolator::GetNumberOfCameras()
{
  return static_cast<int>(this->CameraList->size());
}

//----------------------------------------------------------------------------
double svtkCameraInterpolator::GetMinimumT()
{
  if (this->CameraList->empty())
  {
    return -SVTK_FLOAT_MAX;
  }
  else
  {
    return this->CameraList->front().Time;
  }
}

//----------------------------------------------------------------------------
double svtkCameraInterpolator::GetMaximumT()
{
  if (this->CameraList->empty())
  {
    return SVTK_FLOAT_MAX;
  }
  else
  {
    return this->CameraList->back().Time;
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::Initialize()
{
  this->CameraList->clear();
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::AddCamera(double t, svtkCamera* camera)
{
  int size = static_cast<int>(this->CameraList->size());

  // Check special cases: t at beginning or end of list
  if (size <= 0 || t < this->CameraList->front().Time)
  {
    this->CameraList->push_front(svtkICamera(t, camera));
    return;
  }
  else if (t > this->CameraList->back().Time)
  {
    this->CameraList->push_back(svtkICamera(t, camera));
    return;
  }
  else if (size == 1 && t == this->CameraList->back().Time)
  {
    this->CameraList->front() = svtkICamera(t, camera);
    return;
  }

  // Okay, insert in sorted order
  CameraListIterator iter = this->CameraList->begin();
  CameraListIterator nextIter = ++(this->CameraList->begin());
  for (int i = 0; i < (size - 1); i++, ++iter, ++nextIter)
  {
    if (t == iter->Time)
    {
      (*iter) = svtkICamera(t, camera);
    }
    else if (t > iter->Time && t < nextIter->Time)
    {
      this->CameraList->insert(nextIter, svtkICamera(t, camera));
    }
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::RemoveCamera(double t)
{
  if (t < this->CameraList->front().Time || t > this->CameraList->back().Time)
  {
    return;
  }

  CameraListIterator iter = this->CameraList->begin();
  for (; iter->Time != t && iter != this->CameraList->end(); ++iter)
  {
  }
  if (iter != this->CameraList->end())
  {
    this->CameraList->erase(iter);
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::SetPositionInterpolator(svtkTupleInterpolator* pi)
{
  if (this->PositionInterpolator != pi)
  {
    if (this->PositionInterpolator != nullptr)
    {
      this->PositionInterpolator->Delete();
    }
    this->PositionInterpolator = pi;
    if (this->PositionInterpolator != nullptr)
    {
      this->PositionInterpolator->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::SetFocalPointInterpolator(svtkTupleInterpolator* fpi)
{
  if (this->FocalPointInterpolator != fpi)
  {
    if (this->FocalPointInterpolator != nullptr)
    {
      this->FocalPointInterpolator->Delete();
    }
    this->FocalPointInterpolator = fpi;
    if (this->FocalPointInterpolator != nullptr)
    {
      this->FocalPointInterpolator->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::SetViewUpInterpolator(svtkTupleInterpolator* vupi)
{
  if (this->ViewUpInterpolator != vupi)
  {
    if (this->ViewUpInterpolator != nullptr)
    {
      this->ViewUpInterpolator->Delete();
    }
    this->ViewUpInterpolator = vupi;
    if (this->ViewUpInterpolator != nullptr)
    {
      this->ViewUpInterpolator->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::SetClippingRangeInterpolator(svtkTupleInterpolator* cri)
{
  if (this->ClippingRangeInterpolator != cri)
  {
    if (this->ClippingRangeInterpolator != nullptr)
    {
      this->ClippingRangeInterpolator->Delete();
    }
    this->ClippingRangeInterpolator = cri;
    if (this->ClippingRangeInterpolator != nullptr)
    {
      this->ClippingRangeInterpolator->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::SetParallelScaleInterpolator(svtkTupleInterpolator* psi)
{
  if (this->ParallelScaleInterpolator != psi)
  {
    if (this->ParallelScaleInterpolator != nullptr)
    {
      this->ParallelScaleInterpolator->Delete();
    }
    this->ParallelScaleInterpolator = psi;
    if (this->ParallelScaleInterpolator != nullptr)
    {
      this->ParallelScaleInterpolator->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::SetViewAngleInterpolator(svtkTupleInterpolator* vai)
{
  if (this->ViewAngleInterpolator != vai)
  {
    if (this->ViewAngleInterpolator != nullptr)
    {
      this->ViewAngleInterpolator->Delete();
    }
    this->ViewAngleInterpolator = vai;
    if (this->ViewAngleInterpolator != nullptr)
    {
      this->ViewAngleInterpolator->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::InitializeInterpolation()
{
  if (this->CameraList->empty())
  {
    return;
  }

  // Set up the interpolators if we need to
  if (!this->Initialized || this->GetMTime() > this->InitializeTime)
  {
    if (!this->PositionInterpolator)
    {
      this->PositionInterpolator = svtkTupleInterpolator::New();
    }
    if (!this->FocalPointInterpolator)
    {
      this->FocalPointInterpolator = svtkTupleInterpolator::New();
    }
    if (!this->ViewUpInterpolator)
    {
      this->ViewUpInterpolator = svtkTupleInterpolator::New();
    }
    if (!this->ClippingRangeInterpolator)
    {
      this->ClippingRangeInterpolator = svtkTupleInterpolator::New();
    }
    if (!this->ParallelScaleInterpolator)
    {
      this->ParallelScaleInterpolator = svtkTupleInterpolator::New();
    }
    if (!this->ViewAngleInterpolator)
    {
      this->ViewAngleInterpolator = svtkTupleInterpolator::New();
    }

    this->PositionInterpolator->Initialize();
    this->FocalPointInterpolator->Initialize();
    this->ViewUpInterpolator->Initialize();
    this->ClippingRangeInterpolator->Initialize();
    this->ParallelScaleInterpolator->Initialize();
    this->ViewAngleInterpolator->Initialize();

    this->PositionInterpolator->SetNumberOfComponents(3);
    this->FocalPointInterpolator->SetNumberOfComponents(3);
    this->ViewUpInterpolator->SetNumberOfComponents(3);
    this->ClippingRangeInterpolator->SetNumberOfComponents(2);
    this->ParallelScaleInterpolator->SetNumberOfComponents(1);
    this->ViewAngleInterpolator->SetNumberOfComponents(1);

    if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR)
    {
      this->PositionInterpolator->SetInterpolationTypeToLinear();
      this->FocalPointInterpolator->SetInterpolationTypeToLinear();
      this->ViewUpInterpolator->SetInterpolationTypeToLinear();
      this->ClippingRangeInterpolator->SetInterpolationTypeToLinear();
      this->ParallelScaleInterpolator->SetInterpolationTypeToLinear();
      this->ViewAngleInterpolator->SetInterpolationTypeToLinear();
    }
    else if (this->InterpolationType == INTERPOLATION_TYPE_SPLINE)
    {
      this->PositionInterpolator->SetInterpolationTypeToSpline();
      this->FocalPointInterpolator->SetInterpolationTypeToSpline();
      this->ViewUpInterpolator->SetInterpolationTypeToSpline();
      this->ClippingRangeInterpolator->SetInterpolationTypeToSpline();
      this->ParallelScaleInterpolator->SetInterpolationTypeToSpline();
      this->ViewAngleInterpolator->SetInterpolationTypeToSpline();
    }
    else
    {
      ; // manual override, user manipulates interpolators directly
    }

    // Okay, now we can load the interpolators with data
    CameraListIterator iter = this->CameraList->begin();
    for (; iter != this->CameraList->end(); ++iter)
    {
      this->PositionInterpolator->AddTuple(iter->Time, iter->P);
      this->FocalPointInterpolator->AddTuple(iter->Time, iter->FP);
      this->ViewUpInterpolator->AddTuple(iter->Time, iter->VUP);
      this->ClippingRangeInterpolator->AddTuple(iter->Time, iter->CR);
      this->ViewAngleInterpolator->AddTuple(iter->Time, iter->VA);
      this->ParallelScaleInterpolator->AddTuple(iter->Time, iter->PS);
    }

    this->Initialized = 1;
    this->InitializeTime.Modified();
  }
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::InterpolateCamera(double t, svtkCamera* camera)
{
  if (this->CameraList->empty())
  {
    return;
  }

  // Make sure the camera and this class are initialized properly
  this->InitializeInterpolation();

  // Evaluate the interpolators
  if (t < this->CameraList->front().Time)
  {
    t = this->CameraList->front().Time;
  }

  else if (t > this->CameraList->back().Time)
  {
    t = this->CameraList->back().Time;
  }

  double P[3], FP[3], VUP[3], CR[2], VA[1], PS[1];
  this->PositionInterpolator->InterpolateTuple(t, P);
  this->FocalPointInterpolator->InterpolateTuple(t, FP);
  this->ViewUpInterpolator->InterpolateTuple(t, VUP);
  this->ClippingRangeInterpolator->InterpolateTuple(t, CR);
  this->ViewAngleInterpolator->InterpolateTuple(t, VA);
  this->ParallelScaleInterpolator->InterpolateTuple(t, PS);

  camera->SetPosition(P);
  camera->SetFocalPoint(FP);
  camera->SetViewUp(VUP);
  camera->SetClippingRange(CR);
  camera->SetViewAngle(VA[0]);
  camera->SetParallelScale(PS[0]);
}

//----------------------------------------------------------------------------
void svtkCameraInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "There are " << this->GetNumberOfCameras() << " cameras to be interpolated\n";

  os << indent << "Interpolation Type: ";
  if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR)
  {
    os << "Linear\n";
  }
  else if (this->InterpolationType == INTERPOLATION_TYPE_SPLINE)
  {
    os << "Spline\n";
  }
  else // if ( this->InterpolationType == INTERPOLATION_TYPE_MANUAL )
  {
    os << "Manual\n";
  }

  os << indent << "Position Interpolator: ";
  if (this->PositionInterpolator)
  {
    os << this->PositionInterpolator << "\n";
  }
  else
  {
    os << "(null)\n";
  }

  os << indent << "Focal Point Interpolator: ";
  if (this->FocalPointInterpolator)
  {
    os << this->FocalPointInterpolator << "\n";
  }
  else
  {
    os << "(null)\n";
  }

  os << indent << "View Up Interpolator: ";
  if (this->ViewUpInterpolator)
  {
    os << this->ViewUpInterpolator << "\n";
  }
  else
  {
    os << "(null)\n";
  }

  os << indent << "Clipping Range Interpolator: ";
  if (this->ClippingRangeInterpolator)
  {
    os << this->ClippingRangeInterpolator << "\n";
  }
  else
  {
    os << "(null)\n";
  }

  os << indent << "View Angle Interpolator: ";
  if (this->ViewAngleInterpolator)
  {
    os << this->ViewAngleInterpolator << "\n";
  }
  else
  {
    os << "(null)\n";
  }

  os << indent << "Parallel Scale Interpolator: ";
  if (this->ParallelScaleInterpolator)
  {
    os << this->ParallelScaleInterpolator << "\n";
  }
  else
  {
    os << "(null)\n";
  }
}
