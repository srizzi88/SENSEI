//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_functions_h
#define svtk_m_worklet_zfp_functions_h

#include <svtkm/Math.h>
#include <svtkm/worklet/zfp/ZFPBlockWriter.h>
#include <svtkm/worklet/zfp/ZFPCodec.h>
#include <svtkm/worklet/zfp/ZFPTypeInfo.h>

namespace svtkm
{
namespace worklet
{
namespace zfp
{

template <typename T>
void PrintBits(T bits)
{
  const int bit_size = sizeof(T) * 8;
  for (int i = bit_size - 1; i >= 0; --i)
  {
    T one = 1;
    T mask = one << i;
    int val = (bits & mask) >> T(i);
    printf("%d", val);
  }
  printf("\n");
}

template <typename T>
inline svtkm::UInt32 MinBits(const svtkm::UInt32 bits)
{
  return bits;
}

template <>
inline svtkm::UInt32 MinBits<svtkm::Float32>(const svtkm::UInt32 bits)
{
  return svtkm::Max(bits, 1 + 8u);
}

template <>
inline svtkm::UInt32 MinBits<svtkm::Float64>(const svtkm::UInt32 bits)
{
  return svtkm::Max(bits, 1 + 11u);
}




} // namespace zfp
} // namespace worklet
} // namespace svtkm
#endif //  svtk_m_worklet_zfp_type_info_h
