//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_ThreadIndicesReduceByKey_h
#define svtk_m_exec_arg_ThreadIndicesReduceByKey_h

#include <svtkm/exec/arg/ThreadIndicesBasic.h>

#include <svtkm/exec/internal/ReduceByKeyLookup.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Container for thread indices in a reduce by key invocation.
///
/// This specialization of \c ThreadIndices adds extra indices that deal with a
/// reduce by key. In particular, it save the indices used to map from a unique
/// key index to the group of input values that has that key associated with
/// it.
///
class ThreadIndicesReduceByKey : public svtkm::exec::arg::ThreadIndicesBasic
{
  using Superclass = svtkm::exec::arg::ThreadIndicesBasic;

public:
  template <typename P1, typename P2, typename P3>
  SVTKM_EXEC ThreadIndicesReduceByKey(
    svtkm::Id threadIndex,
    svtkm::Id inIndex,
    svtkm::IdComponent visitIndex,
    svtkm::Id outIndex,
    const svtkm::exec::internal::ReduceByKeyLookup<P1, P2, P3>& keyLookup,
    svtkm::Id globalThreadIndexOffset = 0)
    : Superclass(threadIndex, inIndex, visitIndex, outIndex, globalThreadIndexOffset)
    , ValueOffset(keyLookup.Offsets.Get(inIndex))
    , NumberOfValues(keyLookup.Counts.Get(inIndex))
  {
  }

  SVTKM_EXEC
  svtkm::Id GetValueOffset() const { return this->ValueOffset; }

  SVTKM_EXEC
  svtkm::IdComponent GetNumberOfValues() const { return this->NumberOfValues; }

private:
  svtkm::Id ValueOffset;
  svtkm::IdComponent NumberOfValues;
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_ThreadIndicesReduceByKey_h
