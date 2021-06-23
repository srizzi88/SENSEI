//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_IncidentElementIndices_h
#define svtk_m_exec_arg_IncidentElementIndices_h

#include <svtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the visited indices.
///
/// The \c AspectTagIncidentElementIndices aspect tag causes the \c Fetch class
/// to obtain the indices that map to the current topology element.
///
struct AspectTagIncidentElementIndices
{
};

/// \brief The \c ExecutionSignature tag to get the indices of visited elements.
///
/// In a topology map, there are \em visited and \em incident topology elements
/// specified. The scheduling occurs on the \em visited elements, and for each
/// \em visited element there is some number of incident \em incident elements
/// that are accessible. This \c ExecutionSignature tag provides the indices of
/// the \em incident elements that are incident to the current \em visited
/// element.
///
struct IncidentElementIndices : svtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr svtkm::IdComponent INDEX = 1;
  using AspectTag = svtkm::exec::arg::AspectTagIncidentElementIndices;
};

template <typename FetchTag, typename ConnectivityType, typename ExecObjectType>
struct Fetch<FetchTag,
             svtkm::exec::arg::AspectTagIncidentElementIndices,
             svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>,
             ExecObjectType>
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  using ValueType = typename ThreadIndicesType::IndicesIncidentType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetIndicesIncident();
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

#endif //svtk_m_exec_arg_IncidentElementIndices_h
