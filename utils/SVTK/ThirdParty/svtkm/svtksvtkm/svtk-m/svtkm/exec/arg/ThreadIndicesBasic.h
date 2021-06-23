//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_ThreadIndicesBasic_h
#define svtk_m_exec_arg_ThreadIndicesBasic_h

#include <svtkm/internal/Invocation.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Basic container for thread indices in a worklet invocation
///
/// During the execution of a worklet function in an execution environment
/// thread, SVTK-m has to manage several indices. To simplify this management
/// and to provide a single place to store them (so that they do not have to be
/// recomputed), \c WorkletInvokeFunctor creates a \c ThreadIndices object.
/// This object gets passed to \c Fetch operations to help them load data.
///
/// All \c ThreadIndices classes should implement the functions provided in
/// the \c ThreadIndicesBasic class. (It is in fact a good idea to subclass
/// it.) Other \c ThreadIndices classes may provide additional indices if
/// appropriate for the scheduling.
///
class ThreadIndicesBasic
{
public:
  SVTKM_EXEC
  ThreadIndicesBasic(svtkm::Id threadIndex,
                     svtkm::Id inIndex,
                     svtkm::IdComponent visitIndex,
                     svtkm::Id outIndex,
                     svtkm::Id globalThreadIndexOffset = 0)
    : ThreadIndex(threadIndex)
    , InputIndex(inIndex)
    , OutputIndex(outIndex)
    , VisitIndex(visitIndex)
    , GlobalThreadIndexOffset(globalThreadIndexOffset)
  {
  }

  /// \brief The index of the thread or work invocation.
  ///
  /// This index refers to which instance of the worklet is being invoked. Every invocation of the
  /// worklet has a unique thread index. This is also called the work index depending on the
  /// context.
  ///
  SVTKM_EXEC
  svtkm::Id GetThreadIndex() const { return this->ThreadIndex; }

  /// \brief The index into the input domain.
  ///
  /// This index refers to the input element (array value, cell, etc.) that
  /// this thread is being invoked for. This is the typical index used during
  /// Fetch::Load.
  ///
  SVTKM_EXEC
  svtkm::Id GetInputIndex() const { return this->InputIndex; }

  /// \brief The 3D index into the input domain.
  ///
  /// This index refers to the input element (array value, cell, etc.) that
  /// this thread is being invoked for. If the input domain has 2 or 3
  /// dimensional indexing, this result will preserve that. If the domain
  /// indexing is just one dimensional, the result will have the index in the
  /// first component with the remaining components set to 0.
  ///
  SVTKM_EXEC
  svtkm::Id3 GetInputIndex3D() const { return svtkm::Id3(this->GetInputIndex(), 0, 0); }

  /// \brief The index into the output domain.
  ///
  /// This index refers to the output element (array value, cell, etc.) that
  /// this thread is creating. This is the typical index used during
  /// Fetch::Store.
  ///
  SVTKM_EXEC
  svtkm::Id GetOutputIndex() const { return this->OutputIndex; }

  /// \brief The visit index.
  ///
  /// When multiple output indices have the same input index, they are
  /// distinguished using the visit index.
  ///
  SVTKM_EXEC
  svtkm::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  /// \brief The global index (for streaming).
  ///
  /// Global index (for streaming)
  SVTKM_EXEC
  svtkm::Id GetGlobalIndex() const { return (this->GlobalThreadIndexOffset + this->ThreadIndex); }

private:
  svtkm::Id ThreadIndex;
  svtkm::Id InputIndex;
  svtkm::Id OutputIndex;
  svtkm::IdComponent VisitIndex;
  svtkm::Id GlobalThreadIndexOffset;
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_ThreadIndicesBasic_h
