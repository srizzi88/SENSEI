//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_PointLocatorUniformGrid_h
#define svtk_m_exec_PointLocatorUniformGrid_h

#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/exec/PointLocator.h>

#include <svtkm/VectorAnalysis.h>

namespace svtkm
{
namespace exec
{

template <typename DeviceAdapter>
class SVTKM_ALWAYS_EXPORT PointLocatorUniformGrid final : public svtkm::exec::PointLocator
{
public:
  using CoordPortalType =
    typename svtkm::cont::ArrayHandleVirtualCoordinates::template ExecutionTypes<
      DeviceAdapter>::PortalConst;
  using IdPortalType =
    typename svtkm::cont::ArrayHandle<svtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst;


  PointLocatorUniformGrid() = default;

  PointLocatorUniformGrid(const svtkm::Vec3f& min,
                          const svtkm::Vec3f& max,
                          const svtkm::Id3& dims,
                          const CoordPortalType& coords,
                          const IdPortalType& pointIds,
                          const IdPortalType& cellLower,
                          const IdPortalType& cellUpper)
    : Min(min)
    , Dims(dims)
    , Dxdydz((max - Min) / Dims)
    , Coords(coords)
    , PointIds(pointIds)
    , CellLower(cellLower)
    , CellUpper(cellUpper)
  {
  }

  /// \brief Nearest neighbor search using a Uniform Grid
  ///
  /// Parallel search of nearesat neighbor for each point in the \c queryPoints in the set of
  /// \c coords. Returns neareast neighbot in \c nearestNeighborIds and distances to nearest
  /// neighbor in \c distances.
  ///
  /// \param queryPoint Point coordinates to query for nearest neighbor.
  /// \param nearestNeighborId Neareast neighbor in the training dataset for each points in
  ///                            the test set
  /// \param distance2 Squared distance between query points and their nearest neighbors.
  SVTKM_EXEC virtual void FindNearestNeighbor(const svtkm::Vec3f& queryPoint,
                                             svtkm::Id& nearestNeighborId,
                                             svtkm::FloatDefault& distance2) const override
  {
    //std::cout << "FindNeareastNeighbor: " << queryPoint << std::endl;
    svtkm::Id3 ijk = (queryPoint - this->Min) / this->Dxdydz;
    ijk = svtkm::Max(ijk, svtkm::Id3(0));
    ijk = svtkm::Min(ijk, this->Dims - svtkm::Id3(1));

    nearestNeighborId = -1;
    distance2 = svtkm::Infinity<svtkm::FloatDefault>();

    this->FindInCell(queryPoint, ijk, nearestNeighborId, distance2);

    // TODO: This might stop looking before the absolute nearest neighbor is found.
    svtkm::Id maxLevel = svtkm::Max(svtkm::Max(this->Dims[0], this->Dims[1]), this->Dims[2]);
    svtkm::Id level;
    for (level = 1; (nearestNeighborId < 0) && (level < maxLevel); ++level)
    {
      this->FindInBox(queryPoint, ijk, level, nearestNeighborId, distance2);
    }

    // Search one more level out. This is still not guaranteed to find the closest point
    // in all cases (past level 2), but it will catch most cases where the closest point
    // is just on the other side of a cell boundary.
    this->FindInBox(queryPoint, ijk, level, nearestNeighborId, distance2);
  }

private:
  svtkm::Vec3f Min;
  svtkm::Id3 Dims;
  svtkm::Vec3f Dxdydz;

  CoordPortalType Coords;

  IdPortalType PointIds;
  IdPortalType CellLower;
  IdPortalType CellUpper;

  SVTKM_EXEC void FindInCell(const svtkm::Vec3f& queryPoint,
                            const svtkm::Id3& ijk,
                            svtkm::Id& nearestNeighborId,
                            svtkm::FloatDefault& nearestDistance2) const
  {
    svtkm::Id cellId = ijk[0] + (ijk[1] * this->Dims[0]) + (ijk[2] * this->Dims[0] * this->Dims[1]);
    svtkm::Id lower = this->CellLower.Get(cellId);
    svtkm::Id upper = this->CellUpper.Get(cellId);
    for (svtkm::Id index = lower; index < upper; index++)
    {
      svtkm::Id pointid = this->PointIds.Get(index);
      svtkm::Vec3f point = this->Coords.Get(pointid);
      svtkm::FloatDefault distance2 = svtkm::MagnitudeSquared(point - queryPoint);
      if (distance2 < nearestDistance2)
      {
        nearestNeighborId = pointid;
        nearestDistance2 = distance2;
      }
    }
  }

  SVTKM_EXEC void FindInBox(const svtkm::Vec3f& queryPoint,
                           const svtkm::Id3& boxCenter,
                           svtkm::Id level,
                           svtkm::Id& nearestNeighborId,
                           svtkm::FloatDefault& nearestDistance2) const
  {
    if ((boxCenter[0] - level) >= 0)
    {
      this->FindInXPlane(
        queryPoint, boxCenter - svtkm::Id3(level, 0, 0), level, nearestNeighborId, nearestDistance2);
    }
    if ((boxCenter[0] + level) < this->Dims[0])
    {
      this->FindInXPlane(
        queryPoint, boxCenter + svtkm::Id3(level, 0, 0), level, nearestNeighborId, nearestDistance2);
    }

    if ((boxCenter[1] - level) >= 0)
    {
      this->FindInYPlane(
        queryPoint, boxCenter - svtkm::Id3(0, level, 0), level, nearestNeighborId, nearestDistance2);
    }
    if ((boxCenter[1] + level) < this->Dims[1])
    {
      this->FindInYPlane(
        queryPoint, boxCenter + svtkm::Id3(0, level, 0), level, nearestNeighborId, nearestDistance2);
    }

    if ((boxCenter[2] - level) >= 0)
    {
      this->FindInZPlane(
        queryPoint, boxCenter - svtkm::Id3(0, 0, level), level, nearestNeighborId, nearestDistance2);
    }
    if ((boxCenter[2] + level) < this->Dims[2])
    {
      this->FindInZPlane(
        queryPoint, boxCenter + svtkm::Id3(0, 0, level), level, nearestNeighborId, nearestDistance2);
    }
  }

  SVTKM_EXEC void FindInPlane(const svtkm::Vec3f& queryPoint,
                             const svtkm::Id3& planeCenter,
                             const svtkm::Id3& div,
                             const svtkm::Id3& mod,
                             const svtkm::Id3& origin,
                             svtkm::Id numInPlane,
                             svtkm::Id& nearestNeighborId,
                             svtkm::FloatDefault& nearestDistance2) const
  {
    for (svtkm::Id index = 0; index < numInPlane; ++index)
    {
      svtkm::Id3 ijk = planeCenter + svtkm::Id3(index) / div +
        svtkm::Id3(index % mod[0], index % mod[1], index % mod[2]) + origin;
      if ((ijk[0] >= 0) && (ijk[0] < this->Dims[0]) && (ijk[1] >= 0) && (ijk[1] < this->Dims[1]) &&
          (ijk[2] >= 0) && (ijk[2] < this->Dims[2]))
      {
        this->FindInCell(queryPoint, ijk, nearestNeighborId, nearestDistance2);
      }
    }
  }

  SVTKM_EXEC void FindInXPlane(const svtkm::Vec3f& queryPoint,
                              const svtkm::Id3& planeCenter,
                              svtkm::Id level,
                              svtkm::Id& nearestNeighborId,
                              svtkm::FloatDefault& nearestDistance2) const
  {
    svtkm::Id yWidth = (2 * level) + 1;
    svtkm::Id zWidth = (2 * level) + 1;
    svtkm::Id3 div = { yWidth * zWidth, yWidth * zWidth, yWidth };
    svtkm::Id3 mod = { 1, yWidth, 1 };
    svtkm::Id3 origin = { 0, -level, -level };
    svtkm::Id numInPlane = yWidth * zWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }

  SVTKM_EXEC void FindInYPlane(const svtkm::Vec3f& queryPoint,
                              svtkm::Id3 planeCenter,
                              svtkm::Id level,
                              svtkm::Id& nearestNeighborId,
                              svtkm::FloatDefault& nearestDistance2) const
  {
    svtkm::Id xWidth = (2 * level) - 1;
    svtkm::Id zWidth = (2 * level) + 1;
    svtkm::Id3 div = { xWidth * zWidth, xWidth * zWidth, xWidth };
    svtkm::Id3 mod = { xWidth, 1, 1 };
    svtkm::Id3 origin = { -level + 1, 0, -level };
    svtkm::Id numInPlane = xWidth * zWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }

  SVTKM_EXEC void FindInZPlane(const svtkm::Vec3f& queryPoint,
                              svtkm::Id3 planeCenter,
                              svtkm::Id level,
                              svtkm::Id& nearestNeighborId,
                              svtkm::FloatDefault& nearestDistance2) const
  {
    svtkm::Id xWidth = (2 * level) - 1;
    svtkm::Id yWidth = (2 * level) - 1;
    svtkm::Id3 div = { xWidth * yWidth, xWidth, xWidth * yWidth };
    svtkm::Id3 mod = { xWidth, 1, 1 };
    svtkm::Id3 origin = { -level + 1, -level + 1, 0 };
    svtkm::Id numInPlane = xWidth * yWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }
};
}
}

#endif // svtk_m_exec_PointLocatorUniformGrid_h
