//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayGetValues_h
#define svtk_m_cont_ArrayGetValues_h

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/ErrorExecution.h>
#include <svtkm/cont/Logging.h>

#include <initializer_list>
#include <vector>

namespace svtkm
{
namespace cont
{

/// \brief Obtain a small set of values from an ArrayHandle with minimal device
/// transfers.
///
/// The values in @a data at the indices in @a ids are copied into a new array
/// and returned. This is useful for retrieving a subset of an array from a
/// device without transferring the entire array to the host.
///
/// These functions should not be called repeatedly in a loop to fetch all
/// values from an array handle. The much more efficient way to do this is to
/// use the proper control-side portals (ArrayHandle::GetPortalControl() and
/// ArrayHandle::GetPortalConstControl()).
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment or the device copy fails, a control-side copy is performed.
///
/// Since a serial control-side copy may be used, this method is only intended
/// for copying small subsets of the input data. Larger subsets that would
/// benefit from parallelization should prefer using the ArrayCopy method with
/// an ArrayHandlePermutation.
///
/// This utility provides several convenient overloads:
///
/// A single id may be passed into ArrayGetValue, or multiple ids may be
/// specified to ArrayGetValues as an ArrayHandle<svtkm::Id>, a
/// std::vector<svtkm::Id>, a c-array (pointer and size), or as a brace-enclosed
/// initializer list.
///
/// The single result from ArrayGetValue may be returned or written to an output
/// argument. Multiple results from ArrayGetValues may be returned as an
/// std::vector<T>, or written to an output argument as an ArrayHandle<T> or a
/// std::vector<T>.
///
/// Examples:
///
/// ```
/// svtkm::cont::ArrayHandle<T> data = ...;
///
/// // Fetch the first value in an array handle:
/// T firstVal = svtkm::cont::ArrayGetValue(0, data);
///
/// // Fetch the first and third values in an array handle:
/// std::vector<T> firstAndThird = svtkm::cont::ArrayGetValues({0, 2}, data);
///
/// // Fetch the first and last values in an array handle:
/// std::vector<T> firstAndLast =
///     svtkm::cont::ArrayGetValues({0, data.GetNumberOfValues() - 1}, data);
///
/// // Fetch the first 4 values into an array handle:
/// const std::vector<svtkm::Id> ids{0, 1, 2, 3};
/// svtkm::cont::ArrayHandle<T> firstFour;
/// svtkm::cont::ArrayGetValues(ids, data, firstFour);
/// ```
///
///
///@{
///
template <typename SIds, typename T, typename SData, typename SOut>
SVTKM_CONT void ArrayGetValues(const svtkm::cont::ArrayHandle<svtkm::Id, SIds>& ids,
                              const svtkm::cont::ArrayHandle<T, SData>& data,
                              svtkm::cont::ArrayHandle<T, SOut>& output)
{
  bool copyComplete = false;

  // Find the device that already has a copy of the data:
  svtkm::cont::DeviceAdapterId devId = data.GetDeviceAdapterId();

  if (devId.GetValue() != SVTKM_DEVICE_ADAPTER_UNDEFINED)
  { // Data exists on some device -- use it:
    const auto input = svtkm::cont::make_ArrayHandlePermutation(ids, data);
    copyComplete = svtkm::cont::Algorithm::Copy(devId, input, output);
    if (!copyComplete)
    { // Retry on any device if the first attempt failed.
      SVTKM_LOG_S(svtkm::cont::LogLevel::Error,
                 "Failed to run ArrayGetValues on device '"
                   << devId.GetName()
                   << "'. Falling back to control-side copy.");
      copyComplete = svtkm::cont::Algorithm::Copy(svtkm::cont::DeviceAdapterTagAny{}, input, output);
    }
  }

  if (!copyComplete)
  { // Fallback to a control-side copy if the device copy fails or if the device
    // is undefined:
    const svtkm::Id numVals = ids.GetNumberOfValues();
    auto idPortal = ids.GetPortalConstControl();
    auto dataPortal = data.GetPortalConstControl();
    output.Allocate(numVals);
    auto outPortal = output.GetPortalControl();
    for (svtkm::Id i = 0; i < numVals; ++i)
    {
      outPortal.Set(i, dataPortal.Get(idPortal.Get(i)));
    }
  }
}

template <typename SIds, typename T, typename SData, typename Alloc>
SVTKM_CONT void ArrayGetValues(const svtkm::cont::ArrayHandle<svtkm::Id, SIds>& ids,
                              const svtkm::cont::ArrayHandle<T, SData>& data,
                              std::vector<T, Alloc>& output)
{
  const std::size_t numVals = static_cast<std::size_t>(ids.GetNumberOfValues());

  // Allocate the vector and toss its data pointer into the array handle.
  output.resize(numVals);
  auto result = svtkm::cont::make_ArrayHandle(output, svtkm::CopyFlag::Off);
  svtkm::cont::ArrayGetValues(ids, data, result);
  // Make sure to pull the data back to control before we dealloc the handle
  // that wraps the vec memory:
  result.SyncControlArray();
}

template <typename SIds, typename T, typename SData>
SVTKM_CONT std::vector<T> ArrayGetValues(const svtkm::cont::ArrayHandle<svtkm::Id, SIds>& ids,
                                        const svtkm::cont::ArrayHandle<T, SData>& data)
{
  std::vector<T> result;
  svtkm::cont::ArrayGetValues(ids, data, result);
  return result;
}

template <typename T, typename Alloc, typename SData, typename SOut>
SVTKM_CONT void ArrayGetValues(const std::vector<svtkm::Id, Alloc>& ids,
                              const svtkm::cont::ArrayHandle<T, SData>& data,
                              svtkm::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(ids, svtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename AllocId, typename SData, typename AllocOut>
SVTKM_CONT void ArrayGetValues(const std::vector<svtkm::Id, AllocId>& ids,
                              const svtkm::cont::ArrayHandle<T, SData>& data,
                              std::vector<T, AllocOut>& output)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(ids, svtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename Alloc, typename SData>
SVTKM_CONT std::vector<T> ArrayGetValues(const std::vector<svtkm::Id, Alloc>& ids,
                                        const svtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(ids, svtkm::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename SData, typename SOut>
SVTKM_CONT void ArrayGetValues(const std::initializer_list<svtkm::Id>& ids,
                              const svtkm::cont::ArrayHandle<T, SData>& data,
                              svtkm::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<svtkm::Id>(ids.size()), svtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename SData, typename Alloc>
SVTKM_CONT void ArrayGetValues(const std::initializer_list<svtkm::Id>& ids,
                              const svtkm::cont::ArrayHandle<T, SData>& data,
                              std::vector<T, Alloc>& output)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<svtkm::Id>(ids.size()), svtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}
template <typename T, typename SData>
SVTKM_CONT std::vector<T> ArrayGetValues(const std::initializer_list<svtkm::Id>& ids,
                                        const svtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(
    ids.begin(), static_cast<svtkm::Id>(ids.size()), svtkm::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename SData, typename SOut>
SVTKM_CONT void ArrayGetValues(const svtkm::Id* ids,
                              const svtkm::Id numIds,
                              const svtkm::cont::ArrayHandle<T, SData>& data,
                              svtkm::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(ids, numIds, svtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename SData, typename Alloc>
SVTKM_CONT void ArrayGetValues(const svtkm::Id* ids,
                              const svtkm::Id numIds,
                              const svtkm::cont::ArrayHandle<T, SData>& data,
                              std::vector<T, Alloc>& output)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(ids, numIds, svtkm::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}
template <typename T, typename SData>
SVTKM_CONT std::vector<T> ArrayGetValues(const svtkm::Id* ids,
                                        const svtkm::Id numIds,
                                        const svtkm::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = svtkm::cont::make_ArrayHandle(ids, numIds, svtkm::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename S>
SVTKM_CONT T ArrayGetValue(svtkm::Id id, const svtkm::cont::ArrayHandle<T, S>& data)
{
  const auto idAH = svtkm::cont::make_ArrayHandle(&id, 1, svtkm::CopyFlag::Off);
  auto result = svtkm::cont::ArrayGetValues(idAH, data);
  return result[0];
}

template <typename T, typename S>
SVTKM_CONT void ArrayGetValue(svtkm::Id id, const svtkm::cont::ArrayHandle<T, S>& data, T& val)
{
  val = ArrayGetValue(id, data);
}
/// @}
}
} // namespace svtkm::cont

#endif //svtk_m_cont_ArrayGetValues_h
