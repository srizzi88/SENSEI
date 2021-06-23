//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_Clip_h
#define svtkm_m_worklet_Clip_h

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/DispatcherReduceByKey.h>
#include <svtkm/worklet/Keys.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>
#include <svtkm/worklet/WorkletReduceByKey.h>
#include <svtkm/worklet/clip/ClipTables.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleView.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ImplicitFunctionHandle.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/VariantArrayHandle.h>

#include <utility>
#include <svtkm/exec/FunctorBase.h>

#if defined(THRUST_MAJOR_VERSION) && THRUST_MAJOR_VERSION == 1 && THRUST_MINOR_VERSION == 8 &&     \
  THRUST_SUBMINOR_VERSION < 3
// Workaround a bug in thrust 1.8.0 - 1.8.2 scan implementations which produces
// wrong results
#include <svtkm/exec/cuda/internal/ThrustPatches.h>
SVTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/detail/type_traits.h>
SVTKM_THIRDPARTY_POST_INCLUDE
#define THRUST_SCAN_WORKAROUND
#endif

namespace svtkm
{
namespace worklet
{
struct ClipStats
{
  svtkm::Id NumberOfCells = 0;
  svtkm::Id NumberOfIndices = 0;
  svtkm::Id NumberOfEdgeIndices = 0;

  // Stats for interpolating new points within cell.
  svtkm::Id NumberOfInCellPoints = 0;
  svtkm::Id NumberOfInCellIndices = 0;
  svtkm::Id NumberOfInCellInterpPoints = 0;
  svtkm::Id NumberOfInCellEdgeIndices = 0;

  struct SumOp
  {
    SVTKM_EXEC_CONT
    ClipStats operator()(const ClipStats& stat1, const ClipStats& stat2) const
    {
      ClipStats sum = stat1;
      sum.NumberOfCells += stat2.NumberOfCells;
      sum.NumberOfIndices += stat2.NumberOfIndices;
      sum.NumberOfEdgeIndices += stat2.NumberOfEdgeIndices;
      sum.NumberOfInCellPoints += stat2.NumberOfInCellPoints;
      sum.NumberOfInCellIndices += stat2.NumberOfInCellIndices;
      sum.NumberOfInCellInterpPoints += stat2.NumberOfInCellInterpPoints;
      sum.NumberOfInCellEdgeIndices += stat2.NumberOfInCellEdgeIndices;
      return sum;
    }
  };
};

struct EdgeInterpolation
{
  svtkm::Id Vertex1 = -1;
  svtkm::Id Vertex2 = -1;
  svtkm::Float64 Weight = 0;

  struct LessThanOp
  {
    SVTKM_EXEC
    bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
    {
      return (v1.Vertex1 < v2.Vertex1) || (v1.Vertex1 == v2.Vertex1 && v1.Vertex2 < v2.Vertex2);
    }
  };

  struct EqualToOp
  {
    SVTKM_EXEC
    bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
    {
      return v1.Vertex1 == v2.Vertex1 && v1.Vertex2 == v2.Vertex2;
    }
  };
};

namespace internal
{

template <typename T>
SVTKM_EXEC_CONT T Scale(const T& val, svtkm::Float64 scale)
{
  return static_cast<T>(scale * static_cast<svtkm::Float64>(val));
}

template <typename T, svtkm::IdComponent NumComponents>
SVTKM_EXEC_CONT svtkm::Vec<T, NumComponents> Scale(const svtkm::Vec<T, NumComponents>& val,
                                                 svtkm::Float64 scale)
{
  return val * scale;
}

template <typename Device>
class ExecutionConnectivityExplicit
{
private:
  using UInt8Portal =
    typename svtkm::cont::ArrayHandle<svtkm::UInt8>::template ExecutionTypes<Device>::Portal;
  using IdComponentPortal =
    typename svtkm::cont::ArrayHandle<svtkm::IdComponent>::template ExecutionTypes<Device>::Portal;
  using IdPortal =
    typename svtkm::cont::ArrayHandle<svtkm::Id>::template ExecutionTypes<Device>::Portal;

public:
  SVTKM_CONT
  ExecutionConnectivityExplicit() = default;

  SVTKM_CONT
  ExecutionConnectivityExplicit(svtkm::cont::ArrayHandle<svtkm::UInt8> shapes,
                                svtkm::cont::ArrayHandle<svtkm::IdComponent> numberOfIndices,
                                svtkm::cont::ArrayHandle<svtkm::Id> connectivity,
                                svtkm::cont::ArrayHandle<svtkm::Id> offsets,
                                ClipStats stats)
    : Shapes(shapes.PrepareForOutput(stats.NumberOfCells, Device()))
    , NumberOfIndices(numberOfIndices.PrepareForOutput(stats.NumberOfCells, Device()))
    , Connectivity(connectivity.PrepareForOutput(stats.NumberOfIndices, Device()))
    , Offsets(offsets.PrepareForOutput(stats.NumberOfCells, Device()))
  {
  }

  SVTKM_EXEC
  void SetCellShape(svtkm::Id cellIndex, svtkm::UInt8 shape) { this->Shapes.Set(cellIndex, shape); }

  SVTKM_EXEC
  void SetNumberOfIndices(svtkm::Id cellIndex, svtkm::IdComponent numIndices)
  {
    this->NumberOfIndices.Set(cellIndex, numIndices);
  }

  SVTKM_EXEC
  void SetIndexOffset(svtkm::Id cellIndex, svtkm::Id indexOffset)
  {
    this->Offsets.Set(cellIndex, indexOffset);
  }

  SVTKM_EXEC
  void SetConnectivity(svtkm::Id connectivityIndex, svtkm::Id pointIndex)
  {
    this->Connectivity.Set(connectivityIndex, pointIndex);
  }

private:
  UInt8Portal Shapes;
  IdComponentPortal NumberOfIndices;
  IdPortal Connectivity;
  IdPortal Offsets;
};

class ConnectivityExplicit : svtkm::cont::ExecutionObjectBase
{
public:
  SVTKM_CONT
  ConnectivityExplicit() = default;

  SVTKM_CONT
  ConnectivityExplicit(const svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
                       const svtkm::cont::ArrayHandle<svtkm::IdComponent>& numberOfIndices,
                       const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
                       const svtkm::cont::ArrayHandle<svtkm::Id>& offsets,
                       const ClipStats& stats)
    : Shapes(shapes)
    , NumberOfIndices(numberOfIndices)
    , Connectivity(connectivity)
    , Offsets(offsets)
    , Stats(stats)
  {
  }

  template <typename Device>
  SVTKM_CONT ExecutionConnectivityExplicit<Device> PrepareForExecution(Device) const
  {
    ExecutionConnectivityExplicit<Device> execConnectivity(
      this->Shapes, this->NumberOfIndices, this->Connectivity, this->Offsets, this->Stats);
    return execConnectivity;
  }

private:
  svtkm::cont::ArrayHandle<svtkm::UInt8> Shapes;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> NumberOfIndices;
  svtkm::cont::ArrayHandle<svtkm::Id> Connectivity;
  svtkm::cont::ArrayHandle<svtkm::Id> Offsets;
  svtkm::worklet::ClipStats Stats;
};


} // namespace internal

class Clip
{
  // Add support for invert
public:
  using TypeClipStats = svtkm::List<ClipStats>;

  using TypeEdgeInterp = svtkm::List<EdgeInterpolation>;

  class ComputeStats : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    SVTKM_CONT
    ComputeStats(svtkm::Float64 value, bool invert)
      : Value(value)
      , Invert(invert)
    {
    }

    using ControlSignature =
      void(CellSetIn, FieldInPoint, ExecObject clippingData, FieldOutCell, FieldOutCell);

    using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4, _5);

    using InputDomain = _1;

    template <typename CellShapeTag, typename ScalarFieldVec, typename DeviceAdapter>
    SVTKM_EXEC void operator()(const CellShapeTag shape,
                              const svtkm::IdComponent pointCount,
                              const ScalarFieldVec& scalars,
                              const internal::ClipTables::DevicePortal<DeviceAdapter>& clippingData,
                              ClipStats& clipStat,
                              svtkm::Id& clipDataIndex) const
    {
      (void)shape; // C4100 false positive workaround
      svtkm::Id caseId = 0;
      for (svtkm::IdComponent iter = pointCount - 1; iter >= 0; iter--)
      {
        if (!this->Invert && static_cast<svtkm::Float64>(scalars[iter]) <= this->Value)
        {
          caseId++;
        }
        else if (this->Invert && static_cast<svtkm::Float64>(scalars[iter]) >= this->Value)
        {
          caseId++;
        }
        if (iter > 0)
          caseId *= 2;
      }
      svtkm::Id index = clippingData.GetCaseIndex(shape.Id, caseId);
      clipDataIndex = index;
      svtkm::Id numberOfCells = clippingData.ValueAt(index++);
      clipStat.NumberOfCells = numberOfCells;
      for (svtkm::IdComponent shapes = 0; shapes < numberOfCells; shapes++)
      {
        svtkm::Id cellShape = clippingData.ValueAt(index++);
        svtkm::Id numberOfIndices = clippingData.ValueAt(index++);
        if (cellShape == 0)
        {
          --clipStat.NumberOfCells;
          // Shape is 0, which is a case of interpolating new point within a cell
          // Gather stats for later operation.
          clipStat.NumberOfInCellPoints = 1;
          clipStat.NumberOfInCellInterpPoints = numberOfIndices;
          for (svtkm::IdComponent points = 0; points < numberOfIndices; points++, index++)
          {
            //Find how many points need to be calculated using edge interpolation.
            svtkm::Id element = clippingData.ValueAt(index);
            clipStat.NumberOfInCellEdgeIndices += (element < 100) ? 1 : 0;
          }
        }
        else
        {
          // Collect number of indices required for storing current shape
          clipStat.NumberOfIndices += numberOfIndices;
          // Collect number of new points
          for (svtkm::IdComponent points = 0; points < numberOfIndices; points++, index++)
          {
            //Find how many points need to found using edge interpolation.
            svtkm::Id element = clippingData.ValueAt(index);
            if (element == 255)
            {
              clipStat.NumberOfInCellIndices++;
            }
            else if (element < 100)
            {
              clipStat.NumberOfEdgeIndices++;
            }
          }
        }
      }
    }

  private:
    svtkm::Float64 Value;
    bool Invert;
  };

  class GenerateCellSet : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    SVTKM_CONT
    GenerateCellSet(svtkm::Float64 value)
      : Value(value)
    {
    }

    using ControlSignature = void(CellSetIn,
                                  FieldInPoint,
                                  FieldInCell clipTableIndices,
                                  FieldInCell clipStats,
                                  ExecObject clipTables,
                                  ExecObject connectivityObject,
                                  WholeArrayOut edgePointReverseConnectivity,
                                  WholeArrayOut edgePointInterpolation,
                                  WholeArrayOut inCellReverseConnectivity,
                                  WholeArrayOut inCellEdgeReverseConnectivity,
                                  WholeArrayOut inCellEdgeInterpolation,
                                  WholeArrayOut inCellInterpolationKeys,
                                  WholeArrayOut inCellInterpolationInfo,
                                  WholeArrayOut cellMapOutputToInput);

    using ExecutionSignature = void(CellShape,
                                    WorkIndex,
                                    PointIndices,
                                    _2,
                                    _3,
                                    _4,
                                    _5,
                                    _6,
                                    _7,
                                    _8,
                                    _9,
                                    _10,
                                    _11,
                                    _12,
                                    _13,
                                    _14);

    template <typename CellShapeTag,
              typename PointVecType,
              typename ScalarVecType,
              typename ConnectivityObject,
              typename IdArrayType,
              typename EdgeInterpolationPortalType,
              typename DeviceAdapter>
    SVTKM_EXEC void operator()(const CellShapeTag shape,
                              const svtkm::Id workIndex,
                              const PointVecType points,
                              const ScalarVecType scalars,
                              const svtkm::Id clipDataIndex,
                              const ClipStats clipStats,
                              const internal::ClipTables::DevicePortal<DeviceAdapter>& clippingData,
                              ConnectivityObject& connectivityObject,
                              IdArrayType& edgePointReverseConnectivity,
                              EdgeInterpolationPortalType& edgePointInterpolation,
                              IdArrayType& inCellReverseConnectivity,
                              IdArrayType& inCellEdgeReverseConnectivity,
                              EdgeInterpolationPortalType& inCellEdgeInterpolation,
                              IdArrayType& inCellInterpolationKeys,
                              IdArrayType& inCellInterpolationInfo,
                              IdArrayType& cellMapOutputToInput) const
    {
      (void)shape;
      svtkm::Id clipIndex = clipDataIndex;
      // Start index for the cells of this case.
      svtkm::Id cellIndex = clipStats.NumberOfCells;
      // Start index to store connevtivity of this case.
      svtkm::Id connectivityIndex = clipStats.NumberOfIndices;
      // Start indices for reverse mapping into connectivity for this case.
      svtkm::Id edgeIndex = clipStats.NumberOfEdgeIndices;
      svtkm::Id inCellIndex = clipStats.NumberOfInCellIndices;
      svtkm::Id inCellPoints = clipStats.NumberOfInCellPoints;
      // Start Indices to keep track of interpolation points for new cell.
      svtkm::Id inCellInterpPointIndex = clipStats.NumberOfInCellInterpPoints;
      svtkm::Id inCellEdgeInterpIndex = clipStats.NumberOfInCellEdgeIndices;

      // Iterate over the shapes for the current cell and begin to fill connectivity.
      svtkm::Id numberOfCells = clippingData.ValueAt(clipIndex++);
      for (svtkm::Id cell = 0; cell < numberOfCells; ++cell)
      {
        svtkm::UInt8 cellShape = clippingData.ValueAt(clipIndex++);
        svtkm::IdComponent numberOfPoints = clippingData.ValueAt(clipIndex++);
        if (cellShape == 0)
        {
          // Case for a new cell point.

          // 1. Output the input cell id for which we need to generate new point.
          // 2. Output number of points used for interpolation.
          // 3. If vertex
          //    - Add vertex to connectivity interpolation information.
          // 4. If edge
          //    - Add edge interpolation information for new points.
          //    - Reverse connectivity map for new points.
          // Make an array which has all the elements that need to be used
          // for interpolation.
          for (svtkm::IdComponent point = 0; point < numberOfPoints;
               point++, inCellInterpPointIndex++, clipIndex++)
          {
            svtkm::IdComponent entry =
              static_cast<svtkm::IdComponent>(clippingData.ValueAt(clipIndex));
            inCellInterpolationKeys.Set(inCellInterpPointIndex, workIndex);
            if (entry >= 100)
            {
              inCellInterpolationInfo.Set(inCellInterpPointIndex, points[entry - 100]);
            }
            else
            {
              internal::ClipTables::EdgeVec edge = clippingData.GetEdge(shape.Id, entry);
              SVTKM_ASSERT(edge[0] != 255);
              SVTKM_ASSERT(edge[1] != 255);
              EdgeInterpolation ei;
              ei.Vertex1 = points[edge[0]];
              ei.Vertex2 = points[edge[1]];
              // For consistency purposes keep the points ordered.
              if (ei.Vertex1 > ei.Vertex2)
              {
                this->swap(ei.Vertex1, ei.Vertex2);
                this->swap(edge[0], edge[1]);
              }
              ei.Weight = (static_cast<svtkm::Float64>(scalars[edge[0]]) - this->Value) /
                static_cast<svtkm::Float64>(scalars[edge[1]] - scalars[edge[0]]);

              inCellEdgeReverseConnectivity.Set(inCellEdgeInterpIndex, inCellInterpPointIndex);
              inCellEdgeInterpolation.Set(inCellEdgeInterpIndex, ei);
              inCellEdgeInterpIndex++;
            }
          }
        }
        else
        {
          // Just a normal cell, generate edge representations,

          // 1. Add cell type to connectivity information.
          // 2. If vertex
          //    - Add vertex to connectivity information.
          // 3. If edge point
          //    - Add edge to edge points
          //    - Add edge point index to edge point reverse connectivity.
          // 4. If cell point
          //    - Add cell point index to connectivity
          //      (as there is only one cell point per required cell)
          // 5. Store input cell index against current cell for mapping cell data.
          connectivityObject.SetCellShape(cellIndex, cellShape);
          connectivityObject.SetNumberOfIndices(cellIndex, numberOfPoints);
          connectivityObject.SetIndexOffset(cellIndex, connectivityIndex);
          for (svtkm::IdComponent point = 0; point < numberOfPoints; point++, clipIndex++)
          {
            svtkm::IdComponent entry =
              static_cast<svtkm::IdComponent>(clippingData.ValueAt(clipIndex));
            if (entry == 255) // case of cell point interpolation
            {
              // Add index of the corresponding cell point.
              inCellReverseConnectivity.Set(inCellIndex++, connectivityIndex);
              connectivityObject.SetConnectivity(connectivityIndex, inCellPoints);
              connectivityIndex++;
            }
            else if (entry >= 100) // existing vertex
            {
              connectivityObject.SetConnectivity(connectivityIndex++, points[entry - 100]);
            }
            else // case of a new edge point
            {
              internal::ClipTables::EdgeVec edge = clippingData.GetEdge(shape.Id, entry);
              SVTKM_ASSERT(edge[0] != 255);
              SVTKM_ASSERT(edge[1] != 255);
              EdgeInterpolation ei;
              ei.Vertex1 = points[edge[0]];
              ei.Vertex2 = points[edge[1]];
              // For consistency purposes keep the points ordered.
              if (ei.Vertex1 > ei.Vertex2)
              {
                this->swap(ei.Vertex1, ei.Vertex2);
                this->swap(edge[0], edge[1]);
              }
              ei.Weight = (static_cast<svtkm::Float64>(scalars[edge[0]]) - this->Value) /
                static_cast<svtkm::Float64>(scalars[edge[1]] - scalars[edge[0]]);
              //Add to set of new edge points
              //Add reverse connectivity;
              edgePointReverseConnectivity.Set(edgeIndex, connectivityIndex++);
              edgePointInterpolation.Set(edgeIndex, ei);
              edgeIndex++;
            }
          }
          cellMapOutputToInput.Set(cellIndex, workIndex);
          ++cellIndex;
        }
      }
    }

    template <typename T>
    SVTKM_EXEC void swap(T& v1, T& v2) const
    {
      T temp = v1;
      v1 = v2;
      v2 = temp;
    }

  private:
    svtkm::Float64 Value;
  };

  class ScatterEdgeConnectivity : public svtkm::worklet::WorkletMapField
  {
  public:
    SVTKM_CONT
    ScatterEdgeConnectivity(svtkm::Id edgePointOffset)
      : EdgePointOffset(edgePointOffset)
    {
    }

    using ControlSignature = void(FieldIn sourceValue,
                                  FieldIn destinationIndices,
                                  WholeArrayOut destinationData);

    using ExecutionSignature = void(_1, _2, _3);

    using InputDomain = _1;

    template <typename ConnectivityDataType>
    SVTKM_EXEC void operator()(const svtkm::Id sourceValue,
                              const svtkm::Id destinationIndex,
                              ConnectivityDataType& destinationData) const
    {
      destinationData.Set(destinationIndex, (sourceValue + EdgePointOffset));
    }

  private:
    svtkm::Id EdgePointOffset;
  };

  class ScatterInCellConnectivity : public svtkm::worklet::WorkletMapField
  {
  public:
    SVTKM_CONT
    ScatterInCellConnectivity(svtkm::Id inCellPointOffset)
      : InCellPointOffset(inCellPointOffset)
    {
    }

    using ControlSignature = void(FieldIn destinationIndices, WholeArrayOut destinationData);

    using ExecutionSignature = void(_1, _2);

    using InputDomain = _1;

    template <typename ConnectivityDataType>
    SVTKM_EXEC void operator()(const svtkm::Id destinationIndex,
                              ConnectivityDataType& destinationData) const
    {
      auto sourceValue = destinationData.Get(destinationIndex);
      destinationData.Set(destinationIndex, (sourceValue + InCellPointOffset));
    }

  private:
    svtkm::Id InCellPointOffset;
  };

  Clip()
    : ClipTablesInstance()
    , EdgePointsInterpolation()
    , InCellInterpolationKeys()
    , InCellInterpolationInfo()
    , CellMapOutputToInput()
    , EdgePointsOffset()
    , InCellPointsOffset()
  {
  }

  template <typename CellSetList, typename ScalarsArrayHandle>
  svtkm::cont::CellSetExplicit<> Run(const svtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
                                    const ScalarsArrayHandle& scalars,
                                    svtkm::Float64 value,
                                    bool invert)
  {
    // Create the required output fields.
    svtkm::cont::ArrayHandle<ClipStats> clipStats;
    svtkm::cont::ArrayHandle<svtkm::Id> clipTableIndices;

    ComputeStats statsWorklet(value, invert);
    //Send this CellSet to process
    svtkm::worklet::DispatcherMapTopology<ComputeStats> statsDispatcher(statsWorklet);
    statsDispatcher.Invoke(cellSet, scalars, this->ClipTablesInstance, clipStats, clipTableIndices);

    ClipStats zero;
    svtkm::cont::ArrayHandle<ClipStats> cellSetStats;
    ClipStats total =
      svtkm::cont::Algorithm::ScanExclusive(clipStats, cellSetStats, ClipStats::SumOp(), zero);
    clipStats.ReleaseResources();

    svtkm::cont::ArrayHandle<svtkm::UInt8> shapes;
    svtkm::cont::ArrayHandle<svtkm::IdComponent> numberOfIndices;
    svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
    svtkm::cont::ArrayHandle<svtkm::Id> offsets;
    internal::ConnectivityExplicit connectivityObject(
      shapes, numberOfIndices, connectivity, offsets, total);

    //Begin Process of Constructing the new CellSet.
    svtkm::cont::ArrayHandle<svtkm::Id> edgePointReverseConnectivity;
    edgePointReverseConnectivity.Allocate(total.NumberOfEdgeIndices);
    svtkm::cont::ArrayHandle<EdgeInterpolation> edgeInterpolation;
    edgeInterpolation.Allocate(total.NumberOfEdgeIndices);

    svtkm::cont::ArrayHandle<svtkm::Id> cellPointReverseConnectivity;
    cellPointReverseConnectivity.Allocate(total.NumberOfInCellIndices);
    svtkm::cont::ArrayHandle<svtkm::Id> cellPointEdgeReverseConnectivity;
    cellPointEdgeReverseConnectivity.Allocate(total.NumberOfInCellEdgeIndices);
    svtkm::cont::ArrayHandle<EdgeInterpolation> cellPointEdgeInterpolation;
    cellPointEdgeInterpolation.Allocate(total.NumberOfInCellEdgeIndices);

    this->InCellInterpolationKeys.Allocate(total.NumberOfInCellInterpPoints);
    this->InCellInterpolationInfo.Allocate(total.NumberOfInCellInterpPoints);
    this->CellMapOutputToInput.Allocate(total.NumberOfCells);

    GenerateCellSet cellSetWorklet(value);
    //Send this CellSet to process
    svtkm::worklet::DispatcherMapTopology<GenerateCellSet> cellSetDispatcher(cellSetWorklet);
    cellSetDispatcher.Invoke(cellSet,
                             scalars,
                             clipTableIndices,
                             cellSetStats,
                             this->ClipTablesInstance,
                             connectivityObject,
                             edgePointReverseConnectivity,
                             edgeInterpolation,
                             cellPointReverseConnectivity,
                             cellPointEdgeReverseConnectivity,
                             cellPointEdgeInterpolation,
                             this->InCellInterpolationKeys,
                             this->InCellInterpolationInfo,
                             this->CellMapOutputToInput);

    // Get unique EdgeInterpolation : unique edge points.
    // LowerBound for edgeInterpolation : get index into new edge points array.
    // LowerBound for cellPointEdgeInterpolation : get index into new edge points array.
    svtkm::cont::Algorithm::SortByKey(
      edgeInterpolation, edgePointReverseConnectivity, EdgeInterpolation::LessThanOp());
    svtkm::cont::Algorithm::Copy(edgeInterpolation, this->EdgePointsInterpolation);
    svtkm::cont::Algorithm::Unique(this->EdgePointsInterpolation, EdgeInterpolation::EqualToOp());

    svtkm::cont::ArrayHandle<svtkm::Id> edgeInterpolationIndexToUnique;
    svtkm::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                       edgeInterpolation,
                                       edgeInterpolationIndexToUnique,
                                       EdgeInterpolation::LessThanOp());

    svtkm::cont::ArrayHandle<svtkm::Id> cellInterpolationIndexToUnique;
    svtkm::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                       cellPointEdgeInterpolation,
                                       cellInterpolationIndexToUnique,
                                       EdgeInterpolation::LessThanOp());

    this->EdgePointsOffset = scalars.GetNumberOfValues();
    this->InCellPointsOffset =
      this->EdgePointsOffset + this->EdgePointsInterpolation.GetNumberOfValues();

    // Scatter these values into the connectivity array,
    // scatter indices are given in reverse connectivity.
    ScatterEdgeConnectivity scatterEdgePointConnectivity(this->EdgePointsOffset);
    svtkm::worklet::DispatcherMapField<ScatterEdgeConnectivity> scatterEdgeDispatcher(
      scatterEdgePointConnectivity);
    scatterEdgeDispatcher.Invoke(
      edgeInterpolationIndexToUnique, edgePointReverseConnectivity, connectivity);
    scatterEdgeDispatcher.Invoke(cellInterpolationIndexToUnique,
                                 cellPointEdgeReverseConnectivity,
                                 this->InCellInterpolationInfo);
    // Add offset in connectivity of all new in-cell points.
    ScatterInCellConnectivity scatterInCellPointConnectivity(this->InCellPointsOffset);
    svtkm::worklet::DispatcherMapField<ScatterInCellConnectivity> scatterInCellDispatcher(
      scatterInCellPointConnectivity);
    scatterInCellDispatcher.Invoke(cellPointReverseConnectivity, connectivity);

    svtkm::cont::CellSetExplicit<> output;
    svtkm::Id numberOfPoints = scalars.GetNumberOfValues() +
      this->EdgePointsInterpolation.GetNumberOfValues() + total.NumberOfInCellPoints;

    svtkm::cont::ConvertNumIndicesToOffsets(numberOfIndices, offsets);

    output.Fill(numberOfPoints, shapes, connectivity, offsets);
    return output;
  }

  template <typename DynamicCellSet>
  class ClipWithImplicitFunction
  {
  public:
    SVTKM_CONT
    ClipWithImplicitFunction(Clip* clipper,
                             const DynamicCellSet& cellSet,
                             const svtkm::cont::ImplicitFunctionHandle& function,
                             const bool invert,
                             svtkm::cont::CellSetExplicit<>* result)
      : Clipper(clipper)
      , CellSet(&cellSet)
      , Function(function)
      , Invert(invert)
      , Result(result)
    {
    }

    template <typename ArrayHandleType>
    SVTKM_CONT void operator()(const ArrayHandleType& handle) const
    {
      // Evaluate the implicit function on the input coordinates using
      // ArrayHandleTransform
      svtkm::cont::ArrayHandleTransform<ArrayHandleType, svtkm::cont::ImplicitFunctionValueHandle>
        clipScalars(handle, this->Function);

      // Clip at locations where the implicit function evaluates to 0
      *this->Result = this->Clipper->Run(*this->CellSet, clipScalars, 0.0, this->Invert);
    }

  private:
    Clip* Clipper;
    const DynamicCellSet* CellSet;
    svtkm::cont::ImplicitFunctionHandle Function;
    bool Invert;
    svtkm::cont::CellSetExplicit<>* Result;
  };

  template <typename CellSetList>
  svtkm::cont::CellSetExplicit<> Run(const svtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
                                    const svtkm::cont::ImplicitFunctionHandle& clipFunction,
                                    const svtkm::cont::CoordinateSystem& coords,
                                    const bool invert)
  {
    svtkm::cont::CellSetExplicit<> output;

    ClipWithImplicitFunction<svtkm::cont::DynamicCellSetBase<CellSetList>> clip(
      this, cellSet, clipFunction, invert, &output);

    CastAndCall(coords, clip);
    return output;
  }

  template <typename ArrayHandleType>
  class InterpolateField
  {
  public:
    using ValueType = typename ArrayHandleType::ValueType;
    using TypeMappedValue = svtkm::List<ValueType>;

    InterpolateField(svtkm::cont::ArrayHandle<EdgeInterpolation> edgeInterpolationArray,
                     svtkm::cont::ArrayHandle<svtkm::Id> inCellInterpolationKeys,
                     svtkm::cont::ArrayHandle<svtkm::Id> inCellInterpolationInfo,
                     svtkm::Id edgePointsOffset,
                     svtkm::Id inCellPointsOffset,
                     ArrayHandleType* output)
      : EdgeInterpolationArray(edgeInterpolationArray)
      , InCellInterpolationKeys(inCellInterpolationKeys)
      , InCellInterpolationInfo(inCellInterpolationInfo)
      , EdgePointsOffset(edgePointsOffset)
      , InCellPointsOffset(inCellPointsOffset)
      , Output(output)
    {
    }

    class PerformEdgeInterpolations : public svtkm::worklet::WorkletMapField
    {
    public:
      PerformEdgeInterpolations(svtkm::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }

      using ControlSignature = void(FieldIn edgeInterpolations, WholeArrayInOut outputField);

      using ExecutionSignature = void(_1, _2, WorkIndex);

      template <typename EdgeInterp, typename OutputFieldPortal>
      SVTKM_EXEC void operator()(const EdgeInterp& ei,
                                OutputFieldPortal& field,
                                const svtkm::Id workIndex) const
      {
        using T = typename OutputFieldPortal::ValueType;
        T v1 = field.Get(ei.Vertex1);
        T v2 = field.Get(ei.Vertex2);
        field.Set(this->EdgePointsOffset + workIndex,
                  static_cast<T>(internal::Scale(T(v1 - v2), ei.Weight) + v1));
      }

    private:
      svtkm::Id EdgePointsOffset;
    };

    class PerformInCellInterpolations : public svtkm::worklet::WorkletReduceByKey
    {
    public:
      using ControlSignature = void(KeysIn keys, ValuesIn toReduce, ReducedValuesOut centroid);

      using ExecutionSignature = void(_2, _3);

      template <typename MappedValueVecType, typename MappedValueType>
      SVTKM_EXEC void operator()(const MappedValueVecType& toReduce, MappedValueType& centroid) const
      {
        svtkm::IdComponent numValues = toReduce.GetNumberOfComponents();
        MappedValueType sum = toReduce[0];
        for (svtkm::IdComponent i = 1; i < numValues; i++)
        {
          MappedValueType value = toReduce[i];
          // static_cast is for when MappedValueType is a small int that gets promoted to int32.
          sum = static_cast<MappedValueType>(sum + value);
        }
        centroid = internal::Scale(sum, 1. / static_cast<svtkm::Float64>(numValues));
      }
    };

    template <typename Storage>
    SVTKM_CONT void operator()(const svtkm::cont::ArrayHandle<ValueType, Storage>& field) const
    {
      svtkm::worklet::Keys<svtkm::Id> interpolationKeys(InCellInterpolationKeys);

      svtkm::Id numberOfOriginalValues = field.GetNumberOfValues();
      svtkm::Id numberOfEdgePoints = EdgeInterpolationArray.GetNumberOfValues();
      svtkm::Id numberOfInCellPoints = interpolationKeys.GetUniqueKeys().GetNumberOfValues();

      ArrayHandleType result;
      result.Allocate(numberOfOriginalValues + numberOfEdgePoints + numberOfInCellPoints);
      svtkm::cont::Algorithm::CopySubRange(field, 0, numberOfOriginalValues, result);

      PerformEdgeInterpolations edgeInterpWorklet(numberOfOriginalValues);
      svtkm::worklet::DispatcherMapField<PerformEdgeInterpolations> edgeInterpDispatcher(
        edgeInterpWorklet);
      edgeInterpDispatcher.Invoke(this->EdgeInterpolationArray, result);

      // Perform a gather on output to get all required values for calculation of
      // centroids using the interpolation info array.
      using IdHandle = svtkm::cont::ArrayHandle<svtkm::Id>;
      using ValueHandle = svtkm::cont::ArrayHandle<ValueType>;
      svtkm::cont::ArrayHandlePermutation<IdHandle, ValueHandle> toReduceValues(
        InCellInterpolationInfo, result);

      svtkm::cont::ArrayHandle<ValueType> reducedValues;
      svtkm::worklet::DispatcherReduceByKey<PerformInCellInterpolations>
        inCellInterpolationDispatcher;
      inCellInterpolationDispatcher.Invoke(interpolationKeys, toReduceValues, reducedValues);
      svtkm::Id inCellPointsOffset = numberOfOriginalValues + numberOfEdgePoints;
      svtkm::cont::Algorithm::CopySubRange(
        reducedValues, 0, reducedValues.GetNumberOfValues(), result, inCellPointsOffset);
      *(this->Output) = result;
    }

  private:
    svtkm::cont::ArrayHandle<EdgeInterpolation> EdgeInterpolationArray;
    svtkm::cont::ArrayHandle<svtkm::Id> InCellInterpolationKeys;
    svtkm::cont::ArrayHandle<svtkm::Id> InCellInterpolationInfo;
    svtkm::Id EdgePointsOffset;
    svtkm::Id InCellPointsOffset;
    ArrayHandleType* Output;
  };

  template <typename ValueType, typename StorageType>
  svtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& fieldData) const
  {
    using ResultType = svtkm::cont::ArrayHandle<ValueType>;
    using Worker = InterpolateField<ResultType>;

    ResultType output;

    Worker worker = Worker(this->EdgePointsInterpolation,
                           this->InCellInterpolationKeys,
                           this->InCellInterpolationInfo,
                           this->EdgePointsOffset,
                           this->InCellPointsOffset,
                           &output);
    worker(fieldData);

    return output;
  }

  template <typename ValueType, typename StorageType>
  svtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& fieldData) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = svtkm::cont::make_ArrayHandlePermutation(this->CellMapOutputToInput, fieldData);

    // Copy into an array with default storage:
    svtkm::cont::ArrayHandle<ValueType> result;
    svtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  internal::ClipTables ClipTablesInstance;
  svtkm::cont::ArrayHandle<EdgeInterpolation> EdgePointsInterpolation;
  svtkm::cont::ArrayHandle<svtkm::Id> InCellInterpolationKeys;
  svtkm::cont::ArrayHandle<svtkm::Id> InCellInterpolationInfo;
  svtkm::cont::ArrayHandle<svtkm::Id> CellMapOutputToInput;
  svtkm::Id EdgePointsOffset;
  svtkm::Id InCellPointsOffset;
};
}
} // namespace svtkm::worklet

#if defined(THRUST_SCAN_WORKAROUND)
namespace thrust
{
namespace detail
{

// causes a different code path which does not have the bug
template <>
struct is_integral<svtkm::worklet::ClipStats> : public true_type
{
};
}
} // namespace thrust::detail
#endif

#endif // svtkm_m_worklet_Clip_h
