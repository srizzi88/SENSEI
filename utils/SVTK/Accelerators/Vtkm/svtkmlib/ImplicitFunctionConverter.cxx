//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "ImplicitFunctionConverter.h"

#include "svtkmFilterPolicy.h"

#include "svtkBox.h"
#include "svtkCylinder.h"
#include "svtkPlane.h"
#include "svtkSphere.h"

#include <svtkm/ImplicitFunction.h>

namespace tosvtkm
{

inline svtkm::Vec<svtkm::FloatDefault, 3> MakeFVec3(const double x[3])
{
  return svtkm::Vec<svtkm::FloatDefault, 3>(static_cast<svtkm::FloatDefault>(x[0]),
    static_cast<svtkm::FloatDefault>(x[1]), static_cast<svtkm::FloatDefault>(x[2]));
}

ImplicitFunctionConverter::ImplicitFunctionConverter()
  : InFunction(nullptr)
  , OutFunction()
  , MTime(0)
{
}

void ImplicitFunctionConverter::Set(svtkImplicitFunction* function)
{
  svtkBox* box = nullptr;
  svtkCylinder* cylinder = nullptr;
  svtkPlane* plane = nullptr;
  svtkSphere* sphere = nullptr;

  if ((box = svtkBox::SafeDownCast(function)))
  {
    double xmin[3], xmax[3];
    box->GetXMin(xmin);
    box->GetXMax(xmax);

    auto b = new svtkm::Box(MakeFVec3(xmin), MakeFVec3(xmax));
    this->OutFunction.Reset(b, true);
  }
  else if ((cylinder = svtkCylinder::SafeDownCast(function)))
  {
    double center[3], axis[3], radius;
    cylinder->GetCenter(center);
    cylinder->GetAxis(axis);
    radius = cylinder->GetRadius();

    auto c = new svtkm::Cylinder(
      MakeFVec3(center), MakeFVec3(axis), static_cast<svtkm::FloatDefault>(radius));
    this->OutFunction.Reset(c, true);
  }
  else if ((plane = svtkPlane::SafeDownCast(function)))
  {
    double origin[3], normal[3];
    plane->GetOrigin(origin);
    plane->GetNormal(normal);

    auto p = new svtkm::Plane(MakeFVec3(origin), MakeFVec3(normal));
    this->OutFunction.Reset(p, true);
  }
  else if ((sphere = svtkSphere::SafeDownCast(function)))
  {
    double center[3], radius;
    sphere->GetCenter(center);
    radius = sphere->GetRadius();

    auto s = new svtkm::Sphere(MakeFVec3(center), static_cast<svtkm::FloatDefault>(radius));
    this->OutFunction.Reset(s, true);
  }
  else
  {
    svtkGenericWarningMacro(<< "The implicit functions " << function->GetClassName()
                           << " is not supported by svtk-m.");
    return;
  }

  this->MTime = function->GetMTime();
  this->InFunction = function;
}

const svtkm::cont::ImplicitFunctionHandle& ImplicitFunctionConverter::Get() const
{
  if (this->InFunction && (this->MTime < this->InFunction->GetMTime()))
  {
    svtkBox* box = nullptr;
    svtkCylinder* cylinder = nullptr;
    svtkPlane* plane = nullptr;
    svtkSphere* sphere = nullptr;

    if ((box = svtkBox::SafeDownCast(this->InFunction)))
    {
      double xmin[3], xmax[3];
      box->GetXMin(xmin);
      box->GetXMax(xmax);

      auto b = static_cast<svtkm::Box*>(this->OutFunction.Get());
      b->SetMinPoint(MakeFVec3(xmin));
      b->SetMaxPoint(MakeFVec3(xmax));
    }
    else if ((cylinder = svtkCylinder::SafeDownCast(this->InFunction)))
    {
      double center[3], axis[3], radius;
      cylinder->GetCenter(center);
      cylinder->GetAxis(axis);
      radius = cylinder->GetRadius();

      auto c = static_cast<svtkm::Cylinder*>(this->OutFunction.Get());
      c->SetCenter(MakeFVec3(center));
      c->SetAxis(MakeFVec3(axis));
      c->SetRadius(static_cast<svtkm::FloatDefault>(radius));
    }
    else if ((plane = svtkPlane::SafeDownCast(this->InFunction)))
    {
      double origin[3], normal[3];
      plane->GetOrigin(origin);
      plane->GetNormal(normal);

      auto p = static_cast<svtkm::Plane*>(this->OutFunction.Get());
      p->SetOrigin(MakeFVec3(origin));
      p->SetNormal(MakeFVec3(normal));
    }
    else if ((sphere = svtkSphere::SafeDownCast(this->InFunction)))
    {
      double center[3], radius;
      sphere->GetCenter(center);
      radius = sphere->GetRadius();

      auto s = static_cast<svtkm::Sphere*>(this->OutFunction.Get());
      s->SetCenter(MakeFVec3(center));
      s->SetRadius(static_cast<svtkm::FloatDefault>(radius));
    }

    this->MTime = this->InFunction->GetMTime();
  }

  return this->OutFunction;
}

} // tosvtkm
