//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_ValueCount_h
#define svtk_m_exec_arg_ValueCount_h

#include <svtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesReduceByKey.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the value count.
///
/// The \c AspectTagValueCount aspect tag causes the \c Fetch class to obtain
/// the number of values that map to the key.
///
struct AspectTagValueCount
{
};

/// \brief The \c ExecutionSignature tag to get the number of values.
///
/// A \c WorkletReduceByKey operates by collecting all values associated with
/// identical keys and then giving the worklet a Vec-like object containing all
/// values with a matching key. This \c ExecutionSignature tag provides the
/// number of values associated with the key and given in the Vec-like objects.
///
struct ValueCount : svtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr svtkm::IdComponent INDEX = 1;
  using AspectTag = svtkm::exec::arg::AspectTagValueCount;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag,
             svtkm::exec::arg::AspectTagValueCount,
             svtkm::exec::arg::ThreadIndicesReduceByKey,
             ExecObjectType>
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesReduceByKey;

  using ValueType = svtkm::IdComponent;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetNumberOfValues();
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

#endif //svtk_m_exec_arg_ValueCount_h
