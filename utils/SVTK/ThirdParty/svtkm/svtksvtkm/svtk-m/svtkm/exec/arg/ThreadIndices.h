//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_ThreadIndices_h
#define svtk_m_exec_arg_ThreadIndices_h

#include <svtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <svtkm/exec/arg/Fetch.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the thread indices.
///
/// The \c AspectTagThreadIndices aspect tag causes the \c Fetch class to
/// ignore whatever data is in the associated execution object and return the
/// thread indices.
///
struct AspectTagThreadIndices
{
};

/// \brief The \c ExecutionSignature tag to use to get the thread indices
///
/// When a worklet is dispatched, it broken into pieces defined by the input
/// domain and scheduled on independent threads. During this process multiple
/// indices associated with the input and output can be generated. This tag in
/// the \c ExecutionSignature passes the index for this work. \c WorkletBase
/// contains a typedef that points to this class.
///
struct ThreadIndices : svtkm::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the input domain.
  static constexpr svtkm::IdComponent INDEX = 1;
  using AspectTag = svtkm::exec::arg::AspectTagThreadIndices;
};

template <typename FetchTag, typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<FetchTag, svtkm::exec::arg::AspectTagThreadIndices, ThreadIndicesType, ExecObjectType>
{
  using ValueType = const ThreadIndicesType&;

  SVTKM_EXEC
  const ThreadIndicesType& Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices;
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ThreadIndicesType&) const
  {
    // Store is a no-op.
  }
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_ThreadIndices_h
