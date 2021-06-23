//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_Contour_h
#define svtk_m_worklet_Contour_h

#include <svtkm/BinaryPredicates.h>
#include <svtkm/VectorAnalysis.h>

#include <svtkm/exec/CellDerivative.h>
#include <svtkm/exec/ParametricCoordinates.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleGroupVec.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayHandleZip.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/DispatcherPointNeighborhood.h>
#include <svtkm/worklet/DispatcherReduceByKey.h>
#include <svtkm/worklet/Keys.h>
#include <svtkm/worklet/ScatterCounting.h>
#include <svtkm/worklet/ScatterPermutation.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>
#include <svtkm/worklet/WorkletPointNeighborhood.h>
#include <svtkm/worklet/WorkletReduceByKey.h>

#include <svtkm/worklet/contour/ContourTables.h>
#include <svtkm/worklet/gradient/PointGradient.h>
#include <svtkm/worklet/gradient/StructuredPointGradient.h>

namespace svtkm
{
namespace worklet
{

namespace contour
{

// -----------------------------------------------------------------------------
template <typename S>
svtkm::cont::ArrayHandle<svtkm::Float32, S> make_ScalarField(
  const svtkm::cont::ArrayHandle<svtkm::Float32, S>& ah)
{
  return ah;
}

template <typename S>
svtkm::cont::ArrayHandle<svtkm::Float64, S> make_ScalarField(
  const svtkm::cont::ArrayHandle<svtkm::Float64, S>& ah)
{
  return ah;
}

template <typename S>
svtkm::cont::ArrayHandleCast<svtkm::FloatDefault, svtkm::cont::ArrayHandle<svtkm::UInt8, S>>
make_ScalarField(const svtkm::cont::ArrayHandle<svtkm::UInt8, S>& ah)
{
  return svtkm::cont::make_ArrayHandleCast(ah, svtkm::FloatDefault());
}

template <typename S>
svtkm::cont::ArrayHandleCast<svtkm::FloatDefault, svtkm::cont::ArrayHandle<svtkm::Int8, S>>
make_ScalarField(const svtkm::cont::ArrayHandle<svtkm::Int8, S>& ah)
{
  return svtkm::cont::make_ArrayHandleCast(ah, svtkm::FloatDefault());
}

// ---------------------------------------------------------------------------
template <typename T>
class ClassifyCell : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(WholeArrayIn isoValues,
                                FieldInPoint fieldIn,
                                CellSetIn cellSet,
                                FieldOutCell outNumTriangles,
                                ExecObject classifyTable);
  using ExecutionSignature = void(CellShape, _1, _2, _4, _5);
  using InputDomain = _3;

  template <typename CellShapeType,
            typename IsoValuesType,
            typename FieldInType,
            typename ClassifyTableType>
  SVTKM_EXEC void operator()(CellShapeType shape,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn,
                            svtkm::IdComponent& numTriangles,
                            const ClassifyTableType& classifyTable) const
  {
    svtkm::IdComponent sum = 0;
    svtkm::IdComponent numIsoValues = static_cast<svtkm::IdComponent>(isovalues.GetNumberOfValues());
    svtkm::IdComponent numVerticesPerCell = classifyTable.GetNumVerticesPerCell(shape.Id);

    for (svtkm::Id i = 0; i < numIsoValues; ++i)
    {
      svtkm::IdComponent caseNumber = 0;
      for (svtkm::IdComponent j = 0; j < numVerticesPerCell; ++j)
      {
        caseNumber |= (fieldIn[j] > isovalues[i]) << j;
      }

      sum += classifyTable.GetNumTriangles(shape.Id, caseNumber);
    }
    numTriangles = sum;
  }
};

/// \brief Used to store data need for the EdgeWeightGenerate worklet.
/// This information is not passed as part of the arguments to the worklet as
/// that dramatically increase compile time by 200%
// TODO: remove unused data members.
// -----------------------------------------------------------------------------
class EdgeWeightGenerateMetaData : svtkm::cont::ExecutionObjectBase
{
public:
  template <typename DeviceAdapter>
  class ExecObject
  {
    template <typename FieldType>
    struct PortalTypes
    {
      using HandleType = svtkm::cont::ArrayHandle<FieldType>;
      using ExecutionTypes = typename HandleType::template ExecutionTypes<DeviceAdapter>;

      using Portal = typename ExecutionTypes::Portal;
      using PortalConst = typename ExecutionTypes::PortalConst;
    };

  public:
    ExecObject() = default;

    SVTKM_CONT
    ExecObject(svtkm::Id size,
               svtkm::cont::ArrayHandle<svtkm::FloatDefault>& interpWeights,
               svtkm::cont::ArrayHandle<svtkm::Id2>& interpIds,
               svtkm::cont::ArrayHandle<svtkm::Id>& interpCellIds,
               svtkm::cont::ArrayHandle<svtkm::UInt8>& interpContourId)
      : InterpWeightsPortal(interpWeights.PrepareForOutput(3 * size, DeviceAdapter()))
      , InterpIdPortal(interpIds.PrepareForOutput(3 * size, DeviceAdapter()))
      , InterpCellIdPortal(interpCellIds.PrepareForOutput(3 * size, DeviceAdapter()))
      , InterpContourPortal(interpContourId.PrepareForOutput(3 * size, DeviceAdapter()))
    {
      // Interp needs to be 3 times longer than size as they are per point of the
      // output triangle
    }
    typename PortalTypes<svtkm::FloatDefault>::Portal InterpWeightsPortal;
    typename PortalTypes<svtkm::Id2>::Portal InterpIdPortal;
    typename PortalTypes<svtkm::Id>::Portal InterpCellIdPortal;
    typename PortalTypes<svtkm::UInt8>::Portal InterpContourPortal;
  };

  SVTKM_CONT
  EdgeWeightGenerateMetaData(svtkm::Id size,
                             svtkm::cont::ArrayHandle<svtkm::FloatDefault>& interpWeights,
                             svtkm::cont::ArrayHandle<svtkm::Id2>& interpIds,
                             svtkm::cont::ArrayHandle<svtkm::Id>& interpCellIds,
                             svtkm::cont::ArrayHandle<svtkm::UInt8>& interpContourId)
    : Size(size)
    , InterpWeights(interpWeights)
    , InterpIds(interpIds)
    , InterpCellIds(interpCellIds)
    , InterpContourId(interpContourId)
  {
  }

  template <typename DeviceAdapter>
  SVTKM_CONT ExecObject<DeviceAdapter> PrepareForExecution(DeviceAdapter)
  {
    return ExecObject<DeviceAdapter>(
      this->Size, this->InterpWeights, this->InterpIds, this->InterpCellIds, this->InterpContourId);
  }

private:
  svtkm::Id Size;
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> InterpWeights;
  svtkm::cont::ArrayHandle<svtkm::Id2> InterpIds;
  svtkm::cont::ArrayHandle<svtkm::Id> InterpCellIds;
  svtkm::cont::ArrayHandle<svtkm::UInt8> InterpContourId;
};

/// \brief Compute the weights for each edge that is used to generate
/// a point in the resulting iso-surface
// -----------------------------------------------------------------------------
template <typename T>
class EdgeWeightGenerate : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ScatterType = svtkm::worklet::ScatterCounting;

  template <typename ArrayHandleType>
  SVTKM_CONT static ScatterType MakeScatter(const ArrayHandleType& numOutputTrisPerCell)
  {
    return ScatterType(numOutputTrisPerCell);
  }

  typedef void ControlSignature(CellSetIn cellset, // Cell set
                                WholeArrayIn isoValues,
                                FieldInPoint fieldIn, // Input point field defining the contour
                                ExecObject metaData,  // Metadata for edge weight generation
                                ExecObject classifyTable,
                                ExecObject triTable);
  using ExecutionSignature =
    void(CellShape, _2, _3, _4, _5, _6, InputIndex, WorkIndex, VisitIndex, PointIndices);

  using InputDomain = _1;

  template <typename CellShape,
            typename IsoValuesType,
            typename FieldInType, // Vec-like, one per input point
            typename ClassifyTableType,
            typename TriTableType,
            typename IndicesVecType,
            typename DeviceAdapter>
  SVTKM_EXEC void operator()(const CellShape shape,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn, // Input point field defining the contour
                            const EdgeWeightGenerateMetaData::ExecObject<DeviceAdapter>& metaData,
                            const ClassifyTableType& classifyTable,
                            const TriTableType& triTable,
                            svtkm::Id inputCellId,
                            svtkm::Id outputCellId,
                            svtkm::IdComponent visitIndex,
                            const IndicesVecType& indices) const
  {
    const svtkm::Id outputPointId = 3 * outputCellId;
    using FieldType = typename svtkm::VecTraits<FieldInType>::ComponentType;

    svtkm::IdComponent sum = 0, caseNumber = 0;
    svtkm::IdComponent i = 0,
                      numIsoValues = static_cast<svtkm::IdComponent>(isovalues.GetNumberOfValues());
    svtkm::IdComponent numVerticesPerCell = classifyTable.GetNumVerticesPerCell(shape.Id);

    for (i = 0; i < numIsoValues; ++i)
    {
      const FieldType ivalue = isovalues[i];
      // Compute the Marching Cubes case number for this cell. We need to iterate
      // the isovalues until the sum >= our visit index. But we need to make
      // sure the caseNumber is correct before stopping
      caseNumber = 0;
      for (svtkm::IdComponent j = 0; j < numVerticesPerCell; ++j)
      {
        caseNumber |= (fieldIn[j] > ivalue) << j;
      }

      sum += classifyTable.GetNumTriangles(shape.Id, caseNumber);
      if (sum > visitIndex)
      {
        break;
      }
    }

    visitIndex = sum - visitIndex - 1;

    // Interpolate for vertex positions and associated scalar values
    for (svtkm::IdComponent triVertex = 0; triVertex < 3; triVertex++)
    {
      auto edgeVertices = triTable.GetEdgeVertices(shape.Id, caseNumber, visitIndex, triVertex);
      const FieldType fieldValue0 = fieldIn[edgeVertices.first];
      const FieldType fieldValue1 = fieldIn[edgeVertices.second];

      // Store the input cell id so that we can properly generate the normals
      // in a subsequent call, after we have merged duplicate points
      metaData.InterpCellIdPortal.Set(outputPointId + triVertex, inputCellId);

      metaData.InterpContourPortal.Set(outputPointId + triVertex, static_cast<svtkm::UInt8>(i));

      metaData.InterpIdPortal.Set(
        outputPointId + triVertex,
        svtkm::Id2(indices[edgeVertices.first], indices[edgeVertices.second]));

      svtkm::FloatDefault interpolant = static_cast<svtkm::FloatDefault>(isovalues[i] - fieldValue0) /
        static_cast<svtkm::FloatDefault>(fieldValue1 - fieldValue0);

      metaData.InterpWeightsPortal.Set(outputPointId + triVertex, interpolant);
    }
  }
};

// ---------------------------------------------------------------------------
class MapPointField : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn interpolation_ids,
                                FieldIn interpolation_weights,
                                WholeArrayIn inputField,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  SVTKM_CONT
  MapPointField() {}

  template <typename WeightType, typename InFieldPortalType, typename OutFieldType>
  SVTKM_EXEC void operator()(const svtkm::Id2& low_high,
                            const WeightType& weight,
                            const InFieldPortalType& inPortal,
                            OutFieldType& result) const
  {
    //fetch the low / high values from inPortal
    result = static_cast<OutFieldType>(
      svtkm::Lerp(inPortal.Get(low_high[0]), inPortal.Get(low_high[1]), weight));
  }
};

// ---------------------------------------------------------------------------
struct MultiContourLess
{
  template <typename T>
  SVTKM_EXEC_CONT bool operator()(const T& a, const T& b) const
  {
    return a < b;
  }

  template <typename T, typename U>
  SVTKM_EXEC_CONT bool operator()(const svtkm::Pair<T, U>& a, const svtkm::Pair<T, U>& b) const
  {
    return (a.first < b.first) || (!(b.first < a.first) && (a.second < b.second));
  }

  template <typename T, typename U>
  SVTKM_EXEC_CONT bool operator()(const svtkm::internal::ArrayPortalValueReference<T>& a,
                                 const U& b) const
  {
    U&& t = static_cast<U>(a);
    return t < b;
  }
};

// ---------------------------------------------------------------------------
struct MergeDuplicateValues : svtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn keys,
                                ValuesIn valuesIn1,
                                ValuesIn valuesIn2,
                                ReducedValuesOut valueOut1,
                                ReducedValuesOut valueOut2);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename T,
            typename ValuesInType,
            typename Values2InType,
            typename ValuesOutType,
            typename Values2OutType>
  SVTKM_EXEC void operator()(const T&,
                            const ValuesInType& values1,
                            const Values2InType& values2,
                            ValuesOutType& valueOut1,
                            Values2OutType& valueOut2) const
  {
    valueOut1 = values1[0];
    valueOut2 = values2[0];
  }
};

// ---------------------------------------------------------------------------
struct CopyEdgeIds : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  SVTKM_EXEC
  void operator()(const svtkm::Id2& input, svtkm::Id2& output) const { output = input; }

  template <typename T>
  SVTKM_EXEC void operator()(const svtkm::Pair<T, svtkm::Id2>& input, svtkm::Id2& output) const
  {
    output = input.second;
  }
};

// ---------------------------------------------------------------------------
template <typename KeyType, typename KeyStorage>
void MergeDuplicates(const svtkm::cont::ArrayHandle<KeyType, KeyStorage>& original_keys,
                     svtkm::cont::ArrayHandle<svtkm::FloatDefault>& weights,
                     svtkm::cont::ArrayHandle<svtkm::Id2>& edgeIds,
                     svtkm::cont::ArrayHandle<svtkm::Id>& cellids,
                     svtkm::cont::ArrayHandle<svtkm::Id>& connectivity)
{
  svtkm::cont::ArrayHandle<KeyType> input_keys;
  svtkm::cont::ArrayCopy(original_keys, input_keys);
  svtkm::worklet::Keys<KeyType> keys(input_keys);
  input_keys.ReleaseResources();

  {
    svtkm::worklet::DispatcherReduceByKey<MergeDuplicateValues> dispatcher;
    svtkm::cont::ArrayHandle<svtkm::Id> writeCells;
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> writeWeights;
    dispatcher.Invoke(keys, weights, cellids, writeWeights, writeCells);
    weights = writeWeights;
    cellids = writeCells;
  }

  //need to build the new connectivity
  auto uniqueKeys = keys.GetUniqueKeys();
  svtkm::cont::Algorithm::LowerBounds(
    uniqueKeys, original_keys, connectivity, contour::MultiContourLess());

  //update the edge ids
  svtkm::worklet::DispatcherMapField<CopyEdgeIds> edgeDispatcher;
  edgeDispatcher.Invoke(uniqueKeys, edgeIds);
}

// -----------------------------------------------------------------------------
template <svtkm::IdComponent Comp>
struct EdgeVertex
{
  SVTKM_EXEC svtkm::Id operator()(const svtkm::Id2& edge) const { return edge[Comp]; }
};

class NormalsWorkletPass1 : public svtkm::worklet::WorkletVisitPointsWithCells
{
private:
  using PointIdsArray =
    svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandle<svtkm::Id2>, EdgeVertex<0>>;

public:
  using ControlSignature = void(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                FieldOutPoint normals);

  using ExecutionSignature = void(CellCount, CellIndices, InputIndex, _2, _3, _4, _5);

  using InputDomain = _1;
  using ScatterType = svtkm::worklet::ScatterPermutation<typename PointIdsArray::StorageTag>;

  SVTKM_CONT
  static ScatterType MakeScatter(const svtkm::cont::ArrayHandle<svtkm::Id2>& edges)
  {
    return ScatterType(svtkm::cont::make_ArrayHandleTransform(edges, EdgeVertex<0>()));
  }

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename NormalType>
  SVTKM_EXEC void operator()(const svtkm::IdComponent& numCells,
                            const FromIndexType& cellIds,
                            svtkm::Id pointId,
                            const CellSetInType& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            NormalType& normal) const
  {
    using T = typename WholeFieldIn::ValueType;
    svtkm::worklet::gradient::PointGradient<T> gradient;
    gradient(numCells, cellIds, pointId, geometry, pointCoordinates, inputField, normal);
  }

  template <typename FromIndexType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename NormalType>
  SVTKM_EXEC void operator()(const svtkm::IdComponent& svtkmNotUsed(numCells),
                            const FromIndexType& svtkmNotUsed(cellIds),
                            svtkm::Id pointId,
                            svtkm::exec::ConnectivityStructured<Cell, Point, 3>& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            NormalType& normal) const
  {
    using T = typename WholeFieldIn::ValueType;

    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    svtkm::exec::ConnectivityStructured<Point, Cell, 3> pointGeom(geometry);
    svtkm::exec::arg::ThreadIndicesPointNeighborhood tpn(pointId, pointId, 0, pointId, pointGeom, 0);

    const auto& boundary = tpn.GetBoundaryState();
    auto pointPortal = pointCoordinates.GetPortal();
    auto fieldPortal = inputField.GetPortal();
    svtkm::exec::FieldNeighborhood<decltype(pointPortal)> points(pointPortal, boundary);
    svtkm::exec::FieldNeighborhood<decltype(fieldPortal)> field(fieldPortal, boundary);

    svtkm::worklet::gradient::StructuredPointGradient<T> gradient;
    gradient(boundary, points, field, normal);
  }
};

class NormalsWorkletPass2 : public svtkm::worklet::WorkletVisitPointsWithCells
{
private:
  using PointIdsArray =
    svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandle<svtkm::Id2>, EdgeVertex<1>>;

public:
  typedef void ControlSignature(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                WholeArrayIn weights,
                                FieldInOutPoint normals);

  using ExecutionSignature =
    void(CellCount, CellIndices, InputIndex, _2, _3, _4, WorkIndex, _5, _6);

  using InputDomain = _1;
  using ScatterType = svtkm::worklet::ScatterPermutation<typename PointIdsArray::StorageTag>;

  SVTKM_CONT
  static ScatterType MakeScatter(const svtkm::cont::ArrayHandle<svtkm::Id2>& edges)
  {
    return ScatterType(svtkm::cont::make_ArrayHandleTransform(edges, EdgeVertex<1>()));
  }

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename WholeWeightsIn,
            typename NormalType>
  SVTKM_EXEC void operator()(const svtkm::IdComponent& numCells,
                            const FromIndexType& cellIds,
                            svtkm::Id pointId,
                            const CellSetInType& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            svtkm::Id edgeId,
                            const WholeWeightsIn& weights,
                            NormalType& normal) const
  {
    using T = typename WholeFieldIn::ValueType;
    svtkm::worklet::gradient::PointGradient<T> gradient;
    NormalType grad1;
    gradient(numCells, cellIds, pointId, geometry, pointCoordinates, inputField, grad1);

    NormalType grad0 = normal;
    auto weight = weights.Get(edgeId);
    normal = svtkm::Normal(svtkm::Lerp(grad0, grad1, weight));
  }

  template <typename FromIndexType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename WholeWeightsIn,
            typename NormalType>
  SVTKM_EXEC void operator()(const svtkm::IdComponent& svtkmNotUsed(numCells),
                            const FromIndexType& svtkmNotUsed(cellIds),
                            svtkm::Id pointId,
                            svtkm::exec::ConnectivityStructured<Cell, Point, 3>& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            svtkm::Id edgeId,
                            const WholeWeightsIn& weights,
                            NormalType& normal) const
  {
    using T = typename WholeFieldIn::ValueType;
    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    svtkm::exec::ConnectivityStructured<Point, Cell, 3> pointGeom(geometry);
    svtkm::exec::arg::ThreadIndicesPointNeighborhood tpn(pointId, pointId, 0, pointId, pointGeom, 0);

    const auto& boundary = tpn.GetBoundaryState();
    auto pointPortal = pointCoordinates.GetPortal();
    auto fieldPortal = inputField.GetPortal();
    svtkm::exec::FieldNeighborhood<decltype(pointPortal)> points(pointPortal, boundary);
    svtkm::exec::FieldNeighborhood<decltype(fieldPortal)> field(fieldPortal, boundary);

    svtkm::worklet::gradient::StructuredPointGradient<T> gradient;
    NormalType grad1;
    gradient(boundary, points, field, grad1);

    NormalType grad0 = normal;
    auto weight = weights.Get(edgeId);
    normal = svtkm::Lerp(grad0, grad1, weight);
    const auto mag2 = svtkm::MagnitudeSquared(normal);
    if (mag2 > 0.)
    {
      normal = normal * svtkm::RSqrt(mag2);
    }
  }
};

template <typename NormalCType,
          typename InputFieldType,
          typename InputStorageType,
          typename CellSet>
struct GenerateNormalsDeduced
{
  svtkm::cont::ArrayHandle<svtkm::Vec<NormalCType, 3>>* normals;
  const svtkm::cont::ArrayHandle<InputFieldType, InputStorageType>* field;
  const CellSet* cellset;
  svtkm::cont::ArrayHandle<svtkm::Id2>* edges;
  svtkm::cont::ArrayHandle<svtkm::FloatDefault>* weights;

  template <typename CoordinateSystem>
  void operator()(const CoordinateSystem& coordinates) const
  {
    // To save memory, the normals computation is done in two passes. In the first
    // pass the gradient at the first vertex of each edge is computed and stored in
    // the normals array. In the second pass the gradient at the second vertex is
    // computed and the gradient of the first vertex is read from the normals array.
    // The final normal is interpolated from the two gradient values and stored
    // in the normals array.
    //
    svtkm::worklet::DispatcherMapTopology<NormalsWorkletPass1> dispatcherNormalsPass1(
      NormalsWorkletPass1::MakeScatter(*edges));
    dispatcherNormalsPass1.Invoke(
      *cellset, *cellset, coordinates, contour::make_ScalarField(*field), *normals);

    svtkm::worklet::DispatcherMapTopology<NormalsWorkletPass2> dispatcherNormalsPass2(
      NormalsWorkletPass2::MakeScatter(*edges));
    dispatcherNormalsPass2.Invoke(
      *cellset, *cellset, coordinates, contour::make_ScalarField(*field), *weights, *normals);
  }
};

template <typename NormalCType,
          typename InputFieldType,
          typename InputStorageType,
          typename CellSet,
          typename CoordinateSystem>
void GenerateNormals(svtkm::cont::ArrayHandle<svtkm::Vec<NormalCType, 3>>& normals,
                     const svtkm::cont::ArrayHandle<InputFieldType, InputStorageType>& field,
                     const CellSet& cellset,
                     const CoordinateSystem& coordinates,
                     svtkm::cont::ArrayHandle<svtkm::Id2>& edges,
                     svtkm::cont::ArrayHandle<svtkm::FloatDefault>& weights)
{
  GenerateNormalsDeduced<NormalCType, InputFieldType, InputStorageType, CellSet> functor;
  functor.normals = &normals;
  functor.field = &field;
  functor.cellset = &cellset;
  functor.edges = &edges;
  functor.weights = &weights;


  svtkm::cont::CastAndCall(coordinates, functor);
}
}

/// \brief Compute the isosurface for a uniform grid data set
class Contour
{
public:
  //----------------------------------------------------------------------------
  Contour(bool mergeDuplicates = true)
    : MergeDuplicatePoints(mergeDuplicates)
    , InterpolationWeights()
    , InterpolationEdgeIds()
  {
  }

  //----------------------------------------------------------------------------
  svtkm::cont::ArrayHandle<svtkm::Id2> GetInterpolationEdgeIds() const
  {
    return this->InterpolationEdgeIds;
  }

  //----------------------------------------------------------------------------
  void SetMergeDuplicatePoints(bool merge) { this->MergeDuplicatePoints = merge; }

  //----------------------------------------------------------------------------
  bool GetMergeDuplicatePoints() const { return this->MergeDuplicatePoints; }

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices>
  svtkm::cont::CellSetSingleType<> Run(
    const ValueType* const isovalues,
    const svtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const svtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    svtkm::cont::ArrayHandle<svtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices)
  {
    svtkm::cont::ArrayHandle<svtkm::Vec<CoordinateType, 3>> normals;
    return this->DeduceRun(
      isovalues, numIsoValues, cells, coordinateSystem, input, vertices, normals, false);
  }

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices,
            typename StorageTagNormals>
  svtkm::cont::CellSetSingleType<> Run(
    const ValueType* const isovalues,
    const svtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const svtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    svtkm::cont::ArrayHandle<svtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    svtkm::cont::ArrayHandle<svtkm::Vec<CoordinateType, 3>, StorageTagNormals> normals)
  {
    return this->DeduceRun(
      isovalues, numIsoValues, cells, coordinateSystem, input, vertices, normals, true);
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  svtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {
    using svtkm::worklet::contour::MapPointField;
    MapPointField applyToField;
    svtkm::worklet::DispatcherMapField<MapPointField> applyFieldDispatcher(applyToField);

    svtkm::cont::ArrayHandle<ValueType> output;
    applyFieldDispatcher.Invoke(
      this->InterpolationEdgeIds, this->InterpolationWeights, input, output);
    return output;
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  svtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = svtkm::cont::make_ArrayHandlePermutation(this->CellIdMap, in);

    // Copy into an array with default storage:
    svtkm::cont::ArrayHandle<ValueType> result;
    svtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

  //----------------------------------------------------------------------------
  void ReleaseCellMapArrays() { this->CellIdMap.ReleaseResources(); }

private:
  template <typename ValueType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename StorageTagVertices,
            typename StorageTagNormals,
            typename CoordinateType,
            typename NormalType>
  struct DeduceCellType
  {
    Contour* MC = nullptr;
    const ValueType* isovalues = nullptr;
    const svtkm::Id* numIsoValues = nullptr;
    const CoordinateSystem* coordinateSystem = nullptr;
    const svtkm::cont::ArrayHandle<ValueType, StorageTagField>* inputField = nullptr;
    svtkm::cont::ArrayHandle<svtkm::Vec<CoordinateType, 3>, StorageTagVertices>* vertices;
    svtkm::cont::ArrayHandle<svtkm::Vec<NormalType, 3>, StorageTagNormals>* normals;
    const bool* withNormals;
    svtkm::cont::CellSetSingleType<>* result;

    template <typename CellSetType>
    void operator()(const CellSetType& cells) const
    {
      if (this->MC)
      {
        *this->result = this->MC->DoRun(isovalues,
                                        *numIsoValues,
                                        cells,
                                        *coordinateSystem,
                                        *inputField,
                                        *vertices,
                                        *normals,
                                        *withNormals);
      }
    }
  };

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename StorageTagVertices,
            typename StorageTagNormals,
            typename CoordinateType,
            typename NormalType>
  svtkm::cont::CellSetSingleType<> DeduceRun(
    const ValueType* isovalues,
    const svtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const svtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
    svtkm::cont::ArrayHandle<svtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    svtkm::cont::ArrayHandle<svtkm::Vec<NormalType, 3>, StorageTagNormals> normals,
    bool withNormals)
  {
    svtkm::cont::CellSetSingleType<> outputCells;

    DeduceCellType<ValueType,
                   CoordinateSystem,
                   StorageTagField,
                   StorageTagVertices,
                   StorageTagNormals,
                   CoordinateType,
                   NormalType>
      functor;
    functor.MC = this;
    functor.isovalues = isovalues;
    functor.numIsoValues = &numIsoValues;
    functor.coordinateSystem = &coordinateSystem;
    functor.inputField = &inputField;
    functor.vertices = &vertices;
    functor.normals = &normals;
    functor.withNormals = &withNormals;
    functor.result = &outputCells;

    svtkm::cont::CastAndCall(cells, functor);

    return outputCells;
  }

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename StorageTagVertices,
            typename StorageTagNormals,
            typename CoordinateType,
            typename NormalType>
  svtkm::cont::CellSetSingleType<> DoRun(
    const ValueType* isovalues,
    const svtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const svtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
    svtkm::cont::ArrayHandle<svtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    svtkm::cont::ArrayHandle<svtkm::Vec<NormalType, 3>, StorageTagNormals> normals,
    bool withNormals)
  {
    using svtkm::worklet::contour::ClassifyCell;
    using svtkm::worklet::contour::EdgeWeightGenerate;
    using svtkm::worklet::contour::EdgeWeightGenerateMetaData;
    using svtkm::worklet::contour::MapPointField;

    // Setup the Dispatcher Typedefs
    using ClassifyDispatcher = svtkm::worklet::DispatcherMapTopology<ClassifyCell<ValueType>>;

    using GenerateDispatcher = svtkm::worklet::DispatcherMapTopology<EdgeWeightGenerate<ValueType>>;

    svtkm::cont::ArrayHandle<ValueType> isoValuesHandle =
      svtkm::cont::make_ArrayHandle(isovalues, numIsoValues);

    // Call the ClassifyCell functor to compute the Marching Cubes case numbers
    // for each cell, and the number of vertices to be generated
    svtkm::cont::ArrayHandle<svtkm::IdComponent> numOutputTrisPerCell;
    {
      contour::ClassifyCell<ValueType> classifyCell;
      ClassifyDispatcher dispatcher(classifyCell);
      dispatcher.Invoke(isoValuesHandle, inputField, cells, numOutputTrisPerCell, this->classTable);
    }

    //Pass 2 Generate the edges
    svtkm::cont::ArrayHandle<svtkm::UInt8> contourIds;
    svtkm::cont::ArrayHandle<svtkm::Id> originalCellIdsForPoints;
    {
      auto scatter = EdgeWeightGenerate<ValueType>::MakeScatter(numOutputTrisPerCell);

      // Maps output cells to input cells. Store this for cell field mapping.
      this->CellIdMap = scatter.GetOutputToInputMap();

      EdgeWeightGenerateMetaData metaData(
        scatter.GetOutputRange(numOutputTrisPerCell.GetNumberOfValues()),
        this->InterpolationWeights,
        this->InterpolationEdgeIds,
        originalCellIdsForPoints,
        contourIds);

      EdgeWeightGenerate<ValueType> weightGenerate;
      GenerateDispatcher edgeDispatcher(weightGenerate, scatter);
      edgeDispatcher.Invoke(
        cells,
        //cast to a scalar field if not one, as cellderivative only works on those
        isoValuesHandle,
        inputField,
        metaData,
        this->classTable,
        this->triTable);
    }

    if (numIsoValues <= 1 || !this->MergeDuplicatePoints)
    { //release memory early that we are not going to need again
      contourIds.ReleaseResources();
    }

    svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
    if (this->MergeDuplicatePoints)
    {
      // In all the below cases you will notice that only interpolation ids
      // are updated. That is because MergeDuplicates will internally update
      // the InterpolationWeights and InterpolationOriginCellIds arrays to be the correct for the
      // output. But for InterpolationEdgeIds we need to do it manually once done
      if (numIsoValues == 1)
      {
        contour::MergeDuplicates(this->InterpolationEdgeIds, //keys
                                 this->InterpolationWeights, //values
                                 this->InterpolationEdgeIds, //values
                                 originalCellIdsForPoints,   //values
                                 connectivity);              // computed using lower bounds
      }
      else if (numIsoValues > 1)
      {
        contour::MergeDuplicates(
          svtkm::cont::make_ArrayHandleZip(contourIds, this->InterpolationEdgeIds), //keys
          this->InterpolationWeights,                                              //values
          this->InterpolationEdgeIds,                                              //values
          originalCellIdsForPoints,                                                //values
          connectivity); // computed using lower bounds
      }
    }
    else
    {
      //when we don't merge points, the connectivity array can be represented
      //by a counting array. The danger of doing it this way is that the output
      //type is unknown. That is why we copy it into an explicit array
      svtkm::cont::ArrayHandleIndex temp(this->InterpolationEdgeIds.GetNumberOfValues());
      svtkm::cont::ArrayCopy(temp, connectivity);
    }

    //generate the vertices's
    MapPointField applyToField;
    svtkm::worklet::DispatcherMapField<MapPointField> applyFieldDispatcher(applyToField);

    applyFieldDispatcher.Invoke(
      this->InterpolationEdgeIds, this->InterpolationWeights, coordinateSystem, vertices);

    //assign the connectivity to the cell set
    svtkm::cont::CellSetSingleType<> outputCells;
    outputCells.Fill(vertices.GetNumberOfValues(), svtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);

    //now that the vertices have been generated we can generate the normals
    if (withNormals)
    {
      contour::GenerateNormals(normals,
                               inputField,
                               cells,
                               coordinateSystem,
                               this->InterpolationEdgeIds,
                               this->InterpolationWeights);
    }

    return outputCells;
  }

  bool MergeDuplicatePoints;
  svtkm::worklet::internal::CellClassifyTable classTable;
  svtkm::worklet::internal::TriangleGenerationTable triTable;

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> InterpolationWeights;
  svtkm::cont::ArrayHandle<svtkm::Id2> InterpolationEdgeIds;

  svtkm::cont::ArrayHandle<svtkm::Id> CellIdMap;
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_Contour_h
