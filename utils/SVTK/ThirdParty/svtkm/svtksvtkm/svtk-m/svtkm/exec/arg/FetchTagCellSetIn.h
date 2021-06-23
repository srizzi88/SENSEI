//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_FetchTagCellSetIn_h
#define svtk_m_exec_arg_FetchTagCellSetIn_h

#include <svtkm/exec/arg/AspectTagDefault.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting topology information.
///
/// \c FetchTagCellSetIn is a tag used with the \c Fetch class to retrieve
/// values from a topology object.  This default parameter returns
/// the basis topology type, i.e. cell type in a \c WorkletCellMap.
///
struct FetchTagCellSetIn
{
};

template <typename ConnectivityType, typename ExecObjectType>
struct Fetch<svtkm::exec::arg::FetchTagCellSetIn,
             svtkm::exec::arg::AspectTagDefault,
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
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_FetchTagCellSetIn_h
