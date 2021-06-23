//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_internal_ExecutionArrayInterfaceCuda_h
#define svtk_m_cont_cuda_internal_ExecutionArrayInterfaceCuda_h

#include <svtkm/cont/cuda/DeviceAdapterCuda.h>

namespace svtkm
{
namespace cont
{
namespace internal
{
template <>
struct SVTKM_CONT_EXPORT ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda> final
  : public ExecutionArrayInterfaceBasicBase
{
  //inherit our parents constructor
  using ExecutionArrayInterfaceBasicBase::ExecutionArrayInterfaceBasicBase;

  SVTKM_CONT DeviceAdapterId GetDeviceId() const final;
  SVTKM_CONT void Allocate(TypelessExecutionArray& execArray,
                          svtkm::Id numberOfValues,
                          svtkm::UInt64 sizeOfValue) const final;
  SVTKM_CONT void Free(TypelessExecutionArray& execArray) const final;
  SVTKM_CONT void CopyFromControl(const void* controlPtr,
                                 void* executionPtr,
                                 svtkm::UInt64 numBytes) const final;
  SVTKM_CONT void CopyToControl(const void* executionPtr,
                               void* controlPtr,
                               svtkm::UInt64 numBytes) const final;

  SVTKM_CONT void UsingForRead(const void* controlPtr,
                              const void* executionPtr,
                              svtkm::UInt64 numBytes) const final;
  SVTKM_CONT void UsingForWrite(const void* controlPtr,
                               const void* executionPtr,
                               svtkm::UInt64 numBytes) const final;
  SVTKM_CONT void UsingForReadWrite(const void* controlPtr,
                                   const void* executionPtr,
                                   svtkm::UInt64 numBytes) const final;
};
} // namespace internal
}
} // namespace svtkm::cont

#endif //svtk_m_cont_cuda_internal_ExecutionArrayInterfaceCuda_h
