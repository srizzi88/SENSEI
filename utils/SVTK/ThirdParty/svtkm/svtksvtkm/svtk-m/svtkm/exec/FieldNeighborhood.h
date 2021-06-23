//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_FieldNeighborhood_h
#define svtk_m_exec_FieldNeighborhood_h

#include <svtkm/exec/BoundaryState.h>
#include <svtkm/internal/ArrayPortalUniformPointCoordinates.h>

namespace svtkm
{
namespace exec
{

/// \brief Retrieves field values from a neighborhood.
///
/// \c FieldNeighborhood manages the retrieval of field values within the neighborhood of a
/// \c WorkletPointNeighborhood worklet. The \c Get methods take ijk indices relative to the
/// neighborhood (with 0, 0, 0 being the element visted) and return the field value at that part of
/// the neighborhood. If the requested neighborhood is outside the boundary, a different value will
/// be returned determined by the boundary behavior. A \c BoundaryState object can be used to
/// determine if the neighborhood extends beyond the boundary of the mesh.
///
/// This class is typically constructued using the \c FieldInNeighborhood tag in an
/// \c ExecutionSignature. There is little reason to construct this in user code.
///
/// \c FieldNeighborhood is templated on the array portal from which field values are retrieved.
///
template <typename FieldPortalType>
struct FieldNeighborhood
{
  SVTKM_EXEC
  FieldNeighborhood(const FieldPortalType& portal, const svtkm::exec::BoundaryState& boundary)
    : Boundary(&boundary)
    , Portal(portal)
  {
  }

  using ValueType = typename FieldPortalType::ValueType;

  SVTKM_EXEC
  ValueType Get(svtkm::IdComponent i, svtkm::IdComponent j, svtkm::IdComponent k) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFlatIndexClamp(i, j, k));
  }

  SVTKM_EXEC
  ValueType Get(const svtkm::Id3& ijk) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFlatIndexClamp(ijk));
  }

  svtkm::exec::BoundaryState const* const Boundary;
  FieldPortalType Portal;
};

/// \brief Specialization of Neighborhood for ArrayPortalUniformPointCoordinates
/// We can use fast paths inside ArrayPortalUniformPointCoordinates to allow
/// for very fast computation of the coordinates reachable by the neighborhood
template <>
struct FieldNeighborhood<svtkm::internal::ArrayPortalUniformPointCoordinates>
{
  SVTKM_EXEC
  FieldNeighborhood(const svtkm::internal::ArrayPortalUniformPointCoordinates& portal,
                    const svtkm::exec::BoundaryState& boundary)
    : Boundary(&boundary)
    , Portal(portal)
  {
  }

  using ValueType = svtkm::internal::ArrayPortalUniformPointCoordinates::ValueType;

  SVTKM_EXEC
  ValueType Get(svtkm::IdComponent i, svtkm::IdComponent j, svtkm::IdComponent k) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFullIndexClamp(i, j, k));
  }

  SVTKM_EXEC
  ValueType Get(const svtkm::IdComponent3& ijk) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFullIndexClamp(ijk));
  }

  svtkm::exec::BoundaryState const* const Boundary;
  svtkm::internal::ArrayPortalUniformPointCoordinates Portal;
};
}
} // namespace svtkm::exec

#endif //svtk_m_exec_FieldNeighborhood_h
