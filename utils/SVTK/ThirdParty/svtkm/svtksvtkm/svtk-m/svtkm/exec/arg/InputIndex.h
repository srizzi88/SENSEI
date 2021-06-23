//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_InputIndex_h
#define svtk_m_exec_arg_InputIndex_h

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
/// The \c AspectTagInputIndex aspect tag causes the \c Fetch class to ignore
/// whatever data is in the associated execution object and return the index
/// of the input element.
///
struct AspectTagInputIndex
{
};

/// \brief The \c ExecutionSignature tag to use to get the input index
///
/// When a worklet is dispatched, it broken into pieces defined by the input
/// domain and scheduled on independent threads. This tag in the \c
/// ExecutionSignature passes the index of the input element that the work
/// thread is currently working on. When a worklet has a scatter associated
/// with it, the input and output indices can be different. \c WorkletBase
/// contains a typedef that points to this class.
///
struct InputIndex : svtkm::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the input domain.
  static constexpr svtkm::IdComponent INDEX = 1;
  using AspectTag = svtkm::exec::arg::AspectTagInputIndex;
};

template <typename FetchTag, typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<FetchTag, svtkm::exec::arg::AspectTagInputIndex, ThreadIndicesType, ExecObjectType>
{
  using ValueType = svtkm::Id;

  SVTKM_EXEC
  svtkm::Id Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetInputIndex();
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

#endif //svtk_m_exec_arg_InputIndex_h
