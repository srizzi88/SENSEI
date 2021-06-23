//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_Dispatcher_MapTopology_h
#define svtk_m_worklet_Dispatcher_MapTopology_h

#include <svtkm/TopologyElementTag.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/worklet/WorkletMapTopology.h>
#include <svtkm/worklet/internal/DispatcherBase.h>

namespace svtkm
{
namespace worklet
{

/// \brief Dispatcher for worklets that inherit from \c WorkletMapTopology.
///
template <typename WorkletType>
class DispatcherMapTopology
  : public svtkm::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType>,
                                                   WorkletType,
                                                   svtkm::worklet::detail::WorkletMapTopologyBase>
{
  using Superclass =
    svtkm::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType>,
                                            WorkletType,
                                            svtkm::worklet::detail::WorkletMapTopologyBase>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  SVTKM_CONT DispatcherMapTopology(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  SVTKM_CONT void DoInvoke(Invocation& invocation) const
  {
    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;
    using SchedulingRangeType = typename WorkletType::VisitTopologyType;

    // If you get a compile error on this line, then you have tried to use
    // something that is not a svtkm::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    SVTKM_IS_CELL_SET(InputDomainType);

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const auto& inputDomain = invocation.GetInputDomain();

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BadicInvoke.
    this->BasicInvoke(invocation, internal::scheduling_range(inputDomain, SchedulingRangeType{}));
  }
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_Dispatcher_MapTopology_h
