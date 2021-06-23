//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/rendering/raytracing/VolumeRendererStructured.h>

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/ColorTable.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

namespace
{

template <typename Device>
class RectilinearLocator
{
protected:
  using DefaultHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
  using CartesianArrayHandle =
    svtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;
  using DefaultConstHandle = typename DefaultHandle::ExecutionTypes<Device>::PortalConst;
  using CartesianConstPortal = typename CartesianArrayHandle::ExecutionTypes<Device>::PortalConst;

  DefaultConstHandle CoordPortals[3];
  CartesianConstPortal Coordinates;
  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint, 3>
    Conn;
  svtkm::Id3 PointDimensions;
  svtkm::Vec3f_32 MinPoint;
  svtkm::Vec3f_32 MaxPoint;

public:
  RectilinearLocator(const CartesianArrayHandle& coordinates,
                     svtkm::cont::CellSetStructured<3>& cellset)
    : Coordinates(coordinates.PrepareForInput(Device()))
    , Conn(cellset.PrepareForInput(Device(),
                                   svtkm::TopologyElementTagCell(),
                                   svtkm::TopologyElementTagPoint()))
  {
    CoordPortals[0] = Coordinates.GetFirstPortal();
    CoordPortals[1] = Coordinates.GetSecondPortal();
    CoordPortals[2] = Coordinates.GetThirdPortal();
    PointDimensions = Conn.GetPointDimensions();
    MinPoint[0] =
      static_cast<svtkm::Float32>(coordinates.GetPortalConstControl().GetFirstPortal().Get(0));
    MinPoint[1] =
      static_cast<svtkm::Float32>(coordinates.GetPortalConstControl().GetSecondPortal().Get(0));
    MinPoint[2] =
      static_cast<svtkm::Float32>(coordinates.GetPortalConstControl().GetThirdPortal().Get(0));

    MaxPoint[0] = static_cast<svtkm::Float32>(
      coordinates.GetPortalConstControl().GetFirstPortal().Get(PointDimensions[0] - 1));
    MaxPoint[1] = static_cast<svtkm::Float32>(
      coordinates.GetPortalConstControl().GetSecondPortal().Get(PointDimensions[1] - 1));
    MaxPoint[2] = static_cast<svtkm::Float32>(
      coordinates.GetPortalConstControl().GetThirdPortal().Get(PointDimensions[2] - 1));
  }

  SVTKM_EXEC
  inline bool IsInside(const svtkm::Vec3f_32& point) const
  {
    bool inside = true;
    if (point[0] < MinPoint[0] || point[0] > MaxPoint[0])
      inside = false;
    if (point[1] < MinPoint[1] || point[1] > MaxPoint[1])
      inside = false;
    if (point[2] < MinPoint[2] || point[2] > MaxPoint[2])
      inside = false;
    return inside;
  }

  SVTKM_EXEC
  inline void GetCellIndices(const svtkm::Id3& cell, svtkm::Vec<svtkm::Id, 8>& cellIndices) const
  {
    cellIndices[0] = (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDimensions[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDimensions[0] * PointDimensions[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDimensions[0];
    cellIndices[7] = cellIndices[6] - 1;
  } // GetCellIndices

  //
  // Assumes point inside the data set
  //
  SVTKM_EXEC
  inline void LocateCell(svtkm::Id3& cell,
                         const svtkm::Vec3f_32& point,
                         svtkm::Vec3f_32& invSpacing) const
  {
    for (svtkm::Int32 dim = 0; dim < 3; ++dim)
    {
      //
      // When searching for points, we consider the max value of the cell
      // to be apart of the next cell. If the point falls on the boundary of the
      // data set, then it is technically inside a cell. This checks for that case
      //
      if (point[dim] == MaxPoint[dim])
      {
        cell[dim] = PointDimensions[dim] - 2;
        continue;
      }

      bool found = false;
      svtkm::Float32 minVal = static_cast<svtkm::Float32>(CoordPortals[dim].Get(cell[dim]));
      const svtkm::Id searchDir = (point[dim] - minVal >= 0.f) ? 1 : -1;
      svtkm::Float32 maxVal = static_cast<svtkm::Float32>(CoordPortals[dim].Get(cell[dim] + 1));

      while (!found)
      {
        if (point[dim] >= minVal && point[dim] < maxVal)
        {
          found = true;
          continue;
        }

        cell[dim] += searchDir;
        svtkm::Id nextCellId = searchDir == 1 ? cell[dim] + 1 : cell[dim];
        BOUNDS_CHECK(CoordPortals[dim], nextCellId);
        svtkm::Float32 next = static_cast<svtkm::Float32>(CoordPortals[dim].Get(nextCellId));
        if (searchDir == 1)
        {
          minVal = maxVal;
          maxVal = next;
        }
        else
        {
          maxVal = minVal;
          minVal = next;
        }
      }
      invSpacing[dim] = 1.f / (maxVal - minVal);
    }
  } // LocateCell

  SVTKM_EXEC
  inline svtkm::Id GetCellIndex(const svtkm::Id3& cell) const
  {
    return (cell[2] * (PointDimensions[1] - 1) + cell[1]) * (PointDimensions[0] - 1) + cell[0];
  }

  SVTKM_EXEC
  inline void GetPoint(const svtkm::Id& index, svtkm::Vec3f_32& point) const
  {
    BOUNDS_CHECK(Coordinates, index);
    point = Coordinates.Get(index);
  }

  SVTKM_EXEC
  inline void GetMinPoint(const svtkm::Id3& cell, svtkm::Vec3f_32& point) const
  {
    const svtkm::Id pointIndex =
      (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    point = Coordinates.Get(pointIndex);
  }
}; // class RectilinearLocator

template <typename Device>
class UniformLocator
{
protected:
  using UniformArrayHandle = svtkm::cont::ArrayHandleUniformPointCoordinates;
  using UniformConstPortal = typename UniformArrayHandle::ExecutionTypes<Device>::PortalConst;

  svtkm::Id3 PointDimensions;
  svtkm::Vec3f_32 Origin;
  svtkm::Vec3f_32 InvSpacing;
  svtkm::Vec3f_32 MaxPoint;
  UniformConstPortal Coordinates;
  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint, 3>
    Conn;

public:
  UniformLocator(const UniformArrayHandle& coordinates, svtkm::cont::CellSetStructured<3>& cellset)
    : Coordinates(coordinates.PrepareForInput(Device()))
    , Conn(cellset.PrepareForInput(Device(),
                                   svtkm::TopologyElementTagCell(),
                                   svtkm::TopologyElementTagPoint()))
  {
    Origin = Coordinates.GetOrigin();
    PointDimensions = Conn.GetPointDimensions();
    svtkm::Vec3f_32 spacing = Coordinates.GetSpacing();

    svtkm::Vec3f_32 unitLength;
    unitLength[0] = static_cast<svtkm::Float32>(PointDimensions[0] - 1);
    unitLength[1] = static_cast<svtkm::Float32>(PointDimensions[1] - 1);
    unitLength[2] = static_cast<svtkm::Float32>(PointDimensions[2] - 1);
    MaxPoint = Origin + spacing * unitLength;
    InvSpacing[0] = 1.f / spacing[0];
    InvSpacing[1] = 1.f / spacing[1];
    InvSpacing[2] = 1.f / spacing[2];
  }

  SVTKM_EXEC
  inline bool IsInside(const svtkm::Vec3f_32& point) const
  {
    bool inside = true;
    if (point[0] < Origin[0] || point[0] > MaxPoint[0])
      inside = false;
    if (point[1] < Origin[1] || point[1] > MaxPoint[1])
      inside = false;
    if (point[2] < Origin[2] || point[2] > MaxPoint[2])
      inside = false;
    return inside;
  }

  SVTKM_EXEC
  inline void GetCellIndices(const svtkm::Id3& cell, svtkm::Vec<svtkm::Id, 8>& cellIndices) const
  {
    cellIndices[0] = (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDimensions[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDimensions[0] * PointDimensions[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDimensions[0];
    cellIndices[7] = cellIndices[6] - 1;
  } // GetCellIndices

  SVTKM_EXEC
  inline svtkm::Id GetCellIndex(const svtkm::Id3& cell) const
  {
    return (cell[2] * (PointDimensions[1] - 1) + cell[1]) * (PointDimensions[0] - 1) + cell[0];
  }

  SVTKM_EXEC
  inline void LocateCell(svtkm::Id3& cell,
                         const svtkm::Vec3f_32& point,
                         svtkm::Vec3f_32& invSpacing) const
  {
    svtkm::Vec3f_32 temp = point;
    temp = temp - Origin;
    temp = temp * InvSpacing;
    //make sure that if we border the upper edge, we sample the correct cell
    if (temp[0] == svtkm::Float32(PointDimensions[0] - 1))
      temp[0] = svtkm::Float32(PointDimensions[0] - 2);
    if (temp[1] == svtkm::Float32(PointDimensions[1] - 1))
      temp[1] = svtkm::Float32(PointDimensions[1] - 2);
    if (temp[2] == svtkm::Float32(PointDimensions[2] - 1))
      temp[2] = svtkm::Float32(PointDimensions[2] - 2);
    cell = temp;
    invSpacing = InvSpacing;
  }

  SVTKM_EXEC
  inline void GetPoint(const svtkm::Id& index, svtkm::Vec3f_32& point) const
  {
    BOUNDS_CHECK(Coordinates, index);
    point = Coordinates.Get(index);
  }

  SVTKM_EXEC
  inline void GetMinPoint(const svtkm::Id3& cell, svtkm::Vec3f_32& point) const
  {
    const svtkm::Id pointIndex =
      (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    point = Coordinates.Get(pointIndex);
  }

}; // class UniformLocator


} //namespace


template <typename DeviceAdapterTag, typename LocatorType>
class Sampler : public svtkm::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename svtkm::cont::ArrayHandle<svtkm::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ExecutionTypes<DeviceAdapterTag>::PortalConst;
  ColorArrayPortal ColorMap;
  svtkm::Id ColorMapSize;
  svtkm::Float32 MinScalar;
  svtkm::Float32 SampleDistance;
  svtkm::Float32 InverseDeltaScalar;
  LocatorType Locator;

public:
  SVTKM_CONT
  Sampler(const ColorArrayHandle& colorMap,
          const svtkm::Float32& minScalar,
          const svtkm::Float32& maxScalar,
          const svtkm::Float32& sampleDistance,
          const LocatorType& locator)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag()))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
  {
    ColorMapSize = colorMap.GetNumberOfValues() - 1;
    if ((maxScalar - minScalar) != 0.f)
    {
      InverseDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename ScalarPortalType, typename ColorBufferType>
  SVTKM_EXEC void operator()(const svtkm::Vec3f_32& rayDir,
                            const svtkm::Vec3f_32& rayOrigin,
                            const svtkm::Float32& minDistance,
                            const svtkm::Float32& maxDistance,
                            ColorBufferType& colorBuffer,
                            ScalarPortalType& scalars,
                            const svtkm::Id& pixelIndex) const
  {
    svtkm::Vec4f_32 color;
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    color[0] = colorBuffer.Get(pixelIndex * 4 + 0);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    color[1] = colorBuffer.Get(pixelIndex * 4 + 1);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    color[2] = colorBuffer.Get(pixelIndex * 4 + 2);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    color[3] = colorBuffer.Get(pixelIndex * 4 + 3);

    if (minDistance == -1.f)
    {
      return; //TODO: Compact? or just image subset...
    }
    //get the initial sample position;
    svtkm::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    svtkm::Float32 distance = minDistance + 0.0001f;
    sampleLocation = rayOrigin + distance * rayDir;
    // since the calculations are slightly different, we could hit an
    // edge case where the first sample location may not be in the data set.
    // Thus, advance to the next sample location
    while (!Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      distance += SampleDistance;
      sampleLocation = rayOrigin + distance * rayDir;
    }
    /*
            7----------6
           /|         /|
          4----------5 |
          | |        | |
          | 3--------|-2    z y
          |/         |/     |/
          0----------1      |__ x
    */
    svtkm::Vec3f_32 bottomLeft(0, 0, 0);
    bool newCell = true;
    //check to see if we left the cell
    svtkm::Float32 tx = 0.f;
    svtkm::Float32 ty = 0.f;
    svtkm::Float32 tz = 0.f;
    svtkm::Float32 scalar0 = 0.f;
    svtkm::Float32 scalar1minus0 = 0.f;
    svtkm::Float32 scalar2minus3 = 0.f;
    svtkm::Float32 scalar3 = 0.f;
    svtkm::Float32 scalar4 = 0.f;
    svtkm::Float32 scalar5minus4 = 0.f;
    svtkm::Float32 scalar6minus7 = 0.f;
    svtkm::Float32 scalar7 = 0.f;

    svtkm::Id3 cell(0, 0, 0);
    svtkm::Vec3f_32 invSpacing(0.f, 0.f, 0.f);


    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      svtkm::Float32 mint = svtkm::Min(tx, svtkm::Min(ty, tz));
      svtkm::Float32 maxt = svtkm::Max(tx, svtkm::Max(ty, tz));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;

      if (newCell)
      {

        svtkm::Vec<svtkm::Id, 8> cellIndices;
        Locator.LocateCell(cell, sampleLocation, invSpacing);
        Locator.GetCellIndices(cell, cellIndices);
        Locator.GetPoint(cellIndices[0], bottomLeft);

        scalar0 = svtkm::Float32(scalars.Get(cellIndices[0]));
        svtkm::Float32 scalar1 = svtkm::Float32(scalars.Get(cellIndices[1]));
        svtkm::Float32 scalar2 = svtkm::Float32(scalars.Get(cellIndices[2]));
        scalar3 = svtkm::Float32(scalars.Get(cellIndices[3]));
        scalar4 = svtkm::Float32(scalars.Get(cellIndices[4]));
        svtkm::Float32 scalar5 = svtkm::Float32(scalars.Get(cellIndices[5]));
        svtkm::Float32 scalar6 = svtkm::Float32(scalars.Get(cellIndices[6]));
        scalar7 = svtkm::Float32(scalars.Get(cellIndices[7]));

        // save ourselves a couple extra instructions
        scalar6minus7 = scalar6 - scalar7;
        scalar5minus4 = scalar5 - scalar4;
        scalar1minus0 = scalar1 - scalar0;
        scalar2minus3 = scalar2 - scalar3;

        tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

        newCell = false;
      }

      svtkm::Float32 lerped76 = scalar7 + tx * scalar6minus7;
      svtkm::Float32 lerped45 = scalar4 + tx * scalar5minus4;
      svtkm::Float32 lerpedTop = lerped45 + ty * (lerped76 - lerped45);

      svtkm::Float32 lerped01 = scalar0 + tx * scalar1minus0;
      svtkm::Float32 lerped32 = scalar3 + tx * scalar2minus3;
      svtkm::Float32 lerpedBottom = lerped01 + ty * (lerped32 - lerped01);

      svtkm::Float32 finalScalar = lerpedBottom + tz * (lerpedTop - lerpedBottom);
      //normalize scalar
      finalScalar = (finalScalar - MinScalar) * InverseDeltaScalar;

      svtkm::Id colorIndex =
        static_cast<svtkm::Id>(finalScalar * static_cast<svtkm::Float32>(ColorMapSize));
      if (colorIndex < 0)
        colorIndex = 0;
      if (colorIndex > ColorMapSize)
        colorIndex = ColorMapSize;

      svtkm::Vec4f_32 sampleColor = ColorMap.Get(colorIndex);

      //composite
      sampleColor[3] *= (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * sampleColor[3];
      color[1] = color[1] + sampleColor[1] * sampleColor[3];
      color[2] = color[2] + sampleColor[2] * sampleColor[3];
      color[3] = sampleColor[3] + color[3];
      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      //this is linear could just do an addition
      tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
      ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
      tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

      if (color[3] >= 1.f)
        break;
    }

    color[0] = svtkm::Min(color[0], 1.f);
    color[1] = svtkm::Min(color[1], 1.f);
    color[2] = svtkm::Min(color[2], 1.f);
    color[3] = svtkm::Min(color[3], 1.f);

    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    colorBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    colorBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    colorBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    colorBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //Sampler

template <typename DeviceAdapterTag, typename LocatorType>
class SamplerCellAssoc : public svtkm::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename svtkm::cont::ArrayHandle<svtkm::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ExecutionTypes<DeviceAdapterTag>::PortalConst;
  ColorArrayPortal ColorMap;
  svtkm::Id ColorMapSize;
  svtkm::Float32 MinScalar;
  svtkm::Float32 SampleDistance;
  svtkm::Float32 InverseDeltaScalar;
  LocatorType Locator;

public:
  SVTKM_CONT
  SamplerCellAssoc(const ColorArrayHandle& colorMap,
                   const svtkm::Float32& minScalar,
                   const svtkm::Float32& maxScalar,
                   const svtkm::Float32& sampleDistance,
                   const LocatorType& locator)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag()))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
  {
    ColorMapSize = colorMap.GetNumberOfValues() - 1;
    if ((maxScalar - minScalar) != 0.f)
    {
      InverseDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename ScalarPortalType, typename ColorBufferType>
  SVTKM_EXEC void operator()(const svtkm::Vec3f_32& rayDir,
                            const svtkm::Vec3f_32& rayOrigin,
                            const svtkm::Float32& minDistance,
                            const svtkm::Float32& maxDistance,
                            ColorBufferType& colorBuffer,
                            const ScalarPortalType& scalars,
                            const svtkm::Id& pixelIndex) const
  {
    svtkm::Vec4f_32 color;
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    color[0] = colorBuffer.Get(pixelIndex * 4 + 0);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    color[1] = colorBuffer.Get(pixelIndex * 4 + 1);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    color[2] = colorBuffer.Get(pixelIndex * 4 + 2);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    color[3] = colorBuffer.Get(pixelIndex * 4 + 3);

    if (minDistance == -1.f)
      return; //TODO: Compact? or just image subset...
    //get the initial sample position;
    svtkm::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    svtkm::Float32 distance = minDistance + 0.0001f;
    sampleLocation = rayOrigin + distance * rayDir;
    // since the calculations are slightly different, we could hit an
    // edge case where the first sample location may not be in the data set.
    // Thus, advance to the next sample location
    while (!Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      distance += SampleDistance;
      sampleLocation = rayOrigin + distance * rayDir;
    }

    /*
            7----------6
           /|         /|
          4----------5 |
          | |        | |
          | 3--------|-2    z y
          |/         |/     |/
          0----------1      |__ x
    */
    bool newCell = true;
    svtkm::Float32 tx = 2.f;
    svtkm::Float32 ty = 2.f;
    svtkm::Float32 tz = 2.f;
    svtkm::Float32 scalar0 = 0.f;
    svtkm::Vec4f_32 sampleColor(0.f, 0.f, 0.f, 0.f);
    svtkm::Vec3f_32 bottomLeft(0.f, 0.f, 0.f);
    svtkm::Vec3f_32 invSpacing(0.f, 0.f, 0.f);
    svtkm::Id3 cell(0, 0, 0);
    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      svtkm::Float32 mint = svtkm::Min(tx, svtkm::Min(ty, tz));
      svtkm::Float32 maxt = svtkm::Max(tx, svtkm::Max(ty, tz));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;
      if (newCell)
      {
        Locator.LocateCell(cell, sampleLocation, invSpacing);
        svtkm::Id cellId = Locator.GetCellIndex(cell);

        scalar0 = svtkm::Float32(scalars.Get(cellId));
        svtkm::Float32 normalizedScalar = (scalar0 - MinScalar) * InverseDeltaScalar;
        svtkm::Id colorIndex =
          static_cast<svtkm::Id>(normalizedScalar * static_cast<svtkm::Float32>(ColorMapSize));
        if (colorIndex < 0)
          colorIndex = 0;
        if (colorIndex > ColorMapSize)
          colorIndex = ColorMapSize;
        sampleColor = ColorMap.Get(colorIndex);
        Locator.GetMinPoint(cell, bottomLeft);
        tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];
        newCell = false;
      }

      // just repeatably composite
      svtkm::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];
      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      if (color[3] >= 1.f)
        break;
      tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
      ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
      tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];
    }
    color[0] = svtkm::Min(color[0], 1.f);
    color[1] = svtkm::Min(color[1], 1.f);
    color[2] = svtkm::Min(color[2], 1.f);
    color[3] = svtkm::Min(color[3], 1.f);

    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    colorBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    colorBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    colorBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    colorBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //SamplerCell

class CalcRayStart : public svtkm::worklet::WorkletMapField
{
  svtkm::Float32 Xmin;
  svtkm::Float32 Ymin;
  svtkm::Float32 Zmin;
  svtkm::Float32 Xmax;
  svtkm::Float32 Ymax;
  svtkm::Float32 Zmax;

public:
  SVTKM_CONT
  CalcRayStart(const svtkm::Bounds boundingBox)
  {
    Xmin = static_cast<svtkm::Float32>(boundingBox.X.Min);
    Xmax = static_cast<svtkm::Float32>(boundingBox.X.Max);
    Ymin = static_cast<svtkm::Float32>(boundingBox.Y.Min);
    Ymax = static_cast<svtkm::Float32>(boundingBox.Y.Max);
    Zmin = static_cast<svtkm::Float32>(boundingBox.Z.Min);
    Zmax = static_cast<svtkm::Float32>(boundingBox.Z.Max);
  }

  SVTKM_EXEC
  svtkm::Float32 rcp(svtkm::Float32 f) const { return 1.0f / f; }

  SVTKM_EXEC
  svtkm::Float32 rcp_safe(svtkm::Float32 f) const { return rcp((fabs(f) < 1e-8f) ? 1e-8f : f); }

  using ControlSignature = void(FieldIn, FieldOut, FieldInOut, FieldInOut, FieldIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  template <typename Precision>
  SVTKM_EXEC void operator()(const svtkm::Vec<Precision, 3>& rayDir,
                            svtkm::Float32& minDistance,
                            svtkm::Float32& distance,
                            svtkm::Float32& maxDistance,
                            const svtkm::Vec<Precision, 3>& rayOrigin) const
  {
    svtkm::Float32 dirx = static_cast<svtkm::Float32>(rayDir[0]);
    svtkm::Float32 diry = static_cast<svtkm::Float32>(rayDir[1]);
    svtkm::Float32 dirz = static_cast<svtkm::Float32>(rayDir[2]);
    svtkm::Float32 origx = static_cast<svtkm::Float32>(rayOrigin[0]);
    svtkm::Float32 origy = static_cast<svtkm::Float32>(rayOrigin[1]);
    svtkm::Float32 origz = static_cast<svtkm::Float32>(rayOrigin[2]);

    svtkm::Float32 invDirx = rcp_safe(dirx);
    svtkm::Float32 invDiry = rcp_safe(diry);
    svtkm::Float32 invDirz = rcp_safe(dirz);

    svtkm::Float32 odirx = origx * invDirx;
    svtkm::Float32 odiry = origy * invDiry;
    svtkm::Float32 odirz = origz * invDirz;

    svtkm::Float32 xmin = Xmin * invDirx - odirx;
    svtkm::Float32 ymin = Ymin * invDiry - odiry;
    svtkm::Float32 zmin = Zmin * invDirz - odirz;
    svtkm::Float32 xmax = Xmax * invDirx - odirx;
    svtkm::Float32 ymax = Ymax * invDiry - odiry;
    svtkm::Float32 zmax = Zmax * invDirz - odirz;


    minDistance = svtkm::Max(
      svtkm::Max(svtkm::Max(svtkm::Min(ymin, ymax), svtkm::Min(xmin, xmax)), svtkm::Min(zmin, zmax)),
      minDistance);
    svtkm::Float32 exitDistance =
      svtkm::Min(svtkm::Min(svtkm::Max(ymin, ymax), svtkm::Max(xmin, xmax)), svtkm::Max(zmin, zmax));
    maxDistance = svtkm::Min(maxDistance, exitDistance);
    if (maxDistance < minDistance)
    {
      minDistance = -1.f; //flag for miss
    }
    else
    {
      distance = minDistance;
    }
  }
}; //class CalcRayStart

VolumeRendererStructured::VolumeRendererStructured()
{
  IsSceneDirty = false;
  IsUniformDataSet = true;
  SampleDistance = -1.f;
}

void VolumeRendererStructured::SetColorMap(const svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void VolumeRendererStructured::SetData(const svtkm::cont::CoordinateSystem& coords,
                                       const svtkm::cont::Field& scalarField,
                                       const svtkm::cont::CellSetStructured<3>& cellset,
                                       const svtkm::Range& scalarRange)
{
  IsUniformDataSet = !coords.GetData().IsType<CartesianArrayHandle>();
  IsSceneDirty = true;
  SpatialExtent = coords.GetBounds();
  Coordinates = coords.GetData();
  ScalarField = &scalarField;
  Cellset = cellset;
  ScalarRange = scalarRange;
}

template <typename Precision>
struct VolumeRendererStructured::RenderFunctor
{
protected:
  svtkm::rendering::raytracing::VolumeRendererStructured* Self;
  svtkm::rendering::raytracing::Ray<Precision>& Rays;

public:
  SVTKM_CONT
  RenderFunctor(svtkm::rendering::raytracing::VolumeRendererStructured* self,
                svtkm::rendering::raytracing::Ray<Precision>& rays)
    : Self(self)
    , Rays(rays)
  {
  }

  template <typename Device>
  SVTKM_CONT bool operator()(Device)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);

    this->Self->RenderOnDevice(this->Rays, Device());
    return true;
  }
};

void VolumeRendererStructured::Render(svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays)
{
  RenderFunctor<svtkm::Float32> functor(this, rays);
  svtkm::cont::TryExecute(functor);
}

//void
//VolumeRendererStructured::Render(svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays)
//{
//  RenderFunctor<svtkm::Float64> functor(this, rays);
//  svtkm::cont::TryExecute(functor);
//}

template <typename Precision, typename Device>
void VolumeRendererStructured::RenderOnDevice(svtkm::rendering::raytracing::Ray<Precision>& rays,
                                              Device)
{
  svtkm::cont::Timer renderTimer{ Device() };
  renderTimer.Start();
  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("volume_render_structured");
  logger->AddLogData("device", GetDeviceString(Device()));

  if (SampleDistance <= 0.f)
  {
    svtkm::Vec3f_32 extent;
    extent[0] = static_cast<svtkm::Float32>(this->SpatialExtent.X.Length());
    extent[1] = static_cast<svtkm::Float32>(this->SpatialExtent.Y.Length());
    extent[2] = static_cast<svtkm::Float32>(this->SpatialExtent.Z.Length());
    const svtkm::Float32 defaultNumberOfSamples = 200.f;
    SampleDistance = svtkm::Magnitude(extent) / defaultNumberOfSamples;
  }

  svtkm::cont::Timer timer{ Device() };
  timer.Start();
  svtkm::worklet::DispatcherMapField<CalcRayStart> calcRayStartDispatcher(
    CalcRayStart(this->SpatialExtent));
  calcRayStartDispatcher.SetDevice(Device());
  calcRayStartDispatcher.Invoke(
    rays.Dir, rays.MinDistance, rays.Distance, rays.MaxDistance, rays.Origin);

  svtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("calc_ray_start", time);
  timer.Start();

  const bool isSupportedField = ScalarField->IsFieldCell() || ScalarField->IsFieldPoint();
  if (!isSupportedField)
  {
    throw svtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
  }
  const bool isAssocPoints = ScalarField->IsFieldPoint();

  if (IsUniformDataSet)
  {
    svtkm::cont::ArrayHandleUniformPointCoordinates vertices;
    vertices = Coordinates.Cast<svtkm::cont::ArrayHandleUniformPointCoordinates>();
    UniformLocator<Device> locator(vertices, Cellset);

    if (isAssocPoints)
    {
      svtkm::worklet::DispatcherMapField<Sampler<Device, UniformLocator<Device>>> samplerDispatcher(
        Sampler<Device, UniformLocator<Device>>(ColorMap,
                                                svtkm::Float32(ScalarRange.Min),
                                                svtkm::Float32(ScalarRange.Max),
                                                SampleDistance,
                                                locator));
      samplerDispatcher.SetDevice(Device());
      samplerDispatcher.Invoke(rays.Dir,
                               rays.Origin,
                               rays.MinDistance,
                               rays.MaxDistance,
                               rays.Buffers.at(0).Buffer,
                               ScalarField->GetData().ResetTypes(svtkm::TypeListFieldScalar()));
    }
    else
    {
      svtkm::worklet::DispatcherMapField<SamplerCellAssoc<Device, UniformLocator<Device>>>(
        SamplerCellAssoc<Device, UniformLocator<Device>>(ColorMap,
                                                         svtkm::Float32(ScalarRange.Min),
                                                         svtkm::Float32(ScalarRange.Max),
                                                         SampleDistance,
                                                         locator))
        .Invoke(rays.Dir,
                rays.Origin,
                rays.MinDistance,
                rays.MaxDistance,
                rays.Buffers.at(0).Buffer,
                ScalarField->GetData().ResetTypes(svtkm::TypeListFieldScalar()));
    }
  }
  else
  {
    CartesianArrayHandle vertices;
    vertices = Coordinates.Cast<CartesianArrayHandle>();
    RectilinearLocator<Device> locator(vertices, Cellset);
    if (isAssocPoints)
    {
      svtkm::worklet::DispatcherMapField<Sampler<Device, RectilinearLocator<Device>>>
        samplerDispatcher(
          Sampler<Device, RectilinearLocator<Device>>(ColorMap,
                                                      svtkm::Float32(ScalarRange.Min),
                                                      svtkm::Float32(ScalarRange.Max),
                                                      SampleDistance,
                                                      locator));
      samplerDispatcher.SetDevice(Device());
      samplerDispatcher.Invoke(rays.Dir,
                               rays.Origin,
                               rays.MinDistance,
                               rays.MaxDistance,
                               rays.Buffers.at(0).Buffer,
                               ScalarField->GetData().ResetTypes(svtkm::TypeListFieldScalar()));
    }
    else
    {
      svtkm::worklet::DispatcherMapField<SamplerCellAssoc<Device, RectilinearLocator<Device>>>
        rectilinearLocatorDispatcher(
          SamplerCellAssoc<Device, RectilinearLocator<Device>>(ColorMap,
                                                               svtkm::Float32(ScalarRange.Min),
                                                               svtkm::Float32(ScalarRange.Max),
                                                               SampleDistance,
                                                               locator));
      rectilinearLocatorDispatcher.SetDevice(Device());
      rectilinearLocatorDispatcher.Invoke(
        rays.Dir,
        rays.Origin,
        rays.MinDistance,
        rays.MaxDistance,
        rays.Buffers.at(0).Buffer,
        ScalarField->GetData().ResetTypes(svtkm::TypeListFieldScalar()));
    }
  }

  time = timer.GetElapsedTime();
  logger->AddLogData("sample", time);

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} //Render

void VolumeRendererStructured::SetSampleDistance(const svtkm::Float32& distance)
{
  if (distance <= 0.f)
    throw svtkm::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}
}
}
} //namespace svtkm::rendering::raytracing
