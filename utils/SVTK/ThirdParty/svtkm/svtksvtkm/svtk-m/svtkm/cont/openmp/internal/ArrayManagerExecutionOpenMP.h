//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_openmp_internal_ArrayManagerExecutionOpenMP_h
#define svtk_m_cont_openmp_internal_ArrayManagerExecutionOpenMP_h


#include <svtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#include <svtkm/cont/internal/ArrayExportMacros.h>
#include <svtkm/cont/internal/ArrayManagerExecution.h>
#include <svtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>
#include <svtkm/cont/openmp/internal/ExecutionArrayInterfaceBasicOpenMP.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, svtkm::cont::DeviceAdapterTagOpenMP>
  : public svtkm::cont::internal::ArrayManagerExecutionShareWithControl<T, StorageTag>
{
public:
  using Superclass = svtkm::cont::internal::ArrayManagerExecutionShareWithControl<T, StorageTag>;
  using ValueType = typename Superclass::ValueType;
  using PortalType = typename Superclass::PortalType;
  using PortalConstType = typename Superclass::PortalConstType;
  using StorageType = typename Superclass::StorageType;

  SVTKM_CONT
  ArrayManagerExecution(StorageType* storage)
    : Superclass(storage)
  {
  }

  SVTKM_CONT
  PortalConstType PrepareForInput(bool updateData)
  {
    return this->Superclass::PrepareForInput(updateData);
  }

  SVTKM_CONT
  PortalType PrepareForInPlace(bool updateData)
  {
    return this->Superclass::PrepareForInPlace(updateData);
  }

  SVTKM_CONT
  PortalType PrepareForOutput(svtkm::Id numberOfValues)
  {
    return this->Superclass::PrepareForOutput(numberOfValues);
  }
};


template <typename T>
struct ExecutionPortalFactoryBasic<T, DeviceAdapterTagOpenMP>
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

#ifndef svtk_m_cont_openmp_internal_ArrayManagerExecutionOpenMP_cxx
SVTKM_EXPORT_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagOpenMP)
#endif // !svtk_m_cont_openmp_internal_ArrayManagerExecutionOpenMP_cxx
}
} // namespace svtkm::cont

#endif // svtk_m_cont_openmp_internal_ArrayManagerExecutionOpenMP_h
