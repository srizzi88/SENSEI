//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_ConnectivityExplicit_h
#define svtk_m_exec_ConnectivityExplicit_h

#include <svtkm/CellShape.h>
#include <svtkm/Types.h>

#include <svtkm/VecFromPortal.h>

namespace svtkm
{
namespace exec
{

template <typename ShapesPortalType, typename ConnectivityPortalType, typename OffsetsPortalType>
class ConnectivityExplicit
{
public:
  using SchedulingRangeType = svtkm::Id;

  ConnectivityExplicit() {}

  ConnectivityExplicit(const ShapesPortalType& shapesPortal,
                       const ConnectivityPortalType& connPortal,
                       const OffsetsPortalType& offsetsPortal)
    : Shapes(shapesPortal)
    , Connectivity(connPortal)
    , Offsets(offsetsPortal)
  {
  }

  SVTKM_EXEC
  SchedulingRangeType GetNumberOfElements() const { return this->Shapes.GetNumberOfValues(); }

  using CellShapeTag = svtkm::CellShapeTagGeneric;

  SVTKM_EXEC
  CellShapeTag GetCellShape(svtkm::Id index) const { return CellShapeTag(this->Shapes.Get(index)); }

  SVTKM_EXEC
  svtkm::IdComponent GetNumberOfIndices(svtkm::Id index) const
  {
    return static_cast<svtkm::IdComponent>(this->Offsets.Get(index + 1) - this->Offsets.Get(index));
  }

  using IndicesType = svtkm::VecFromPortal<ConnectivityPortalType>;

  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  SVTKM_EXEC
  IndicesType GetIndices(svtkm::Id index) const
  {
    const svtkm::Id offset = this->Offsets.Get(index);
    const svtkm::Id endOffset = this->Offsets.Get(index + 1);
    const auto length = static_cast<svtkm::IdComponent>(endOffset - offset);

    return IndicesType(this->Connectivity, length, offset);
  }

private:
  ShapesPortalType Shapes;
  ConnectivityPortalType Connectivity;
  OffsetsPortalType Offsets;
};

} // namespace exec
} // namespace svtkm

#endif //  svtk_m_exec_ConnectivityExplicit_h
