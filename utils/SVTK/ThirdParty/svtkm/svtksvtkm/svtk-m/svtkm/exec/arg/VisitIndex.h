//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_VisitIndex_h
#define svtk_m_exec_arg_VisitIndex_h

#include <svtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <svtkm/exec/arg/Fetch.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the work index.
///
/// The \c AspectTagVisitIndex aspect tag causes the \c Fetch class to ignore
/// whatever data is in the associated execution object and return the visit
/// index.
///
struct AspectTagVisitIndex
{
};

/// \brief The \c ExecutionSignature tag to use to get the visit index
///
/// When a worklet is dispatched, there is a scatter operation defined that
/// optionally allows each input to go to multiple output entries. When one
/// input is assigned to multiple outputs, there needs to be a mechanism to
/// uniquely identify which output is which. The visit index is a value between
/// 0 and the number of outputs a particular input goes to. This tag in the \c
/// ExecutionSignature passes the visit index for this work. \c WorkletBase
/// contains a typedef that points to this class.
///
struct VisitIndex : svtkm::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the input domain.
  static constexpr svtkm::IdComponent INDEX = 1;
  using AspectTag = svtkm::exec::arg::AspectTagVisitIndex;
};

template <typename FetchTag, typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<FetchTag, svtkm::exec::arg::AspectTagVisitIndex, ThreadIndicesType, ExecObjectType>
{
  using ValueType = svtkm::IdComponent;

  SVTKM_EXEC
  svtkm::IdComponent Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetVisitIndex();
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

#endif //svtk_m_exec_arg_VisitIndex_h
