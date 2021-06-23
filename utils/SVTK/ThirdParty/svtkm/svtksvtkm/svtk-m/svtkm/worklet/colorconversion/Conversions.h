//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_colorconversion_Conversions_h
#define svtk_m_worklet_colorconversion_Conversions_h

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{

template <typename T>
SVTKM_EXEC inline svtkm::UInt8 ColorToUChar(T t)
{
  return static_cast<svtkm::UInt8>(t);
}

template <>
SVTKM_EXEC inline svtkm::UInt8 ColorToUChar(svtkm::Float64 t)
{
  return static_cast<svtkm::UInt8>(t * 255.0f + 0.5f);
}

template <>
SVTKM_EXEC inline svtkm::UInt8 ColorToUChar(svtkm::Float32 t)
{
  return static_cast<svtkm::UInt8>(t * 255.0f + 0.5f);
}


SVTKM_EXEC inline void Clamp(svtkm::Float32& val)
{
  val = svtkm::Min(255.0f, svtkm::Max(0.0f, val));
}
SVTKM_EXEC inline void Clamp(svtkm::Vec2f_32& val)
{
  val[0] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[0]));
  val[1] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[1]));
}
SVTKM_EXEC inline void Clamp(svtkm::Vec3f_32& val)
{
  val[0] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[0]));
  val[1] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[1]));
  val[2] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[2]));
}
SVTKM_EXEC inline void Clamp(svtkm::Vec4f_32& val)
{
  val[0] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[0]));
  val[1] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[1]));
  val[2] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[2]));
  val[3] = svtkm::Min(255.0f, svtkm::Max(0.0f, val[3]));
}
}
}
}
#endif
