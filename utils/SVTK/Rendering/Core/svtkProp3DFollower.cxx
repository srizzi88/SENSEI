/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp3DFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProp3DFollower.h"

#include "svtkAssemblyPaths.h"
#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkTexture.h"
#include "svtkTransform.h"

#include <cmath>

svtkStandardNewMacro(svtkProp3DFollower);

svtkCxxSetObjectMacro(svtkProp3DFollower, Camera, svtkCamera);

//----------------------------------------------------------------------
// Creates a follower with no camera set
svtkProp3DFollower::svtkProp3DFollower()
{
  this->Camera = nullptr;
  this->Device = nullptr;

  this->InternalMatrix = svtkMatrix4x4::New();
}

//----------------------------------------------------------------------
svtkProp3DFollower::~svtkProp3DFollower()
{
  if (this->Camera)
  {
    this->Camera->UnRegister(this);
  }

  if (this->Device)
  {
    this->Device->Delete();
  }

  this->InternalMatrix->Delete();
}

//----------------------------------------------------------------------------
void svtkProp3DFollower::SetProp3D(svtkProp3D* prop)
{
  if (this->Device != prop)
  {
    if (this->Device != nullptr)
    {
      this->Device->Delete();
    }
    this->Device = prop;
    if (this->Device != nullptr)
    {
      this->Device->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkProp3D* svtkProp3DFollower::GetProp3D()
{
  return this->Device;
}

//----------------------------------------------------------------------------
void svtkProp3DFollower::ComputeMatrix()
{
  if (this->GetMTime() > this->MatrixMTime ||
    (this->Camera && this->Camera->GetMTime() > this->MatrixMTime))
  {
    this->GetOrientation();
    this->Transform->Push();
    this->Transform->Identity();
    this->Transform->PostMultiply();

    this->Transform->Translate(-this->Origin[0], -this->Origin[1], -this->Origin[2]);
    // scale
    this->Transform->Scale(this->Scale[0], this->Scale[1], this->Scale[2]);

    // rotate
    this->Transform->RotateY(this->Orientation[1]);
    this->Transform->RotateX(this->Orientation[0]);
    this->Transform->RotateZ(this->Orientation[2]);

    if (this->Camera)
    {
      double *pos, *vup, distance;
      double Rx[3], Ry[3], Rz[3];

      svtkMatrix4x4* matrix = this->InternalMatrix;
      matrix->Identity();

      // do the rotation
      // first rotate y
      pos = this->Camera->GetPosition();
      vup = this->Camera->GetViewUp();

      if (this->Camera->GetParallelProjection())
      {
        this->Camera->GetDirectionOfProjection(Rz);
        Rz[0] = -Rz[0];
        Rz[1] = -Rz[1];
        Rz[2] = -Rz[2];
      }
      else
      {
        distance = sqrt((pos[0] - this->Position[0]) * (pos[0] - this->Position[0]) +
          (pos[1] - this->Position[1]) * (pos[1] - this->Position[1]) +
          (pos[2] - this->Position[2]) * (pos[2] - this->Position[2]));
        for (int i = 0; i < 3; i++)
        {
          Rz[i] = (pos[i] - this->Position[i]) / distance;
        }
      }

      // instead use the view right angle:
      double dop[3], vur[3];
      this->Camera->GetDirectionOfProjection(dop);

      svtkMath::Cross(dop, vup, vur);
      svtkMath::Normalize(vur);

      svtkMath::Cross(Rz, vur, Ry);
      svtkMath::Normalize(Ry);
      svtkMath::Cross(Ry, Rz, Rx);

      matrix->Element[0][0] = Rx[0];
      matrix->Element[1][0] = Rx[1];
      matrix->Element[2][0] = Rx[2];
      matrix->Element[0][1] = Ry[0];
      matrix->Element[1][1] = Ry[1];
      matrix->Element[2][1] = Ry[2];
      matrix->Element[0][2] = Rz[0];
      matrix->Element[1][2] = Rz[1];
      matrix->Element[2][2] = Rz[2];

      this->Transform->Concatenate(matrix);
    }

    // translate to projection reference point PRP
    // this is the camera's position blasted through
    // the current matrix
    this->Transform->Translate(this->Origin[0] + this->Position[0],
      this->Origin[1] + this->Position[1], this->Origin[2] + this->Position[2]);

    // apply user defined matrix last if there is one
    if (this->UserMatrix)
    {
      this->Transform->Concatenate(this->UserMatrix);
    }

    this->Transform->PreMultiply();
    this->Transform->GetMatrix(this->Matrix);
    this->MatrixMTime.Modified();
    this->Transform->Pop();
  }
}

//-----------------------------------------------------------------------------
double* svtkProp3DFollower::GetBounds()
{
  if (this->Device)
  {
    this->ComputeMatrix();
    this->Device->SetUserMatrix(this->Matrix);
    return this->Device->GetBounds();
  }
  else
  {
    return nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkProp3DFollower::ReleaseGraphicsResources(svtkWindow* w)
{
  if (this->Device)
  {
    this->Device->ReleaseGraphicsResources(w);
  }
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkProp3DFollower::HasTranslucentPolygonalGeometry()
{
  if (this->Device)
  {
    return this->Device->HasTranslucentPolygonalGeometry();
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------
int svtkProp3DFollower::RenderOpaqueGeometry(svtkViewport* vp)
{
  if (this->Device)
  {
    this->ComputeMatrix();
    this->Device->SetUserMatrix(this->Matrix);
    if (this->GetPropertyKeys())
    {
      this->Device->SetPropertyKeys(this->GetPropertyKeys());
    }
    if (this->GetVisibility())
    {
      return this->Device->RenderOpaqueGeometry(vp);
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
int svtkProp3DFollower::RenderTranslucentPolygonalGeometry(svtkViewport* vp)
{
  if (this->Device)
  {
    this->ComputeMatrix();
    this->Device->SetUserMatrix(this->Matrix);
    if (this->GetPropertyKeys())
    {
      this->Device->SetPropertyKeys(this->GetPropertyKeys());
    }
    if (this->GetVisibility())
    {
      return this->Device->RenderTranslucentPolygonalGeometry(vp);
    }
  }
  return 0;
}

//----------------------------------------------------------------------
int svtkProp3DFollower::RenderVolumetricGeometry(svtkViewport* vp)
{
  if (this->Device)
  {
    this->ComputeMatrix();
    this->Device->SetUserMatrix(this->Matrix);
    if (this->GetPropertyKeys())
    {
      this->Device->SetPropertyKeys(this->GetPropertyKeys());
    }
    if (this->GetVisibility())
    {
      return this->Device->RenderVolumetricGeometry(vp);
    }
  }
  return 0;
}

//----------------------------------------------------------------------
void svtkProp3DFollower::ShallowCopy(svtkProp* prop)
{
  svtkProp3DFollower* f = svtkProp3DFollower::SafeDownCast(prop);
  if (f != nullptr)
  {
    this->SetCamera(f->GetCamera());
  }

  // Now do superclass
  this->svtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void svtkProp3DFollower::InitPathTraversal()
{
  if (this->Device)
  {
    this->Device->InitPathTraversal();
  }
}

//----------------------------------------------------------------------------
svtkAssemblyPath* svtkProp3DFollower::GetNextPath()
{
  if (this->Device)
  {
    return this->Device->GetNextPath();
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------
void svtkProp3DFollower::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Camera)
  {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Camera: (none)\n";
  }
}
