//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_internal_IndicesExtrude_h
#define svtk_m_internal_IndicesExtrude_h

#include <svtkm/Math.h>
#include <svtkm/Types.h>

namespace svtkm
{
namespace exec
{

struct IndicesExtrude
{
  IndicesExtrude() = default;

  SVTKM_EXEC
  IndicesExtrude(svtkm::Vec3i_32 pointIds1,
                 svtkm::Int32 plane1,
                 svtkm::Vec3i_32 pointIds2,
                 svtkm::Int32 plane2,
                 svtkm::Int32 numberOfPointsPerPlane)
    : PointIds{ pointIds1, pointIds2 }
    , Planes{ plane1, plane2 }
    , NumberOfPointsPerPlane(numberOfPointsPerPlane)
  {
  }

  SVTKM_EXEC
  svtkm::Id operator[](svtkm::IdComponent index) const
  {
    SVTKM_ASSERT(index >= 0 && index < 6);
    if (index < 3)
    {
      return (static_cast<svtkm::Id>(this->NumberOfPointsPerPlane) * this->Planes[0]) +
        this->PointIds[0][index];
    }
    else
    {
      return (static_cast<svtkm::Id>(this->NumberOfPointsPerPlane) * this->Planes[1]) +
        this->PointIds[1][index - 3];
    }
  }

  SVTKM_EXEC
  constexpr svtkm::IdComponent GetNumberOfComponents() const { return 6; }

  template <typename T, svtkm::IdComponent DestSize>
  SVTKM_EXEC void CopyInto(svtkm::Vec<T, DestSize>& dest) const
  {
    for (svtkm::IdComponent i = 0; i < svtkm::Min(6, DestSize); ++i)
    {
      dest[i] = (*this)[i];
    }
  }

  svtkm::Vec3i_32 PointIds[2];
  svtkm::Int32 Planes[2];
  svtkm::Int32 NumberOfPointsPerPlane;
};

template <typename ConnectivityPortalType>
struct ReverseIndicesExtrude
{
  ReverseIndicesExtrude() = default;

  SVTKM_EXEC
  ReverseIndicesExtrude(const ConnectivityPortalType conn,
                        svtkm::Id offset1,
                        svtkm::IdComponent length1,
                        svtkm::Id offset2,
                        svtkm::IdComponent length2,
                        svtkm::IdComponent plane1,
                        svtkm::IdComponent plane2,
                        svtkm::Int32 numberOfCellsPerPlane)
    : Connectivity(conn)
    , Offset1(offset1)
    , Offset2(offset2)
    , Length1(length1)
    , NumberOfComponents(length1 + length2)
    , CellOffset1(plane1 * numberOfCellsPerPlane)
    , CellOffset2(plane2 * numberOfCellsPerPlane)
  {
  }

  SVTKM_EXEC
  svtkm::Id operator[](svtkm::IdComponent index) const
  {
    SVTKM_ASSERT(index >= 0 && index < (this->NumberOfComponents));
    if (index < this->Length1)
    {
      return this->Connectivity.Get(this->Offset1 + index) + this->CellOffset1;
    }
    else
    {
      return this->Connectivity.Get(this->Offset2 + index - this->Length1) + this->CellOffset2;
    }
  }

  SVTKM_EXEC
  svtkm::IdComponent GetNumberOfComponents() const { return this->NumberOfComponents; }

  template <typename T, svtkm::IdComponent DestSize>
  SVTKM_EXEC void CopyInto(svtkm::Vec<T, DestSize>& dest) const
  {
    for (svtkm::IdComponent i = 0; i < svtkm::Min(this->NumberOfComponents, DestSize); ++i)
    {
      dest[i] = (*this)[i];
    }
  }

  ConnectivityPortalType Connectivity;
  svtkm::Id Offset1, Offset2;
  svtkm::IdComponent Length1;
  svtkm::IdComponent NumberOfComponents;
  svtkm::Id CellOffset1, CellOffset2;
};
}
}

#endif //svtkm_m_internal_IndicesExtrude_h
