//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_DispatcherReduceByKey_h
#define svtk_m_worklet_DispatcherReduceByKey_h

#include <svtkm/cont/DeviceAdapter.h>

#include <svtkm/worklet/WorkletReduceByKey.h>

#include <svtkm/worklet/internal/DispatcherBase.h>

namespace svtkm
{
namespace worklet
{

/// \brief Dispatcher for worklets that inherit from \c WorkletReduceByKey.
///
template <typename WorkletType>
class DispatcherReduceByKey
  : public svtkm::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType>,
                                                   WorkletType,
                                                   svtkm::worklet::WorkletReduceByKey>
{
  using Superclass = svtkm::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType>,
                                                             WorkletType,
                                                             svtkm::worklet::WorkletReduceByKey>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  SVTKM_CONT DispatcherReduceByKey(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  void DoInvoke(Invocation& invocation) const
  {
    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // If you get a compile error on this line, then you have tried to use
    // something other than svtkm::worklet::Keys as the input domain, which
    // is illegal.
    SVTKM_STATIC_ASSERT_MSG(
      (svtkm::cont::arg::TypeCheck<svtkm::cont::arg::TypeCheckTagKeys, InputDomainType>::value),
      "Invalid input domain for WorkletReduceByKey.");

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BasicInvoke.
    this->BasicInvoke(invocation, internal::scheduling_range(inputDomain));
  }
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_DispatcherReduceByKey_h
