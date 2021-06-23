//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayRangeCompute.hxx>

namespace svtkm
{
namespace cont
{

void ThrowArrayRangeComputeFailed()
{
  throw svtkm::cont::ErrorExecution("Failed to run ArrayRangeComputation on any device.");
}

#define SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(T, Storage)                                                \
  SVTKM_CONT                                                                                        \
  svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(                                          \
    const svtkm::cont::ArrayHandle<T, Storage>& input, svtkm::cont::DeviceAdapterId device)          \
  {                                                                                                \
    return detail::ArrayRangeComputeImpl(input, device);                                           \
  }                                                                                                \
  struct SwallowSemicolon
#define SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(T, N, Storage)                                           \
  SVTKM_CONT                                                                                        \
  svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(                                          \
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, Storage>& input,                                \
    svtkm::cont::DeviceAdapterId device)                                                            \
  {                                                                                                \
    return detail::ArrayRangeComputeImpl(input, device);                                           \
  }                                                                                                \
  struct SwallowSemicolon

SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(char, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::Int8, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::UInt8, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::Int16, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::UInt16, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::Int32, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::UInt32, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::Int64, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::UInt64, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::Float32, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T(svtkm::Float64, svtkm::cont::StorageTagBasic);

SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Int32, 2, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Int64, 2, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Float32, 2, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Float64, 2, svtkm::cont::StorageTagBasic);

SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Int32, 3, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Int64, 3, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Float32, 3, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Float64, 3, svtkm::cont::StorageTagBasic);

SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(char, 4, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Int8, 4, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::UInt8, 4, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Float32, 4, svtkm::cont::StorageTagBasic);
SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC(svtkm::Float64, 4, svtkm::cont::StorageTagBasic);

#undef SVTKM_ARRAY_RANGE_COMPUTE_IMPL_T
#undef SVTKM_ARRAY_RANGE_COMPUTE_IMPL_VEC

// Special implementation for regular point coordinates, which are easy
// to determine.
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(
  const svtkm::cont::ArrayHandle<svtkm::Vec3f,
                                svtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>& array,
  svtkm::cont::DeviceAdapterId)
{
  svtkm::internal::ArrayPortalUniformPointCoordinates portal = array.GetPortalConstControl();

  // In this portal we know that the min value is the first entry and the
  // max value is the last entry.
  svtkm::Vec3f minimum = portal.Get(0);
  svtkm::Vec3f maximum = portal.Get(portal.GetNumberOfValues() - 1);

  svtkm::cont::ArrayHandle<svtkm::Range> rangeArray;
  rangeArray.Allocate(3);
  svtkm::cont::ArrayHandle<svtkm::Range>::PortalControl outPortal = rangeArray.GetPortalControl();
  outPortal.Set(0, svtkm::Range(minimum[0], maximum[0]));
  outPortal.Set(1, svtkm::Range(minimum[1], maximum[1]));
  outPortal.Set(2, svtkm::Range(minimum[2], maximum[2]));

  return rangeArray;
}
}
} // namespace svtkm::cont
