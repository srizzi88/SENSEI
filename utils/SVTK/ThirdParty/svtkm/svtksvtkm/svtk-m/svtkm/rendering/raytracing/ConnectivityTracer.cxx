//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/rendering/raytracing/ConnectivityTracer.h>

#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/cont/internal/DeviceAdapterListHelpers.h>

#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/CellIntersector.h>
#include <svtkm/rendering/raytracing/CellSampler.h>
#include <svtkm/rendering/raytracing/CellTables.h>
#include <svtkm/rendering/raytracing/MeshConnectivityBase.h>
#include <svtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <svtkm/rendering/raytracing/Worklets.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <iomanip>

#ifndef CELL_SHAPE_ZOO
#define CELL_SHAPE_ZOO 255
#endif

#ifndef CELL_SHAPE_STRUCTURED
#define CELL_SHAPE_STRUCTURED 254
#endif

namespace svtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class AdjustSample : public svtkm::worklet::WorkletMapField
{
  svtkm::Float64 SampleDistance;

public:
  SVTKM_CONT
  AdjustSample(const svtkm::Float64 sampleDistance)
    : SampleDistance(sampleDistance)
  {
  }
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);
  template <typename FloatType>
  SVTKM_EXEC inline void operator()(const svtkm::UInt8& status, FloatType& currentDistance) const
  {
    if (status != RAY_ACTIVE)
      return;

    currentDistance += FMod(currentDistance, (FloatType)SampleDistance);
  }
}; //class AdvanceRay

template <typename FloatType>
void RayTracking<FloatType>::Compact(svtkm::cont::ArrayHandle<FloatType>& compactedDistances,
                                     svtkm::cont::ArrayHandle<UInt8>& masks)
{
  //
  // These distances are stored in the rays, and it has
  // already been compacted.
  //
  CurrentDistance = compactedDistances;

  svtkm::cont::ArrayHandleCast<svtkm::Id, svtkm::cont::ArrayHandle<svtkm::UInt8>> castedMasks(masks);

  bool distance1IsEnter = EnterDist == &Distance1;

  svtkm::cont::ArrayHandle<FloatType> compactedDistance1;
  svtkm::cont::Algorithm::CopyIf(Distance1, masks, compactedDistance1);
  Distance1 = compactedDistance1;

  svtkm::cont::ArrayHandle<FloatType> compactedDistance2;
  svtkm::cont::Algorithm::CopyIf(Distance2, masks, compactedDistance2);
  Distance2 = compactedDistance2;

  svtkm::cont::ArrayHandle<svtkm::Int32> compactedExitFace;
  svtkm::cont::Algorithm::CopyIf(ExitFace, masks, compactedExitFace);
  ExitFace = compactedExitFace;

  if (distance1IsEnter)
  {
    EnterDist = &Distance1;
    ExitDist = &Distance2;
  }
  else
  {
    EnterDist = &Distance2;
    ExitDist = &Distance1;
  }
}

template <typename FloatType>
void RayTracking<FloatType>::Init(const svtkm::Id size,
                                  svtkm::cont::ArrayHandle<FloatType>& distances)
{

  ExitFace.Allocate(size);
  Distance1.Allocate(size);
  Distance2.Allocate(size);

  CurrentDistance = distances;
  //
  // Set the initial Distances
  //
  svtkm::worklet::DispatcherMapField<CopyAndOffset<FloatType>> resetDistancesDispatcher(
    CopyAndOffset<FloatType>(0.0f));
  resetDistancesDispatcher.Invoke(distances, *EnterDist);

  //
  // Init the exit faces. This value is used to load the next cell
  // base on the cell and face it left
  //
  svtkm::cont::ArrayHandleConstant<svtkm::Int32> negOne(-1, size);
  svtkm::cont::Algorithm::Copy(negOne, ExitFace);

  svtkm::cont::ArrayHandleConstant<FloatType> negOnef(-1.f, size);
  svtkm::cont::Algorithm::Copy(negOnef, *ExitDist);
}

template <typename FloatType>
void RayTracking<FloatType>::Swap()
{
  svtkm::cont::ArrayHandle<FloatType>* tmpPtr;
  tmpPtr = EnterDist;
  EnterDist = ExitDist;
  ExitDist = tmpPtr;
}

} //namespace detail

void ConnectivityTracer::Init()
{
  //
  // Check to see if a sample distance was set
  //
  if (SampleDistance <= 0)
  {
    svtkm::Bounds coordsBounds = Coords.GetBounds();
    BoundingBox[0] = svtkm::Float32(coordsBounds.X.Min);
    BoundingBox[1] = svtkm::Float32(coordsBounds.X.Max);
    BoundingBox[2] = svtkm::Float32(coordsBounds.Y.Min);
    BoundingBox[3] = svtkm::Float32(coordsBounds.Y.Max);
    BoundingBox[4] = svtkm::Float32(coordsBounds.Z.Min);
    BoundingBox[5] = svtkm::Float32(coordsBounds.Z.Max);

    BackgroundColor[0] = 1.f;
    BackgroundColor[1] = 1.f;
    BackgroundColor[2] = 1.f;
    BackgroundColor[3] = 1.f;
    const svtkm::Float32 defaultSampleRate = 200.f;
    // We need to set some default sample distance
    svtkm::Vec3f_32 extent;
    extent[0] = BoundingBox[1] - BoundingBox[0];
    extent[1] = BoundingBox[3] - BoundingBox[2];
    extent[2] = BoundingBox[5] - BoundingBox[4];
    SampleDistance = svtkm::Magnitude(extent) / defaultSampleRate;
  }
}

svtkm::Id ConnectivityTracer::GetNumberOfMeshCells() const
{
  return CellSet.GetNumberOfCells();
}

void ConnectivityTracer::SetColorMap(const svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void ConnectivityTracer::SetVolumeData(const svtkm::cont::Field& scalarField,
                                       const svtkm::Range& scalarBounds,
                                       const svtkm::cont::DynamicCellSet& cellSet,
                                       const svtkm::cont::CoordinateSystem& coords)
{
  //TODO: Need a way to tell if we have been updated
  ScalarField = scalarField;
  ScalarBounds = scalarBounds;
  CellSet = cellSet;
  Coords = coords;
  MeshConnIsConstructed = false;

  const bool isSupportedField = ScalarField.IsFieldCell() || ScalarField.IsFieldPoint();
  if (!isSupportedField)
  {
    throw svtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
  }
  FieldAssocPoints = ScalarField.IsFieldPoint();

  this->Integrator = Volume;

  if (MeshContainer == nullptr)
  {
    delete MeshContainer;
  }
  MeshConnectivityBuilder builder;
  MeshContainer = builder.BuildConnectivity(cellSet, coords);
}

void ConnectivityTracer::SetEnergyData(const svtkm::cont::Field& absorption,
                                       const svtkm::Int32 numBins,
                                       const svtkm::cont::DynamicCellSet& cellSet,
                                       const svtkm::cont::CoordinateSystem& coords,
                                       const svtkm::cont::Field& emission)
{
  bool isSupportedField = absorption.GetAssociation() == svtkm::cont::Field::Association::CELL_SET;
  if (!isSupportedField)
    throw svtkm::cont::ErrorBadValue("Absorption Field '" + absorption.GetName() +
                                    "' not accociated with cells");
  ScalarField = absorption;
  CellSet = cellSet;
  Coords = coords;
  MeshConnIsConstructed = false;
  // Check for emission
  HasEmission = false;

  if (emission.GetAssociation() != svtkm::cont::Field::Association::ANY)
  {
    if (emission.GetAssociation() != svtkm::cont::Field::Association::CELL_SET)
      throw svtkm::cont::ErrorBadValue("Emission Field '" + emission.GetName() +
                                      "' not accociated with cells");
    HasEmission = true;
    EmissionField = emission;
  }
  // Do some basic range checking
  if (numBins < 1)
    throw svtkm::cont::ErrorBadValue("Number of energy bins is less than 1");
  svtkm::Id binCount = ScalarField.GetNumberOfValues();
  svtkm::Id cellCount = this->GetNumberOfMeshCells();
  if (cellCount != (binCount / svtkm::Id(numBins)))
  {
    std::stringstream message;
    message << "Invalid number of absorption bins\n";
    message << "Number of cells: " << cellCount << "\n";
    message << "Number of field values: " << binCount << "\n";
    message << "Number of bins: " << numBins << "\n";
    throw svtkm::cont::ErrorBadValue(message.str());
  }
  if (HasEmission)
  {
    binCount = EmissionField.GetNumberOfValues();
    if (cellCount != (binCount / svtkm::Id(numBins)))
    {
      std::stringstream message;
      message << "Invalid number of emission bins\n";
      message << "Number of cells: " << cellCount << "\n";
      message << "Number of field values: " << binCount << "\n";
      message << "Number of bins: " << numBins << "\n";
      throw svtkm::cont::ErrorBadValue(message.str());
    }
  }
  //TODO: Need a way to tell if we have been updated
  this->Integrator = Energy;

  if (MeshContainer == nullptr)
  {
    delete MeshContainer;
  }
  MeshConnectivityBuilder builder;
  MeshContainer = builder.BuildConnectivity(cellSet, coords);
}

void ConnectivityTracer::SetBackgroundColor(const svtkm::Vec4f_32& backgroundColor)
{
  BackgroundColor = backgroundColor;
}

void ConnectivityTracer::SetSampleDistance(const svtkm::Float32& distance)
{
  if (distance <= 0.f)
    throw svtkm::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}

void ConnectivityTracer::ResetTimers()
{
  IntersectTime = 0.;
  IntegrateTime = 0.;
  SampleTime = 0.;
  LostRayTime = 0.;
  MeshEntryTime = 0.;
}

void ConnectivityTracer::LogTimers()
{
  Logger* logger = Logger::GetInstance();
  logger->AddLogData("intersect ", IntersectTime);
  logger->AddLogData("integrate ", IntegrateTime);
  logger->AddLogData("sample_cells ", SampleTime);
  logger->AddLogData("lost_rays ", LostRayTime);
  logger->AddLogData("mesh_entry", LostRayTime);
}

template <typename FloatType>
void ConnectivityTracer::PrintRayStatus(Ray<FloatType>& rays)
{
  svtkm::Id raysExited = RayOperations::GetStatusCount(rays, RAY_EXITED_MESH);
  svtkm::Id raysActive = RayOperations::GetStatusCount(rays, RAY_ACTIVE);
  svtkm::Id raysAbandoned = RayOperations::GetStatusCount(rays, RAY_ABANDONED);
  svtkm::Id raysExitedDom = RayOperations::GetStatusCount(rays, RAY_EXITED_DOMAIN);
  std::cout << "\r Ray Status " << std::setw(10) << std::left << " Lost " << std::setw(10)
            << std::left << RaysLost << std::setw(10) << std::left << " Exited " << std::setw(10)
            << std::left << raysExited << std::setw(10) << std::left << " Active " << std::setw(10)
            << raysActive << std::setw(10) << std::left << " Abandoned " << std::setw(10)
            << raysAbandoned << " Exited Domain " << std::setw(10) << std::left << raysExitedDom
            << "\n";
}

//
//  Advance Ray
//      After a ray leaves the mesh, we need to check to see
//      of the ray re-enters the mesh within this domain. This
//      function moves the ray forward some offset to prevent
//      "shadowing" and hitting the same exit point.
//
template <typename FloatType>
class AdvanceRay : public svtkm::worklet::WorkletMapField
{
  FloatType Offset;

public:
  SVTKM_CONT
  AdvanceRay(const FloatType offset = 0.00001)
    : Offset(offset)
  {
  }
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  SVTKM_EXEC inline void operator()(const svtkm::UInt8& status, FloatType& distance) const
  {
    if (status == RAY_EXITED_MESH)
      distance += Offset;
  }
}; //class AdvanceRay

class LocateCell : public svtkm::worklet::WorkletMapField
{
private:
  CellIntersector<255> Intersector;

public:
  LocateCell() {}

  using ControlSignature = void(FieldInOut,
                                WholeArrayIn,
                                FieldIn,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldIn,
                                ExecObject meshConnectivity);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  template <typename FloatType, typename PointPortalType>
  SVTKM_EXEC inline void operator()(svtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   const svtkm::Vec<FloatType, 3>& dir,
                                   FloatType& enterDistance,
                                   FloatType& exitDistance,
                                   svtkm::Int32& enterFace,
                                   svtkm::UInt8& rayStatus,
                                   const svtkm::Vec<FloatType, 3>& origin,
                                   const MeshWrapper& meshConn) const
  {
    if (enterFace != -1 && rayStatus == RAY_ACTIVE)
    {
      currentCell = meshConn.GetConnectingCell(currentCell, enterFace);
      if (currentCell == -1)
        rayStatus = RAY_EXITED_MESH;
      enterFace = -1;
    }
    //This ray is dead or exited the mesh and needs re-entry
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    FloatType xpoints[8];
    FloatType ypoints[8];
    FloatType zpoints[8];
    svtkm::Id cellConn[8];
    FloatType distances[6];

    const svtkm::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
    //load local cell data
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(vertices, cellConn[i]);
      svtkm::Vec<FloatType, 3> point = svtkm::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
      xpoints[i] = point[0];
      ypoints[i] = point[1];
      zpoints[i] = point[2];
    }
    const svtkm::UInt8 cellShape = meshConn.GetCellShape(currentCell);
    Intersector.IntersectCell(xpoints, ypoints, zpoints, dir, origin, distances, cellShape);

    CellTables tables;
    const svtkm::Int32 numFaces = tables.FaceLookUp(tables.CellTypeLookUp(cellShape), 1);
    //svtkm::Int32 minFace = 6;
    svtkm::Int32 maxFace = -1;

    FloatType minDistance = static_cast<FloatType>(1e32);
    FloatType maxDistance = static_cast<FloatType>(-1);
    int hitCount = 0;
    for (svtkm::Int32 i = 0; i < numFaces; ++i)
    {
      FloatType dist = distances[i];

      if (dist != -1)
      {
        hitCount++;
        if (dist < minDistance)
        {
          minDistance = dist;
          //minFace = i;
        }
        if (dist > maxDistance)
        {
          maxDistance = dist;
          maxFace = i;
        }
      }
    }

    if (maxDistance <= enterDistance || minDistance == maxDistance)
    {
      rayStatus = RAY_LOST;
    }
    else
    {
      enterDistance = minDistance;
      exitDistance = maxDistance;
      enterFace = maxFace;
    }

  } //operator
};  //class LocateCell

class RayBumper : public svtkm::worklet::WorkletMapField
{
private:
  CellIntersector<255> Intersector;
  const svtkm::UInt8 FailureStatus; // the status to assign ray if we fail to find the intersection
public:
  RayBumper(svtkm::UInt8 failureStatus = RAY_ABANDONED)
    : FailureStatus(failureStatus)
  {
  }


  using ControlSignature = void(FieldInOut,
                                WholeArrayIn,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldIn,
                                FieldInOut,
                                ExecObject meshConnectivity);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  template <typename FloatType, typename PointPortalType>
  SVTKM_EXEC inline void operator()(svtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   FloatType& enterDistance,
                                   FloatType& exitDistance,
                                   svtkm::Int32& enterFace,
                                   svtkm::UInt8& rayStatus,
                                   const svtkm::Vec<FloatType, 3>& origin,
                                   svtkm::Vec<FloatType, 3>& rdir,
                                   const MeshWrapper& meshConn) const
  {
    // We only process lost rays
    if (rayStatus != RAY_LOST)
    {
      return;
    }

    FloatType xpoints[8];
    FloatType ypoints[8];
    FloatType zpoints[8];
    svtkm::Id cellConn[8];
    FloatType distances[6];

    svtkm::Vec<FloatType, 3> centroid(0., 0., 0.);

    const svtkm::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
    //load local cell data
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(vertices, cellConn[i]);
      svtkm::Vec<FloatType, 3> point = svtkm::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
      centroid = centroid + point;
      xpoints[i] = point[0];
      ypoints[i] = point[1];
      zpoints[i] = point[2];
    }

    FloatType invNumIndices = static_cast<FloatType>(1.) / static_cast<FloatType>(numIndices);
    centroid[0] = centroid[0] * invNumIndices;
    centroid[1] = centroid[1] * invNumIndices;
    centroid[2] = centroid[2] * invNumIndices;

    svtkm::Vec<FloatType, 3> toCentroid = centroid - origin;
    svtkm::Normalize(toCentroid);

    svtkm::Vec<FloatType, 3> dir = rdir;
    svtkm::Vec<FloatType, 3> bump = toCentroid - dir;
    dir = dir + RAY_TUG_EPSILON * bump;

    svtkm::Normalize(dir);
    rdir = dir;

    const svtkm::UInt8 cellShape = meshConn.GetCellShape(currentCell);
    Intersector.IntersectCell(xpoints, ypoints, zpoints, rdir, origin, distances, cellShape);

    CellTables tables;
    const svtkm::Int32 numFaces = tables.FaceLookUp(tables.CellTypeLookUp(cellShape), 1);

    //svtkm::Int32 minFace = 6;
    svtkm::Int32 maxFace = -1;
    FloatType minDistance = static_cast<FloatType>(1e32);
    FloatType maxDistance = static_cast<FloatType>(-1);
    int hitCount = 0;
    for (int i = 0; i < numFaces; ++i)
    {
      FloatType dist = distances[i];

      if (dist != -1)
      {
        hitCount++;
        if (dist < minDistance)
        {
          minDistance = dist;
          //minFace = i;
        }
        if (dist >= maxDistance)
        {
          maxDistance = dist;
          maxFace = i;
        }
      }
    }
    if (minDistance >= maxDistance)
    {
      rayStatus = FailureStatus;
    }
    else
    {
      enterDistance = minDistance;
      exitDistance = maxDistance;
      enterFace = maxFace;
      rayStatus = RAY_ACTIVE; //re-activate ray
    }

  } //operator
};  //class RayBumper

class AddPathLengths : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  AddPathLengths() {}

  using ControlSignature = void(FieldIn,     // ray status
                                FieldIn,     // cell enter distance
                                FieldIn,     // cell exit distance
                                FieldInOut); // ray absorption data

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename FloatType>
  SVTKM_EXEC inline void operator()(const svtkm::UInt8& rayStatus,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& distance) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }

    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;
    distance += segmentLength;
  }
};

class Integrate : public svtkm::worklet::WorkletMapField
{
private:
  const svtkm::Int32 NumBins;
  const svtkm::Float32 UnitScalar;

public:
  SVTKM_CONT
  Integrate(const svtkm::Int32 numBins, const svtkm::Float32 unitScalar)
    : NumBins(numBins)
    , UnitScalar(unitScalar)
  {
  }

  using ControlSignature = void(FieldIn,         // ray status
                                FieldIn,         // cell enter distance
                                FieldIn,         // cell exit distance
                                FieldInOut,      // current distance
                                WholeArrayIn,    // cell absorption data array
                                WholeArrayInOut, // ray absorption data
                                FieldIn);        // current cell

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, WorkIndex);

  template <typename FloatType, typename CellDataPortalType, typename RayDataPortalType>
  SVTKM_EXEC inline void operator()(const svtkm::UInt8& rayStatus,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   const CellDataPortalType& cellData,
                                   RayDataPortalType& energyBins,
                                   const svtkm::Id& currentCell,
                                   const svtkm::Id& rayIndex) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;

    svtkm::Id rayOffset = NumBins * rayIndex;
    svtkm::Id cellOffset = NumBins * currentCell;
    for (svtkm::Int32 i = 0; i < NumBins; ++i)
    {
      BOUNDS_CHECK(cellData, cellOffset + i);
      FloatType absorb = static_cast<FloatType>(cellData.Get(cellOffset + i));
      absorb *= UnitScalar;
      absorb = svtkm::Exp(-absorb * segmentLength);
      BOUNDS_CHECK(energyBins, rayOffset + i);
      FloatType intensity = static_cast<FloatType>(energyBins.Get(rayOffset + i));
      energyBins.Set(rayOffset + i, intensity * absorb);
    }
    currentDistance = exitDistance;
  }
};

class IntegrateEmission : public svtkm::worklet::WorkletMapField
{
private:
  const svtkm::Int32 NumBins;
  const svtkm::Float32 UnitScalar;
  bool DivideEmisByAbsorb;

public:
  SVTKM_CONT
  IntegrateEmission(const svtkm::Int32 numBins,
                    const svtkm::Float32 unitScalar,
                    const bool divideEmisByAbsorb)
    : NumBins(numBins)
    , UnitScalar(unitScalar)
    , DivideEmisByAbsorb(divideEmisByAbsorb)
  {
  }

  using ControlSignature = void(FieldIn,         // ray status
                                FieldIn,         // cell enter distance
                                FieldIn,         // cell exit distance
                                FieldInOut,      // current distance
                                WholeArrayIn,    // cell absorption data array
                                WholeArrayIn,    // cell emission data array
                                WholeArrayInOut, // ray absorption data
                                WholeArrayInOut, // ray emission data
                                FieldIn);        // current cell

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, WorkIndex);

  template <typename FloatType,
            typename CellAbsPortalType,
            typename CellEmisPortalType,
            typename RayDataPortalType>
  SVTKM_EXEC inline void operator()(const svtkm::UInt8& rayStatus,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   const CellAbsPortalType& absorptionData,
                                   const CellEmisPortalType& emissionData,
                                   RayDataPortalType& absorptionBins,
                                   RayDataPortalType& emissionBins,
                                   const svtkm::Id& currentCell,
                                   const svtkm::Id& rayIndex) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;

    svtkm::Id rayOffset = NumBins * rayIndex;
    svtkm::Id cellOffset = NumBins * currentCell;
    for (svtkm::Int32 i = 0; i < NumBins; ++i)
    {
      BOUNDS_CHECK(absorptionData, cellOffset + i);
      FloatType absorb = static_cast<FloatType>(absorptionData.Get(cellOffset + i));
      BOUNDS_CHECK(emissionData, cellOffset + i);
      FloatType emission = static_cast<FloatType>(emissionData.Get(cellOffset + i));

      absorb *= UnitScalar;
      emission *= UnitScalar;

      if (DivideEmisByAbsorb)
      {
        emission /= absorb;
      }

      FloatType tmp = svtkm::Exp(-absorb * segmentLength);
      BOUNDS_CHECK(absorptionBins, rayOffset + i);

      //
      // Traditionally, we would only keep track of a single intensity value per ray
      // per bin and we would integrate from the beginning to end of the ray. In a
      // distributed memory setting, we would move cell data around so that the
      // entire ray could be traced, but in situ, moving that much cell data around
      // could blow memory. Here we are keeping track of two values. Total absorption
      // through this contiguous segment of the mesh, and the amount of emitted energy
      // that makes it out of this mesh segment. If this is really run on a single node,
      // we can get the final energy value by multiplying the background intensity by
      // the total absorption of the mesh segment and add in the amount of emitted
      // energy that escapes.
      //
      FloatType absorbIntensity = static_cast<FloatType>(absorptionBins.Get(rayOffset + i));
      FloatType emissionIntensity = static_cast<FloatType>(emissionBins.Get(rayOffset + i));

      absorptionBins.Set(rayOffset + i, absorbIntensity * tmp);

      emissionIntensity = emissionIntensity * tmp + emission * (1.f - tmp);

      BOUNDS_CHECK(emissionBins, rayOffset + i);
      emissionBins.Set(rayOffset + i, emissionIntensity);
    }
    currentDistance = exitDistance;
  }
};
//
//  IdentifyMissedRay is a debugging routine that detects
//  rays that fail to have any value because of a external
//  intersection and cell intersection mismatch
//
//
class IdentifyMissedRay : public svtkm::worklet::WorkletMapField
{
public:
  svtkm::Id Width;
  svtkm::Id Height;
  svtkm::Vec4f_32 BGColor;
  IdentifyMissedRay(const svtkm::Id width, const svtkm::Id height, svtkm::Vec4f_32 bgcolor)
    : Width(width)
    , Height(height)
    , BGColor(bgcolor)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2);


  SVTKM_EXEC inline bool IsBGColor(const svtkm::Vec4f_32 color) const
  {
    bool isBG = false;

    if (color[0] == BGColor[0] && color[1] == BGColor[1] && color[2] == BGColor[2] &&
        color[3] == BGColor[3])
      isBG = true;
    return isBG;
  }

  template <typename ColorBufferType>
  SVTKM_EXEC inline void operator()(const svtkm::Id& pixelId, ColorBufferType& buffer) const
  {
    svtkm::Id x = pixelId % Width;
    svtkm::Id y = pixelId / Width;

    // Conservative check, we only want to check pixels in the middle
    if (x <= 0 || y <= 0)
      return;
    if (x >= Width - 1 || y >= Height - 1)
      return;
    svtkm::Vec4f_32 pixel;
    pixel[0] = static_cast<svtkm::Float32>(buffer.Get(pixelId * 4 + 0));
    pixel[1] = static_cast<svtkm::Float32>(buffer.Get(pixelId * 4 + 1));
    pixel[2] = static_cast<svtkm::Float32>(buffer.Get(pixelId * 4 + 2));
    pixel[3] = static_cast<svtkm::Float32>(buffer.Get(pixelId * 4 + 3));
    if (!IsBGColor(pixel))
      return;
    svtkm::Id p0 = (y)*Width + (x + 1);
    svtkm::Id p1 = (y)*Width + (x - 1);
    svtkm::Id p2 = (y + 1) * Width + (x);
    svtkm::Id p3 = (y - 1) * Width + (x);
    pixel[0] = static_cast<svtkm::Float32>(buffer.Get(p0 * 4 + 0));
    pixel[1] = static_cast<svtkm::Float32>(buffer.Get(p0 * 4 + 1));
    pixel[2] = static_cast<svtkm::Float32>(buffer.Get(p0 * 4 + 2));
    pixel[3] = static_cast<svtkm::Float32>(buffer.Get(p0 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<svtkm::Float32>(buffer.Get(p1 * 4 + 0));
    pixel[1] = static_cast<svtkm::Float32>(buffer.Get(p1 * 4 + 1));
    pixel[2] = static_cast<svtkm::Float32>(buffer.Get(p1 * 4 + 2));
    pixel[3] = static_cast<svtkm::Float32>(buffer.Get(p1 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<svtkm::Float32>(buffer.Get(p2 * 4 + 0));
    pixel[1] = static_cast<svtkm::Float32>(buffer.Get(p2 * 4 + 1));
    pixel[2] = static_cast<svtkm::Float32>(buffer.Get(p2 * 4 + 2));
    pixel[3] = static_cast<svtkm::Float32>(buffer.Get(p2 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<svtkm::Float32>(buffer.Get(p3 * 4 + 0));
    pixel[1] = static_cast<svtkm::Float32>(buffer.Get(p3 * 4 + 1));
    pixel[2] = static_cast<svtkm::Float32>(buffer.Get(p3 * 4 + 2));
    pixel[3] = static_cast<svtkm::Float32>(buffer.Get(p3 * 4 + 3));
    if (IsBGColor(pixel))
      return;

    printf("Possible error ray missed ray %d\n", (int)pixelId);
  }
};

template <typename FloatType>
class SampleCellAssocCells : public svtkm::worklet::WorkletMapField
{
private:
  CellSampler<255> Sampler;
  FloatType SampleDistance;
  FloatType MinScalar;
  FloatType InvDeltaScalar;

public:
  SampleCellAssocCells(const FloatType& sampleDistance,
                       const FloatType& minScalar,
                       const FloatType& maxScalar)
    : SampleDistance(sampleDistance)
    , MinScalar(minScalar)
  {
    InvDeltaScalar = (minScalar == maxScalar) ? 1.f : 1.f / (maxScalar - minScalar);
  }


  using ControlSignature = void(FieldIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldIn,
                                FieldInOut,
                                FieldInOut,
                                WholeArrayIn,
                                WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);

  template <typename ScalarPortalType, typename ColorMapType, typename FrameBufferType>
  SVTKM_EXEC inline void operator()(const svtkm::Id& currentCell,
                                   ScalarPortalType& scalarPortal,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   svtkm::UInt8& rayStatus,
                                   const ColorMapType& colorMap,
                                   FrameBufferType& frameBuffer,
                                   const svtkm::Id& pixelIndex) const
  {

    if (rayStatus != RAY_ACTIVE)
      return;

    svtkm::Vec4f_32 color;
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    color[0] = static_cast<svtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 0));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    color[1] = static_cast<svtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 1));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    color[2] = static_cast<svtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 2));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    color[3] = static_cast<svtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 3));

    svtkm::Float32 scalar;
    BOUNDS_CHECK(scalarPortal, currentCell);
    scalar = svtkm::Float32(scalarPortal.Get(currentCell));
    //
    // There can be mismatches in the initial enter distance and the current distance
    // due to lost rays at cell borders. For now,
    // we will just advance the current position to the enter distance, since otherwise,
    // the pixel would never be sampled.
    //
    if (currentDistance < enterDistance)
      currentDistance = enterDistance;

    const svtkm::Id colorMapSize = colorMap.GetNumberOfValues();
    svtkm::Float32 lerpedScalar;
    lerpedScalar = static_cast<svtkm::Float32>((scalar - MinScalar) * InvDeltaScalar);
    svtkm::Id colorIndex = svtkm::Id(lerpedScalar * svtkm::Float32(colorMapSize));
    if (colorIndex < 0)
      colorIndex = 0;
    if (colorIndex >= colorMapSize)
      colorIndex = colorMapSize - 1;
    BOUNDS_CHECK(colorMap, colorIndex);
    svtkm::Vec4f_32 sampleColor = colorMap.Get(colorIndex);

    while (enterDistance <= currentDistance && currentDistance <= exitDistance)
    {
      //composite
      svtkm::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];

      if (color[3] > 1.)
      {
        rayStatus = RAY_TERMINATED;
        break;
      }
      currentDistance += SampleDistance;
    }

    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    frameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    frameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    frameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    frameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class Sample cell

template <typename FloatType>
class SampleCellAssocPoints : public svtkm::worklet::WorkletMapField
{
private:
  CellSampler<255> Sampler;
  FloatType SampleDistance;
  FloatType MinScalar;
  FloatType InvDeltaScalar;

public:
  SampleCellAssocPoints(const FloatType& sampleDistance,
                        const FloatType& minScalar,
                        const FloatType& maxScalar)
    : SampleDistance(sampleDistance)
    , MinScalar(minScalar)
  {
    InvDeltaScalar = (minScalar == maxScalar) ? 1.f : 1.f / (maxScalar - minScalar);
  }


  using ControlSignature = void(FieldIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldIn,
                                FieldInOut,
                                FieldIn,
                                FieldInOut,
                                FieldIn,
                                ExecObject meshConnectivity,
                                WholeArrayIn,
                                WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex, _9, _10, _11, _12);

  template <typename PointPortalType,
            typename ScalarPortalType,
            typename ColorMapType,
            typename FrameBufferType>
  SVTKM_EXEC inline void operator()(const svtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   ScalarPortalType& scalarPortal,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   const svtkm::Vec3f_32& dir,
                                   svtkm::UInt8& rayStatus,
                                   const svtkm::Id& pixelIndex,
                                   const svtkm::Vec<FloatType, 3>& origin,
                                   MeshWrapper& meshConn,
                                   const ColorMapType& colorMap,
                                   FrameBufferType& frameBuffer) const
  {

    if (rayStatus != RAY_ACTIVE)
      return;

    svtkm::Vec4f_32 color;
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    color[0] = static_cast<svtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 0));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    color[1] = static_cast<svtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 1));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    color[2] = static_cast<svtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 2));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    color[3] = static_cast<svtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 3));

    if (color[3] >= 1.f)
    {
      rayStatus = RAY_TERMINATED;
      return;
    }
    svtkm::Vec<svtkm::Float32, 8> scalars;
    svtkm::Vec<svtkm::Vec<FloatType, 3>, 8> points;
    // silence "may" be uninitialized warning
    for (svtkm::Int32 i = 0; i < 8; ++i)
    {
      scalars[i] = 0.f;
      points[i] = svtkm::Vec<FloatType, 3>(0.f, 0.f, 0.f);
    }
    //load local scalar cell data
    svtkm::Id cellConn[8];
    const svtkm::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(scalarPortal, cellConn[i]);
      scalars[i] = static_cast<svtkm::Float32>(scalarPortal.Get(cellConn[i]));
      BOUNDS_CHECK(vertices, cellConn[i]);
      points[i] = svtkm::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
    }
    //
    // There can be mismatches in the initial enter distance and the current distance
    // due to lost rays at cell borders. For now,
    // we will just advance the current position to the enter distance, since otherwise,
    // the pixel would never be sampled.
    //
    if (currentDistance < enterDistance)
    {
      currentDistance = enterDistance;
    }

    const svtkm::Id colorMapSize = colorMap.GetNumberOfValues();
    const svtkm::Int32 cellShape = meshConn.GetCellShape(currentCell);

    while (enterDistance <= currentDistance && currentDistance <= exitDistance)
    {
      svtkm::Vec<FloatType, 3> sampleLoc = origin + currentDistance * dir;
      svtkm::Float32 lerpedScalar;
      bool validSample =
        Sampler.SampleCell(points, scalars, sampleLoc, lerpedScalar, *this, cellShape);
      if (!validSample)
      {
        //
        // There is a slight mismatch between intersections and parametric coordinates
        // which results in a invalid sample very close to the cell edge. Just throw
        // this sample away, and move to the next sample.
        //

        //There should be a sample here, so offset and try again.

        currentDistance += 0.00001f;
        continue;
      }
      lerpedScalar = static_cast<svtkm::Float32>((lerpedScalar - MinScalar) * InvDeltaScalar);
      svtkm::Id colorIndex = svtkm::Id(lerpedScalar * svtkm::Float32(colorMapSize));

      colorIndex = svtkm::Min(svtkm::Max(colorIndex, svtkm::Id(0)), colorMapSize - 1);
      BOUNDS_CHECK(colorMap, colorIndex);
      svtkm::Vec4f_32 sampleColor = colorMap.Get(colorIndex);
      //composite
      sampleColor[3] *= (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * sampleColor[3];
      color[1] = color[1] + sampleColor[1] * sampleColor[3];
      color[2] = color[2] + sampleColor[2] * sampleColor[3];
      color[3] = sampleColor[3] + color[3];

      if (color[3] >= 1.0)
      {
        rayStatus = RAY_TERMINATED;
        break;
      }
      currentDistance += SampleDistance;
    }

    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    frameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    frameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    frameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    frameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class Sample cell

template <typename FloatType>
void ConnectivityTracer::IntersectCell(Ray<FloatType>& rays,
                                       detail::RayTracking<FloatType>& tracker)
{
  svtkm::cont::Timer timer;
  timer.Start();
  svtkm::worklet::DispatcherMapField<LocateCell> locateDispatch;
  locateDispatch.Invoke(rays.HitIdx,
                        this->Coords,
                        rays.Dir,
                        *(tracker.EnterDist),
                        *(tracker.ExitDist),
                        tracker.ExitFace,
                        rays.Status,
                        rays.Origin,
                        MeshContainer);

  if (this->CountRayStatus)
    RaysLost = RayOperations::GetStatusCount(rays, RAY_LOST);
  this->IntersectTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::AccumulatePathLengths(Ray<FloatType>& rays,
                                               detail::RayTracking<FloatType>& tracker)
{
  svtkm::worklet::DispatcherMapField<AddPathLengths> dispatcher;
  dispatcher.Invoke(
    rays.Status, *(tracker.EnterDist), *(tracker.ExitDist), rays.GetBuffer("path_lengths").Buffer);
}

template <typename FloatType>
void ConnectivityTracer::FindLostRays(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker)
{
  svtkm::cont::Timer timer;
  timer.Start();

  svtkm::worklet::DispatcherMapField<RayBumper> bumpDispatch;
  bumpDispatch.Invoke(rays.HitIdx,
                      this->Coords,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.ExitFace,
                      rays.Status,
                      rays.Origin,
                      rays.Dir,
                      MeshContainer);

  this->LostRayTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::SampleCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker)
{
  using SampleP = SampleCellAssocPoints<FloatType>;
  using SampleC = SampleCellAssocCells<FloatType>;
  svtkm::cont::Timer timer;
  timer.Start();

  SVTKM_ASSERT(rays.Buffers.at(0).GetNumChannels() == 4);

  if (FieldAssocPoints)
  {
    svtkm::worklet::DispatcherMapField<SampleP> dispatcher(
      SampleP(this->SampleDistance,
              svtkm::Float32(this->ScalarBounds.Min),
              svtkm::Float32(this->ScalarBounds.Max)));
    dispatcher.Invoke(rays.HitIdx,
                      this->Coords,
                      this->ScalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.CurrentDistance,
                      rays.Dir,
                      rays.Status,
                      rays.Origin,
                      MeshContainer,
                      this->ColorMap,
                      rays.Buffers.at(0).Buffer);
  }
  else
  {
    svtkm::worklet::DispatcherMapField<SampleC> dispatcher(
      SampleC(this->SampleDistance,
              svtkm::Float32(this->ScalarBounds.Min),
              svtkm::Float32(this->ScalarBounds.Max)));

    dispatcher.Invoke(rays.HitIdx,
                      this->ScalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.CurrentDistance,
                      rays.Status,
                      this->ColorMap,
                      rays.Buffers.at(0).Buffer);
  }

  this->SampleTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::IntegrateCells(Ray<FloatType>& rays,
                                        detail::RayTracking<FloatType>& tracker)
{
  svtkm::cont::Timer timer;
  timer.Start();
  if (HasEmission)
  {
    bool divideEmisByAbsorp = false;
    svtkm::cont::ArrayHandle<FloatType> absorp = rays.Buffers.at(0).Buffer;
    svtkm::cont::ArrayHandle<FloatType> emission = rays.GetBuffer("emission").Buffer;
    svtkm::worklet::DispatcherMapField<IntegrateEmission> dispatcher(
      IntegrateEmission(rays.Buffers.at(0).GetNumChannels(), UnitScalar, divideEmisByAbsorp));
    dispatcher.Invoke(rays.Status,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      rays.Distance,
                      this->ScalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                      this->EmissionField.GetData().ResetTypes(ScalarRenderingTypes()),
                      absorp,
                      emission,
                      rays.HitIdx);
  }
  else
  {
    svtkm::worklet::DispatcherMapField<Integrate> dispatcher(
      Integrate(rays.Buffers.at(0).GetNumChannels(), UnitScalar));
    dispatcher.Invoke(rays.Status,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      rays.Distance,
                      this->ScalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                      rays.Buffers.at(0).Buffer,
                      rays.HitIdx);
  }

  IntegrateTime += timer.GetElapsedTime();
}

// template <typename FloatType>
// void ConnectivityTracer<CellType>::PrintDebugRay(Ray<FloatType>& rays, svtkm::Id rayId)
// {
//   svtkm::Id index = -1;
//   for (svtkm::Id i = 0; i < rays.NumRays; ++i)
//   {
//     if (rays.PixelIdx.GetPortalControl().Get(i) == rayId)
//     {
//       index = i;
//       break;
//     }
//   }
//   if (index == -1)
//   {
//     return;
//   }

//   std::cout << "++++++++RAY " << rayId << "++++++++\n";
//   std::cout << "Status: " << (int)rays.Status.GetPortalControl().Get(index) << "\n";
//   std::cout << "HitIndex: " << rays.HitIdx.GetPortalControl().Get(index) << "\n";
//   std::cout << "Dist " << rays.Distance.GetPortalControl().Get(index) << "\n";
//   std::cout << "MinDist " << rays.MinDistance.GetPortalControl().Get(index) << "\n";
//   std::cout << "Origin " << rays.Origin.GetPortalConstControl().Get(index) << "\n";
//   std::cout << "Dir " << rays.Dir.GetPortalConstControl().Get(index) << "\n";
//   std::cout << "+++++++++++++++++++++++++\n";
// }

template <typename FloatType>
void ConnectivityTracer::OffsetMinDistances(Ray<FloatType>& rays)
{
  svtkm::worklet::DispatcherMapField<AdvanceRay<FloatType>> dispatcher(
    AdvanceRay<FloatType>(FloatType(0.001)));
  dispatcher.Invoke(rays.Status, rays.MinDistance);
}

template <typename FloatType>
void ConnectivityTracer::FindMeshEntry(Ray<FloatType>& rays)
{
  svtkm::cont::Timer entryTimer;
  entryTimer.Start();
  //
  // if ray misses the external face it will be marked RAY_EXITED_MESH
  //
  MeshContainer->FindEntry(rays);
  MeshEntryTime += entryTimer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::IntegrateMeshSegment(Ray<FloatType>& rays)
{
  this->Init(); // sets sample distance
  detail::RayTracking<FloatType> rayTracker;
  rayTracker.Init(rays.NumRays, rays.Distance);

  bool hasPathLengths = rays.HasBuffer("path_lengths");

  if (this->Integrator == Volume)
  {
    svtkm::worklet::DispatcherMapField<detail::AdjustSample> adispatcher(SampleDistance);
    adispatcher.Invoke(rays.Status, rayTracker.CurrentDistance);
  }

  while (RayOperations::RaysInMesh(rays))
  {
    //
    // Rays the leave the mesh will be marked as RAYEXITED_MESH
    this->IntersectCell(rays, rayTracker);
    //
    // If the ray was lost due to precision issues, we find it.
    // If it is marked RAY_ABANDONED, then something went wrong.
    //
    this->FindLostRays(rays, rayTracker);
    //
    // integrate along the ray
    //
    if (this->Integrator == Volume)
      this->SampleCells(rays, rayTracker);
    else
      this->IntegrateCells(rays, rayTracker);

    if (hasPathLengths)
    {
      this->AccumulatePathLengths(rays, rayTracker);
    }
    //swap enter and exit distances
    rayTracker.Swap();
    if (this->CountRayStatus)
      this->PrintRayStatus(rays);
  } //for
}

template <typename FloatType>
void ConnectivityTracer::FullTrace(Ray<FloatType>& rays)
{

  this->RaysLost = 0;
  RayOperations::ResetStatus(rays, RAY_EXITED_MESH);

  if (this->CountRayStatus)
  {
    this->PrintRayStatus(rays);
  }

  bool cullMissedRays = true;
  bool workRemaining = true;

  do
  {
    FindMeshEntry(rays);

    if (cullMissedRays)
    {
      svtkm::cont::ArrayHandle<UInt8> activeRays;
      activeRays = RayOperations::CompactActiveRays(rays);
      cullMissedRays = false;
    }

    IntegrateMeshSegment(rays);

    workRemaining = RayOperations::RaysProcessed(rays) != rays.NumRays;
    //
    // Ensure that we move the current distance forward some
    // epsilon so we don't re-enter the cell we just left.
    //
    if (workRemaining)
    {
      RayOperations::CopyDistancesToMin(rays);
      this->OffsetMinDistances(rays);
    }
  } while (workRemaining);
}

template <typename FloatType>
std::vector<PartialComposite<FloatType>> ConnectivityTracer::PartialTrace(Ray<FloatType>& rays)
{

  bool hasPathLengths = rays.HasBuffer("path_lengths");
  this->RaysLost = 0;
  RayOperations::ResetStatus(rays, RAY_EXITED_MESH);

  std::vector<PartialComposite<FloatType>> partials;

  if (this->CountRayStatus)
  {
    this->PrintRayStatus(rays);
  }

  bool workRemaining = true;

  do
  {
    FindMeshEntry(rays);

    svtkm::cont::ArrayHandle<UInt8> activeRays;
    activeRays = RayOperations::CompactActiveRays(rays);

    if (rays.NumRays == 0)
      break;

    IntegrateMeshSegment(rays);

    PartialComposite<FloatType> partial;
    partial.Buffer = rays.Buffers.at(0).Copy();
    svtkm::cont::Algorithm::Copy(rays.Distance, partial.Distances);
    svtkm::cont::Algorithm::Copy(rays.PixelIdx, partial.PixelIds);

    if (HasEmission && this->Integrator == Energy)
    {
      partial.Intensities = rays.GetBuffer("emission").Copy();
    }
    if (hasPathLengths)
    {
      partial.PathLengths = rays.GetBuffer("path_lengths").Copy().Buffer;
    }
    partials.push_back(partial);

    // reset buffers
    if (this->Integrator == Volume)
    {
      svtkm::cont::ArrayHandle<FloatType> signature;
      signature.Allocate(4);
      signature.GetPortalControl().Set(0, 0.f);
      signature.GetPortalControl().Set(1, 0.f);
      signature.GetPortalControl().Set(2, 0.f);
      signature.GetPortalControl().Set(3, 0.f);
      rays.Buffers.at(0).InitChannels(signature);
    }
    else
    {
      rays.Buffers.at(0).InitConst(1.f);
      if (HasEmission)
      {
        rays.GetBuffer("emission").InitConst(0.f);
      }
      if (hasPathLengths)
      {
        rays.GetBuffer("path_lengths").InitConst(0.f);
      }
    }

    workRemaining = RayOperations::RaysProcessed(rays) != rays.NumRays;
    //
    // Ensure that we move the current distance forward some
    // epsilon so we don't re-enter the cell we just left.
    //
    if (workRemaining)
    {
      RayOperations::CopyDistancesToMin(rays);
      this->OffsetMinDistances(rays);
    }
  } while (workRemaining);

  return partials;
}

template class detail::RayTracking<svtkm::Float32>;
template class detail::RayTracking<svtkm::Float64>;

template struct PartialComposite<svtkm::Float32>;
template struct PartialComposite<svtkm::Float64>;

template void ConnectivityTracer::FullTrace<svtkm::Float32>(Ray<svtkm::Float32>& rays);

template std::vector<PartialComposite<svtkm::Float32>>
ConnectivityTracer::PartialTrace<svtkm::Float32>(Ray<svtkm::Float32>& rays);

template void ConnectivityTracer::IntegrateMeshSegment<svtkm::Float32>(Ray<svtkm::Float32>& rays);

template void ConnectivityTracer::FindMeshEntry<svtkm::Float32>(Ray<svtkm::Float32>& rays);

template void ConnectivityTracer::FullTrace<svtkm::Float64>(Ray<svtkm::Float64>& rays);

template std::vector<PartialComposite<svtkm::Float64>>
ConnectivityTracer::PartialTrace<svtkm::Float64>(Ray<svtkm::Float64>& rays);

template void ConnectivityTracer::IntegrateMeshSegment<svtkm::Float64>(Ray<svtkm::Float64>& rays);

template void ConnectivityTracer::FindMeshEntry<svtkm::Float64>(Ray<svtkm::Float64>& rays);
}
}
} // namespace svtkm::rendering::raytracing
