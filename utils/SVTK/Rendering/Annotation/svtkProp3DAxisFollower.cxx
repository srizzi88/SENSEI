/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp3DAxisFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProp3DAxisFollower.h"

#include "svtkAxisActor.h"
#include "svtkBoundingBox.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkProperty.h"
#include "svtkTexture.h"
#include "svtkTransform.h"
#include "svtkViewport.h"

#include <cmath>

svtkStandardNewMacro(svtkProp3DAxisFollower);

// List of vectors per axis (depending on which one needs to be
// followed.
// Order here is X, Y, and Z.
// Set of two axis aligned vectors that would define the Y vector.
// Order is MINMIN, MINMAX, MAXMAX, MAXMIN
namespace
{
const double AxisAlignedY[3][4][2][3] = {
  { { { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } }, { { 0.0, 1.0, 0.0 }, { 0.0, 0.0, -1.0 } },
    { { 0.0, -1.0, 0.0 }, { 0.0, 0.0, -1.0 } }, { { 0.0, -1.0, 0.0 }, { 0.0, 0.0, 1.0 } } },
  { { { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 } }, { { 1.0, 0.0, 0.0 }, { 0.0, 0.0, -1.0 } },
    { { -1.0, 0.0, 0.0 }, { 0.0, 0.0, -1.0 } }, { { -1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 } } },
  { { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 } }, { { 1.0, 0.0, 0.0 }, { 0.0, -1.0, 0.0 } },
    { { -1.0, 0.0, 0.0 }, { 0.0, -1.0, 0.0 } }, { { -1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 } } }
};
}
//----------------------------------------------------------------------
// Creates a follower with no camera set
svtkProp3DAxisFollower::svtkProp3DAxisFollower()
{
  this->AutoCenter = 1;

  this->EnableDistanceLOD = 0;
  this->DistanceLODThreshold = 0.80;

  this->EnableViewAngleLOD = 1;
  this->ViewAngleLODThreshold = 0.34;

  this->ScreenOffsetVector[0] = 0.0;
  this->ScreenOffsetVector[1] = 10.0;

  this->Axis = nullptr;
  this->Viewport = nullptr;

  this->TextUpsideDown = -1;
  this->VisibleAtCurrentViewAngle = -1;
}

//----------------------------------------------------------------------
svtkProp3DAxisFollower::~svtkProp3DAxisFollower() = default;

//----------------------------------------------------------------------
void svtkProp3DAxisFollower::SetAxis(svtkAxisActor* axis)
{
  if (!axis)
  {
    svtkErrorMacro("Invalid or nullptr axis\n");
    return;
  }

  if (this->Axis != axis)
  {
    // \NOTE: Don't increment the ref count of axis as it could lead to
    // circular references.
    this->Axis = axis;
    this->Modified();
  }
}

//----------------------------------------------------------------------
svtkAxisActor* svtkProp3DAxisFollower::GetAxis()
{
  return this->Axis;
}

//----------------------------------------------------------------------
void svtkProp3DAxisFollower::SetViewport(svtkViewport* vp)
{
  if (this->Viewport != vp)
  {
    // \NOTE: Don't increment the ref count of svtkViewport as it could lead to
    // circular references.
    this->Viewport = vp;
    this->Modified();
  }
}

//----------------------------------------------------------------------
svtkViewport* svtkProp3DAxisFollower::GetViewport()
{
  return this->Viewport;
}

//----------------------------------------------------------------------------
void svtkProp3DAxisFollower::CalculateOrthogonalVectors(
  double rX[3], double rY[3], double rZ[3], svtkAxisActor* axis, double* dop, svtkViewport* viewport)
{
  if (!rX || !rY || !rZ)
  {
    svtkErrorMacro("Invalid or nullptr direction vectors\n");
    return;
  }

  if (!axis)
  {
    svtkErrorMacro("Invalid or nullptr axis\n");
    return;
  }

  if (!dop)
  {
    svtkErrorMacro("Invalid or nullptr direction of projection vector\n");
    return;
  }

  if (!viewport)
  {
    svtkErrorMacro("Invalid or nullptr renderer\n");
    return;
  }

  svtkMatrix4x4* cameraMatrix = this->Camera->GetViewTransformMatrix();

  svtkCoordinate* c1Axis = axis->GetPoint1Coordinate();
  svtkCoordinate* c2Axis = axis->GetPoint2Coordinate();
  double* axisPt1 = c1Axis->GetComputedWorldValue(viewport);
  double* axisPt2 = c2Axis->GetComputedWorldValue(viewport);

  rX[0] = axisPt2[0] - axisPt1[0];
  rX[1] = axisPt2[1] - axisPt1[1];
  rX[2] = axisPt2[2] - axisPt1[2];
  svtkMath::Normalize(rX);

  if (rX[0] != dop[0] || rX[1] != dop[1] || rX[2] != dop[2])
  {
    // Get Y
    svtkMath::Cross(rX, dop, rY);
    svtkMath::Normalize(rY);

    // Get Z
    svtkMath::Cross(rX, rY, rZ);
    svtkMath::Normalize(rZ);
  }
  else
  {
    svtkMath::Perpendiculars(rX, rY, rZ, 0.);
  }
  double a[3], b[3];

  // Need homogeneous points.
  double homoPt1[4] = { axisPt1[0], axisPt1[1], axisPt1[2], 1.0 };
  double homoPt2[4] = { axisPt2[0], axisPt2[1], axisPt2[2], 1.0 };

  double* viewCoordinatePt1 = cameraMatrix->MultiplyDoublePoint(homoPt1);
  a[0] = viewCoordinatePt1[0];
  a[1] = viewCoordinatePt1[1];
  a[2] = viewCoordinatePt1[2];

  double* viewCoordinatePt2 = cameraMatrix->MultiplyDoublePoint(homoPt2);
  b[0] = viewCoordinatePt2[0];
  b[1] = viewCoordinatePt2[1];
  b[2] = viewCoordinatePt2[2];

  // If the text is upside down, we make a 180 rotation to keep it readable.
  if (this->IsTextUpsideDown(a, b))
  {
    this->TextUpsideDown = 1;
    rX[0] = -rX[0];
    rX[1] = -rX[1];
    rX[2] = -rX[2];
    rZ[0] = -rZ[0];
    rZ[1] = -rZ[1];
    rZ[2] = -rZ[2];
  }
  else
  {
    this->TextUpsideDown = 0;
  }
}

//----------------------------------------------------------------------------
double svtkProp3DAxisFollower::AutoScale(
  svtkViewport* viewport, svtkCamera* camera, double screenSize, double position[3])
{
  double newScale = 0.0;

  if (!viewport)
  {
    std::cerr << "Invalid or nullptr viewport \n";
    return newScale;
  }

  if (!camera)
  {
    std::cerr << "Invalid or nullptr camera \n";
    return newScale;
  }

  if (!position)
  {
    std::cerr << "Invalid or nullptr position \n";
    return newScale;
  }

  double factor = 1;
  if (viewport->GetSize()[1] > 0)
  {
    factor = 2.0 * screenSize * tan(svtkMath::RadiansFromDegrees(camera->GetViewAngle() / 2.0)) /
      viewport->GetSize()[1];
  }

  double dist = sqrt(svtkMath::Distance2BetweenPoints(position, camera->GetPosition()));
  newScale = factor * dist;

  return newScale;
}

//----------------------------------------------------------------------------
void svtkProp3DAxisFollower::ComputeMatrix()
{
  if (!this->Axis)
  {
    svtkErrorMacro("ERROR: Invalid axis\n");
    return;
  }

  if (this->EnableDistanceLOD && !this->TestDistanceVisibility())
  {
    this->SetVisibility(0);
    return;
  }

  // check whether or not need to rebuild the matrix
  if (this->GetMTime() > this->MatrixMTime ||
    (this->Camera && this->Camera->GetMTime() > this->MatrixMTime))
  {
    this->GetOrientation();
    this->Transform->Push();
    this->Transform->Identity();
    this->Transform->PostMultiply();
    this->Transform->GetMatrix(this->Matrix);

    double pivotPoint[3] = { this->Origin[0], this->Origin[1], this->Origin[2] };

    if (this->AutoCenter)
    {
      // Don't apply the user matrix when retrieving the center.
      this->Device->SetUserMatrix(nullptr);

      double* center = this->Device->GetCenter();
      pivotPoint[0] = center[0];
      pivotPoint[1] = center[1];
      pivotPoint[2] = center[2];
    }

    // Move pivot point to origin
    this->Transform->Translate(-pivotPoint[0], -pivotPoint[1], -pivotPoint[2]);
    // Scale
    this->Transform->Scale(this->Scale[0], this->Scale[1], this->Scale[2]);

    // Rotate
    this->Transform->RotateY(this->Orientation[1]);
    this->Transform->RotateX(this->Orientation[0]);
    this->Transform->RotateZ(this->Orientation[2]);

    double translation[3] = { 0.0, 0.0, 0.0 };
    if (this->Axis)
    {
      svtkMatrix4x4* matrix = this->InternalMatrix;
      matrix->Identity();
      double rX[3], rY[3], rZ[3];

      this->ComputeRotationAndTranlation(this->Viewport, translation, rX, rY, rZ, this->Axis);

      svtkMath::Normalize(rX);
      svtkMath::Normalize(rY);
      svtkMath::Normalize(rZ);

      matrix->Element[0][0] = rX[0];
      matrix->Element[1][0] = rX[1];
      matrix->Element[2][0] = rX[2];
      matrix->Element[0][1] = rY[0];
      matrix->Element[1][1] = rY[1];
      matrix->Element[2][1] = rY[2];
      matrix->Element[0][2] = rZ[0];
      matrix->Element[1][2] = rZ[1];
      matrix->Element[2][2] = rZ[2];

      this->Transform->Concatenate(matrix);
    }

    this->Transform->Translate(this->Origin[0] + this->Position[0] + translation[0],
      this->Origin[1] + this->Position[1] + translation[1],
      this->Origin[2] + this->Position[2] + translation[2]);

    // Apply user defined matrix last if there is one
    if (this->UserMatrix)
    {
      this->Transform->Concatenate(this->UserMatrix);
    }

    this->Transform->PreMultiply();
    this->Transform->GetMatrix(this->Matrix);
    this->MatrixMTime.Modified();
    this->Transform->Pop();
  }

  this->SetVisibility(this->VisibleAtCurrentViewAngle);
}

//-----------------------------------------------------------------------------
void svtkProp3DAxisFollower ::ComputeRotationAndTranlation(svtkViewport* viewport,
  double translation[3], double rX[3], double rY[3], double rZ[3], svtkAxisActor* axis)
{
  double autoScaleHoriz =
    this->AutoScale(viewport, this->Camera, this->ScreenOffsetVector[0], this->Position);
  double autoScaleVert =
    this->AutoScale(viewport, this->Camera, this->ScreenOffsetVector[1], this->Position);

  double dop[3];
  this->Camera->GetDirectionOfProjection(dop);
  svtkMath::Normalize(dop);

  this->CalculateOrthogonalVectors(rX, rY, rZ, axis, dop, viewport);

  double dotVal = svtkMath::Dot(rZ, dop);

  double origRx[3] = { rX[0], rX[1], rX[2] };
  double origRy[3] = { rY[0], rY[1], rY[2] };

  // NOTE: Basically the idea here is that dotVal will be positive
  // only when we have projection direction aligned with our z directon
  // and when that happens it means that our Y is inverted.
  if (dotVal > 0)
  {
    rY[0] = -rY[0];
    rY[1] = -rY[1];
    rY[2] = -rY[2];
  }

  // Check visibility at current view angle.
  if (this->EnableViewAngleLOD)
  {
    this->ExecuteViewAngleVisibility(rZ);
  }

  // Since we already stored all the possible Y axes that are geometry aligned,
  // we compare our vertical vector with these vectors and if it aligns then we
  // translate in opposite direction.
  int axisPosition = this->Axis->GetAxisPosition();
  int vertSign;
  double vertDotVal1 =
    svtkMath::Dot(AxisAlignedY[this->Axis->GetAxisType()][axisPosition][0], origRy);
  double vertDotVal2 =
    svtkMath::Dot(AxisAlignedY[this->Axis->GetAxisType()][axisPosition][1], origRy);

  if (fabs(vertDotVal1) > fabs(vertDotVal2))
  {
    vertSign = (vertDotVal1 > 0 ? -1 : 1);
  }
  else
  {
    vertSign = (vertDotVal2 > 0 ? -1 : 1);
  }

  int horizSign = this->TextUpsideDown ? -1 : 1;
  translation[0] = origRy[0] * autoScaleVert * vertSign + origRx[0] * autoScaleHoriz * horizSign;
  translation[1] = origRy[1] * autoScaleVert * vertSign + origRx[1] * autoScaleHoriz * horizSign;
  translation[2] = origRy[2] * autoScaleVert * vertSign + origRx[2] * autoScaleHoriz * horizSign;
}

//----------------------------------------------------------------------
void svtkProp3DAxisFollower::ComputerAutoCenterTranslation(
  const double& svtkNotUsed(autoScaleFactor), double translation[3])
{
  if (!translation)
  {
    svtkErrorMacro("ERROR: Invalid or nullptr translation\n");
    return;
  }

  const double* bounds = this->GetProp3D()->GetBounds();

  // Offset by half of width.
  double halfWidth = (bounds[1] - bounds[0]) * 0.5 * this->Scale[0];

  if (this->TextUpsideDown == 1)
  {
    halfWidth = -halfWidth;
  }

  if (this->Axis->GetAxisType() == svtkAxisActor::SVTK_AXIS_TYPE_X)
  {
    translation[0] = translation[0] - halfWidth;
  }
  else if (this->Axis->GetAxisType() == svtkAxisActor::SVTK_AXIS_TYPE_Y)
  {
    translation[1] = translation[1] - halfWidth;
  }
  else if (this->Axis->GetAxisType() == svtkAxisActor::SVTK_AXIS_TYPE_Z)
  {
    translation[2] = translation[2] - halfWidth;
  }
  else
  {
    // Do nothing.
  }
}

//----------------------------------------------------------------------
int svtkProp3DAxisFollower::TestDistanceVisibility()
{
  if (!this->Camera->GetParallelProjection())
  {
    double cameraClippingRange[2];

    this->Camera->GetClippingRange(cameraClippingRange);

    // We are considering the far clip plane for evaluation. In certain
    // odd conditions it might not work.
    const double maxVisibleDistanceFromCamera =
      this->DistanceLODThreshold * (cameraClippingRange[1]);

    double dist =
      sqrt(svtkMath::Distance2BetweenPoints(this->Camera->GetPosition(), this->Position));

    if (dist > maxVisibleDistanceFromCamera)
    {
      // Need to make sure we are not looking at a flat axis and therefore should enable it anyway
      if (this->Axis)
      {
        svtkBoundingBox bbox(this->Axis->GetBounds());
        return (bbox.GetDiagonalLength() > (cameraClippingRange[1] - cameraClippingRange[0])) ? 1
                                                                                              : 0;
      }
      return 0;
    }
    else
    {
      return 1;
    }
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------
void svtkProp3DAxisFollower::ExecuteViewAngleVisibility(double normal[3])
{
  if (!normal)
  {
    svtkErrorMacro("ERROR: Invalid or nullptr normal\n");
    return;
  }

  double* cameraPos = this->Camera->GetPosition();
  double dir[3] = { this->Position[0] - cameraPos[0], this->Position[1] - cameraPos[1],
    this->Position[2] - cameraPos[2] };
  svtkMath::Normalize(dir);
  double dotDir = svtkMath::Dot(dir, normal);
  if (fabs(dotDir) < this->ViewAngleLODThreshold)
  {
    this->VisibleAtCurrentViewAngle = 0;
  }
  else
  {
    this->VisibleAtCurrentViewAngle = 1;
  }
}

//----------------------------------------------------------------------
void svtkProp3DAxisFollower::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AutoCenter: (" << this->AutoCenter << ")\n";
  os << indent << "EnableDistanceLOD: (" << this->EnableDistanceLOD << ")\n";
  os << indent << "DistanceLODThreshold: (" << this->DistanceLODThreshold << ")\n";
  os << indent << "EnableViewAngleLOD: (" << this->EnableViewAngleLOD << ")\n";
  os << indent << "ViewAngleLODThreshold: (" << this->ViewAngleLODThreshold << ")\n";
  os << indent << "ScreenOffsetVector: (" << this->ScreenOffsetVector[0] << " "
     << this->ScreenOffsetVector[1] << ")\n";

  if (this->Axis)
  {
    os << indent << "Axis: (" << this->Axis << ")\n";
  }
  else
  {
    os << indent << "Axis: (none)\n";
  }
}

//----------------------------------------------------------------------
void svtkProp3DAxisFollower::ShallowCopy(svtkProp* prop)
{
  svtkProp3DAxisFollower* f = svtkProp3DAxisFollower::SafeDownCast(prop);
  if (f != nullptr)
  {
    this->SetAutoCenter(f->GetAutoCenter());
    this->SetEnableDistanceLOD(f->GetEnableDistanceLOD());
    this->SetDistanceLODThreshold(f->GetDistanceLODThreshold());
    this->SetEnableViewAngleLOD(f->GetEnableViewAngleLOD());
    this->SetViewAngleLODThreshold(f->GetViewAngleLODThreshold());
    this->SetScreenOffsetVector(f->GetScreenOffsetVector());
    this->SetAxis(f->GetAxis());
  }

  // Now do superclass
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
bool svtkProp3DAxisFollower::IsTextUpsideDown(double* a, double* b)
{
  double angle = svtkMath::RadiansFromDegrees(this->Orientation[2]);
  return (b[0] - a[0]) * cos(angle) - (b[1] - a[1]) * sin(angle) < 0;
}

//----------------------------------------------------------------------
void svtkProp3DAxisFollower::SetScreenOffset(double offset)
{
  this->SetScreenOffsetVector(1, offset);
}

//----------------------------------------------------------------------
double svtkProp3DAxisFollower::GetScreenOffset()
{
  return this->GetScreenOffsetVector()[1];
}

//----------------------------------------------------------------------
int svtkProp3DAxisFollower::RenderOpaqueGeometry(svtkViewport* viewport)
{
  this->SetViewport(viewport);
  return this->Superclass::RenderOpaqueGeometry(viewport);
}

//----------------------------------------------------------------------
int svtkProp3DAxisFollower::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  this->SetViewport(viewport);
  return this->Superclass::RenderTranslucentPolygonalGeometry(viewport);
}

//----------------------------------------------------------------------
int svtkProp3DAxisFollower::RenderVolumetricGeometry(svtkViewport* viewport)
{
  this->SetViewport(viewport);
  return this->Superclass::RenderVolumetricGeometry(viewport);
}
