//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Sampler_h
#define svtk_m_rendering_raytracing_Sampler_h
#include <svtkm/Math.h>
#include <svtkm/VectorAnalysis.h>
namespace svtkm
{
namespace rendering
{
namespace raytracing
{

template <svtkm::Int32 Base>
SVTKM_EXEC void Halton2D(const svtkm::Int32& sampleNum, svtkm::Vec2f_32& coord)
{
  //generate base2 halton
  svtkm::Float32 x = 0.0f;
  svtkm::Float32 xadd = 1.0f;
  svtkm::UInt32 b2 = 1 + static_cast<svtkm::UInt32>(sampleNum);
  while (b2 != 0)
  {
    xadd *= 0.5f;
    if ((b2 & 1) != 0)
      x += xadd;
    b2 >>= 1;
  }

  svtkm::Float32 y = 0.0f;
  svtkm::Float32 yadd = 1.0f;
  svtkm::Int32 bn = 1 + sampleNum;
  while (bn != 0)
  {
    yadd *= 1.0f / (svtkm::Float32)Base;
    y += (svtkm::Float32)(bn % Base) * yadd;
    bn /= Base;
  }

  coord[0] = x;
  coord[1] = y;
} // Halton2D

SVTKM_EXEC
svtkm::Vec3f_32 CosineWeightedHemisphere(const svtkm::Int32& sampleNum, const svtkm::Vec3f_32& normal)
{
  //generate orthoganal basis about normal
  int kz = 0;
  if (svtkm::Abs(normal[0]) > svtkm::Abs(normal[1]))
  {
    if (svtkm::Abs(normal[0]) > svtkm::Abs(normal[2]))
      kz = 0;
    else
      kz = 2;
  }
  else
  {
    if (svtkm::Abs(normal[1]) > svtkm::Abs(normal[2]))
      kz = 1;
    else
      kz = 2;
  }
  svtkm::Vec3f_32 notNormal;
  notNormal[0] = 0.f;
  notNormal[1] = 0.f;
  notNormal[2] = 0.f;
  notNormal[kz] = 1.f;

  svtkm::Vec3f_32 xAxis = svtkm::Cross(normal, notNormal);
  svtkm::Normalize(xAxis);
  svtkm::Vec3f_32 yAxis = svtkm::Cross(normal, xAxis);
  svtkm::Normalize(yAxis);

  svtkm::Vec2f_32 xy;
  Halton2D<3>(sampleNum, xy);
  const svtkm::Float32 r = Sqrt(xy[0]);
  const svtkm::Float32 theta = 2 * static_cast<svtkm::Float32>(svtkm::Pi()) * xy[1];

  svtkm::Vec3f_32 direction(0.f, 0.f, 0.f);
  direction[0] = r * svtkm::Cos(theta);
  direction[1] = r * svtkm::Sin(theta);
  direction[2] = svtkm::Sqrt(svtkm::Max(0.0f, 1.f - xy[0]));

  svtkm::Vec3f_32 sampleDir;
  sampleDir[0] = svtkm::dot(direction, xAxis);
  sampleDir[1] = svtkm::dot(direction, yAxis);
  sampleDir[2] = svtkm::dot(direction, normal);
  return sampleDir;
}
}
}
} // namespace svtkm::rendering::raytracing
#endif
