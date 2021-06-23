//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_internal_ArrayExportMacros_h
#define svtk_m_cont_internal_ArrayExportMacros_h

/// Declare extern template instantiations for all ArrayHandle transfer
/// infrastructure from a header file.
#define SVTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(Type, Device)                    \
  namespace internal                                                                               \
  {                                                                                                \
  extern template struct SVTKM_CONT_TEMPLATE_EXPORT ExecutionPortalFactoryBasic<Type, Device>;      \
  }                                                                                                \
  extern template SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::PortalConst ArrayHandle<Type, StorageTagBasic>::PrepareForInput(Device) const;        \
  extern template SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::Portal ArrayHandle<Type, StorageTagBasic>::PrepareForOutput(svtkm::Id, Device);        \
  extern template SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<    \
    Device>::Portal ArrayHandle<Type, StorageTagBasic>::PrepareForInPlace(Device);                 \
  extern template SVTKM_CONT_TEMPLATE_EXPORT void                                                   \
  ArrayHandle<Type, StorageTagBasic>::PrepareForDevice(const std::unique_lock<std::mutex>&,        \
                                                       Device) const;

#define SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(BasicType, Device)                              \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(BasicType, Device)                     \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                       \
    SVTKM_PASS_COMMAS(svtkm::Vec<BasicType, 2>), Device)                                             \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                       \
    SVTKM_PASS_COMMAS(svtkm::Vec<BasicType, 3>), Device)                                             \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                       \
    SVTKM_PASS_COMMAS(svtkm::Vec<BasicType, 4>), Device)

/// call SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER for all svtkm types.
#define SVTKM_EXPORT_ARRAYHANDLES_FOR_DEVICE_ADAPTER(Device)                                        \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(char, Device)                                         \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Int8, Device)                                   \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::UInt8, Device)                                  \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Int16, Device)                                  \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::UInt16, Device)                                 \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Int32, Device)                                  \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::UInt32, Device)                                 \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Int64, Device)                                  \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::UInt64, Device)                                 \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Float32, Device)                                \
  SVTKM_EXPORT_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Float64, Device)

/// Instantiate templates for all ArrayHandle transfer infrastructure from an
/// implementation file.
#define SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(Type, Device)               \
  namespace internal                                                                               \
  {                                                                                                \
  template struct SVTKM_CONT_EXPORT ExecutionPortalFactoryBasic<Type, Device>;                      \
  }                                                                                                \
  template SVTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<                    \
    Device>::PortalConst ArrayHandle<Type, StorageTagBasic>::PrepareForInput(Device) const;        \
  template SVTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal     \
    ArrayHandle<Type, StorageTagBasic>::PrepareForOutput(svtkm::Id, Device);                        \
  template SVTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>::ExecutionTypes<Device>::Portal     \
    ArrayHandle<Type, StorageTagBasic>::PrepareForInPlace(Device);                                 \
  template SVTKM_CONT_EXPORT void ArrayHandle<Type, StorageTagBasic>::PrepareForDevice(             \
    const std::unique_lock<std::mutex>&, Device) const;

#define SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(BasicType, Device)                         \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(BasicType, Device)                \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                  \
    SVTKM_PASS_COMMAS(svtkm::Vec<BasicType, 2>), Device)                                             \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                  \
    SVTKM_PASS_COMMAS(svtkm::Vec<BasicType, 3>), Device)                                             \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_VALUE_TYPE_AND_DEVICE_ADAPTER(                                  \
    SVTKM_PASS_COMMAS(svtkm::Vec<BasicType, 4>), Device)

#define SVTKM_INSTANTIATE_ARRAYHANDLES_FOR_DEVICE_ADAPTER(Device)                                   \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(char, Device)                                    \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Int8, Device)                              \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::UInt8, Device)                             \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Int16, Device)                             \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::UInt16, Device)                            \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Int32, Device)                             \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::UInt32, Device)                            \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Int64, Device)                             \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::UInt64, Device)                            \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Float32, Device)                           \
  SVTKM_INSTANTIATE_ARRAYHANDLE_FOR_DEVICE_ADAPTER(svtkm::Float64, Device)

#include <svtkm/cont/ArrayHandle.h>

#endif // svtk_m_cont_internal_ArrayExportMacros_h
