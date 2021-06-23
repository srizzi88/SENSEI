//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_MatrixHelpers_h
#define svtk_m_rendering_MatrixHelpers_h

#include <svtkm/Matrix.h>
#include <svtkm/VectorAnalysis.h>

namespace svtkm
{
namespace rendering
{

struct MatrixHelpers
{
  static SVTKM_CONT void CreateOGLMatrix(const svtkm::Matrix<svtkm::Float32, 4, 4>& mtx,
                                        svtkm::Float32* oglM)
  {
    oglM[0] = mtx[0][0];
    oglM[1] = mtx[1][0];
    oglM[2] = mtx[2][0];
    oglM[3] = mtx[3][0];
    oglM[4] = mtx[0][1];
    oglM[5] = mtx[1][1];
    oglM[6] = mtx[2][1];
    oglM[7] = mtx[3][1];
    oglM[8] = mtx[0][2];
    oglM[9] = mtx[1][2];
    oglM[10] = mtx[2][2];
    oglM[11] = mtx[3][2];
    oglM[12] = mtx[0][3];
    oglM[13] = mtx[1][3];
    oglM[14] = mtx[2][3];
    oglM[15] = mtx[3][3];
  }

  static SVTKM_CONT svtkm::Matrix<svtkm::Float32, 4, 4> ViewMatrix(const svtkm::Vec3f_32& position,
                                                                const svtkm::Vec3f_32& lookAt,
                                                                const svtkm::Vec3f_32& up)
  {
    svtkm::Vec3f_32 viewDir = position - lookAt;
    svtkm::Vec3f_32 right = svtkm::Cross(up, viewDir);
    svtkm::Vec3f_32 ru = svtkm::Cross(viewDir, right);

    svtkm::Normalize(viewDir);
    svtkm::Normalize(right);
    svtkm::Normalize(ru);

    svtkm::Matrix<svtkm::Float32, 4, 4> matrix;
    svtkm::MatrixIdentity(matrix);

    matrix(0, 0) = right[0];
    matrix(0, 1) = right[1];
    matrix(0, 2) = right[2];
    matrix(1, 0) = ru[0];
    matrix(1, 1) = ru[1];
    matrix(1, 2) = ru[2];
    matrix(2, 0) = viewDir[0];
    matrix(2, 1) = viewDir[1];
    matrix(2, 2) = viewDir[2];

    matrix(0, 3) = -svtkm::Dot(right, position);
    matrix(1, 3) = -svtkm::Dot(ru, position);
    matrix(2, 3) = -svtkm::Dot(viewDir, position);

    return matrix;
  }

  static SVTKM_CONT svtkm::Matrix<svtkm::Float32, 4, 4> WorldMatrix(const svtkm::Vec3f_32& neworigin,
                                                                 const svtkm::Vec3f_32& newx,
                                                                 const svtkm::Vec3f_32& newy,
                                                                 const svtkm::Vec3f_32& newz)
  {
    svtkm::Matrix<svtkm::Float32, 4, 4> matrix;
    svtkm::MatrixIdentity(matrix);

    matrix(0, 0) = newx[0];
    matrix(0, 1) = newy[0];
    matrix(0, 2) = newz[0];
    matrix(1, 0) = newx[1];
    matrix(1, 1) = newy[1];
    matrix(1, 2) = newz[1];
    matrix(2, 0) = newx[2];
    matrix(2, 1) = newy[2];
    matrix(2, 2) = newz[2];

    matrix(0, 3) = neworigin[0];
    matrix(1, 3) = neworigin[1];
    matrix(2, 3) = neworigin[2];

    return matrix;
  }

  static SVTKM_CONT svtkm::Matrix<svtkm::Float32, 4, 4> CreateScale(const svtkm::Float32 x,
                                                                 const svtkm::Float32 y,
                                                                 const svtkm::Float32 z)
  {
    svtkm::Matrix<svtkm::Float32, 4, 4> matrix;
    svtkm::MatrixIdentity(matrix);
    matrix[0][0] = x;
    matrix[1][1] = y;
    matrix[2][2] = z;

    return matrix;
  }

  static SVTKM_CONT svtkm::Matrix<svtkm::Float32, 4, 4> TrackballMatrix(svtkm::Float32 p1x,
                                                                     svtkm::Float32 p1y,
                                                                     svtkm::Float32 p2x,
                                                                     svtkm::Float32 p2y)
  {
    const svtkm::Float32 RADIUS = 0.80f;     //z value lookAt x = y = 0.0
    const svtkm::Float32 COMPRESSION = 3.5f; // multipliers for x and y.
    const svtkm::Float32 AR3 = RADIUS * RADIUS * RADIUS;

    svtkm::Matrix<svtkm::Float32, 4, 4> matrix;

    svtkm::MatrixIdentity(matrix);
    if (p1x == p2x && p1y == p2y)
    {
      return matrix;
    }

    svtkm::Vec3f_32 p1(p1x, p1y, AR3 / ((p1x * p1x + p1y * p1y) * COMPRESSION + AR3));
    svtkm::Vec3f_32 p2(p2x, p2y, AR3 / ((p2x * p2x + p2y * p2y) * COMPRESSION + AR3));
    svtkm::Vec3f_32 axis = svtkm::Normal(svtkm::Cross(p2, p1));

    svtkm::Vec3f_32 p2_p1(p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]);
    svtkm::Float32 t = svtkm::Magnitude(p2_p1);
    t = svtkm::Min(svtkm::Max(t, -1.0f), 1.0f);
    svtkm::Float32 phi = static_cast<svtkm::Float32>(-2.0f * asin(t / (2.0f * RADIUS)));
    svtkm::Float32 val = static_cast<svtkm::Float32>(sin(phi / 2.0f));
    axis[0] *= val;
    axis[1] *= val;
    axis[2] *= val;

    //quaternion
    svtkm::Float32 q[4] = { axis[0], axis[1], axis[2], static_cast<svtkm::Float32>(cos(phi / 2.0f)) };

    // normalize quaternion to unit magnitude
    t = 1.0f /
      static_cast<svtkm::Float32>(sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]));
    q[0] *= t;
    q[1] *= t;
    q[2] *= t;
    q[3] *= t;

    matrix(0, 0) = 1 - 2 * (q[1] * q[1] + q[2] * q[2]);
    matrix(0, 1) = 2 * (q[0] * q[1] + q[2] * q[3]);
    matrix(0, 2) = (2 * (q[2] * q[0] - q[1] * q[3]));

    matrix(1, 0) = 2 * (q[0] * q[1] - q[2] * q[3]);
    matrix(1, 1) = 1 - 2 * (q[2] * q[2] + q[0] * q[0]);
    matrix(1, 2) = (2 * (q[1] * q[2] + q[0] * q[3]));

    matrix(2, 0) = (2 * (q[2] * q[0] + q[1] * q[3]));
    matrix(2, 1) = (2 * (q[1] * q[2] - q[0] * q[3]));
    matrix(2, 2) = (1 - 2 * (q[1] * q[1] + q[0] * q[0]));

    return matrix;
  }
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_MatrixHelpers_h
