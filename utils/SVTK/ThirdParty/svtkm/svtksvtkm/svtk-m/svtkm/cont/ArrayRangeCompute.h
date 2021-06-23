//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayRangeCompute_h
#define svtk_m_cont_ArrayRangeCompute_h

#include <svtkm/Range.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <svtkm/cont/DeviceAdapterTag.h>

namespace svtkm
{
namespace cont
{

/// \brief Compute the range of the data in an array handle.
///
/// Given an \c ArrayHandle, this function computes the range (min and max) of
/// the values in the array. For arrays containing Vec values, the range is
/// computed for each component.
///
/// This method optionally takes a \c svtkm::cont::DeviceAdapterId to control which
/// devices to try.
///
/// The result is returned in an \c ArrayHandle of \c Range objects. There is
/// one value in the returned array for every component of the input's value
/// type.
///
template <typename ArrayHandleType>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(
  const ArrayHandleType& input,
  svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny());

// Precompiled versions of ArrayRangeCompute
#define SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(T, Storage)                                             \
  SVTKM_CONT_EXPORT                                                                                 \
  SVTKM_CONT                                                                                        \
  svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(                                          \
    const svtkm::cont::ArrayHandle<T, Storage>& input,                                              \
    svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny())
#define SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(T, N, Storage)                                        \
  SVTKM_CONT_EXPORT                                                                                 \
  SVTKM_CONT                                                                                        \
  svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(                                          \
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, Storage>& input,                                \
    svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny())

SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(char, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::Int8, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::UInt8, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::Int16, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::UInt16, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::Int32, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::UInt32, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::Int64, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::UInt64, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::Float32, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T(svtkm::Float64, svtkm::cont::StorageTagBasic);

SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Int32, 2, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Int64, 2, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Float32, 2, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Float64, 2, svtkm::cont::StorageTagBasic);

SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Int32, 3, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Int64, 3, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Float32, 3, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Float64, 3, svtkm::cont::StorageTagBasic);

SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(char, 4, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Int8, 4, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::UInt8, 4, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Float32, 4, svtkm::cont::StorageTagBasic);
SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC(svtkm::Float64, 4, svtkm::cont::StorageTagBasic);

#undef SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_T
#undef SVTK_M_ARRAY_RANGE_COMPUTE_EXPORT_VEC

SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(
  const svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>& input,
  svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny());

SVTKM_CONT_EXPORT SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(
  const svtkm::cont::ArrayHandle<svtkm::Vec3f,
                                svtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>& array,
  svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny());

// Implementation of composite vectors
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(
  const svtkm::cont::ArrayHandle<svtkm::Vec3f_32,
                                typename svtkm::cont::ArrayHandleCompositeVector<
                                  svtkm::cont::ArrayHandle<svtkm::Float32>,
                                  svtkm::cont::ArrayHandle<svtkm::Float32>,
                                  svtkm::cont::ArrayHandle<svtkm::Float32>>::StorageTag>& input,
  svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny());

SVTKM_CONT_EXPORT SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(
  const svtkm::cont::ArrayHandle<svtkm::Vec3f_64,
                                typename svtkm::cont::ArrayHandleCompositeVector<
                                  svtkm::cont::ArrayHandle<svtkm::Float64>,
                                  svtkm::cont::ArrayHandle<svtkm::Float64>,
                                  svtkm::cont::ArrayHandle<svtkm::Float64>>::StorageTag>& input,
  svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny());

// Implementation of cartesian products
template <typename T, typename ST1, typename ST2, typename ST3>
SVTKM_CONT inline svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>,
                                svtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>& input,
  svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny())
{
  svtkm::cont::ArrayHandle<svtkm::Range> result;
  result.Allocate(3);

  svtkm::cont::ArrayHandle<svtkm::Range> componentRangeArray;
  svtkm::Range componentRange;

  svtkm::cont::ArrayHandle<T, ST1> firstArray = input.GetStorage().GetFirstArray();
  componentRangeArray = svtkm::cont::ArrayRangeCompute(firstArray, device);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(0, componentRange);

  svtkm::cont::ArrayHandle<T, ST2> secondArray = input.GetStorage().GetSecondArray();
  componentRangeArray = svtkm::cont::ArrayRangeCompute(secondArray, device);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(1, componentRange);

  svtkm::cont::ArrayHandle<T, ST3> thirdArray = input.GetStorage().GetThirdArray();
  componentRangeArray = svtkm::cont::ArrayRangeCompute(thirdArray, device);
  componentRange = componentRangeArray.GetPortalConstControl().Get(0);
  result.GetPortalControl().Set(2, componentRange);

  return result;
}

SVTKM_CONT_EXPORT void ThrowArrayRangeComputeFailed();
}
} // namespace svtkm::cont

#endif //svtk_m_cont_ArrayRangeCompute_h
