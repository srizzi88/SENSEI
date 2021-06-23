//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_Wavelets_h
#define svtk_m_worklet_Wavelets_h

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/Math.h>

namespace svtkm
{
namespace worklet
{
namespace wavelets
{

enum DWTMode
{ // boundary extension modes
  SYMH,
  SYMW,
  ASYMH,
  ASYMW
};

enum ExtensionDirection
{         // which side of a cube to extend
  LEFT,   // X direction
  RIGHT,  // X direction     Y
  TOP,    // Y direction     |   Z
  BOTTOM, // Y direction     |  /
  FRONT,  // Z direction     | /
  BACK    // Z direction     |/________ X
};

// Worklet for 3D signal extension
// It operates on a specified part of a big cube
class ExtensionWorklet3D : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension
                                WholeArrayIn); // signal
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  ExtensionWorklet3D(svtkm::Id extdimX,
                     svtkm::Id extdimY,
                     svtkm::Id extdimZ,
                     svtkm::Id sigdimX,
                     svtkm::Id sigdimY,
                     svtkm::Id sigdimZ,
                     svtkm::Id sigstartX,
                     svtkm::Id sigstartY,
                     svtkm::Id sigstartZ,
                     svtkm::Id sigpretendX,
                     svtkm::Id sigpretendY,
                     svtkm::Id sigpretendZ,
                     DWTMode m, // SYMH, SYMW, etc.
                     ExtensionDirection dir,
                     bool pad_zero)
    : extDimX(extdimX)
    , extDimY(extdimY)
    , extDimZ(extdimZ)
    , sigDimX(sigdimX)
    , sigDimY(sigdimY)
    , sigDimZ(sigdimZ)
    , sigStartX(sigstartX)
    , sigStartY(sigstartY)
    , sigStartZ(sigstartZ)
    , sigPretendDimX(sigpretendX)
    , sigPretendDimY(sigpretendY)
    , sigPretendDimZ(sigpretendZ)
    , mode(m)
    , direction(dir)
    , padZero(pad_zero)
  {
    (void)sigDimZ;
  }

  // Index translation helper
  SVTKM_EXEC_CONT
  void Ext1Dto3D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (extDimX * extDimY);
    y = (idx - z * extDimX * extDimY) / extDimX;
    x = idx % extDimX;
  }

  // Index translation helper
  SVTKM_EXEC_CONT
  svtkm::Id Sig3Dto1D(svtkm::Id x, svtkm::Id y, svtkm::Id z) const
  {
    return z * sigDimX * sigDimY + y * sigDimX + x;
  }

  // Index translation helper
  SVTKM_EXEC_CONT
  svtkm::Id SigPretend3Dto1D(svtkm::Id x, svtkm::Id y, svtkm::Id z) const
  {
    return (z + sigStartZ) * sigDimX * sigDimY + (y + sigStartY) * sigDimX + x + sigStartX;
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const svtkm::Id& workIndex) const
  {
    svtkm::Id extX, extY, extZ;
    svtkm::Id sigPretendX, sigPretendY, sigPretendZ;
    Ext1Dto3D(workIndex, extX, extY, extZ);
    typename PortalOutType::ValueType sym = 1.0;
    if (mode == ASYMH || mode == ASYMW)
    {
      sym = -1.0;
    }
    if (direction == LEFT)
    {
      sigPretendY = extY;
      sigPretendZ = extZ;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendX = extDimX - extX - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendX = extDimX - extX;
      }
    }
    else if (direction == RIGHT)
    {
      sigPretendY = extY;
      sigPretendZ = extZ;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendX = sigPretendDimX - extX - 1;
      }
      else
      {
        sigPretendX = sigPretendDimX - extX - 2;
      }
      if (padZero)
      {
        sigPretendX++;
      }
    }
    else if (direction == TOP)
    {
      sigPretendX = extX;
      sigPretendZ = extZ;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendY = extDimY - extY - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendY = extDimY - extY;
      }
    }
    else if (direction == BOTTOM)
    {
      sigPretendX = extX;
      sigPretendZ = extZ;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendY = sigPretendDimY - extY - 1;
      }
      else
      {
        sigPretendY = sigPretendDimY - extY - 2;
      }
      if (padZero)
      {
        sigPretendY++;
      }
    }
    else if (direction == FRONT)
    {
      sigPretendX = extX;
      sigPretendY = extY;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendZ = extDimZ - extZ - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendZ = extDimZ - extZ;
      }
    }
    else if (direction == BACK)
    {
      sigPretendX = extX;
      sigPretendY = extY;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendZ = sigPretendDimZ - extZ - 1;
      }
      else
      {
        sigPretendZ = sigPretendDimZ - extZ - 2;
      }
      if (padZero)
      {
        sigPretendZ++;
      }
    }
    else
    {
      sigPretendX = sigPretendY = sigPretendZ = 0; // so the compiler doesn't complain
    }

    if (sigPretendX == sigPretendDimX || // decides to pad a zero
        sigPretendY == sigPretendDimY ||
        sigPretendZ == sigPretendDimZ)
    {
      portalOut.Set(workIndex, 0.0);
    }
    else
    {
      portalOut.Set(workIndex,
                    sym * portalIn.Get(SigPretend3Dto1D(sigPretendX, sigPretendY, sigPretendZ)));
    }
  }

private:
  const svtkm::Id extDimX, extDimY, extDimZ, sigDimX, sigDimY, sigDimZ;
  const svtkm::Id sigStartX, sigStartY, sigStartZ;                // defines a small cube to work on
  const svtkm::Id sigPretendDimX, sigPretendDimY, sigPretendDimZ; // small cube dims
  const DWTMode mode;
  const ExtensionDirection direction;
  const bool padZero; // treat sigIn as having a zero at the end
};

//=============================================================================

//  Y
//
//  |      Z
//  |     /
//  |    /
//  |   /
//  |  /
//  | /
//  |/------------- X
//
// The following 3 classes perform the same functionaliry in 3 directions
class IndexTranslator3CubesLeftRight
{
public:
  IndexTranslator3CubesLeftRight(svtkm::Id x_1,
                                 svtkm::Id y_1,
                                 svtkm::Id z_1,
                                 svtkm::Id x_2,
                                 svtkm::Id y_2,
                                 svtkm::Id z_2,
                                 svtkm::Id startx_2,
                                 svtkm::Id starty_2,
                                 svtkm::Id startz_2,
                                 svtkm::Id pretendx_2,
                                 svtkm::Id pretendy_2,
                                 svtkm::Id pretendz_2,
                                 svtkm::Id x_3,
                                 svtkm::Id y_3,
                                 svtkm::Id z_3)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , startX2(startx_2)
    , startY2(starty_2)
    , startZ2(startz_2)
    , pretendDimX2(pretendx_2)
    , pretendDimY2(pretendy_2)
    , pretendDimZ2(pretendz_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
  {
    (void)dimZ1;
    (void)dimZ2;
    (void)dimZ3;
    (void)pretendDimY2;
    (void)pretendDimZ2;
  }

  SVTKM_EXEC_CONT
  void Translate3Dto1D(svtkm::Id inX,
                       svtkm::Id inY,
                       svtkm::Id inZ, // 3D indices as input
                       svtkm::Id& cube,
                       svtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimX1 <= inX && inX < (dimX1 + pretendDimX2))
    {
      svtkm::Id inX_local = inX - dimX1;
      cube = 2;
      idx = (inZ + startZ2) * dimX2 * dimY2 + (inY + startY2) * dimX2 + (inX_local + startX2);
    }
    else if (0 <= inX && inX < dimX1)
    {
      cube = 1;
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimX1 + pretendDimX2) <= inX && inX < (dimX1 + pretendDimX2 + dimX3))
    {
      svtkm::Id inX_local = inX - dimX1 - pretendDimX2;
      cube = 3;
      idx = inZ * dimX3 * dimY3 + inY * dimX3 + inX_local;
    }
  }

private:
  const svtkm::Id dimX1, dimY1, dimZ1; // left extension
  const svtkm::Id dimX2, dimY2, dimZ2; // actual signal dims
  const svtkm::Id startX2, startY2, startZ2, pretendDimX2, pretendDimY2, pretendDimZ2;
  const svtkm::Id dimX3, dimY3, dimZ3; // right extension
};

class IndexTranslator3CubesTopDown
{
public:
  IndexTranslator3CubesTopDown(svtkm::Id x_1,
                               svtkm::Id y_1,
                               svtkm::Id z_1,
                               svtkm::Id x_2,
                               svtkm::Id y_2,
                               svtkm::Id z_2,
                               svtkm::Id startx_2,
                               svtkm::Id starty_2,
                               svtkm::Id startz_2,
                               svtkm::Id pretendx_2,
                               svtkm::Id pretendy_2,
                               svtkm::Id pretendz_2,
                               svtkm::Id x_3,
                               svtkm::Id y_3,
                               svtkm::Id z_3)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , startX2(startx_2)
    , startY2(starty_2)
    , startZ2(startz_2)
    , pretendDimX2(pretendx_2)
    , pretendDimY2(pretendy_2)
    , pretendDimZ2(pretendz_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
  {
    (void)dimZ1;
    (void)dimZ2;
    (void)dimZ3;
    (void)pretendDimX2;
    (void)pretendDimZ2;
  }

  SVTKM_EXEC_CONT
  void Translate3Dto1D(svtkm::Id inX,
                       svtkm::Id inY,
                       svtkm::Id inZ, // 3D indices as input
                       svtkm::Id& cube,
                       svtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimY1 <= inY && inY < (dimY1 + pretendDimY2))
    {
      svtkm::Id inY_local = inY - dimY1;
      cube = 2;
      idx = (inZ + startZ2) * dimX2 * dimY2 + (inY_local + startY2) * dimX2 + inX + startX2;
    }
    else if (0 <= inY && inY < dimY1)
    {
      cube = 1;
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimY1 + pretendDimY2) <= inY && inY < (dimY1 + pretendDimY2 + dimY3))
    {
      svtkm::Id inY_local = inY - dimY1 - pretendDimY2;
      cube = 3;
      idx = inZ * dimX3 * dimY3 + inY_local * dimX3 + inX;
    }
  }

private:
  const svtkm::Id dimX1, dimY1, dimZ1; // left extension
  const svtkm::Id dimX2, dimY2, dimZ2; // actual signal dims
  const svtkm::Id startX2, startY2, startZ2, pretendDimX2, pretendDimY2, pretendDimZ2;
  const svtkm::Id dimX3, dimY3, dimZ3; // right extension
};

class IndexTranslator3CubesFrontBack
{
public:
  IndexTranslator3CubesFrontBack(svtkm::Id x_1,
                                 svtkm::Id y_1,
                                 svtkm::Id z_1,
                                 svtkm::Id x_2,
                                 svtkm::Id y_2,
                                 svtkm::Id z_2,
                                 svtkm::Id startx_2,
                                 svtkm::Id starty_2,
                                 svtkm::Id startz_2,
                                 svtkm::Id pretendx_2,
                                 svtkm::Id pretendy_2,
                                 svtkm::Id pretendz_2,
                                 svtkm::Id x_3,
                                 svtkm::Id y_3,
                                 svtkm::Id z_3)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , startX2(startx_2)
    , startY2(starty_2)
    , startZ2(startz_2)
    , pretendDimX2(pretendx_2)
    , pretendDimY2(pretendy_2)
    , pretendDimZ2(pretendz_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
  {
    (void)dimZ2;
    (void)pretendDimX2;
    (void)pretendDimY2;
  }

  SVTKM_EXEC_CONT
  void Translate3Dto1D(svtkm::Id inX,
                       svtkm::Id inY,
                       svtkm::Id inZ, // 3D indices as input
                       svtkm::Id& cube,
                       svtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimZ1 <= inZ && inZ < (dimZ1 + pretendDimZ2))
    {
      svtkm::Id inZ_local = inZ - dimZ1;
      cube = 2;
      idx = (inZ_local + startZ2) * dimX2 * dimY2 + (inY + startY2) * dimX2 + inX + startX2;
    }
    else if (0 <= inZ && inZ < dimZ1)
    {
      cube = 1;
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimZ1 + pretendDimZ2) <= inZ && inZ < (dimZ1 + pretendDimZ2 + dimZ3))
    {
      svtkm::Id inZ_local = inZ - dimZ1 - pretendDimZ2;
      cube = 3;
      idx = inZ_local * dimX3 * dimY3 + inY * dimX3 + inX;
    }
  }

private:
  const svtkm::Id dimX1, dimY1, dimZ1; // left extension
  const svtkm::Id dimX2, dimY2, dimZ2; // actual signal dims
  const svtkm::Id startX2, startY2, startZ2, pretendDimX2, pretendDimY2, pretendDimZ2;
  const svtkm::Id dimX3, dimY3, dimZ3; // right extension
};

//=============================================================================

//
//  ---------------------------------------------------
//  |      |          |      |      |          |      |
//  |cube1 |  cube5   |cube2 |cube3 |  cube5   |cube4 |
//  | ext1 |    cA    | ext2 | ext3 |    cD    | ext4 |
//  | (x1) |   (xa)   | (x2) | (x3) |   (xd)   | (x4) |
//  |      |          |      |      |          |      |
//  ----------------------------------------------------
// The following 3 classes perform the same functionaliry in 3 directions
class IndexTranslator6CubesLeftRight
{
public:
  IndexTranslator6CubesLeftRight(svtkm::Id x_1,
                                 svtkm::Id y_1,
                                 svtkm::Id z_1,
                                 svtkm::Id x_2,
                                 svtkm::Id y_2,
                                 svtkm::Id z_2,
                                 svtkm::Id x_3,
                                 svtkm::Id y_3,
                                 svtkm::Id z_3,
                                 svtkm::Id x_4,
                                 svtkm::Id y_4,
                                 svtkm::Id z_4,
                                 svtkm::Id x_a,
                                 svtkm::Id y_a,
                                 svtkm::Id z_a,
                                 svtkm::Id x_d,
                                 svtkm::Id y_d,
                                 svtkm::Id z_d,
                                 svtkm::Id x_5,
                                 svtkm::Id y_5,
                                 svtkm::Id z_5,
                                 svtkm::Id start_x5,
                                 svtkm::Id start_y5,
                                 svtkm::Id start_z5)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
    , dimX4(x_4)
    , dimY4(y_4)
    , dimZ4(z_4)
    , dimXa(x_a)
    , dimYa(y_a)
    , dimZa(z_a)
    , dimXd(x_d)
    , dimYd(y_d)
    , dimZd(z_d)
    , dimX5(x_5)
    , dimY5(y_5)
    , dimZ5(z_5)
    , startX5(start_x5)
    , startY5(start_y5)
    , startZ5(start_z5)
  {
    (void)dimYa;
    (void)dimYd;
    (void)dimZa;
    (void)dimZd;
    (void)dimZ5;
    (void)dimZ4;
    (void)dimZ3;
    (void)dimZ2;
    (void)dimZ1;
  }

  SVTKM_EXEC_CONT
  void Translate3Dto1D(svtkm::Id inX,
                       svtkm::Id inY,
                       svtkm::Id inZ, // 3D indices as input
                       svtkm::Id& cube,
                       svtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimX1 <= inX && inX < (dimX1 + dimXa))
    {
      svtkm::Id inX_local = inX - dimX1;
      cube = 5; // cAcD
      idx = (inZ + startZ5) * dimX5 * dimY5 + (inY + startY5) * dimX5 + (inX_local + startX5);
    }
    else if ((dimX1 + dimXa + dimX2 + dimX3) <= inX &&
             inX < (dimX1 + dimXa + dimX2 + dimX3 + dimXd))
    {
      svtkm::Id inX_local = inX - dimX1 - dimX2 - dimX3; // no -dimXa since this is on the same cube
      cube = 5;                                         // cAcD
      idx = (inZ + startZ5) * dimX5 * dimY5 + (inY + startY5) * dimX5 + (inX_local + startX5);
    }
    else if (0 <= inX && inX < dimX1)
    {
      cube = 1; // ext1
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimX1 + dimXa) <= inX && inX < (dimX1 + dimXa + dimX2))
    {
      svtkm::Id inX_local = inX - dimX1 - dimXa;
      cube = 2; // ext2
      idx = inZ * dimX2 * dimY2 + inY * dimX2 + inX_local;
    }
    else if ((dimX1 + dimXa + dimX2) <= inX && inX < (dimX1 + dimXa + dimX2 + dimX3))
    {
      svtkm::Id inX_local = inX - dimX1 - dimXa - dimX2;
      cube = 3; // ext3
      idx = inZ * dimX3 * dimY3 + inY * dimX3 + inX_local;
    }
    else if ((dimX1 + dimXa + dimX2 + dimX3 + dimXd) <= inX &&
             inX < (dimX1 + dimXa + dimX2 + dimX3 + dimXd + dimX4))
    {
      svtkm::Id inX_local = inX - dimX1 - dimXa - dimX2 - dimX3 - dimXd;
      cube = 4; // ext4
      idx = inZ * dimX4 * dimY4 + inY * dimX4 + inX_local;
    }
  }

private:
  const svtkm::Id dimX1, dimY1, dimZ1, dimX2, dimY2, dimZ2;       // extension cube sizes
  const svtkm::Id dimX3, dimY3, dimZ3, dimX4, dimY4, dimZ4;       // extension cube sizes
  const svtkm::Id dimXa, dimYa, dimZa, dimXd, dimYd, dimZd;       // signal cube sizes
  const svtkm::Id dimX5, dimY5, dimZ5, startX5, startY5, startZ5; // entire cube size
};

class IndexTranslator6CubesTopDown
{
public:
  IndexTranslator6CubesTopDown(svtkm::Id x_1,
                               svtkm::Id y_1,
                               svtkm::Id z_1,
                               svtkm::Id x_2,
                               svtkm::Id y_2,
                               svtkm::Id z_2,
                               svtkm::Id x_3,
                               svtkm::Id y_3,
                               svtkm::Id z_3,
                               svtkm::Id x_4,
                               svtkm::Id y_4,
                               svtkm::Id z_4,
                               svtkm::Id x_a,
                               svtkm::Id y_a,
                               svtkm::Id z_a,
                               svtkm::Id x_d,
                               svtkm::Id y_d,
                               svtkm::Id z_d,
                               svtkm::Id x_5,
                               svtkm::Id y_5,
                               svtkm::Id z_5,
                               svtkm::Id start_x5,
                               svtkm::Id start_y5,
                               svtkm::Id start_z5)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
    , dimX4(x_4)
    , dimY4(y_4)
    , dimZ4(z_4)
    , dimXa(x_a)
    , dimYa(y_a)
    , dimZa(z_a)
    , dimXd(x_d)
    , dimYd(y_d)
    , dimZd(z_d)
    , dimX5(x_5)
    , dimY5(y_5)
    , dimZ5(z_5)
    , startX5(start_x5)
    , startY5(start_y5)
    , startZ5(start_z5)
  {
    (void)dimXa;
    (void)dimXd;
    (void)dimZd;
    (void)dimZa;
    (void)dimZ5;
    (void)dimZ4;
    (void)dimZ3;
    (void)dimZ2;
    (void)dimZ1;
  }

  SVTKM_EXEC_CONT
  void Translate3Dto1D(svtkm::Id inX,
                       svtkm::Id inY,
                       svtkm::Id inZ, // 3D indices as input
                       svtkm::Id& cube,
                       svtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimY1 <= inY && inY < (dimY1 + dimYa))
    {
      svtkm::Id inY_local = inY - dimY1;
      cube = 5; // cAcD
      idx = (inZ + startZ5) * dimX5 * dimY5 + (inY_local + startY5) * dimX5 + (inX + startX5);
    }
    else if ((dimY1 + dimYa + dimY2 + dimY3) <= inY &&
             inY < (dimY1 + dimYa + dimY2 + dimY3 + dimYd))
    {
      svtkm::Id inY_local = inY - dimY1 - dimY2 - dimY3;
      cube = 5; // cAcD
      idx = (inZ + startZ5) * dimX5 * dimY5 + (inY_local + startY5) * dimX5 + (inX + startX5);
    }
    else if (0 <= inY && inY < dimY1)
    {
      cube = 1; // ext1
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimY1 + dimYa) <= inY && inY < (dimY1 + dimYa + dimY2))
    {
      svtkm::Id inY_local = inY - dimY1 - dimYa;
      cube = 2; // ext2
      idx = inZ * dimX2 * dimY2 + inY_local * dimX2 + inX;
    }
    else if ((dimY1 + dimYa + dimY2) <= inY && inY < (dimY1 + dimYa + dimY2 + dimY3))
    {
      svtkm::Id inY_local = inY - dimY1 - dimYa - dimY2;
      cube = 3; // ext3
      idx = inZ * dimX3 * dimY3 + inY_local * dimX3 + inX;
    }
    else if ((dimY1 + dimYa + dimY2 + dimY3 + dimYd) <= inY &&
             inY < (dimY1 + dimYa + dimY2 + dimY3 + dimYd + dimY4))
    {
      svtkm::Id inY_local = inY - dimY1 - dimYa - dimY2 - dimY3 - dimYd;
      cube = 4; // ext4
      idx = inZ * dimX4 * dimY4 + inY_local * dimX4 + inX;
    }
  }

private:
  const svtkm::Id dimX1, dimY1, dimZ1, dimX2, dimY2, dimZ2;       // extension cube sizes
  const svtkm::Id dimX3, dimY3, dimZ3, dimX4, dimY4, dimZ4;       // extension cube sizes
  const svtkm::Id dimXa, dimYa, dimZa, dimXd, dimYd, dimZd;       // signal cube sizes
  const svtkm::Id dimX5, dimY5, dimZ5, startX5, startY5, startZ5; // entire cube size
};

class IndexTranslator6CubesFrontBack
{
public:
  SVTKM_EXEC_CONT
  IndexTranslator6CubesFrontBack(svtkm::Id x_1,
                                 svtkm::Id y_1,
                                 svtkm::Id z_1,
                                 svtkm::Id x_2,
                                 svtkm::Id y_2,
                                 svtkm::Id z_2,
                                 svtkm::Id x_3,
                                 svtkm::Id y_3,
                                 svtkm::Id z_3,
                                 svtkm::Id x_4,
                                 svtkm::Id y_4,
                                 svtkm::Id z_4,
                                 svtkm::Id x_a,
                                 svtkm::Id y_a,
                                 svtkm::Id z_a,
                                 svtkm::Id x_d,
                                 svtkm::Id y_d,
                                 svtkm::Id z_d,
                                 svtkm::Id x_5,
                                 svtkm::Id y_5,
                                 svtkm::Id z_5,
                                 svtkm::Id start_x5,
                                 svtkm::Id start_y5,
                                 svtkm::Id start_z5)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimZ1(z_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , dimZ2(z_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , dimZ3(z_3)
    , dimX4(x_4)
    , dimY4(y_4)
    , dimZ4(z_4)
    , dimXa(x_a)
    , dimYa(y_a)
    , dimZa(z_a)
    , dimXd(x_d)
    , dimYd(y_d)
    , dimZd(z_d)
    , dimX5(x_5)
    , dimY5(y_5)
    , dimZ5(z_5)
    , startX5(start_x5)
    , startY5(start_y5)
    , startZ5(start_z5)
  {
    (void)dimXd;
    (void)dimXa;
    (void)dimYd;
    (void)dimYa;
    (void)dimZ5;
  }

  SVTKM_EXEC_CONT
  void Translate3Dto1D(svtkm::Id inX,
                       svtkm::Id inY,
                       svtkm::Id inZ, // 3D indices as input
                       svtkm::Id& cube,
                       svtkm::Id& idx) const // which cube, and idx of that cube
  {
    if (dimZ1 <= inZ && inZ < (dimZ1 + dimZa))
    {
      svtkm::Id inZ_local = inZ - dimZ1;
      cube = 5; // cAcD
      idx = (inZ_local + startZ5) * dimX5 * dimY5 + (inY + startY5) * dimX5 + (inX + startX5);
    }
    else if ((dimZ1 + dimZa + dimZ2 + dimZ3) <= inZ &&
             inZ < (dimZ1 + dimZa + dimZ2 + dimZ3 + dimZd))
    {
      svtkm::Id inZ_local = inZ - dimZ1 - dimZ2 - dimZ3;
      cube = 5; // cAcD
      idx = (inZ_local + startZ5) * dimX5 * dimY5 + (inY + startY5) * dimX5 + (inX + startX5);
    }
    else if (0 <= inZ && inZ < dimZ1)
    {
      cube = 1; // ext1
      idx = inZ * dimX1 * dimY1 + inY * dimX1 + inX;
    }
    else if ((dimZ1 + dimZa) <= inZ && inZ < (dimZ1 + dimZa + dimZ2))
    {
      svtkm::Id inZ_local = inZ - dimZ1 - dimZa;
      cube = 2; // ext2
      idx = inZ_local * dimX2 * dimY2 + inY * dimX2 + inX;
    }
    else if ((dimZ1 + dimZa + dimZ2) <= inZ && inZ < (dimZ1 + dimZa + dimZ2 + dimZ3))
    {
      svtkm::Id inZ_local = inZ - dimZ1 - dimZa - dimZ2;
      cube = 3; // ext3
      idx = inZ_local * dimX3 * dimY3 + inY * dimX3 + inX;
    }
    else if ((dimZ1 + dimZa + dimZ2 + dimZ3 + dimZd) <= inZ &&
             inZ < (dimZ1 + dimZa + dimZ2 + dimZ3 + dimZd + dimZ4))
    {
      svtkm::Id inZ_local = inZ - dimZ1 - dimZa - dimZ2 - dimZ3 - dimZd;
      cube = 4; // ext4
      idx = inZ_local * dimX4 * dimY4 + inY * dimX4 + inX;
    }
  }

private:
  const svtkm::Id dimX1, dimY1, dimZ1, dimX2, dimY2, dimZ2;       // extension cube sizes
  const svtkm::Id dimX3, dimY3, dimZ3, dimX4, dimY4, dimZ4;       // extension cube sizes
  const svtkm::Id dimXa, dimYa, dimZa, dimXd, dimYd, dimZd;       // signal cube sizes
  const svtkm::Id dimX5, dimY5, dimZ5, startX5, startY5, startZ5; // entire cube size
};

//=============================================================================

// Worklet: 3D forward transform along X (left-right)
class ForwardTransform3DLeftRight : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // left extension
                                WholeArrayIn,   // signal
                                WholeArrayIn,   // right extension
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);
  using InputDomain = _6;

  ForwardTransform3DLeftRight(svtkm::Id filter_len,
                              svtkm::Id approx_len,
                              bool odd_low,
                              svtkm::Id dimX1,
                              svtkm::Id dimY1,
                              svtkm::Id dimZ1,
                              svtkm::Id dimX2,
                              svtkm::Id dimY2,
                              svtkm::Id dimZ2,
                              svtkm::Id startX2,
                              svtkm::Id startY2,
                              svtkm::Id startZ2,
                              svtkm::Id pretendX2,
                              svtkm::Id pretendY2,
                              svtkm::Id pretendZ2,
                              svtkm::Id dimX3,
                              svtkm::Id dimY3,
                              svtkm::Id dimZ3)
    : filterLen(filter_len)
    , approxLen(approx_len)
    , outDimX(pretendX2)
    , outDimY(pretendY2)
    //, outDimZ(pretendZ2)
    , translator(dimX1,
                 dimY1,
                 dimZ1,
                 dimX2,
                 dimY2,
                 dimZ2,
                 startX2,
                 startY2,
                 startZ2,
                 pretendX2,
                 pretendY2,
                 pretendZ2,
                 dimX3,
                 dimY3,
                 dimZ3)
  {
    this->lstart = odd_low ? 1 : 0;
    this->hstart = 1;
  }

  SVTKM_EXEC_CONT
  void Output1Dto3D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (this->outDimX * this->outDimY);
    y = (idx - z * this->outDimX * this->outDimY) / this->outDimX;
    x = idx % this->outDimX;
  }
  SVTKM_EXEC_CONT
  svtkm::Id Output3Dto1D(svtkm::Id x, svtkm::Id y, svtkm::Id z) const
  {
    return z * outDimX * outDimY + y * outDimX + x;
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3>
  SVTKM_EXEC_CONT VAL GetVal(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            svtkm::Id inCube,
                            svtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(portal2.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(portal1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(portal3.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename FilterPortalType,
            typename OutputPortalType>
  SVTKM_EXEC_CONT void operator()(const InPortalType1& inPortal1,     // left extension
                                 const InPortalType2& inPortal2,     // signal
                                 const InPortalType3& inPortal3,     // right extension
                                 const FilterPortalType& lowFilter,  // lowFilter
                                 const FilterPortalType& highFilter, // highFilter
                                 OutputPortalType& coeffOut,
                                 const svtkm::Id& workIndex) const
  {
    svtkm::Id workX, workY, workZ, output1D;
    this->Output1Dto3D(workIndex, workX, workY, workZ);
    svtkm::Id inputCube = 0, inputIdx = 0;
    using OutputValueType = typename OutputPortalType::ValueType;

    if (workX % 2 == 0) // calculate cA
    {
      svtkm::Id xl = lstart + workX;
      VAL sum = MAKEVAL(0.0);
      for (svtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(xl, workY, workZ, inputCube, inputIdx);
        sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        xl++;
      }
      output1D = Output3Dto1D(workX / 2, workY, workZ);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
    else // calculate cD
    {
      svtkm::Id xh = hstart + workX - 1;
      VAL sum = MAKEVAL(0.0);
      for (svtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(xh, workY, workZ, inputCube, inputIdx);
        sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        xh++;
      }
      output1D = Output3Dto1D((workX - 1) / 2 + approxLen, workY, workZ);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen, approxLen;
  const svtkm::Id outDimX;
  const svtkm::Id outDimY;
  //const svtkm::Id outDimZ; Not used
  const IndexTranslator3CubesLeftRight translator;
  svtkm::Id lstart, hstart;
};

class ForwardTransform3DTopDown : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // left extension
                                WholeArrayIn,   // signal
                                WholeArrayIn,   // right extension
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);
  using InputDomain = _6;

  ForwardTransform3DTopDown(svtkm::Id filter_len,
                            svtkm::Id approx_len,
                            bool odd_low,
                            svtkm::Id dimX1,
                            svtkm::Id dimY1,
                            svtkm::Id dimZ1,
                            svtkm::Id dimX2,
                            svtkm::Id dimY2,
                            svtkm::Id dimZ2,
                            svtkm::Id startX2,
                            svtkm::Id startY2,
                            svtkm::Id startZ2,
                            svtkm::Id pretendX2,
                            svtkm::Id pretendY2,
                            svtkm::Id pretendZ2,
                            svtkm::Id dimX3,
                            svtkm::Id dimY3,
                            svtkm::Id dimZ3)
    : filterLen(filter_len)
    , approxLen(approx_len)
    , outDimX(pretendX2)
    , outDimY(pretendY2)
    //, outDimZ(pretendZ2)
    , translator(dimX1,
                 dimY1,
                 dimZ1,
                 dimX2,
                 dimY2,
                 dimZ2,
                 startX2,
                 startY2,
                 startZ2,
                 pretendX2,
                 pretendY2,
                 pretendZ2,
                 dimX3,
                 dimY3,
                 dimZ3)
  {
    this->lstart = odd_low ? 1 : 0;
    this->hstart = 1;
  }

  SVTKM_EXEC_CONT
  void Output1Dto3D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }
  SVTKM_EXEC_CONT
  svtkm::Id Output3Dto1D(svtkm::Id x, svtkm::Id y, svtkm::Id z) const
  {
    return z * outDimX * outDimY + y * outDimX + x;
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3>
  SVTKM_EXEC_CONT VAL GetVal(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            svtkm::Id inCube,
                            svtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(portal2.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(portal1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(portal3.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename FilterPortalType,
            typename OutputPortalType>
  SVTKM_EXEC_CONT void operator()(const InPortalType1& inPortal1,     // top extension
                                 const InPortalType2& inPortal2,     // signal
                                 const InPortalType3& inPortal3,     // down extension
                                 const FilterPortalType& lowFilter,  // lowFilter
                                 const FilterPortalType& highFilter, // highFilter
                                 OutputPortalType& coeffOut,
                                 const svtkm::Id& workIndex) const
  {
    svtkm::Id workX, workY, workZ, output1D;
    Output1Dto3D(workIndex, workX, workY, workZ);
    svtkm::Id inputCube = 0, inputIdx = 0;
    using OutputValueType = typename OutputPortalType::ValueType;

    if (workY % 2 == 0) // calculate cA
    {
      svtkm::Id yl = lstart + workY;
      VAL sum = MAKEVAL(0.0);
      for (svtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(workX, yl, workZ, inputCube, inputIdx);
        sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        yl++;
      }
      output1D = Output3Dto1D(workX, workY / 2, workZ);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
    else // calculate cD
    {
      svtkm::Id yh = hstart + workY - 1;
      VAL sum = MAKEVAL(0.0);
      for (svtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(workX, yh, workZ, inputCube, inputIdx);
        sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        yh++;
      }
      output1D = Output3Dto1D(workX, (workY - 1) / 2 + approxLen, workZ);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen, approxLen;
  const svtkm::Id outDimX;
  const svtkm::Id outDimY;
  //const svtkm::Id outDimZ; Not used
  const IndexTranslator3CubesTopDown translator;
  svtkm::Id lstart, hstart;
};

class ForwardTransform3DFrontBack : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // left extension
                                WholeArrayIn,   // signal
                                WholeArrayIn,   // right extension
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);
  using InputDomain = _6;

  ForwardTransform3DFrontBack(svtkm::Id filter_len,
                              svtkm::Id approx_len,
                              bool odd_low,
                              svtkm::Id dimX1,
                              svtkm::Id dimY1,
                              svtkm::Id dimZ1,
                              svtkm::Id dimX2,
                              svtkm::Id dimY2,
                              svtkm::Id dimZ2,
                              svtkm::Id startX2,
                              svtkm::Id startY2,
                              svtkm::Id startZ2,
                              svtkm::Id pretendX2,
                              svtkm::Id pretendY2,
                              svtkm::Id pretendZ2,
                              svtkm::Id dimX3,
                              svtkm::Id dimY3,
                              svtkm::Id dimZ3)
    : filterLen(filter_len)
    , approxLen(approx_len)
    , outDimX(pretendX2)
    , outDimY(pretendY2)
    //, outDimZ(pretendZ2)
    , translator(dimX1,
                 dimY1,
                 dimZ1,
                 dimX2,
                 dimY2,
                 dimZ2,
                 startX2,
                 startY2,
                 startZ2,
                 pretendX2,
                 pretendY2,
                 pretendZ2,
                 dimX3,
                 dimY3,
                 dimZ3)
  {
    this->lstart = odd_low ? 1 : 0;
    this->hstart = 1;
  }

  SVTKM_EXEC_CONT
  void Output1Dto3D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }
  SVTKM_EXEC_CONT
  svtkm::Id Output3Dto1D(svtkm::Id x, svtkm::Id y, svtkm::Id z) const
  {
    return z * outDimX * outDimY + y * outDimX + x;
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3>
  SVTKM_EXEC_CONT VAL GetVal(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            svtkm::Id inCube,
                            svtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(portal2.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(portal1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(portal3.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename FilterPortalType,
            typename OutputPortalType>
  SVTKM_EXEC_CONT void operator()(const InPortalType1& inPortal1, // top extension
                                 const InPortalType2& inPortal2, // signal
                                 const InPortalType3& inPortal3, // down extension
                                 const FilterPortalType& lowFilter,
                                 const FilterPortalType& highFilter,
                                 OutputPortalType& coeffOut,
                                 const svtkm::Id& workIndex) const
  {
    svtkm::Id workX, workY, workZ, output1D;
    Output1Dto3D(workIndex, workX, workY, workZ);
    svtkm::Id inputCube = 0, inputIdx = 0;
    using OutputValueType = typename OutputPortalType::ValueType;

    if (workZ % 2 == 0) // calculate cA
    {
      svtkm::Id zl = lstart + workZ;
      VAL sum = MAKEVAL(0.0);
      for (svtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(workX, workY, zl, inputCube, inputIdx);
        sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        zl++;
      }
      output1D = Output3Dto1D(workX, workY, workZ / 2);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
    else // calculate cD
    {
      svtkm::Id zh = hstart + workZ - 1;
      VAL sum = MAKEVAL(0.0);
      for (svtkm::Id k = filterLen - 1; k > -1; k--)
      {
        translator.Translate3Dto1D(workX, workY, zh, inputCube, inputIdx);
        sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputCube, inputIdx);
        zh++;
      }
      output1D = Output3Dto1D(workX, workY, (workZ - 1) / 2 + approxLen);
      coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen, approxLen;
  const svtkm::Id outDimX;
  const svtkm::Id outDimY;
  //const svtkm::Id outDimZ; Not used
  const IndexTranslator3CubesFrontBack translator;
  svtkm::Id lstart, hstart;
};

//=============================================================================

//
//  ---------------------------------------------------
//  |      |          |      |      |          |      |
//  |cube1 |  cube5   |cube2 |cube3 |  cube5   |cube4 |
//  | ext1 |    cA    | ext2 | ext3 |    cD    | ext4 |
//  | (x1) |   (xa)   | (x2) | (x3) |   (xd)   | (x4) |
//  |      |          |      |      |          |      |
//  ----------------------------------------------------
// The following 3 classes perform the same functionaliry in 3 directions
//
// Worklet: perform a simple 3D inverse transform along X direction (Left-right)
class InverseTransform3DLeftRight : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, // ext1
                                WholeArrayIn, // ext2
                                WholeArrayIn, // ext3
                                WholeArrayIn, // ext4
                                WholeArrayIn, // cA+cD (signal)
                                WholeArrayIn, // lowFilter
                                WholeArrayIn, // highFilter
                                FieldOut);    // outptu coefficients
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);
  using InputDomain = _8;

  // Constructor
  InverseTransform3DLeftRight(svtkm::Id fil_len,
                              svtkm::Id x_1,
                              svtkm::Id y_1,
                              svtkm::Id z_1, // ext1
                              svtkm::Id x_2,
                              svtkm::Id y_2,
                              svtkm::Id z_2, // ext2
                              svtkm::Id x_3,
                              svtkm::Id y_3,
                              svtkm::Id z_3, // ext3
                              svtkm::Id x_4,
                              svtkm::Id y_4,
                              svtkm::Id z_4, // ext4
                              svtkm::Id x_a,
                              svtkm::Id y_a,
                              svtkm::Id z_a, // cA
                              svtkm::Id x_d,
                              svtkm::Id y_d,
                              svtkm::Id z_d, // cD
                              svtkm::Id x_5,
                              svtkm::Id y_5,
                              svtkm::Id z_5, // signal, actual dims
                              svtkm::Id startX5,
                              svtkm::Id startY5,
                              svtkm::Id startZ5)
    : filterLen(fil_len)
    , outDimX(x_a + x_d)
    , outDimY(y_a)
    //, outDimZ(z_a)
    , cALenExtended(x_1 + x_a + x_2)
    , translator(x_1,
                 y_1,
                 z_1,
                 x_2,
                 y_2,
                 z_2,
                 x_3,
                 y_3,
                 z_3,
                 x_4,
                 y_4,
                 z_4,
                 x_a,
                 y_a,
                 z_a,
                 x_d,
                 y_d,
                 z_d,
                 x_5,
                 y_5,
                 z_5,
                 startX5,
                 startY5,
                 startZ5)
  {
  }

  SVTKM_EXEC_CONT
  void Output1Dto3D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5>
  SVTKM_EXEC_CONT VAL GetVal(const InPortalType1& ext1,
                            const InPortalType2& ext2,
                            const InPortalType3& ext3,
                            const InPortalType4& ext4,
                            const InPortalType5& sig5,
                            svtkm::Id inCube,
                            svtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(ext2.Get(inIdx));
    }
    else if (inCube == 4)
    {
      return MAKEVAL(ext4.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(ext1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(ext3.Get(inIdx));
    }
    else if (inCube == 5)
    {
      return MAKEVAL(sig5.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5,
            typename FilterPortalType,
            typename OutputValueType>
  SVTKM_EXEC void operator()(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            const InPortalType4& portal4,
                            const InPortalType5& portal5,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputValueType& coeffOut,
                            const svtkm::Id& workIdx) const
  {
    svtkm::Id workX, workY, workZ;
    svtkm::Id k1, k2, xi;
    svtkm::Id inputCube = 0, inputIdx = 0;
    Output1Dto3D(workIdx, workX, workY, workZ);

    if (filterLen % 2 != 0) // odd filter
    {
      if (workX % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      xi = (workX + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(xi, workY, workZ, inputCube, inputIdx);
        sum += lowFilter.Get(k1) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        xi++;
        k1 -= 2;
      }
      xi = workX / 2;
      while (k2 > -1)
      {
        translator.Translate3Dto1D(xi + cALenExtended, workY, workZ, inputCube, inputIdx);
        sum += highFilter.Get(k2) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        xi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
    else // even filter
    {
      if ((filterLen / 2) % 2 != 0) // odd length half filter
      {
        xi = workX / 2;
        if (workX % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else // even length half filter
      {
        xi = (workX + 1) / 2;
        if (workX % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(xi, workY, workZ, inputCube, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        translator.Translate3Dto1D(xi + cALenExtended, workY, workZ, inputCube, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        xi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen;
  svtkm::Id outDimX;
  svtkm::Id outDimY;
  // svtkm::Id outDimZ; Not used
  svtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6CubesLeftRight translator;
};

class InverseTransform3DTopDown : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, // ext1
                                WholeArrayIn, // ext2
                                WholeArrayIn, // ext3
                                WholeArrayIn, // ext4
                                WholeArrayIn, // cA+cD (signal)
                                WholeArrayIn, // lowFilter
                                WholeArrayIn, // highFilter
                                FieldOut);    // outptu coefficients
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);
  using InputDomain = _8;

  // Constructor
  InverseTransform3DTopDown(svtkm::Id fil_len,
                            svtkm::Id x_1,
                            svtkm::Id y_1,
                            svtkm::Id z_1, // ext1
                            svtkm::Id x_2,
                            svtkm::Id y_2,
                            svtkm::Id z_2, // ext2
                            svtkm::Id x_3,
                            svtkm::Id y_3,
                            svtkm::Id z_3, // ext3
                            svtkm::Id x_4,
                            svtkm::Id y_4,
                            svtkm::Id z_4, // ext4
                            svtkm::Id x_a,
                            svtkm::Id y_a,
                            svtkm::Id z_a, // cA
                            svtkm::Id x_d,
                            svtkm::Id y_d,
                            svtkm::Id z_d, // cD
                            svtkm::Id x_5,
                            svtkm::Id y_5,
                            svtkm::Id z_5, // signal, actual dims
                            svtkm::Id startX5,
                            svtkm::Id startY5,
                            svtkm::Id startZ5)
    : filterLen(fil_len)
    , outDimX(x_a)
    , outDimY(y_a + y_d)
    //, outDimZ(z_a)
    , cALenExtended(y_1 + y_a + y_2)
    , translator(x_1,
                 y_1,
                 z_1,
                 x_2,
                 y_2,
                 z_2,
                 x_3,
                 y_3,
                 z_3,
                 x_4,
                 y_4,
                 z_4,
                 x_a,
                 y_a,
                 z_a,
                 x_d,
                 y_d,
                 z_d,
                 x_5,
                 y_5,
                 z_5,
                 startX5,
                 startY5,
                 startZ5)
  {
  }

  SVTKM_EXEC_CONT
  void Output1Dto3D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5>
  SVTKM_EXEC_CONT VAL GetVal(const InPortalType1& ext1,
                            const InPortalType2& ext2,
                            const InPortalType3& ext3,
                            const InPortalType4& ext4,
                            const InPortalType5& sig5,
                            svtkm::Id inCube,
                            svtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(ext2.Get(inIdx));
    }
    else if (inCube == 4)
    {
      return MAKEVAL(ext4.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(ext1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(ext3.Get(inIdx));
    }
    else if (inCube == 5)
    {
      return MAKEVAL(sig5.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5,
            typename FilterPortalType,
            typename OutputValueType>
  SVTKM_EXEC void operator()(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            const InPortalType4& portal4,
                            const InPortalType5& portal5,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputValueType& coeffOut,
                            const svtkm::Id& workIdx) const
  {
    svtkm::Id workX, workY, workZ;
    svtkm::Id k1, k2, yi;
    svtkm::Id inputCube = 0, inputIdx = 0;
    Output1Dto3D(workIdx, workX, workY, workZ);

    if (filterLen % 2 != 0) // odd filter
    {
      if (workY % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      yi = (workY + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(workX, yi, workZ, inputCube, inputIdx);
        sum += lowFilter.Get(k1) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        yi++;
        k1 -= 2;
      }
      yi = workY / 2;
      while (k2 > -1)
      {
        translator.Translate3Dto1D(workX, yi + cALenExtended, workZ, inputCube, inputIdx);
        sum += highFilter.Get(k2) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        yi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
    else // even filter
    {
      if ((filterLen / 2) % 2 != 0)
      {
        yi = workY / 2;
        if (workY % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else
      {
        yi = (workY + 1) / 2;
        if (workY % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(workX, yi, workZ, inputCube, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        translator.Translate3Dto1D(workX, yi + cALenExtended, workZ, inputCube, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        yi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen;
  svtkm::Id outDimX;
  svtkm::Id outDimY;
  //svtkm::Id outDimZ; Not used
  svtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6CubesTopDown translator;
};

class InverseTransform3DFrontBack : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, // ext1
                                WholeArrayIn, // ext2
                                WholeArrayIn, // ext3
                                WholeArrayIn, // ext4
                                WholeArrayIn, // cA+cD (signal)
                                WholeArrayIn, // lowFilter
                                WholeArrayIn, // highFilter
                                FieldOut);    // outptu coefficients
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);
  using InputDomain = _8;

  // Constructor
  InverseTransform3DFrontBack(svtkm::Id fil_len,
                              svtkm::Id x_1,
                              svtkm::Id y_1,
                              svtkm::Id z_1, // ext1
                              svtkm::Id x_2,
                              svtkm::Id y_2,
                              svtkm::Id z_2, // ext2
                              svtkm::Id x_3,
                              svtkm::Id y_3,
                              svtkm::Id z_3, // ext3
                              svtkm::Id x_4,
                              svtkm::Id y_4,
                              svtkm::Id z_4, // ext4
                              svtkm::Id x_a,
                              svtkm::Id y_a,
                              svtkm::Id z_a, // cA
                              svtkm::Id x_d,
                              svtkm::Id y_d,
                              svtkm::Id z_d, // cD
                              svtkm::Id x_5,
                              svtkm::Id y_5,
                              svtkm::Id z_5, // signal, actual dims
                              svtkm::Id startX5,
                              svtkm::Id startY5,
                              svtkm::Id startZ5)
    : filterLen(fil_len)
    , outDimX(x_a)
    , outDimY(y_a)
    //, outDimZ(z_a + z_d)
    , cALenExtended(z_1 + z_a + z_2)
    , translator(x_1,
                 y_1,
                 z_1,
                 x_2,
                 y_2,
                 z_2,
                 x_3,
                 y_3,
                 z_3,
                 x_4,
                 y_4,
                 z_4,
                 x_a,
                 y_a,
                 z_a,
                 x_d,
                 y_d,
                 z_d,
                 x_5,
                 y_5,
                 z_5,
                 startX5,
                 startY5,
                 startZ5)
  {
    /* printf("InverseTransform3DFrontBack: \n");
    printf("  output dims: (%lld, %lld, %lld)\n", outDimX, outDimY, outDimZ ); */
  }

  SVTKM_EXEC_CONT
  void Output1Dto3D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (outDimX * outDimY);
    y = (idx - z * outDimX * outDimY) / outDimX;
    x = idx % outDimX;
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5>
  SVTKM_EXEC_CONT VAL GetVal(const InPortalType1& ext1,
                            const InPortalType2& ext2,
                            const InPortalType3& ext3,
                            const InPortalType4& ext4,
                            const InPortalType5& sig5,
                            svtkm::Id inCube,
                            svtkm::Id inIdx) const
  {
    if (inCube == 2)
    {
      return MAKEVAL(ext2.Get(inIdx));
    }
    else if (inCube == 4)
    {
      return MAKEVAL(ext4.Get(inIdx));
    }
    else if (inCube == 1)
    {
      return MAKEVAL(ext1.Get(inIdx));
    }
    else if (inCube == 3)
    {
      return MAKEVAL(ext3.Get(inIdx));
    }
    else if (inCube == 5)
    {
      return MAKEVAL(sig5.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalType5,
            typename FilterPortalType,
            typename OutputValueType>
  SVTKM_EXEC void operator()(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            const InPortalType4& portal4,
                            const InPortalType5& portal5,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputValueType& coeffOut,
                            const svtkm::Id& workIdx) const
  {
    svtkm::Id workX, workY, workZ;
    svtkm::Id k1, k2, zi;
    svtkm::Id inputCube = 0, inputIdx = 0;
    Output1Dto3D(workIdx, workX, workY, workZ);

    if (filterLen % 2 != 0) // odd filter
    {
      if (workZ % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      zi = (workZ + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(workX, workY, zi, inputCube, inputIdx);
        sum += lowFilter.Get(k1) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        zi++;
        k1 -= 2;
      }
      zi = workZ / 2;
      while (k2 > -1)
      {
        translator.Translate3Dto1D(workX, workY, zi + cALenExtended, inputCube, inputIdx);
        sum += highFilter.Get(k2) *
          GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        zi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
    else // even filter
    {
      if ((filterLen / 2) % 2 != 0)
      {
        zi = workZ / 2;
        if (workZ % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else
      {
        zi = (workZ + 1) / 2;
        if (workZ % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate3Dto1D(workX, workY, zi, inputCube, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        translator.Translate3Dto1D(workX, workY, zi + cALenExtended, inputCube, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portal5, inputCube, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        zi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen;
  svtkm::Id outDimX;
  svtkm::Id outDimY;
  //svtkm::Id outDimZ; Not used
  svtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6CubesFrontBack translator;
};

//=============================================================================

//  ---------------------------------------------------
//  |      |          |      |      |          |      |
//  |      |          |      |      |          |      |
//  | ext1 |    cA    | ext2 | ext3 |    cD    | ext4 |
//  | (x1) |   (xa)   | (x2) | (x3) |   (xd)   | (x4) |
//  |      |          |      |      |          |      |
//  ----------------------------------------------------
//  matrix1: ext1
//  matrix2: ext2
//  matrix3: ext3
//  matrix4: ext4
//  matrix5: cA + cD
class IndexTranslator6Matrices
{
public:
  IndexTranslator6Matrices(svtkm::Id x_1,
                           svtkm::Id y_1,
                           svtkm::Id x_a,
                           svtkm::Id y_a,
                           svtkm::Id x_2,
                           svtkm::Id y_2,
                           svtkm::Id x_3,
                           svtkm::Id y_3,
                           svtkm::Id x_d,
                           svtkm::Id y_d,
                           svtkm::Id x_4,
                           svtkm::Id y_4,
                           svtkm::Id x_5,
                           svtkm::Id y_5, // actual size of matrix5
                           svtkm::Id start_x5,
                           svtkm::Id start_y5, // start indices of pretend matrix
                           bool mode)
    : x1(x_1)
    , y1(y_1)
    , xa(x_a)
    , ya(y_a)
    , x2(x_2)
    , y2(y_2)
    , x3(x_3)
    , y3(y_3)
    , xd(x_d)
    , yd(y_d)
    , x4(x_4)
    , y4(y_4)
    , x5(x_5)
    , y5(y_5)
    , startX5(start_x5)
    , startY5(start_y5)
    , modeLR(mode)
  {
    // Get pretend matrix dims
    if (modeLR)
    {
      pretendX5 = xa + xd;
      pretendY5 = y1;
    }
    else
    {
      pretendX5 = x1;
      pretendY5 = ya + yd;
    }
    (void)y5;
  }

  SVTKM_EXEC_CONT
  void Translate2Dto1D(svtkm::Id inX,
                       svtkm::Id inY, // 2D indices as input
                       svtkm::Id& mat,
                       svtkm::Id& idx) const // which matrix, and idx of that matrix
  {
    if (modeLR) // left-right mode
    {
      if (0 <= inX && inX < x1)
      {
        mat = 1; // ext1
        idx = inY * x1 + inX;
      }
      else if (x1 <= inX && inX < (x1 + xa))
      {
        mat = 5; // cAcD
        idx = (inY + startY5) * x5 + (inX - x1 + startX5);
      }
      else if ((x1 + xa) <= inX && inX < (x1 + xa + x2))
      {
        mat = 2; // ext2
        idx = inY * x2 + (inX - x1 - xa);
      }
      else if ((x1 + xa + x2) <= inX && inX < (x1 + xa + x2 + x3))
      {
        mat = 3; // ext3
        idx = inY * x3 + (inX - x1 - xa - x2);
      }
      else if ((x1 + xa + x2 + x3) <= inX && inX < (x1 + xa + x2 + x3 + xd))
      {
        mat = 5; // cAcD
        idx = (inY + startY5) * x5 + (inX - x1 - x2 - x3 + startX5);
      }
      else if ((x1 + xa + x2 + x3 + xd) <= inX && inX < (x1 + xa + x2 + x3 + xd + x4))
      {
        mat = 4; // ext4
        idx = inY * x4 + (inX - x1 - xa - x2 - x3 - xd);
      }
    }
    else // top-down mode
    {
      if (0 <= inY && inY < y1)
      {
        mat = 1; // ext1
        idx = inY * x1 + inX;
      }
      else if (y1 <= inY && inY < (y1 + ya))
      {
        mat = 5; // cAcD
        idx = (inY - y1 + startY5) * x5 + inX + startX5;
      }
      else if ((y1 + ya) <= inY && inY < (y1 + ya + y2))
      {
        mat = 2; // ext2
        idx = (inY - y1 - ya) * x2 + inX;
      }
      else if ((y1 + ya + y2) <= inY && inY < (y1 + ya + y2 + y3))
      {
        mat = 3; // ext3
        idx = (inY - y1 - ya - y2) * x3 + inX;
      }
      else if ((y1 + ya + y2 + y3) <= inY && inY < (y1 + ya + y2 + y3 + yd))
      {
        mat = 5; // cAcD
        idx = (inY - y1 - y2 - y3 + startY5) * x5 + inX + startX5;
      }
      else if ((y1 + ya + y2 + y3 + yd) <= inY && inY < (y1 + ya + y2 + y3 + yd + y4))
      {
        mat = 4; // ext4
        idx = (inY - y1 - ya - y2 - y3 - yd) * x4 + inX;
      }
    }
  }

private:
  const svtkm::Id x1, y1, xa, ya, x2, y2, x3, y3, xd, yd, x4, y4;
  svtkm::Id x5, y5, startX5, startY5, pretendX5, pretendY5;
  const bool modeLR; // true = left-right mode; false = top-down mode.
};

//       ................
//       .              .
//  -----.--------------.-----
//  |    . |          | .    |
//  |    . |          | .    |
//  | ext1 |   mat2   | ext2 |
//  | (x1) |   (x2)   | (x3) |
//  |    . |          | .    |
//  -----.--------------.-----
//       ................
class IndexTranslator3Matrices
{
public:
  IndexTranslator3Matrices(svtkm::Id x_1,
                           svtkm::Id y_1,
                           svtkm::Id x_2,
                           svtkm::Id y_2, // actual dims of mat2
                           svtkm::Id startx_2,
                           svtkm::Id starty_2, // start idx of pretend
                           svtkm::Id pretendx_2,
                           svtkm::Id pretendy_2, // pretend dims
                           svtkm::Id x_3,
                           svtkm::Id y_3,
                           bool mode)
    : dimX1(x_1)
    , dimY1(y_1)
    , dimX2(x_2)
    , dimY2(y_2)
    , startX2(startx_2)
    , startY2(starty_2)
    , pretendDimX2(pretendx_2)
    , pretendDimY2(pretendy_2)
    , dimX3(x_3)
    , dimY3(y_3)
    , mode_lr(mode)
  {
    (void)dimY2;
  }

  SVTKM_EXEC_CONT
  void Translate2Dto1D(svtkm::Id inX,
                       svtkm::Id inY, // 2D indices as input
                       svtkm::Id& mat,
                       svtkm::Id& idx) const // which matrix, and idx of that matrix
  {
    if (mode_lr) // left-right mode
    {
      if (0 <= inX && inX < dimX1)
      {
        mat = 1;
        idx = inY * dimX1 + inX;
      }
      else if (dimX1 <= inX && inX < (dimX1 + pretendDimX2))
      {
        mat = 2;
        idx = (inY + startY2) * dimX2 + (inX + startX2 - dimX1);
      }
      else if ((dimX1 + pretendDimX2) <= inX && inX < (dimX1 + pretendDimX2 + dimX3))
      {
        mat = 3;
        idx = inY * dimX3 + (inX - dimX1 - pretendDimX2);
      }
    }
    else // top-down mode
    {
      if (0 <= inY && inY < dimY1)
      {
        mat = 1;
        idx = inY * dimX1 + inX;
      }
      else if (dimY1 <= inY && inY < (dimY1 + pretendDimY2))
      {
        mat = 2;
        idx = (inY + startY2 - dimY1) * dimX2 + inX + startX2;
      }
      else if ((dimY1 + pretendDimY2) <= inY && inY < (dimY1 + pretendDimY2 + dimY3))
      {
        mat = 3;
        idx = (inY - dimY1 - pretendDimY2) * dimX3 + inX;
      }
    }
  }

private:
  const svtkm::Id dimX1, dimY1;
  const svtkm::Id dimX2, dimY2, startX2, startY2, pretendDimX2, pretendDimY2;
  const svtkm::Id dimX3, dimY3;
  const bool mode_lr; // true: left right mode; false: top down mode.
};

// Worklet for 2D signal extension
// This implementation operates on a specified part of a big rectangle
class ExtensionWorklet2D : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  ExtensionWorklet2D(svtkm::Id extdimX,
                     svtkm::Id extdimY,
                     svtkm::Id sigdimX,
                     svtkm::Id sigdimY,
                     svtkm::Id sigstartX,
                     svtkm::Id sigstartY,
                     svtkm::Id sigpretendX,
                     svtkm::Id sigpretendY,
                     DWTMode m,
                     ExtensionDirection dir,
                     bool pad_zero)
    : extDimX(extdimX)
    , extDimY(extdimY)
    , sigDimX(sigdimX)
    , sigDimY(sigdimY)
    , sigStartX(sigstartX)
    , sigStartY(sigstartY)
    , sigPretendDimX(sigpretendX)
    , sigPretendDimY(sigpretendY)
    , mode(m)
    , direction(dir)
    , padZero(pad_zero)
  {
    (void)sigDimY;
  }

  // Index translation helper
  SVTKM_EXEC_CONT
  void Ext1Dto2D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y) const
  {
    x = idx % extDimX;
    y = idx / extDimX;
  }

  // Index translation helper
  SVTKM_EXEC_CONT
  svtkm::Id Sig2Dto1D(svtkm::Id x, svtkm::Id y) const { return y * sigDimX + x; }

  // Index translation helper
  SVTKM_EXEC_CONT
  svtkm::Id SigPretend2Dto1D(svtkm::Id x, svtkm::Id y) const
  {
    return (y + sigStartY) * sigDimX + x + sigStartX;
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const svtkm::Id& workIndex) const
  {
    svtkm::Id extX, extY, sigPretendX, sigPretendY;
    sigPretendX = sigPretendY = 0;
    Ext1Dto2D(workIndex, extX, extY);
    typename PortalOutType::ValueType sym = 1.0;
    if (mode == ASYMH || mode == ASYMW)
    {
      sym = -1.0;
    }
    if (direction == LEFT)
    {
      sigPretendY = extY;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendX = extDimX - extX - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendX = extDimX - extX;
      }
    }
    else if (direction == TOP)
    {
      sigPretendX = extX;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendY = extDimY - extY - 1;
      }
      else // mode == SYMW || mode == ASYMW
      {
        sigPretendY = extDimY - extY;
      }
    }
    else if (direction == RIGHT)
    {
      sigPretendY = extY;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendX = sigPretendDimX - extX - 1;
      }
      else
      {
        sigPretendX = sigPretendDimX - extX - 2;
      }
      if (padZero)
      {
        sigPretendX++;
      }
    }
    else if (direction == BOTTOM)
    {
      sigPretendX = extX;
      if (mode == SYMH || mode == ASYMH)
      {
        sigPretendY = sigPretendDimY - extY - 1;
      }
      else
      {
        sigPretendY = sigPretendDimY - extY - 2;
      }
      if (padZero)
      {
        sigPretendY++;
      }
    }

    if (sigPretendX == sigPretendDimX || sigPretendY == sigPretendDimY)
    {
      portalOut.Set(workIndex, 0.0);
    }
    else
    {
      portalOut.Set(workIndex, sym * portalIn.Get(SigPretend2Dto1D(sigPretendX, sigPretendY)));
    }
  }

private:
  const svtkm::Id extDimX, extDimY, sigDimX, sigDimY;
  const svtkm::Id sigStartX, sigStartY, sigPretendDimX, sigPretendDimY;
  const DWTMode mode;
  const ExtensionDirection direction;
  const bool padZero; // treat sigIn as having a column/row zeros
};

// Worklet: perform a simple 2D forward transform
class ForwardTransform2D : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // left/top extension
                                WholeArrayIn,   // sigIn
                                WholeArrayIn,   // right/bottom extension
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);
  using InputDomain = _6;

  // Constructor
  ForwardTransform2D(svtkm::Id filter_len,
                     svtkm::Id approx_len,
                     bool odd_low,
                     bool mode_lr,
                     svtkm::Id x1,
                     svtkm::Id y1, // dims of left/top extension
                     svtkm::Id x2,
                     svtkm::Id y2, // dims of signal
                     svtkm::Id startx2,
                     svtkm::Id starty2, // start idx of signal
                     svtkm::Id pretendx2,
                     svtkm::Id pretendy2, // pretend dims of signal
                     svtkm::Id x3,
                     svtkm::Id y3) // dims of right/bottom extension
    : filterLen(filter_len),
      approxLen(approx_len),
      outDimX(pretendx2),
      //outDimY(pretendy2),
      oddlow(odd_low),
      modeLR(mode_lr),
      translator(x1, y1, x2, y2, startx2, starty2, pretendx2, pretendy2, x3, y3, mode_lr)
  {
    this->SetStartPosition();
  }

  SVTKM_EXEC_CONT
  void Output1Dto2D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y) const
  {
    x = idx % outDimX;
    y = idx / outDimX;
  }
  SVTKM_EXEC_CONT
  svtkm::Id Output2Dto1D(svtkm::Id x, svtkm::Id y) const { return y * outDimX + x; }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1, typename InPortalType2, typename InPortalType3>
  SVTKM_EXEC_CONT VAL GetVal(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            svtkm::Id inMatrix,
                            svtkm::Id inIdx) const
  {
    if (inMatrix == 1)
    {
      return MAKEVAL(portal1.Get(inIdx));
    }
    else if (inMatrix == 2)
    {
      return MAKEVAL(portal2.Get(inIdx));
    }
    else if (inMatrix == 3)
    {
      return MAKEVAL(portal3.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename FilterPortalType,
            typename OutputPortalType>
  SVTKM_EXEC_CONT void operator()(const InPortalType1& inPortal1, // left/top extension
                                 const InPortalType2& inPortal2, // signal
                                 const InPortalType3& inPortal3, // right/bottom extension
                                 const FilterPortalType& lowFilter,
                                 const FilterPortalType& highFilter,
                                 OutputPortalType& coeffOut,
                                 const svtkm::Id& workIndex) const
  {
    svtkm::Id workX, workY, output1D;
    Output1Dto2D(workIndex, workX, workY);
    svtkm::Id inputMatrix = 0, inputIdx = 0;
    using OutputValueType = typename OutputPortalType::ValueType;

    if (modeLR)
    {
      if (workX % 2 == 0) // calculate cA
      {
        svtkm::Id xl = lstart + workX;
        VAL sum = MAKEVAL(0.0);
        for (svtkm::Id k = filterLen - 1; k > -1; k--)
        {
          translator.Translate2Dto1D(xl, workY, inputMatrix, inputIdx);
          sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx);
          xl++;
        }
        output1D = Output2Dto1D(workX / 2, workY);
        coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
      }
      else // calculate cD
      {
        svtkm::Id xh = hstart + workX - 1;
        VAL sum = MAKEVAL(0.0);
        for (svtkm::Id k = filterLen - 1; k > -1; k--)
        {
          translator.Translate2Dto1D(xh, workY, inputMatrix, inputIdx);
          sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx);
          xh++;
        }
        output1D = Output2Dto1D((workX - 1) / 2 + approxLen, workY);
        coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
      }
    }
    else // top-down order
    {
      if (workY % 2 == 0) // calculate cA
      {
        svtkm::Id yl = lstart + workY;
        VAL sum = MAKEVAL(0.0);
        for (svtkm::Id k = filterLen - 1; k > -1; k--)
        {
          translator.Translate2Dto1D(workX, yl, inputMatrix, inputIdx);
          sum += lowFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx);
          yl++;
        }
        output1D = Output2Dto1D(workX, workY / 2);
        coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
      }
      else // calculate cD
      {
        svtkm::Id yh = hstart + workY - 1;
        VAL sum = MAKEVAL(0.0);
        for (svtkm::Id k = filterLen - 1; k > -1; k--)
        {
          translator.Translate2Dto1D(workX, yh, inputMatrix, inputIdx);
          sum += highFilter.Get(k) * GetVal(inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx);
          yh++;
        }
        output1D = Output2Dto1D(workX, (workY - 1) / 2 + approxLen);
        coeffOut.Set(output1D, static_cast<OutputValueType>(sum));
      }
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen, approxLen;
  const svtkm::Id outDimX;
  //const svtkm::Id outDimY; Not used
  bool oddlow;
  bool modeLR; // true = left right; false = top down.
  const IndexTranslator3Matrices translator;
  svtkm::Id lstart, hstart;

  SVTKM_EXEC_CONT
  void SetStartPosition()
  {
    this->lstart = this->oddlow ? 1 : 0;
    this->hstart = 1;
  }
};

//  ---------------------------------------------------
//  |      |          |      |      |          |      |
//  |      |          |      |      |          |      |
//  | ext1 |    cA    | ext2 | ext3 |    cD    | ext4 |
//  | (x1) |   (xa)   | (x2) | (x3) |   (xd)   | (x4) |
//  |      |          |      |      |          |      |
//  ----------------------------------------------------
//  portal1: ext1
//  portal2: ext2
//  portal3: ext3
//  portal4: ext4
//  portal5: cA + cD
// Worklet: perform a simple 2D inverse transform
class InverseTransform2D : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, // ext1
                                WholeArrayIn, // ext2
                                WholeArrayIn, // ext3
                                WholeArrayIn, // ext4
                                WholeArrayIn, // cA+cD (signal)
                                WholeArrayIn, // lowFilter
                                WholeArrayIn, // highFilter
                                FieldOut);    // outptu coeffs
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);
  using InputDomain = _8;

  // Constructor
  InverseTransform2D(svtkm::Id fil_len,
                     svtkm::Id x_1,
                     svtkm::Id y_1, // ext1
                     svtkm::Id x_a,
                     svtkm::Id y_a, // cA
                     svtkm::Id x_2,
                     svtkm::Id y_2, // ext2
                     svtkm::Id x_3,
                     svtkm::Id y_3, // ext3
                     svtkm::Id x_d,
                     svtkm::Id y_d, // cD
                     svtkm::Id x_4,
                     svtkm::Id y_4, // ext4
                     svtkm::Id x_5,
                     svtkm::Id y_5,
                     svtkm::Id startX5,
                     svtkm::Id startY5,
                     bool mode_lr)
    : filterLen(fil_len)
    , translator(x_1,
                 y_1,
                 x_a,
                 y_a,
                 x_2,
                 y_2,
                 x_3,
                 y_3,
                 x_d,
                 y_d,
                 x_4,
                 y_4,
                 x_5,
                 y_5,
                 startX5,
                 startY5,
                 mode_lr)
    , modeLR(mode_lr)
  {
    if (modeLR)
    {
      outputDimX = x_a + x_d;
      outputDimY = y_1;
      cALenExtended = x_1 + x_a + x_2;
    }
    else
    {
      outputDimX = x_1;
      outputDimY = y_a + y_d;
      cALenExtended = y_1 + y_a + y_2;
    }
  }

  SVTKM_EXEC_CONT
  void Output1Dto2D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y) const
  {
    x = idx % outputDimX;
    y = idx / outputDimX;
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalTypecAcD>
  SVTKM_EXEC_CONT VAL GetVal(const InPortalType1& ext1,
                            const InPortalType2& ext2,
                            const InPortalType3& ext3,
                            const InPortalType4& ext4,
                            const InPortalTypecAcD& cAcD,
                            svtkm::Id inMatrix,
                            svtkm::Id inIdx) const
  {
    if (inMatrix == 1)
    {
      return MAKEVAL(ext1.Get(inIdx));
    }
    else if (inMatrix == 2)
    {
      return MAKEVAL(ext2.Get(inIdx));
    }
    else if (inMatrix == 3)
    {
      return MAKEVAL(ext3.Get(inIdx));
    }
    else if (inMatrix == 4)
    {
      return MAKEVAL(ext4.Get(inIdx));
    }
    else if (inMatrix == 5)
    {
      return MAKEVAL(cAcD.Get(inIdx));
    }
    else
    {
      return -1;
    }
  }

  template <typename InPortalType1,
            typename InPortalType2,
            typename InPortalType3,
            typename InPortalType4,
            typename InPortalTypecAcD,
            typename FilterPortalType,
            typename OutputValueType>
  SVTKM_EXEC void operator()(const InPortalType1& portal1,
                            const InPortalType2& portal2,
                            const InPortalType3& portal3,
                            const InPortalType4& portal4,
                            const InPortalTypecAcD& portalcAcD,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputValueType& coeffOut,
                            const svtkm::Id& workIdx) const
  {
    svtkm::Id workX, workY;
    svtkm::Id k1 = 0, k2 = 0, xi = 0, yi = 0, inputMatrix = 0, inputIdx = 0;
    Output1Dto2D(workIdx, workX, workY);

    // left-right, odd filter
    if (modeLR && (filterLen % 2 != 0))
    {
      if (workX % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      xi = (workX + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate2Dto1D(xi, workY, inputMatrix, inputIdx);
        sum += lowFilter.Get(k1) *
          GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        xi++;
        k1 -= 2;
      }
      xi = workX / 2;
      while (k2 > -1)
      {
        translator.Translate2Dto1D(xi + cALenExtended, workY, inputMatrix, inputIdx);
        sum += highFilter.Get(k2) *
          GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        xi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }

    // top-down, odd filter
    else if (!modeLR && (filterLen % 2 != 0))
    {
      if (workY % 2 != 0)
      {
        k1 = filterLen - 2;
        k2 = filterLen - 1;
      }
      else
      {
        k1 = filterLen - 1;
        k2 = filterLen - 2;
      }

      VAL sum = 0.0;
      yi = (workY + 1) / 2;
      while (k1 > -1)
      {
        translator.Translate2Dto1D(workX, yi, inputMatrix, inputIdx);
        VAL cA = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        sum += lowFilter.Get(k1) * cA;
        yi++;
        k1 -= 2;
      }
      yi = workY / 2;
      while (k2 > -1)
      {
        translator.Translate2Dto1D(workX, yi + cALenExtended, inputMatrix, inputIdx);
        VAL cD = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        sum += highFilter.Get(k2) * cD;
        yi++;
        k2 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }

    // left-right, even filter
    else if (modeLR && (filterLen % 2 == 0))
    {
      if ((filterLen / 2) % 2 != 0) // odd length half filter
      {
        xi = workX / 2;
        if (workX % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else // even length half filter
      {
        xi = (workX + 1) / 2;
        if (workX % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate2Dto1D(xi, workY, inputMatrix, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        translator.Translate2Dto1D(xi + cALenExtended, workY, inputMatrix, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        xi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }

    // top-down, even filter
    else
    {
      if ((filterLen / 2) % 2 != 0)
      {
        yi = workY / 2;
        if (workY % 2 != 0)
        {
          k1 = filterLen - 1;
        }
        else
        {
          k1 = filterLen - 2;
        }
      }
      else
      {
        yi = (workY + 1) / 2;
        if (workY % 2 != 0)
        {
          k1 = filterLen - 2;
        }
        else
        {
          k1 = filterLen - 1;
        }
      }
      VAL cA, cD;
      VAL sum = 0.0;
      while (k1 > -1)
      {
        translator.Translate2Dto1D(workX, yi, inputMatrix, inputIdx);
        cA = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        translator.Translate2Dto1D(workX, yi + cALenExtended, inputMatrix, inputIdx);
        cD = GetVal(portal1, portal2, portal3, portal4, portalcAcD, inputMatrix, inputIdx);
        sum += lowFilter.Get(k1) * cA + highFilter.Get(k1) * cD;
        yi++;
        k1 -= 2;
      }
      coeffOut = static_cast<OutputValueType>(sum);
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen;
  svtkm::Id outputDimX, outputDimY;
  svtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  const IndexTranslator6Matrices translator;
  const bool modeLR;
};

// Worklet: perform a simple 1D forward transform
class ForwardTransform : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // sigIn
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // cA followed by cD
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  using InputDomain = _1;

  // Constructor
  ForwardTransform(svtkm::Id filLen,
                   svtkm::Id approx_len,
                   svtkm::Id detail_len,
                   bool odd_low,
                   bool odd_high)
    : filterLen(filLen)
    , approxLen(approx_len)
    , detailLen(detail_len)
    , oddlow(odd_low)
    , oddhigh(odd_high)
  {
    this->SetStartPosition();
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename FilterPortalType, typename OutputPortalType>
  SVTKM_EXEC void operator()(const InputPortalType& signalIn,
                            const FilterPortalType lowFilter,
                            const FilterPortalType highFilter,
                            OutputPortalType& coeffOut,
                            const svtkm::Id& workIndex) const
  {
    using OutputValueType = typename OutputPortalType::ValueType;
    if (workIndex < approxLen + detailLen)
    {
      if (workIndex % 2 == 0) // calculate cA
      {
        svtkm::Id xl = xlstart + workIndex;
        VAL sum = MAKEVAL(0.0);
        for (svtkm::Id k = filterLen - 1; k >= 0; k--)
        {
          sum += lowFilter.Get(k) * MAKEVAL(signalIn.Get(xl++));
        }
        svtkm::Id outputIdx = workIndex / 2; // put cA at the beginning
        coeffOut.Set(outputIdx, static_cast<OutputValueType>(sum));
      }
      else // calculate cD
      {
        VAL sum = MAKEVAL(0.0);
        svtkm::Id xh = xhstart + workIndex - 1;
        for (svtkm::Id k = filterLen - 1; k >= 0; k--)
        {
          sum += highFilter.Get(k) * MAKEVAL(signalIn.Get(xh++));
        }
        svtkm::Id outputIdx = approxLen + (workIndex - 1) / 2; // put cD after cA
        coeffOut.Set(outputIdx, static_cast<OutputValueType>(sum));
      }
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen, approxLen, detailLen; // filter and outcome coeff length.
  bool oddlow, oddhigh;
  svtkm::Id xlstart, xhstart;

  SVTKM_EXEC_CONT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow ? 1 : 0;
    this->xhstart = this->oddhigh ? 1 : 0;
  }
};

// Worklet: perform an 1D inverse transform for odd length, symmetric filters.
class InverseTransformOdd : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // Input: coeffs, cA followed by cD
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // output
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  InverseTransformOdd(svtkm::Id filLen, svtkm::Id ca_len, svtkm::Id ext_len)
    : filterLen(filLen)
    //, cALen(ca_len)
    , cALen2(ca_len * 2)
    , cALenExtended(ext_len)
  {
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename FilterPortalType, typename OutputPortalType>
  SVTKM_EXEC void operator()(const InputPortalType& coeffs,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputPortalType& sigOut,
                            svtkm::Id workIndex) const
  {
    if (workIndex >= cALen2) // valid calculation region
    {
      return;
    }
    svtkm::Id xi1 = (workIndex + 1) / 2;                     // coeff indices
    svtkm::Id xi2 = this->cALenExtended + ((workIndex) / 2); // coeff indices
    VAL sum = 0.0;

    const bool odd = workIndex % 2 != 0;
    if (odd)
    {
      svtkm::Id k1 = this->filterLen - 2;
      svtkm::Id k2 = this->filterLen - 1;
      for (; k1 >= 0; k1 -= 2, k2 -= 2)
      {
        sum += lowFilter.Get(k1) * MAKEVAL(coeffs.Get(xi1++));
        sum += highFilter.Get(k2) * MAKEVAL(coeffs.Get(xi2++));
      }
      if (k2 >= 0)
      {
        sum += highFilter.Get(k2) * MAKEVAL(coeffs.Get(xi2++));
      }
    }
    else //even
    {
      svtkm::Id k1 = this->filterLen - 1;
      svtkm::Id k2 = this->filterLen - 2;
      for (; k2 >= 0; k1 -= 2, k2 -= 2)
      {
        sum += lowFilter.Get(k1) * MAKEVAL(coeffs.Get(xi1++));
        sum += highFilter.Get(k2) * MAKEVAL(coeffs.Get(xi2++));
      }
      if (k1 >= 0)
      {
        sum += lowFilter.Get(k1) * MAKEVAL(coeffs.Get(xi1++));
      }
    }

    sigOut.Set(workIndex, static_cast<typename OutputPortalType::ValueType>(sum));
  }

#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen; // filter length.
  //const svtkm::Id cALen;         // Number of actual cAs  (Not used)
  const svtkm::Id cALen2;        //  = cALen * 2
  const svtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
};

// Worklet: perform an 1D inverse transform for even length, symmetric filters.
class InverseTransformEven : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn,   // Input: coeffs, cA followed by cD
                                WholeArrayIn,   // lowFilter
                                WholeArrayIn,   // highFilter
                                WholeArrayOut); // output
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  using InputDomain = _1;

  // Constructor
  InverseTransformEven(svtkm::Id filtL, svtkm::Id cAL, svtkm::Id cALExt, bool m)
    : filterLen(filtL)
    //, cALen(cAL)
    , cALen2(cAL * 2)
    , cALenExtended(cALExt)
    , matlab(m)
  {
  }

// Use 64-bit float for convolution calculation
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
  template <typename InputPortalType, typename FilterPortalType, typename OutputPortalType>
  SVTKM_EXEC void operator()(const InputPortalType& coeffs,
                            const FilterPortalType& lowFilter,
                            const FilterPortalType& highFilter,
                            OutputPortalType& sigOut,
                            const svtkm::Id& workIndex) const
  {
    if (workIndex < cALen2) // valid calculation region
    {
      svtkm::Id xi; // coeff indices
      svtkm::Id k;  // indices for low and high filter
      VAL sum = 0.0;

      if (matlab || (filterLen / 2) % 2 != 0) // odd length half filter
      {
        xi = workIndex / 2;
        if (workIndex % 2 != 0)
        {
          k = filterLen - 1;
        }
        else
        {
          k = filterLen - 2;
        }
      }
      else
      {
        xi = (workIndex + 1) / 2;
        if (workIndex % 2 != 0)
        {
          k = filterLen - 2;
        }
        else
        {
          k = filterLen - 1;
        }
      }

      while (k > -1) // k >= 0
      {
        sum += lowFilter.Get(k) * MAKEVAL(coeffs.Get(xi)) +            // cA
          highFilter.Get(k) * MAKEVAL(coeffs.Get(xi + cALenExtended)); // cD
        xi++;
        k -= 2;
      }

      sigOut.Set(workIndex, static_cast<typename OutputPortalType::ValueType>(sum));
    }
  }
#undef MAKEVAL
#undef VAL

private:
  const svtkm::Id filterLen; // filter length.
  //const svtkm::Id cALen;         // Number of actual cAs (not used)
  const svtkm::Id cALen2;        //  = cALen * 2
  const svtkm::Id cALenExtended; // Number of cA at the beginning of input, followed by cD
  bool matlab;                  // followed the naming convention from VAPOR
                                // It's always false for the 1st 4 filters.
};

class ThresholdWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut); // Thresholding in-place
  using ExecutionSignature = void(_1);
  using InputDomain = _1;

  // Constructor
  ThresholdWorklet(svtkm::Float64 t)
    : threshold(t)
    , // must pass in a positive val
    neg_threshold(t * -1.0)
  {
  }

  template <typename ValueType>
  SVTKM_EXEC void operator()(ValueType& coeffVal) const
  {
    if (neg_threshold < coeffVal && coeffVal < threshold)
    {
      coeffVal = 0.0;
    }
  }

private:
  svtkm::Float64 threshold;     // positive
  svtkm::Float64 neg_threshold; // negative
};

class SquaredDeviation : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);
  using InputDomain = _1;

  // Constructor
  template <typename ValueType>
  SVTKM_EXEC_CONT SquaredDeviation(ValueType t)
  {
    this->mean = static_cast<svtkm::Float64>(t);
  }

  template <typename ValueType>
  SVTKM_EXEC ValueType operator()(const ValueType& num) const
  {
    svtkm::Float64 num64 = static_cast<svtkm::Float64>(num);
    svtkm::Float64 diff = this->mean - num64;
    return static_cast<ValueType>(diff * diff);
  }

private:
  svtkm::Float64 mean;
};

class Differencer : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = _3(_1, _2);
  using InputDomain = _1;

  template <typename ValueType1, typename ValueType2>
  SVTKM_EXEC ValueType1 operator()(const ValueType1& v1, const ValueType2& v2) const
  {
    return v1 - static_cast<ValueType1>(v2);
  }
};

class SquareWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);
  using InputDomain = _1;

  template <typename ValueType>
  SVTKM_EXEC ValueType operator()(const ValueType& v) const
  {
    return (v * v);
  }
};

class CopyWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  CopyWorklet(svtkm::Id idx) { this->startIdx = idx; }

  template <typename PortalInType, typename PortalOutType>
  SVTKM_EXEC void operator()(const PortalInType& portalIn,
                            PortalOutType& portalOut,
                            const svtkm::Id& workIndex) const
  {
    portalOut.Set((startIdx + workIndex), portalIn.Get(workIndex));
  }

private:
  svtkm::Id startIdx;
};

// Worklet for 1D signal extension no. 1
class LeftSYMHExtentionWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  LeftSYMHExtentionWorklet(svtkm::Id len)
    : addLen(len)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const svtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(this->addLen - workIndex - 1));
  }

private:
  svtkm::Id addLen;
};

// Worklet for 1D signal extension no. 2
class LeftSYMWExtentionWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  LeftSYMWExtentionWorklet(svtkm::Id len)
    : addLen(len)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const svtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(this->addLen - workIndex));
  }

private:
  svtkm::Id addLen;
};

// Worklet for 1D signal extension no. 3
class LeftASYMHExtentionWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  LeftASYMHExtentionWorklet(svtkm::Id len)
    : addLen(len)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC_CONT void operator()(PortalOutType& portalOut,
                                 const PortalInType& portalIn,
                                 const svtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(addLen - workIndex - 1) * (-1.0));
  }

private:
  svtkm::Id addLen;
};

// Worklet for 1D signal extension no. 4
class LeftASYMWExtentionWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  LeftASYMWExtentionWorklet(svtkm::Id len)
    : addLen(len)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC_CONT void operator()(PortalOutType& portalOut,
                                 const PortalInType& portalIn,
                                 const svtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(addLen - workIndex) * (-1.0));
  }

private:
  svtkm::Id addLen;
};

// Worklet for 1D signal extension no. 5
class RightSYMHExtentionWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  RightSYMHExtentionWorklet(svtkm::Id sigInl)
    : sigInLen(sigInl)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const svtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(this->sigInLen - workIndex - 1));
  }

private:
  svtkm::Id sigInLen;
};

// Worklet for 1D signal extension no. 6
class RightSYMWExtentionWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  RightSYMWExtentionWorklet(svtkm::Id sigInl)
    : sigInLen(sigInl)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC void operator()(PortalOutType& portalOut,
                            const PortalInType& portalIn,
                            const svtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(this->sigInLen - workIndex - 2));
  }

private:
  svtkm::Id sigInLen;
};

// Worklet for 1D signal extension no. 7
class RightASYMHExtentionWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  RightASYMHExtentionWorklet(svtkm::Id sigInl)
    : sigInLen(sigInl)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC_CONT void operator()(PortalOutType& portalOut,
                                 const PortalInType& portalIn,
                                 const svtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(sigInLen - workIndex - 1) * (-1.0));
  }

private:
  svtkm::Id sigInLen;
};

// Worklet for 1D signal extension no. 8
class RightASYMWExtentionWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayOut, // extension part
                                WholeArrayIn); // signal part
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  // Constructor
  SVTKM_EXEC_CONT
  RightASYMWExtentionWorklet(svtkm::Id sigInl)
    : sigInLen(sigInl)
  {
  }

  template <typename PortalOutType, typename PortalInType>
  SVTKM_EXEC_CONT void operator()(PortalOutType& portalOut,
                                 const PortalInType& portalIn,
                                 const svtkm::Id& workIndex) const
  {
    portalOut.Set(workIndex, portalIn.Get(sigInLen - workIndex - 2) * (-1.0));
  }

private:
  svtkm::Id sigInLen;
};

// Assign zero to a single index
class AssignZeroWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  // Constructor
  SVTKM_EXEC_CONT
  AssignZeroWorklet(svtkm::Id idx)
    : zeroIdx(idx)
  {
  }

  template <typename PortalType>
  SVTKM_EXEC void operator()(PortalType& array, const svtkm::Id& workIdx) const
  {
    if (workIdx == this->zeroIdx)
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
  }

private:
  svtkm::Id zeroIdx;
};

// Assign zero to a row or a column in a 2D array.
// Change row or column is controlled by negative indices.
class AssignZero2DWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  // Constructor
  SVTKM_EXEC_CONT
  AssignZero2DWorklet(svtkm::Id x, svtkm::Id y, svtkm::Id zero_x, svtkm::Id zero_y)
    : dimX(x)
    , dimY(y)
    , zeroX(zero_x)
    , zeroY(zero_y)
  {
    (void)dimY;
  }

  // Index translation helper
  SVTKM_EXEC_CONT
  void GetLogicalDim(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y) const
  {
    x = idx % dimX;
    y = idx / dimX;
  }

  template <typename PortalType>
  SVTKM_EXEC void operator()(PortalType& array, const svtkm::Id& workIdx) const
  {
    svtkm::Id x, y;
    GetLogicalDim(workIdx, x, y);
    if (zeroY < 0 && x == zeroX) // assign zero to a column
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
    else if (zeroX < 0 && y == zeroY) // assign zero to a row
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
  }

private:
  svtkm::Id dimX, dimY;
  svtkm::Id zeroX, zeroY; // element at (zeroX, zeroY) will be assigned zero.
                         // each becomes a wild card if negative
};

// Assign zero to a plane (2D) in a 3D cube.
// Which plane to assign zero is controlled by negative indices.
class AssignZero3DWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  // Constructor
  SVTKM_EXEC_CONT
  AssignZero3DWorklet(svtkm::Id x,
                      svtkm::Id y,
                      svtkm::Id z,
                      svtkm::Id zero_x,
                      svtkm::Id zero_y,
                      svtkm::Id zero_z)
    : dimX(x)
    , dimY(y)
    , dimZ(z)
    , zeroX(zero_x)
    , zeroY(zero_y)
    , zeroZ(zero_z)
  {
    (void)dimZ;
  }

  // Index translation helper
  SVTKM_EXEC_CONT
  void GetLogicalDim(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (dimX * dimY);
    y = (idx - z * dimX * dimY) / dimX;
    x = idx % dimX;
  }

  template <typename PortalType>
  SVTKM_EXEC void operator()(PortalType& array, const svtkm::Id& workIdx) const
  {
    svtkm::Id x, y, z;
    GetLogicalDim(workIdx, x, y, z);
    if (zeroZ < 0 && zeroY < 0 && x == zeroX) // plane perpendicular to X axis
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
    else if (zeroZ < 0 && zeroX < 0 && y == zeroY) // plane perpendicular to Y axis
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
    else if (zeroY < 0 && zeroX < 0 && z == zeroZ) // plane perpendicular to Z axis
    {
      array.Set(workIdx, static_cast<typename PortalType::ValueType>(0.0));
    }
  }

private:
  svtkm::Id dimX, dimY, dimZ;
  svtkm::Id zeroX, zeroY, zeroZ; // element at (zeroX, zeroY, zeroZ) will be assigned zero.
                                // each becomes a wild card if negative
};

// Worklet: Copies a small rectangle to become a part of a big rectangle
class RectangleCopyTo : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn,        // Input, small rectangle
                                WholeArrayOut); // Output, big rectangle
  using ExecutionSignature = void(_1, _2, WorkIndex);

  // Constructor
  SVTKM_EXEC_CONT
  RectangleCopyTo(svtkm::Id inx,
                  svtkm::Id iny,
                  svtkm::Id outx,
                  svtkm::Id outy,
                  svtkm::Id xStart,
                  svtkm::Id yStart)
    : inXLen(inx)
    , inYLen(iny)
    , outXLen(outx)
    , outYLen(outy)
    , outXStart(xStart)
    , outYStart(yStart)
  {
    (void)outYLen;
    (void)inYLen;
  }

  SVTKM_EXEC_CONT
  void GetLogicalDimOfInputRect(const svtkm::Id& idx, svtkm::Id& x, svtkm::Id& y) const
  {
    x = idx % inXLen;
    y = idx / inXLen;
  }

  SVTKM_EXEC_CONT
  svtkm::Id Get1DIdxOfOutputRect(svtkm::Id x, svtkm::Id y) const { return y * outXLen + x; }

  template <typename ValueInType, typename PortalOutType>
  SVTKM_EXEC void operator()(const ValueInType& valueIn,
                            PortalOutType& arrayOut,
                            const svtkm::Id& workIdx) const
  {
    svtkm::Id xOfIn, yOfIn;
    GetLogicalDimOfInputRect(workIdx, xOfIn, yOfIn);
    svtkm::Id outputIdx = Get1DIdxOfOutputRect(xOfIn + outXStart, yOfIn + outYStart);
    arrayOut.Set(outputIdx, valueIn);
  }

private:
  svtkm::Id inXLen, inYLen;
  svtkm::Id outXLen, outYLen;
  svtkm::Id outXStart, outYStart;
};

// Worklet: Copies a small cube to become a part of a big cube
class CubeCopyTo : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn,        // Input, small cube
                                WholeArrayOut); // Output, big cube
  using ExecutionSignature = void(_1, _2, WorkIndex);

  // Constructor
  SVTKM_EXEC_CONT
  CubeCopyTo(svtkm::Id inx,
             svtkm::Id iny,
             svtkm::Id inz,
             svtkm::Id outx,
             svtkm::Id outy,
             svtkm::Id outz,
             svtkm::Id xStart,
             svtkm::Id yStart,
             svtkm::Id zStart)
    : inDimX(inx)
    , inDimY(iny)
    , inDimZ(inz)
    , outDimX(outx)
    , outDimY(outy)
    , outDimZ(outz)
    , outStartX(xStart)
    , outStartY(yStart)
    , outStartZ(zStart)
  {
    (void)outDimZ;
    (void)inDimZ;
  }

  SVTKM_EXEC_CONT
  void GetLogicalDimOfInputCube(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (inDimX * inDimY);
    y = (idx - z * inDimX * inDimY) / inDimX;
    x = idx % inDimX;
  }

  SVTKM_EXEC_CONT
  svtkm::Id Get1DIdxOfOutputCube(svtkm::Id x, svtkm::Id y, svtkm::Id z) const
  {
    return z * outDimX * outDimY + y * outDimX + x;
  }

  template <typename ValueInType, typename PortalOutType>
  SVTKM_EXEC void operator()(const ValueInType& valueIn,
                            PortalOutType& arrayOut,
                            const svtkm::Id& workIdx) const
  {
    svtkm::Id inX, inY, inZ;
    GetLogicalDimOfInputCube(workIdx, inX, inY, inZ);
    svtkm::Id outputIdx = Get1DIdxOfOutputCube(inX + outStartX, inY + outStartY, inZ + outStartZ);
    arrayOut.Set(outputIdx, valueIn);
  }

private:
  const svtkm::Id inDimX, inDimY, inDimZ;          // input small cube
  const svtkm::Id outDimX, outDimY, outDimZ;       // output big cube
  const svtkm::Id outStartX, outStartY, outStartZ; // where to put
};

} // namespace wavelets
} // namespace worlet
} // namespace svtkm

#endif // svtk_m_worklet_Wavelets_h
