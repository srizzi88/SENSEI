/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContext3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContext3D.h"
#include "svtkContextDevice3D.h"
#include "svtkTransform.h"

#include "svtkObjectFactory.h"

#include <cassert>

svtkStandardNewMacro(svtkContext3D);

void svtkContext3D::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Context Device: ";
  if (this->Device)
  {
    os << endl;
    this->Device->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

bool svtkContext3D::Begin(svtkContextDevice3D* device)
{
  if (this->Device == device)
  {
    return true;
  }
  this->Device = device;
  return true;
}

svtkContextDevice3D* svtkContext3D::GetDevice()
{
  return this->Device;
}

bool svtkContext3D::End()
{
  if (this->Device)
  {
    this->Device = nullptr;
  }
  return true;
}

void svtkContext3D::DrawLine(const svtkVector3f& start, const svtkVector3f& end)
{
  assert(this->Device);
  svtkVector3f line[2] = { start, end };
  this->Device->DrawPoly(line[0].GetData(), 2);
}

void svtkContext3D::DrawPoly(const float* points, int n)
{
  assert(this->Device);
  this->Device->DrawPoly(points, n);
}

void svtkContext3D::DrawPoint(const svtkVector3f& point)
{
  assert(this->Device);
  this->Device->DrawPoints(point.GetData(), 1);
}

void svtkContext3D::DrawPoints(const float* points, int n)
{
  assert(this->Device);
  this->Device->DrawPoints(points, n);
}

void svtkContext3D::DrawPoints(const float* points, int n, unsigned char* colors, int nc_comps)
{
  assert(this->Device);
  this->Device->DrawPoints(points, n, colors, nc_comps);
}

void svtkContext3D::DrawTriangleMesh(const float* mesh, int n, const unsigned char* colors, int nc)
{
  assert(this->Device);
  this->Device->DrawTriangleMesh(mesh, n, colors, nc);
}

void svtkContext3D::ApplyPen(svtkPen* pen)
{
  assert(this->Device);
  this->Device->ApplyPen(pen);
}

void svtkContext3D::ApplyBrush(svtkBrush* brush)
{
  assert(this->Device);
  this->Device->ApplyBrush(brush);
}

void svtkContext3D::SetTransform(svtkTransform* transform)
{
  if (transform)
  {
    this->Device->SetMatrix(transform->GetMatrix());
  }
}

svtkTransform* svtkContext3D::GetTransform()
{
  if (this->Device && this->Transform)
  {
    this->Device->GetMatrix(this->Transform->GetMatrix());
    return this->Transform;
  }
  return nullptr;
}

void svtkContext3D::AppendTransform(svtkTransform* transform)
{
  if (transform)
  {
    this->Device->MultiplyMatrix(transform->GetMatrix());
  }
}

void svtkContext3D::PushMatrix()
{
  this->Device->PushMatrix();
}

void svtkContext3D::PopMatrix()
{
  this->Device->PopMatrix();
}

void svtkContext3D::EnableClippingPlane(int i, double* planeEquation)
{
  assert(this->Device);
  this->Device->EnableClippingPlane(i, planeEquation);
}

void svtkContext3D::DisableClippingPlane(int i)
{
  assert(this->Device);
  this->Device->DisableClippingPlane(i);
}

svtkContext3D::svtkContext3D() = default;

svtkContext3D::~svtkContext3D() = default;
