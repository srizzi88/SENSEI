//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_ConnectivityTracer_h
#define svtk_m_rendering_raytracing_ConnectivityTracer_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/rendering/raytracing/MeshConnectivityContainers.h>
#include <svtkm/rendering/raytracing/PartialComposite.h>


namespace svtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

//forward declare so we can be friends
struct RenderFunctor;

//
//  Ray tracker manages memory and pointer
//  swapping for current cell intersection data
//
template <typename FloatType>
class RayTracking
{
public:
  svtkm::cont::ArrayHandle<svtkm::Int32> ExitFace;
  svtkm::cont::ArrayHandle<FloatType> CurrentDistance;
  svtkm::cont::ArrayHandle<FloatType> Distance1;
  svtkm::cont::ArrayHandle<FloatType> Distance2;
  svtkm::cont::ArrayHandle<FloatType>* EnterDist;
  svtkm::cont::ArrayHandle<FloatType>* ExitDist;

  RayTracking()
  {
    EnterDist = &Distance1;
    ExitDist = &Distance2;
  }

  void Compact(svtkm::cont::ArrayHandle<FloatType>& compactedDistances,
               svtkm::cont::ArrayHandle<UInt8>& masks);

  void Init(const svtkm::Id size, svtkm::cont::ArrayHandle<FloatType>& distances);

  void Swap();
};

} //namespace detail

/**
 * \brief ConnectivityTracer is volumetric ray tracer for unstructured
 *        grids. Capabilities include volume rendering and integrating
 *        absorption and emission of N energy groups for simulated
 *        radiograhy.
 */
class SVTKM_RENDERING_EXPORT ConnectivityTracer
{
public:
  ConnectivityTracer()
    : MeshContainer(nullptr)
    , CountRayStatus(false)
    , UnitScalar(1.f)
  {
  }

  ~ConnectivityTracer()
  {
    if (MeshContainer != nullptr)
    {
      delete MeshContainer;
    }
  }

  enum IntegrationMode
  {
    Volume,
    Energy
  };

  void SetVolumeData(const svtkm::cont::Field& scalarField,
                     const svtkm::Range& scalarBounds,
                     const svtkm::cont::DynamicCellSet& cellSet,
                     const svtkm::cont::CoordinateSystem& coords);

  void SetEnergyData(const svtkm::cont::Field& absorption,
                     const svtkm::Int32 numBins,
                     const svtkm::cont::DynamicCellSet& cellSet,
                     const svtkm::cont::CoordinateSystem& coords,
                     const svtkm::cont::Field& emission);

  void SetBackgroundColor(const svtkm::Vec4f_32& backgroundColor);
  void SetSampleDistance(const svtkm::Float32& distance);
  void SetColorMap(const svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colorMap);

  MeshConnContainer* GetMeshContainer() { return MeshContainer; }

  void Init();

  void SetDebugOn(bool on) { CountRayStatus = on; }

  void SetUnitScalar(const svtkm::Float32 unitScalar) { UnitScalar = unitScalar; }


  svtkm::Id GetNumberOfMeshCells() const;

  void ResetTimers();
  void LogTimers();

  ///
  /// Traces rays fully through the mesh. Rays can exit and re-enter
  /// multiple times before leaving the domain. This is fast path for
  /// structured meshs or meshes that are not interlocking.
  /// Note: rays will be compacted
  ///
  template <typename FloatType>
  void FullTrace(Ray<FloatType>& rays);

  ///
  /// Integrates rays through the mesh. If rays leave the mesh and
  /// re-enter, then those become two separate partial composites.
  /// This is need to support domain decompositions that are like
  /// puzzle pieces. Note: rays will be compacted
  ///
  template <typename FloatType>
  std::vector<PartialComposite<FloatType>> PartialTrace(Ray<FloatType>& rays);

  ///
  /// Integrates the active rays though the mesh until all rays
  /// have exited.
  ///  Precondition: rays.HitIdx is set to a valid mesh cell
  ///
  template <typename FloatType>
  void IntegrateMeshSegment(Ray<FloatType>& rays);

  ///
  /// Find the entry point in the mesh
  ///
  template <typename FloatType>
  void FindMeshEntry(Ray<FloatType>& rays);

private:
  template <typename FloatType>
  void IntersectCell(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void AccumulatePathLengths(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void FindLostRays(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void SampleCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void IntegrateCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void OffsetMinDistances(Ray<FloatType>& rays);

  template <typename FloatType>
  void PrintRayStatus(Ray<FloatType>& rays);

protected:
  // Data set info
  svtkm::cont::Field ScalarField;
  svtkm::cont::Field EmissionField;
  svtkm::cont::DynamicCellSet CellSet;
  svtkm::cont::CoordinateSystem Coords;
  svtkm::Range ScalarBounds;
  svtkm::Float32 BoundingBox[6];

  svtkm::cont::ArrayHandle<svtkm::Vec4f_32> ColorMap;

  svtkm::Vec4f_32 BackgroundColor;
  svtkm::Float32 SampleDistance;
  svtkm::Id RaysLost;
  IntegrationMode Integrator;

  MeshConnContainer* MeshContainer;
  //
  // flags
  bool CountRayStatus;
  bool MeshConnIsConstructed;
  bool DebugFiltersOn;
  bool ReEnterMesh; // Do not try to re-enter the mesh
  bool CreatePartialComposites;
  bool FieldAssocPoints;
  bool HasEmission; // Mode for integrating through energy bins

  // timers
  svtkm::Float64 IntersectTime;
  svtkm::Float64 IntegrateTime;
  svtkm::Float64 SampleTime;
  svtkm::Float64 LostRayTime;
  svtkm::Float64 MeshEntryTime;
  svtkm::Float32 UnitScalar;

}; // class ConnectivityTracer<CellType,ConnectivityType>
}
}
} // namespace svtkm::rendering::raytracing
#endif
