//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayCopy_h
#define svtk_m_cont_ArrayCopy_h

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/ErrorExecution.h>
#include <svtkm/cont/Logging.h>

// TODO: When virtual arrays are available, compile the implementation in a .cxx/.cu file. Common
// arrays are copied directly but anything else would be copied through virtual methods.

namespace svtkm
{
namespace cont
{

namespace detail
{

// normal element-wise copy:
template <typename InArrayType, typename OutArrayType>
void ArrayCopyImpl(const InArrayType& in, OutArrayType& out, std::false_type /* Copy storage */)
{
  // Find the device that already has a copy of the data:
  svtkm::cont::DeviceAdapterId devId = in.GetDeviceAdapterId();

  // If the data is not on any device, let the runtime tracker pick an available
  // parallel copy algorithm.
  if (devId.GetValue() == SVTKM_DEVICE_ADAPTER_UNDEFINED)
  {
    devId = svtkm::cont::make_DeviceAdapterId(SVTKM_DEVICE_ADAPTER_ANY);
  }

  bool success = svtkm::cont::Algorithm::Copy(devId, in, out);

  if (!success && devId.GetValue() != SVTKM_DEVICE_ADAPTER_ANY)
  { // Retry on any device if the first attempt failed.
    SVTKM_LOG_S(svtkm::cont::LogLevel::Error,
               "Failed to run ArrayCopy on device '" << devId.GetName()
                                                     << "'. Retrying on any device.");
    success = svtkm::cont::Algorithm::Copy(svtkm::cont::DeviceAdapterTagAny{}, in, out);
  }

  if (!success)
  {
    throw svtkm::cont::ErrorExecution("Failed to run ArrayCopy on any device.");
  }
}

// Copy storage for implicit arrays, must be of same type:
template <typename ArrayType>
void ArrayCopyImpl(const ArrayType& in, ArrayType& out, std::true_type /* Copy storage */)
{
  // This is only called if in/out are the same type and the handle is not
  // writable. This allows read-only implicit array handles to be copied.
  auto newStorage = in.GetStorage();
  out = ArrayType(newStorage);
}

} // namespace detail

/// \brief Does a deep copy from one array to another array.
///
/// Given a source \c ArrayHandle and a destination \c ArrayHandle, this
/// function allocates the destination \c ArrayHandle to the correct size and
/// deeply copies all the values from the source to the destination.
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment, the runtime device tracker is used to try to find another
/// device.
///
/// This should work on some non-writable array handles as well, as long as
/// both \a source and \a destination are the same type.
///
template <typename InValueType, typename InStorage, typename OutValueType, typename OutStorage>
SVTKM_CONT void ArrayCopy(const svtkm::cont::ArrayHandle<InValueType, InStorage>& source,
                         svtkm::cont::ArrayHandle<OutValueType, OutStorage>& destination)
{
  using InArrayType = svtkm::cont::ArrayHandle<InValueType, InStorage>;
  using OutArrayType = svtkm::cont::ArrayHandle<OutValueType, OutStorage>;
  using SameTypes = std::is_same<InArrayType, OutArrayType>;
  using IsWritable = svtkm::cont::internal::IsWritableArrayHandle<OutArrayType>;

  // There are three cases handled here:
  // 1. Output is writable:
  //    -> Do element-wise copy (normal copy behavior)
  // 2. Output is not writable and arrays are same type:
  //    -> just copy storage (special case for implicit array cloning)
  // 3. Output is not writable and arrays are different types:
  //    -> fail (cannot copy)

  // Give a nice error message for case 3:
  SVTKM_STATIC_ASSERT_MSG(IsWritable::value || SameTypes::value,
                         "Cannot copy to a read-only array with a different "
                         "type than the source.");

  using JustCopyStorage = std::integral_constant<bool, SameTypes::value && !IsWritable::value>;

  // Static dispatch cases 1 & 2
  detail::ArrayCopyImpl(source, destination, JustCopyStorage{});
}
}
} // namespace svtkm::cont

#endif //svtk_m_cont_ArrayCopy_h
