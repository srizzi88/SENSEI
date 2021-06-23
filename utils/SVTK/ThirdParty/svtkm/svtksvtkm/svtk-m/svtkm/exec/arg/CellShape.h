//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_CellShape_h
#define svtk_m_exec_arg_CellShape_h

#include <svtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the cell shape.
///
/// The \c AspectTagCellShape aspect tag causes the \c Fetch class to
/// obtain the type of element (e.g. cell cell) from the topology object.
///
struct AspectTagCellShape
{
};

/// \brief The \c ExecutionSignature tag to use to get the cell shape.
///
struct CellShape : svtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr svtkm::IdComponent INDEX = 1;
  using AspectTag = svtkm::exec::arg::AspectTagCellShape;
};

template <typename FetchTag, typename ConnectivityType, typename ExecObjectType>
struct Fetch<FetchTag,
             svtkm::exec::arg::AspectTagCellShape,
             svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>,
             ExecObjectType>
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  using ValueType = typename ThreadIndicesType::CellShapeTag;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetCellShape();
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

#endif //svtk_m_exec_arg_CellShape_h
