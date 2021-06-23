//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_VertexClustering_h
#define svtk_m_worklet_VertexClustering_h

#include <svtkm/BinaryOperators.h>
#include <svtkm/BinaryPredicates.h>
#include <svtkm/Types.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleDiscard.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Logging.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/DispatcherReduceByKey.h>
#include <svtkm/worklet/Keys.h>
#include <svtkm/worklet/StableSortIndices.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>
#include <svtkm/worklet/WorkletReduceByKey.h>

//#define __SVTKM_VERTEX_CLUSTERING_BENCHMARK

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
#include <svtkm/cont/Timer.h>
#endif

namespace svtkm
{
namespace worklet
{

namespace internal
{

/// Selects the representative point somewhat randomly from the pool of points
/// in a cluster.
struct SelectRepresentativePoint : public svtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn clusterIds, ValuesIn points, ReducedValuesOut repPoints);
  using ExecutionSignature = _3(_2);
  using InputDomain = _1;

  template <typename PointsInVecType>
  SVTKM_EXEC typename PointsInVecType::ComponentType operator()(
    const PointsInVecType& pointsIn) const
  {
    // Grab the point from the middle of the set. This usually does a decent
    // job of selecting a representative point that won't emphasize the cluster
    // partitions.
    //
    // Note that we must use the stable sorting with the worklet::Keys for this
    // to be reproducible across backends.
    return pointsIn[pointsIn.GetNumberOfComponents() / 2];
  }

  struct RunTrampoline
  {
    template <typename InputPointsArrayType, typename KeyType>
    SVTKM_CONT void operator()(const InputPointsArrayType& points,
                              const svtkm::worklet::Keys<KeyType>& keys,
                              svtkm::cont::VariantArrayHandle& output) const
    {

      svtkm::cont::ArrayHandle<typename InputPointsArrayType::ValueType> out;
      svtkm::worklet::DispatcherReduceByKey<SelectRepresentativePoint> dispatcher;
      dispatcher.Invoke(keys, points, out);

      output = out;
    }
  };

  template <typename KeyType, typename InputDynamicPointsArrayType>
  SVTKM_CONT static svtkm::cont::VariantArrayHandle Run(
    const svtkm::worklet::Keys<KeyType>& keys,
    const InputDynamicPointsArrayType& inputPoints)
  {
    svtkm::cont::VariantArrayHandle output;
    RunTrampoline trampoline;
    svtkm::cont::CastAndCall(inputPoints, trampoline, keys, output);
    return output;
  }
};

template <typename ValueType, typename StorageType, typename IndexArrayType>
SVTKM_CONT svtkm::cont::ArrayHandle<ValueType> ConcretePermutationArray(
  const IndexArrayType& indices,
  const svtkm::cont::ArrayHandle<ValueType, StorageType>& values)
{
  svtkm::cont::ArrayHandle<ValueType> result;
  auto tmp = svtkm::cont::make_ArrayHandlePermutation(indices, values);
  svtkm::cont::ArrayCopy(tmp, result);
  return result;
}

template <typename T, svtkm::IdComponent N>
svtkm::cont::ArrayHandle<T> copyFromVec(svtkm::cont::ArrayHandle<svtkm::Vec<T, N>> const& other)
{
  const T* vmem = reinterpret_cast<const T*>(&*other.GetPortalConstControl().GetIteratorBegin());
  svtkm::cont::ArrayHandle<T> mem =
    svtkm::cont::make_ArrayHandle(vmem, other.GetNumberOfValues() * N);
  svtkm::cont::ArrayHandle<T> result;
  svtkm::cont::ArrayCopy(mem, result);
  return result;
}

} // namespace internal

struct VertexClustering
{
  using PointIdMapType = svtkm::cont::ArrayHandlePermutation<svtkm::cont::ArrayHandle<svtkm::Id>,
                                                            svtkm::cont::ArrayHandle<svtkm::Id>>;

  struct GridInfo
  {
    svtkm::Id3 dim;
    svtkm::Vec3f_64 origin;
    svtkm::Vec3f_64 bin_size;
    svtkm::Vec3f_64 inv_bin_size;
  };

  // input: points  output: cid of the points
  class MapPointsWorklet : public svtkm::worklet::WorkletMapField
  {
  private:
    GridInfo Grid;

  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

    SVTKM_CONT
    MapPointsWorklet(const GridInfo& grid)
      : Grid(grid)
    {
    }

    /// determine grid resolution for clustering
    template <typename PointType>
    SVTKM_EXEC svtkm::Id GetClusterId(const PointType& p) const
    {
      using ComponentType = typename PointType::ComponentType;
      PointType gridOrigin(static_cast<ComponentType>(this->Grid.origin[0]),
                           static_cast<ComponentType>(this->Grid.origin[1]),
                           static_cast<ComponentType>(this->Grid.origin[2]));

      PointType p_rel = (p - gridOrigin) * this->Grid.inv_bin_size;

      svtkm::Id x = svtkm::Min(static_cast<svtkm::Id>(p_rel[0]), this->Grid.dim[0] - 1);
      svtkm::Id y = svtkm::Min(static_cast<svtkm::Id>(p_rel[1]), this->Grid.dim[1] - 1);
      svtkm::Id z = svtkm::Min(static_cast<svtkm::Id>(p_rel[2]), this->Grid.dim[2] - 1);

      return x + this->Grid.dim[0] * (y + this->Grid.dim[1] * z); // get a unique hash value
    }

    template <typename PointType>
    SVTKM_EXEC void operator()(const PointType& point, svtkm::Id& cid) const
    {
      cid = this->GetClusterId(point);
      SVTKM_ASSERT(cid >= 0); // the id could overflow if too many cells
    }
  };

  class MapCellsWorklet : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint pointClusterIds,
                                  FieldOutCell cellClusterIds);
    using ExecutionSignature = void(_2, _3);

    SVTKM_CONT
    MapCellsWorklet() {}

    // TODO: Currently only works with Triangle cell types
    template <typename ClusterIdsVecType>
    SVTKM_EXEC void operator()(const ClusterIdsVecType& pointClusterIds,
                              svtkm::Id3& cellClusterId) const
    {
      cellClusterId[0] = pointClusterIds[0];
      cellClusterId[1] = pointClusterIds[1];
      cellClusterId[2] = pointClusterIds[2];
    }
  };

  /// pass 3
  class IndexingWorklet : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, WholeArrayOut);
    using ExecutionSignature = void(WorkIndex, _1, _2); // WorkIndex: use svtkm indexing

    template <typename OutPortalType>
    SVTKM_EXEC void operator()(const svtkm::Id& counter,
                              const svtkm::Id& cid,
                              const OutPortalType& outPortal) const
    {
      outPortal.Set(cid, counter);
    }
  };

  class Cid2PointIdWorklet : public svtkm::worklet::WorkletMapField
  {
    svtkm::Id NPoints;

    SVTKM_EXEC
    void rotate(svtkm::Id3& ids) const
    {
      svtkm::Id temp = ids[0];
      ids[0] = ids[1];
      ids[1] = ids[2];
      ids[2] = temp;
    }

  public:
    using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);
    using ExecutionSignature = void(_1, _2, _3);

    SVTKM_CONT
    Cid2PointIdWorklet(svtkm::Id nPoints)
      : NPoints(nPoints)
    {
    }

    template <typename InPortalType>
    SVTKM_EXEC void operator()(const svtkm::Id3& cid3,
                              svtkm::Id3& pointId3,
                              const InPortalType& inPortal) const
    {
      if (cid3[0] == cid3[1] || cid3[0] == cid3[2] || cid3[1] == cid3[2])
      {
        pointId3[0] = pointId3[1] = pointId3[2] = this->NPoints; // invalid cell to be removed
      }
      else
      {
        pointId3[0] = inPortal.Get(cid3[0]);
        pointId3[1] = inPortal.Get(cid3[1]);
        pointId3[2] = inPortal.Get(cid3[2]);

        // Sort triangle point ids so that the same triangle will have the same signature
        // Rotate these ids making the first one the smallest
        if (pointId3[0] > pointId3[1] || pointId3[0] > pointId3[2])
        {
          rotate(pointId3);
          if (pointId3[0] > pointId3[1] || pointId3[0] > pointId3[2])
          {
            rotate(pointId3);
          }
        }
      }
    }
  };

  using TypeInt64 = svtkm::List<svtkm::Int64>;

  class Cid3HashWorklet : public svtkm::worklet::WorkletMapField
  {
  private:
    svtkm::Int64 NPoints;

  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

    SVTKM_CONT
    Cid3HashWorklet(svtkm::Id nPoints)
      : NPoints(nPoints)
    {
    }

    SVTKM_EXEC
    void operator()(const svtkm::Id3& cid, svtkm::Int64& cidHash) const
    {
      cidHash =
        cid[0] + this->NPoints * (cid[1] + this->NPoints * cid[2]); // get a unique hash value
    }
  };

  class Cid3UnhashWorklet : public svtkm::worklet::WorkletMapField
  {
  private:
    svtkm::Int64 NPoints;

  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

    SVTKM_CONT
    Cid3UnhashWorklet(svtkm::Id nPoints)
      : NPoints(nPoints)
    {
    }

    SVTKM_EXEC
    void operator()(const svtkm::Int64& cidHash, svtkm::Id3& cid) const
    {
      cid[0] = static_cast<svtkm::Id>(cidHash % this->NPoints);
      svtkm::Int64 t = cidHash / this->NPoints;
      cid[1] = static_cast<svtkm::Id>(t % this->NPoints);
      cid[2] = static_cast<svtkm::Id>(t / this->NPoints);
    }
  };

public:
  ///////////////////////////////////////////////////
  /// \brief VertexClustering: Mesh simplification
  ///
  template <typename DynamicCellSetType, typename DynamicCoordinateHandleType>
  svtkm::cont::DataSet Run(const DynamicCellSetType& cellSet,
                          const DynamicCoordinateHandleType& coordinates,
                          const svtkm::Bounds& bounds,
                          const svtkm::Id3& nDivisions)
  {
    SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf, "VertexClustering Worklet");

    /// determine grid resolution for clustering
    GridInfo gridInfo;
    {
      gridInfo.origin[0] = bounds.X.Min;
      gridInfo.origin[1] = bounds.Y.Min;
      gridInfo.origin[2] = bounds.Z.Min;
      gridInfo.dim[0] = nDivisions[0];
      gridInfo.dim[1] = nDivisions[1];
      gridInfo.dim[2] = nDivisions[2];
      gridInfo.bin_size[0] = bounds.X.Length() / static_cast<svtkm::Float64>(nDivisions[0]);
      gridInfo.bin_size[1] = bounds.Y.Length() / static_cast<svtkm::Float64>(nDivisions[1]);
      gridInfo.bin_size[2] = bounds.Z.Length() / static_cast<svtkm::Float64>(nDivisions[2]);
      gridInfo.inv_bin_size[0] = 1. / gridInfo.bin_size[0];
      gridInfo.inv_bin_size[1] = 1. / gridInfo.bin_size[1];
      gridInfo.inv_bin_size[2] = 1. / gridInfo.bin_size[2];
    }

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
    svtkm::cont::Timer totalTimer;
    totalTimer.Start();
    svtkm::cont::Timer timer;
    timer.Start();
#endif

    //////////////////////////////////////////////
    /// start algorithm

    /// pass 1 : assign points with (cluster) ids based on the grid it falls in
    ///
    /// map points
    svtkm::cont::ArrayHandle<svtkm::Id> pointCidArray;

    svtkm::worklet::DispatcherMapField<MapPointsWorklet> mapPointsDispatcher(
      (MapPointsWorklet(gridInfo)));
    mapPointsDispatcher.Invoke(coordinates, pointCidArray);

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
    timer.stop();
    std::cout << "Time map points (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// pass 2 : Choose a representative point from each cluster for the output:
    svtkm::cont::VariantArrayHandle repPointArray;
    {
      svtkm::worklet::Keys<svtkm::Id> keys;
      keys.BuildArrays(pointCidArray, svtkm::worklet::KeysSortType::Stable);

      // For mapping properties, this map will select an arbitrary point from
      // the cluster:
      this->PointIdMap =
        svtkm::cont::make_ArrayHandlePermutation(keys.GetOffsets(), keys.GetSortedValuesMap());

      // Compute representative points from each cluster (may not match the
      // PointIdMap indexing)
      repPointArray = internal::SelectRepresentativePoint::Run(keys, coordinates);
    }

    auto repPointCidArray =
      svtkm::cont::make_ArrayHandlePermutation(this->PointIdMap, pointCidArray);

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after reducing points (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// Pass 3 : Decimated mesh generation
    ///          For each original triangle, only output vertices from
    ///          three different clusters

    /// map each triangle vertex to the cluster id's
    /// of the cell vertices
    svtkm::cont::ArrayHandle<svtkm::Id3> cid3Array;

    svtkm::worklet::DispatcherMapTopology<MapCellsWorklet> mapCellsDispatcher;
    mapCellsDispatcher.Invoke(cellSet, pointCidArray, cid3Array);

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after clustering cells (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// preparation: Get the indexes of the clustered points to prepare for new cell array
    svtkm::cont::ArrayHandle<svtkm::Id> cidIndexArray;
    cidIndexArray.Allocate(gridInfo.dim[0] * gridInfo.dim[1] * gridInfo.dim[2]);

    svtkm::worklet::DispatcherMapField<IndexingWorklet> indexingDispatcher;
    indexingDispatcher.Invoke(repPointCidArray, cidIndexArray);

    pointCidArray.ReleaseResources();
    repPointCidArray.ReleaseResources();

    ///
    /// map: convert each triangle vertices from original point id to the new cluster indexes
    ///      If the triangle is degenerated, set the ids to <nPoints, nPoints, nPoints>
    ///      This ensures it will be placed at the end of the array when sorted.
    ///
    svtkm::Id nPoints = repPointArray.GetNumberOfValues();

    svtkm::cont::ArrayHandle<svtkm::Id3> pointId3Array;

    svtkm::worklet::DispatcherMapField<Cid2PointIdWorklet> cid2PointIdDispatcher(
      (Cid2PointIdWorklet(nPoints)));
    cid2PointIdDispatcher.Invoke(cid3Array, pointId3Array, cidIndexArray);

    cid3Array.ReleaseResources();
    cidIndexArray.ReleaseResources();

    bool doHashing = (nPoints < (1 << 21)); // Check whether we can hash Id3 into 64-bit integers

    if (doHashing)
    {
      /// Create hashed array
      svtkm::cont::ArrayHandle<svtkm::Int64> pointId3HashArray;

      svtkm::worklet::DispatcherMapField<Cid3HashWorklet> cid3HashDispatcher(
        (Cid3HashWorklet(nPoints)));
      cid3HashDispatcher.Invoke(pointId3Array, pointId3HashArray);

      pointId3Array.ReleaseResources();

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      this->CellIdMap = svtkm::worklet::StableSortIndices::Sort(pointId3HashArray);
      svtkm::worklet::StableSortIndices::Unique(pointId3HashArray, this->CellIdMap);

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      // Create a temporary permutation array and use that for unhashing.
      auto tmpPerm = svtkm::cont::make_ArrayHandlePermutation(this->CellIdMap, pointId3HashArray);

      // decode
      svtkm::worklet::DispatcherMapField<Cid3UnhashWorklet> cid3UnhashDispatcher(
        (Cid3UnhashWorklet(nPoints)));
      cid3UnhashDispatcher.Invoke(tmpPerm, pointId3Array);
    }
    else
    {
#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      this->CellIdMap = svtkm::worklet::StableSortIndices::Sort(pointId3Array);
      svtkm::worklet::StableSortIndices::Unique(pointId3Array, this->CellIdMap);

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      // Permute the connectivity array into a basic array handle. Use a
      // temporary array handle to avoid memory aliasing.
      {
        svtkm::cont::ArrayHandle<svtkm::Id3> tmp;
        tmp = internal::ConcretePermutationArray(this->CellIdMap, pointId3Array);
        pointId3Array = tmp;
      }
    }

    // remove the last element if invalid
    svtkm::Id cells = pointId3Array.GetNumberOfValues();
    if (cells > 0 && pointId3Array.GetPortalConstControl().Get(cells - 1)[2] >= nPoints)
    {
      cells--;
      pointId3Array.Shrink(cells);
      this->CellIdMap.Shrink(cells);
    }

    /// output
    svtkm::cont::DataSet output;
    output.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coordinates", repPointArray));

    svtkm::cont::CellSetSingleType<> triangles;
    triangles.Fill(repPointArray.GetNumberOfValues(),
                   svtkm::CellShapeTagTriangle::Id,
                   3,
                   internal::copyFromVec(pointId3Array));
    output.SetCellSet(triangles);

#ifdef __SVTKM_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Wrap-up (s): " << timer.GetElapsedTime() << std::endl;
    svtkm::Float64 t = totalTimer.GetElapsedTime();
    std::cout << "Time (s): " << t << std::endl;
    std::cout << "number of output points: " << repPointArray.GetNumberOfValues() << std::endl;
    std::cout << "number of output cells: " << pointId3Array.GetNumberOfValues() << std::endl;
#endif

    return output;
  }

  template <typename ValueType, typename StorageType>
  svtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {
    return internal::ConcretePermutationArray(this->PointIdMap, input);
  }

  template <typename ValueType, typename StorageType>
  svtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {
    return internal::ConcretePermutationArray(this->CellIdMap, input);
  }

private:
  PointIdMapType PointIdMap;
  svtkm::cont::ArrayHandle<svtkm::Id> CellIdMap;
}; // struct VertexClustering
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_VertexClustering_h
