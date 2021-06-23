//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayRangeCompute_hxx
#define svtk_m_cont_ArrayRangeCompute_hxx

#include <svtkm/cont/ArrayRangeCompute.h>

#include <svtkm/BinaryOperators.h>
#include <svtkm/VecTraits.h>

#include <svtkm/cont/Algorithm.h>

#include <limits>

namespace svtkm
{
namespace cont
{

namespace detail
{

struct ArrayRangeComputeFunctor
{
  template <typename Device, typename T, typename S>
  SVTKM_CONT bool operator()(Device,
                            const svtkm::cont::ArrayHandle<T, S>& handle,
                            const svtkm::Vec<T, 2>& initialValue,
                            svtkm::Vec<T, 2>& result) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<Device>;
    result = Algorithm::Reduce(handle, initialValue, svtkm::MinAndMax<T>());
    return true;
  }
};

template <typename T, typename S>
inline svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeComputeImpl(
  const svtkm::cont::ArrayHandle<T, S>& input,
  svtkm::cont::DeviceAdapterId device)
{
  using VecTraits = svtkm::VecTraits<T>;
  using CT = typename VecTraits::ComponentType;
  //We want to minimize the amount of code that we do in try execute as
  //it is repeated for each
  svtkm::cont::ArrayHandle<svtkm::Range> range;
  range.Allocate(VecTraits::NUM_COMPONENTS);

  if (input.GetNumberOfValues() < 1)
  {
    auto portal = range.GetPortalControl();
    for (svtkm::IdComponent i = 0; i < VecTraits::NUM_COMPONENTS; ++i)
    {
      portal.Set(i, svtkm::Range());
    }
  }
  else
  {
    //We used the limits, so that we don't need to sync the array handle
    //
    svtkm::Vec<T, 2> result;
    svtkm::Vec<T, 2> initial;
    initial[0] = T(std::numeric_limits<CT>::max());
    initial[1] = T(std::numeric_limits<CT>::lowest());

    const bool rangeComputed = svtkm::cont::TryExecuteOnDevice(
      device, detail::ArrayRangeComputeFunctor{}, input, initial, result);
    if (!rangeComputed)
    {
      ThrowArrayRangeComputeFailed();
    }
    else
    {
      auto portal = range.GetPortalControl();
      for (svtkm::IdComponent i = 0; i < VecTraits::NUM_COMPONENTS; ++i)
      {
        portal.Set(i,
                   svtkm::Range(VecTraits::GetComponent(result[0], i),
                               VecTraits::GetComponent(result[1], i)));
      }
    }
  }
  return range;
}

} // namespace detail


SVTKM_CONT
inline svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(
  const svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>& input,
  svtkm::cont::DeviceAdapterId device)
{
  using UniformHandleType = ArrayHandleUniformPointCoordinates;
  using RectilinearHandleType =
    svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>>;

  if (input.IsType<UniformHandleType>())
  {
    using T = typename UniformHandleType::ValueType;
    using S = typename UniformHandleType::StorageTag;
    const svtkm::cont::internal::detail::StorageVirtual* storage =
      input.GetStorage().GetStorageVirtual();
    const auto* castStorage =
      storage->Cast<svtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();

    return ArrayRangeCompute(castStorage->GetHandle(), device);
  }
  else if (input.IsType<RectilinearHandleType>())
  {
    using T = typename RectilinearHandleType::ValueType;
    using S = typename RectilinearHandleType::StorageTag;
    const svtkm::cont::internal::detail::StorageVirtual* storage =
      input.GetStorage().GetStorageVirtual();
    const auto* castStorage =
      storage->Cast<svtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();

    return ArrayRangeCompute(castStorage->GetHandle(), device);
  }
  else
  {
    return detail::ArrayRangeComputeImpl(input, device);
  }
}

template <typename ArrayHandleType>
inline svtkm::cont::ArrayHandle<svtkm::Range> ArrayRangeCompute(const ArrayHandleType& input,
                                                              svtkm::cont::DeviceAdapterId device)
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  return detail::ArrayRangeComputeImpl(input, device);
}
}
} // namespace svtkm::cont

#endif //svtk_m_cont_ArrayRangeCompute_hxx
