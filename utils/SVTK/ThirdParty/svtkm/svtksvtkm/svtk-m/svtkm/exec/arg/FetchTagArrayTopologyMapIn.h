//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_FetchTagArrayTopologyMapIn_h
#define svtk_m_exec_arg_FetchTagArrayTopologyMapIn_h

#include <svtkm/exec/arg/AspectTagDefault.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h>

#include <svtkm/TopologyElementTag.h>

#include <svtkm/internal/ArrayPortalUniformPointCoordinates.h>

#include <svtkm/VecAxisAlignedPointCoordinates.h>
#include <svtkm/exec/ConnectivityStructured.h>

#include <svtkm/VecFromPortalPermute.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting array values determined by topology connections.
///
/// \c FetchTagArrayTopologyMapIn is a tag used with the \c Fetch class to
/// retrieve values from an array portal. The fetch uses indexing based on
/// the topology structure used for the input domain.
///
struct FetchTagArrayTopologyMapIn
{
};

namespace detail
{

// This internal class defines how a TopologyMapIn fetch loads from field data
// based on the connectivity class and the object holding the field data. The
// default implementation gets a Vec of indices and an array portal for the
// field and delivers a VecFromPortalPermute. Specializations could have more
// efficient implementations. For example, if the connectivity is structured
// and the field is regular point coordinates, it is much faster to compute the
// field directly.

template <typename ConnectivityType, typename FieldExecObjectType>
struct FetchArrayTopologyMapInImplementation
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  // ThreadIndicesTopologyMap has special incident element indices that are
  // stored in a Vec-like object.
  using IndexVecType = typename ThreadIndicesType::IndicesIncidentType;

  // The FieldExecObjectType is expected to behave like an ArrayPortal.
  using PortalType = FieldExecObjectType;

  using ValueType = svtkm::VecFromPortalPermute<IndexVecType, PortalType>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  static ValueType Load(const ThreadIndicesType& indices, const FieldExecObjectType& field)
  {
    // It is important that we give the VecFromPortalPermute (ValueType) a
    // pointer that will stay around during the time the Vec is valid. Thus, we
    // should make sure that indices is a reference that goes up the stack at
    // least as far as the returned VecFromPortalPermute is used.
    return ValueType(indices.GetIndicesIncidentPointer(), field);
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  static ValueType Load(const ThreadIndicesType& indices, const FieldExecObjectType* const field)
  {
    // It is important that we give the VecFromPortalPermute (ValueType) a
    // pointer that will stay around during the time the Vec is valid. Thus, we
    // should make sure that indices is a reference that goes up the stack at
    // least as far as the returned VecFromPortalPermute is used.
    return ValueType(indices.GetIndicesIncidentPointer(), field);
  }
};

static inline SVTKM_EXEC svtkm::VecAxisAlignedPointCoordinates<1> make_VecAxisAlignedPointCoordinates(
  const svtkm::Vec3f& origin,
  const svtkm::Vec3f& spacing,
  const svtkm::Vec<svtkm::Id, 1>& logicalId)
{
  svtkm::Vec3f offsetOrigin(
    origin[0] + spacing[0] * static_cast<svtkm::FloatDefault>(logicalId[0]), origin[1], origin[2]);
  return svtkm::VecAxisAlignedPointCoordinates<1>(offsetOrigin, spacing);
}

static inline SVTKM_EXEC svtkm::VecAxisAlignedPointCoordinates<1> make_VecAxisAlignedPointCoordinates(
  const svtkm::Vec3f& origin,
  const svtkm::Vec3f& spacing,
  svtkm::Id logicalId)
{
  return make_VecAxisAlignedPointCoordinates(origin, spacing, svtkm::Vec<svtkm::Id, 1>(logicalId));
}

static inline SVTKM_EXEC svtkm::VecAxisAlignedPointCoordinates<2> make_VecAxisAlignedPointCoordinates(
  const svtkm::Vec3f& origin,
  const svtkm::Vec3f& spacing,
  const svtkm::Id2& logicalId)
{
  svtkm::Vec3f offsetOrigin(origin[0] + spacing[0] * static_cast<svtkm::FloatDefault>(logicalId[0]),
                           origin[1] + spacing[1] * static_cast<svtkm::FloatDefault>(logicalId[1]),
                           origin[2]);
  return svtkm::VecAxisAlignedPointCoordinates<2>(offsetOrigin, spacing);
}

static inline SVTKM_EXEC svtkm::VecAxisAlignedPointCoordinates<3> make_VecAxisAlignedPointCoordinates(
  const svtkm::Vec3f& origin,
  const svtkm::Vec3f& spacing,
  const svtkm::Id3& logicalId)
{
  svtkm::Vec3f offsetOrigin(origin[0] + spacing[0] * static_cast<svtkm::FloatDefault>(logicalId[0]),
                           origin[1] + spacing[1] * static_cast<svtkm::FloatDefault>(logicalId[1]),
                           origin[2] + spacing[2] * static_cast<svtkm::FloatDefault>(logicalId[2]));
  return svtkm::VecAxisAlignedPointCoordinates<3>(offsetOrigin, spacing);
}

template <svtkm::IdComponent NumDimensions>
struct FetchArrayTopologyMapInImplementation<
  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell,
                                     svtkm::TopologyElementTagPoint,
                                     NumDimensions>,
  svtkm::internal::ArrayPortalUniformPointCoordinates>

{
  using ConnectivityType = svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell,
                                                              svtkm::TopologyElementTagPoint,
                                                              NumDimensions>;
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  using ValueType = svtkm::VecAxisAlignedPointCoordinates<NumDimensions>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  static ValueType Load(const ThreadIndicesType& indices,
                        const svtkm::internal::ArrayPortalUniformPointCoordinates& field)
  {
    // This works because the logical cell index is the same as the logical
    // point index of the first point on the cell.
    return svtkm::exec::arg::detail::make_VecAxisAlignedPointCoordinates(
      field.GetOrigin(), field.GetSpacing(), indices.GetIndexLogical());
  }
};

template <typename PermutationPortal, svtkm::IdComponent NumDimensions>
struct FetchArrayTopologyMapInImplementation<
  svtkm::exec::ConnectivityPermutedVisitCellsWithPoints<
    PermutationPortal,
    svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell,
                                       svtkm::TopologyElementTagPoint,
                                       NumDimensions>>,
  svtkm::internal::ArrayPortalUniformPointCoordinates>

{
  using ConnectivityType = svtkm::exec::ConnectivityPermutedVisitCellsWithPoints<
    PermutationPortal,
    svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell,
                                       svtkm::TopologyElementTagPoint,
                                       NumDimensions>>;
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  using ValueType = svtkm::VecAxisAlignedPointCoordinates<NumDimensions>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  static ValueType Load(const ThreadIndicesType& indices,
                        const svtkm::internal::ArrayPortalUniformPointCoordinates& field)
  {
    // This works because the logical cell index is the same as the logical
    // point index of the first point on the cell.

    // we have a flat index but we need 3d uniform coordinates, so we
    // need to take an flat index and convert to logical index
    return svtkm::exec::arg::detail::make_VecAxisAlignedPointCoordinates(
      field.GetOrigin(), field.GetSpacing(), indices.GetIndexLogical());
  }
};

} // namespace detail

template <typename ConnectivityType, typename ExecObjectType>
struct Fetch<svtkm::exec::arg::FetchTagArrayTopologyMapIn,
             svtkm::exec::arg::AspectTagDefault,
             svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>,
             ExecObjectType>
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  using Implementation =
    detail::FetchArrayTopologyMapInImplementation<ConnectivityType, ExecObjectType>;

  using ValueType = typename Implementation::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& field) const
  {
    return Implementation::Load(indices, field);
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

#endif //svtk_m_exec_arg_FetchTagArrayTopologyMapIn_h
