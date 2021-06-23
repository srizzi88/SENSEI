//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_testing_ThreadIndicesTesting_h
#define svtk_m_exec_arg_testing_ThreadIndicesTesting_h

#include <svtkm/Types.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Simplified version of ThreadIndices for unit testing purposes
///
class ThreadIndicesTesting
{
public:
  SVTKM_EXEC_CONT
  ThreadIndicesTesting(svtkm::Id index)
    : InputIndex(index)
    , OutputIndex(index)
    , VisitIndex(0)
  {
  }

  SVTKM_EXEC_CONT
  ThreadIndicesTesting(svtkm::Id inputIndex, svtkm::Id outputIndex, svtkm::IdComponent visitIndex)
    : InputIndex(inputIndex)
    , OutputIndex(outputIndex)
    , VisitIndex(visitIndex)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetInputIndex() const { return this->InputIndex; }

  SVTKM_EXEC_CONT
  svtkm::Id3 GetInputIndex3D() const { return svtkm::Id3(this->GetInputIndex(), 0, 0); }

  SVTKM_EXEC_CONT
  svtkm::Id GetOutputIndex() const { return this->OutputIndex; }

  SVTKM_EXEC_CONT
  svtkm::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  SVTKM_EXEC_CONT
  svtkm::Id GetGlobalIndex() const { return this->OutputIndex; }

private:
  svtkm::Id InputIndex;
  svtkm::Id OutputIndex;
  svtkm::IdComponent VisitIndex;
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_testing_ThreadIndicesTesting_h
