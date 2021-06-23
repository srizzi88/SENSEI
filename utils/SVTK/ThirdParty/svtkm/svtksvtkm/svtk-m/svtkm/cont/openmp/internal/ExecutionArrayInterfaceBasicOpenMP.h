//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_openmp_internal_ExecutionArrayInterfaceBasicOpenMP_h
#define svtk_m_cont_openmp_internal_ExecutionArrayInterfaceBasicOpenMP_h

#include <svtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>
#include <svtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <>
struct SVTKM_CONT_EXPORT ExecutionArrayInterfaceBasic<DeviceAdapterTagOpenMP> final
  : public ExecutionArrayInterfaceBasicShareWithControl
{
  //inherit our parents constructor
  using ExecutionArrayInterfaceBasicShareWithControl::ExecutionArrayInterfaceBasicShareWithControl;

  SVTKM_CONT
  DeviceAdapterId GetDeviceId() const final;
};

} // namespace internal
}
} // namespace svtkm::cont

#endif //svtk_m_cont_serial_internal_ExecutionArrayInterfaceBasicSerial_h
