//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h
#define svtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h

#include <svtkm/cont/internal/ArrayExportMacros.h>
#include <svtkm/cont/internal/ArrayManagerExecution.h>
#include <svtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>
#include <svtkm/cont/serial/internal/DeviceAdapterTagSerial.h>
#include <svtkm/cont/serial/internal/ExecutionArrayInterfaceBasicSerial.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, svtkm::cont::DeviceAdapterTagSerial>
  : public svtkm::cont::internal::ArrayManagerExecutionShareWithControl<T, StorageTag>
{
public:
  using Superclass = svtkm::cont::internal::ArrayManagerExecutionShareWithControl<T, StorageTag>;
  using ValueType = typename Superclass::ValueType;
  using PortalType = typename Superclass::PortalType;
  using PortalConstType = typename Superclass::PortalConstType;

  SVTKM_CONT
  ArrayManagerExecution(typename Superclass::StorageType* storage)
    : Superclass(storage)
  {
  }
};

template <typename T>
struct ExecutionPortalFactoryBasic<T, DeviceAdapterTagSerial>
  : public ExecutionPortalFactoryBasicShareWithControl<T>
{
  using Superclass = ExecutionPortalFactoryBasicShareWithControl<T>;

  using typename Superclass::ValueType;
  using typename Superclass::PortalType;
  using typename Superclass::PortalConstType;
  using Superclass::CreatePortal;
  using Superclass::CreatePortalConst;
};

} // namespace internal

#ifndef svtk_m_cont_serial_internal_ArrayManagerExecutionSerial_cxx
SVTKM_EXPORT_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagSerial)
#endif // !svtk_m_cont_serial_internal_ArrayManagerExecutionSerial_cxx
}
} // namespace svtkm::cont

#endif //svtk_m_cont_serial_internal_ArrayManagerExecutionSerial_h
