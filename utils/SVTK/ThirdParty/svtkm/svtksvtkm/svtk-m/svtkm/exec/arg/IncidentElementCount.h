//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_IncidentElementCount_h
#define svtk_m_exec_arg_IncidentElementCount_h

#include <svtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the incident element count.
///
/// The \c AspectTagIncidentElementCount aspect tag causes the \c Fetch class to
/// obtain the number of indices that map to the current topology element.
///
struct AspectTagIncidentElementCount
{
};

/// \brief The \c ExecutionSignature tag to get the number of incident elements.
///
/// In a topology map, there are \em visited and \em incident topology elements
/// specified. The scheduling occurs on the \em visited elements, and for each
/// \em visited element there is some number of incident \em incident elements
/// that are accessible. This \c ExecutionSignature tag provides the number of
/// these \em incident elements that are accessible.
///
struct IncidentElementCount : svtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr svtkm::IdComponent INDEX = 1;
  using AspectTag = svtkm::exec::arg::AspectTagIncidentElementCount;
};

template <typename FetchTag, typename ConnectivityType, typename ExecObjectType>
struct Fetch<FetchTag,
             svtkm::exec::arg::AspectTagIncidentElementCount,
             svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>,
             ExecObjectType>
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  using ValueType = svtkm::IdComponent;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetIndicesIncident().GetNumberOfComponents();
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_IncidentElementCount_h
