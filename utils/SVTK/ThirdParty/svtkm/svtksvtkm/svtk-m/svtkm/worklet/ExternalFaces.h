//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_ExternalFaces_h
#define svtk_m_worklet_ExternalFaces_h

#include <svtkm/CellShape.h>
#include <svtkm/Hash.h>
#include <svtkm/Math.h>

#include <svtkm/exec/CellFace.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConcatenate.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleGroupVec.h>
#include <svtkm/cont/ArrayHandleGroupVecVariable.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayHandleView.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/DispatcherReduceByKey.h>
#include <svtkm/worklet/Keys.h>
#include <svtkm/worklet/ScatterCounting.h>
#include <svtkm/worklet/WorkletMapTopology.h>
#include <svtkm/worklet/WorkletReduceByKey.h>

namespace svtkm
{
namespace worklet
{

struct ExternalFaces
{
  //Worklet that returns the number of external faces for each structured cell
  class NumExternalFacesPerStructuredCell : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet,
                                  FieldOut numFacesInCell,
                                  FieldInPoint pointCoordinates);
    using ExecutionSignature = _2(CellShape, _3);
    using InputDomain = _1;

    SVTKM_CONT
    NumExternalFacesPerStructuredCell(const svtkm::Vec3f_64& min_point,
                                      const svtkm::Vec3f_64& max_point)
      : MinPoint(min_point)
      , MaxPoint(max_point)
    {
    }

    SVTKM_EXEC
    inline svtkm::IdComponent CountExternalFacesOnDimension(svtkm::Float64 grid_min,
                                                           svtkm::Float64 grid_max,
                                                           svtkm::Float64 cell_min,
                                                           svtkm::Float64 cell_max) const
    {
      svtkm::IdComponent count = 0;

      bool cell_min_at_grid_boundary = cell_min <= grid_min;
      bool cell_max_at_grid_boundary = cell_max >= grid_max;

      if (cell_min_at_grid_boundary && !cell_max_at_grid_boundary)
      {
        count++;
      }
      else if (!cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        count++;
      }
      else if (cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        count += 2;
      }

      return count;
    }

    template <typename CellShapeTag, typename PointCoordVecType>
    SVTKM_EXEC svtkm::IdComponent operator()(CellShapeTag shape,
                                           const PointCoordVecType& pointCoordinates) const
    {
      (void)shape; // C4100 false positive workaround
      SVTKM_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      svtkm::IdComponent count = 0;

      count += CountExternalFacesOnDimension(
        MinPoint[0], MaxPoint[0], pointCoordinates[0][0], pointCoordinates[1][0]);

      count += CountExternalFacesOnDimension(
        MinPoint[1], MaxPoint[1], pointCoordinates[0][1], pointCoordinates[3][1]);

      count += CountExternalFacesOnDimension(
        MinPoint[2], MaxPoint[2], pointCoordinates[0][2], pointCoordinates[4][2]);

      return count;
    }

  private:
    svtkm::Vec3f_64 MinPoint;
    svtkm::Vec3f_64 MaxPoint;
  };


  //Worklet that finds face connectivity for each structured cell
  class BuildConnectivityStructured : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet,
                                  WholeCellSetIn<> inputCell,
                                  FieldOut faceShapes,
                                  FieldOut facePointCount,
                                  FieldOut faceConnectivity,
                                  FieldInPoint pointCoordinates);
    using ExecutionSignature = void(CellShape, VisitIndex, InputIndex, _2, _3, _4, _5, _6);
    using InputDomain = _1;

    using ScatterType = svtkm::worklet::ScatterCounting;

    template <typename CountArrayType>
    SVTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      SVTKM_IS_ARRAY_HANDLE(CountArrayType);
      return ScatterType(countArray);
    }

    SVTKM_CONT
    BuildConnectivityStructured(const svtkm::Vec3f_64& min_point, const svtkm::Vec3f_64& max_point)
      : MinPoint(min_point)
      , MaxPoint(max_point)
    {
    }

    enum FaceType
    {
      FACE_GRID_MIN,
      FACE_GRID_MAX,
      FACE_GRID_MIN_AND_MAX,
      FACE_NONE
    };

    SVTKM_EXEC
    inline bool FoundFaceOnDimension(svtkm::Float64 grid_min,
                                     svtkm::Float64 grid_max,
                                     svtkm::Float64 cell_min,
                                     svtkm::Float64 cell_max,
                                     svtkm::IdComponent& faceIndex,
                                     svtkm::IdComponent& count,
                                     svtkm::IdComponent dimensionFaceOffset,
                                     svtkm::IdComponent visitIndex) const
    {
      bool cell_min_at_grid_boundary = cell_min <= grid_min;
      bool cell_max_at_grid_boundary = cell_max >= grid_max;

      FaceType Faces = FaceType::FACE_NONE;

      if (cell_min_at_grid_boundary && !cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MIN;
      }
      else if (!cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MAX;
      }
      else if (cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MIN_AND_MAX;
      }

      if (Faces == FaceType::FACE_NONE)
        return false;

      if (Faces == FaceType::FACE_GRID_MIN)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset;
          return true;
        }
        else
        {
          count++;
        }
      }
      else if (Faces == FaceType::FACE_GRID_MAX)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset + 1;
          return true;
        }
        else
        {
          count++;
        }
      }
      else if (Faces == FaceType::FACE_GRID_MIN_AND_MAX)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset;
          return true;
        }
        count++;
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset + 1;
          return true;
        }
        count++;
      }

      return false;
    }

    template <typename PointCoordVecType>
    SVTKM_EXEC inline svtkm::IdComponent FindFaceIndexForVisit(
      svtkm::IdComponent visitIndex,
      const PointCoordVecType& pointCoordinates) const
    {
      svtkm::IdComponent count = 0;
      svtkm::IdComponent faceIndex = 0;
      // Search X dimension
      if (!FoundFaceOnDimension(MinPoint[0],
                                MaxPoint[0],
                                pointCoordinates[0][0],
                                pointCoordinates[1][0],
                                faceIndex,
                                count,
                                0,
                                visitIndex))
      {
        // Search Y dimension
        if (!FoundFaceOnDimension(MinPoint[1],
                                  MaxPoint[1],
                                  pointCoordinates[0][1],
                                  pointCoordinates[3][1],
                                  faceIndex,
                                  count,
                                  2,
                                  visitIndex))
        {
          // Search Z dimension
          FoundFaceOnDimension(MinPoint[2],
                               MaxPoint[2],
                               pointCoordinates[0][2],
                               pointCoordinates[4][2],
                               faceIndex,
                               count,
                               4,
                               visitIndex);
        }
      }
      return faceIndex;
    }

    template <typename CellShapeTag,
              typename CellSetType,
              typename PointCoordVecType,
              typename ConnectivityType>
    SVTKM_EXEC void operator()(CellShapeTag shape,
                              svtkm::IdComponent visitIndex,
                              svtkm::Id inputIndex,
                              const CellSetType& cellSet,
                              svtkm::UInt8& shapeOut,
                              svtkm::IdComponent& numFacePointsOut,
                              ConnectivityType& faceConnectivity,
                              const PointCoordVecType& pointCoordinates) const
    {
      SVTKM_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      svtkm::IdComponent faceIndex = FindFaceIndexForVisit(visitIndex, pointCoordinates);

      const svtkm::IdComponent numFacePoints =
        svtkm::exec::CellFaceNumberOfPoints(faceIndex, shape, *this);
      SVTKM_ASSERT(numFacePoints == faceConnectivity.GetNumberOfComponents());

      typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(inputIndex);

      shapeOut = svtkm::CELL_SHAPE_QUAD;
      numFacePointsOut = 4;

      for (svtkm::IdComponent facePointIndex = 0; facePointIndex < numFacePoints; facePointIndex++)
      {
        faceConnectivity[facePointIndex] =
          inCellIndices[svtkm::exec::CellFaceLocalIndex(facePointIndex, faceIndex, shape, *this)];
      }
    }

  private:
    svtkm::Vec3f_64 MinPoint;
    svtkm::Vec3f_64 MaxPoint;
  };

  //Worklet that returns the number of faces for each cell/shape
  class NumFacesPerCell : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet, FieldOut numFacesInCell);
    using ExecutionSignature = _2(CellShape);
    using InputDomain = _1;

    template <typename CellShapeTag>
    SVTKM_EXEC svtkm::IdComponent operator()(CellShapeTag shape) const
    {
      return svtkm::exec::CellFaceNumberOfFaces(shape, *this);
    }
  };

  //Worklet that identifies a cell face by a hash value. Not necessarily completely unique.
  class FaceHash : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldOut faceHashes,
                                  FieldOut originCells,
                                  FieldOut originFaces);
    using ExecutionSignature = void(_2, _3, _4, CellShape, PointIndices, InputIndex, VisitIndex);
    using InputDomain = _1;

    using ScatterType = svtkm::worklet::ScatterCounting;

    template <typename CellShapeTag, typename CellNodeVecType>
    SVTKM_EXEC void operator()(svtkm::HashType& faceHash,
                              svtkm::Id& cellIndex,
                              svtkm::IdComponent& faceIndex,
                              CellShapeTag shape,
                              const CellNodeVecType& cellNodeIds,
                              svtkm::Id inputIndex,
                              svtkm::IdComponent visitIndex) const
    {
      faceHash = svtkm::Hash(svtkm::exec::CellFaceCanonicalId(visitIndex, shape, cellNodeIds, *this));

      cellIndex = inputIndex;
      faceIndex = visitIndex;
    }
  };

  // Worklet that identifies the number of cells written out per face.
  // Because there can be collisions in the face ids, this instance might
  // represent multiple faces, which have to be checked. The resulting
  // number is the total number of external faces.
  class FaceCounts : public svtkm::worklet::WorkletReduceByKey
  {
  public:
    using ControlSignature = void(KeysIn keys,
                                  WholeCellSetIn<> inputCells,
                                  ValuesIn originCells,
                                  ValuesIn originFaces,
                                  ReducedValuesOut numOutputCells);
    using ExecutionSignature = _5(_2, _3, _4);
    using InputDomain = _1;

    template <typename CellSetType, typename OriginCellsType, typename OriginFacesType>
    SVTKM_EXEC svtkm::IdComponent operator()(const CellSetType& cellSet,
                                           const OriginCellsType& originCells,
                                           const OriginFacesType& originFaces) const
    {
      svtkm::IdComponent numCellsOnHash = originCells.GetNumberOfComponents();
      SVTKM_ASSERT(originFaces.GetNumberOfComponents() == numCellsOnHash);

      // Start by assuming all faces are unique, then remove one for each
      // face we find a duplicate for.
      svtkm::IdComponent numExternalFaces = numCellsOnHash;

      for (svtkm::IdComponent myIndex = 0;
           myIndex < numCellsOnHash - 1; // Don't need to check last face
           myIndex++)
      {
        svtkm::Id3 myFace =
          svtkm::exec::CellFaceCanonicalId(originFaces[myIndex],
                                          cellSet.GetCellShape(originCells[myIndex]),
                                          cellSet.GetIndices(originCells[myIndex]),
                                          *this);
        for (svtkm::IdComponent otherIndex = myIndex + 1; otherIndex < numCellsOnHash; otherIndex++)
        {
          svtkm::Id3 otherFace =
            svtkm::exec::CellFaceCanonicalId(originFaces[otherIndex],
                                            cellSet.GetCellShape(originCells[otherIndex]),
                                            cellSet.GetIndices(originCells[otherIndex]),
                                            *this);
          if (myFace == otherFace)
          {
            // Faces are the same. Must be internal. Remove 2, one for each face. We don't have to
            // worry about otherFace matching anything else because a proper topology will have at
            // most 2 cells sharing a face, so there should be no more matches.
            numExternalFaces -= 2;
            break;
          }
        }
      }

      return numExternalFaces;
    }
  };

private:
  // Resolves duplicate hashes by finding a specified unique face for a given hash.
  // Given a cell set (from a WholeCellSetIn) and the cell/face id pairs for each face
  // associated with a given hash, returns the index of the cell/face provided of the
  // visitIndex-th unique face. Basically, this method searches through all the cell/face
  // pairs looking for unique sets and returns the one associated with visitIndex.
  template <typename CellSetType, typename OriginCellsType, typename OriginFacesType>
  SVTKM_EXEC static svtkm::IdComponent FindUniqueFace(const CellSetType& cellSet,
                                                    const OriginCellsType& originCells,
                                                    const OriginFacesType& originFaces,
                                                    svtkm::IdComponent visitIndex,
                                                    const svtkm::exec::FunctorBase* self)
  {
    svtkm::IdComponent numCellsOnHash = originCells.GetNumberOfComponents();
    SVTKM_ASSERT(originFaces.GetNumberOfComponents() == numCellsOnHash);

    // Find the visitIndex-th unique face.
    svtkm::IdComponent numFound = 0;
    svtkm::IdComponent myIndex = 0;
    while (true)
    {
      SVTKM_ASSERT(myIndex < numCellsOnHash);
      svtkm::Id3 myFace = svtkm::exec::CellFaceCanonicalId(originFaces[myIndex],
                                                         cellSet.GetCellShape(originCells[myIndex]),
                                                         cellSet.GetIndices(originCells[myIndex]),
                                                         *self);
      bool foundPair = false;
      for (svtkm::IdComponent otherIndex = myIndex + 1; otherIndex < numCellsOnHash; otherIndex++)
      {
        svtkm::Id3 otherFace =
          svtkm::exec::CellFaceCanonicalId(originFaces[otherIndex],
                                          cellSet.GetCellShape(originCells[otherIndex]),
                                          cellSet.GetIndices(originCells[otherIndex]),
                                          *self);
        if (myFace == otherFace)
        {
          // Faces are the same. Must be internal.
          foundPair = true;
          break;
        }
      }

      if (!foundPair)
      {
        if (numFound == visitIndex)
        {
          break;
        }
        else
        {
          numFound++;
        }
      }

      myIndex++;
    }

    return myIndex;
  }

public:
  // Worklet that returns the number of points for each outputted face.
  // Have to manage the case where multiple faces have the same hash.
  class NumPointsPerFace : public svtkm::worklet::WorkletReduceByKey
  {
  public:
    using ControlSignature = void(KeysIn keys,
                                  WholeCellSetIn<> inputCells,
                                  ValuesIn originCells,
                                  ValuesIn originFaces,
                                  ReducedValuesOut numPointsInFace);
    using ExecutionSignature = _5(_2, _3, _4, VisitIndex);
    using InputDomain = _1;

    using ScatterType = svtkm::worklet::ScatterCounting;

    template <typename CountArrayType>
    SVTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      SVTKM_IS_ARRAY_HANDLE(CountArrayType);
      return ScatterType(countArray);
    }

    template <typename CellSetType, typename OriginCellsType, typename OriginFacesType>
    SVTKM_EXEC svtkm::IdComponent operator()(const CellSetType& cellSet,
                                           const OriginCellsType& originCells,
                                           const OriginFacesType& originFaces,
                                           svtkm::IdComponent visitIndex) const
    {
      svtkm::IdComponent myIndex =
        ExternalFaces::FindUniqueFace(cellSet, originCells, originFaces, visitIndex, this);

      return svtkm::exec::CellFaceNumberOfPoints(
        originFaces[myIndex], cellSet.GetCellShape(originCells[myIndex]), *this);
    }
  };

  // Worklet that returns the shape and connectivity for each external face
  class BuildConnectivity : public svtkm::worklet::WorkletReduceByKey
  {
  public:
    using ControlSignature = void(KeysIn keys,
                                  WholeCellSetIn<> inputCells,
                                  ValuesIn originCells,
                                  ValuesIn originFaces,
                                  ReducedValuesOut shapesOut,
                                  ReducedValuesOut connectivityOut,
                                  ReducedValuesOut cellIdMapOut);
    using ExecutionSignature = void(_2, _3, _4, VisitIndex, _5, _6, _7);
    using InputDomain = _1;

    using ScatterType = svtkm::worklet::ScatterCounting;

    template <typename CellSetType,
              typename OriginCellsType,
              typename OriginFacesType,
              typename ConnectivityType>
    SVTKM_EXEC void operator()(const CellSetType& cellSet,
                              const OriginCellsType& originCells,
                              const OriginFacesType& originFaces,
                              svtkm::IdComponent visitIndex,
                              svtkm::UInt8& shapeOut,
                              ConnectivityType& connectivityOut,
                              svtkm::Id& cellIdMapOut) const
    {
      const svtkm::IdComponent myIndex =
        ExternalFaces::FindUniqueFace(cellSet, originCells, originFaces, visitIndex, this);
      const svtkm::IdComponent myFace = originFaces[myIndex];


      typename CellSetType::CellShapeTag shapeIn = cellSet.GetCellShape(originCells[myIndex]);
      shapeOut = svtkm::exec::CellFaceShape(myFace, shapeIn, *this);
      cellIdMapOut = originCells[myIndex];

      const svtkm::IdComponent numFacePoints =
        svtkm::exec::CellFaceNumberOfPoints(myFace, shapeIn, *this);

      SVTKM_ASSERT(numFacePoints == connectivityOut.GetNumberOfComponents());

      typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(originCells[myIndex]);

      for (svtkm::IdComponent facePointIndex = 0; facePointIndex < numFacePoints; facePointIndex++)
      {
        connectivityOut[facePointIndex] =
          inCellIndices[svtkm::exec::CellFaceLocalIndex(facePointIndex, myFace, shapeIn, *this)];
      }
    }
  };

  class IsPolyDataCell : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet, FieldOut isPolyDataCell);
    using ExecutionSignature = _2(CellShape);
    using InputDomain = _1;

    template <typename CellShapeTag>
    SVTKM_EXEC svtkm::IdComponent operator()(CellShapeTag shape) const
    {
      return !svtkm::exec::CellFaceNumberOfFaces(shape, *this);
    }
  };

  class CountPolyDataCellPoints : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ScatterType = svtkm::worklet::ScatterCounting;

    using ControlSignature = void(CellSetIn inCellSet, FieldOut numPoints);
    using ExecutionSignature = _2(PointCount);
    using InputDomain = _1;

    SVTKM_EXEC svtkm::Id operator()(svtkm::Id count) const { return count; }
  };

  class PassPolyDataCells : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ScatterType = svtkm::worklet::ScatterCounting;

    using ControlSignature = void(CellSetIn inputTopology,
                                  FieldOut shapes,
                                  FieldOut pointIndices,
                                  FieldOut cellIdMapOut);
    using ExecutionSignature = void(CellShape, PointIndices, InputIndex, _2, _3, _4);

    template <typename CellShape, typename InPointIndexType, typename OutPointIndexType>
    SVTKM_EXEC void operator()(const CellShape& inShape,
                              const InPointIndexType& inPoints,
                              svtkm::Id inputIndex,
                              svtkm::UInt8& outShape,
                              OutPointIndexType& outPoints,
                              svtkm::Id& cellIdMapOut) const
    {
      cellIdMapOut = inputIndex;
      outShape = inShape.Id;

      svtkm::IdComponent numPoints = inPoints.GetNumberOfComponents();
      SVTKM_ASSERT(numPoints == outPoints.GetNumberOfComponents());
      for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        outPoints[pointIndex] = inPoints[pointIndex];
      }
    }
  };

  template <typename T>
  struct BiasFunctor
  {
    SVTKM_EXEC_CONT
    BiasFunctor(T bias = T(0))
      : Bias(bias)
    {
    }

    SVTKM_EXEC_CONT
    T operator()(T x) const { return x + this->Bias; }

    T Bias;
  };

public:
  SVTKM_CONT
  ExternalFaces()
    : PassPolyData(true)
  {
  }

  SVTKM_CONT
  void SetPassPolyData(bool flag) { this->PassPolyData = flag; }

  SVTKM_CONT
  bool GetPassPolyData() const { return this->PassPolyData; }

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

  void ReleaseCellMapArrays() { this->CellIdMap.ReleaseResources(); }


  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry for regular grids.
  ///
  /// Faster Run() method for uniform and rectilinear grid types.
  /// Uses grid extents to find cells on the boundaries of the grid.
  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  SVTKM_CONT void Run(
    const svtkm::cont::CellSetStructured<3>& inCellSet,
    const svtkm::cont::CoordinateSystem& coord,
    svtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    svtkm::Vec3f_64 MinPoint;
    svtkm::Vec3f_64 MaxPoint;

    svtkm::Id3 PointDimensions = inCellSet.GetPointDimensions();

    using DefaultHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
    using CartesianArrayHandle =
      svtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

    auto coordData = coord.GetData();
    if (coordData.IsType<CartesianArrayHandle>())
    {
      const auto vertices = coordData.Cast<CartesianArrayHandle>();
      const auto vertsSize = vertices.GetNumberOfValues();
      const auto tmp = svtkm::cont::ArrayGetValues({ 0, vertsSize - 1 }, vertices);
      MinPoint = tmp[0];
      MaxPoint = tmp[1];
    }
    else
    {
      auto vertices = coordData.Cast<svtkm::cont::ArrayHandleUniformPointCoordinates>();
      auto Coordinates = vertices.GetPortalConstControl();

      MinPoint = Coordinates.GetOrigin();
      svtkm::Vec3f_64 spacing = Coordinates.GetSpacing();

      svtkm::Vec3f_64 unitLength;
      unitLength[0] = static_cast<svtkm::Float64>(PointDimensions[0] - 1);
      unitLength[1] = static_cast<svtkm::Float64>(PointDimensions[1] - 1);
      unitLength[2] = static_cast<svtkm::Float64>(PointDimensions[2] - 1);
      MaxPoint = MinPoint + spacing * unitLength;
    }

    // Create a worklet to count the number of external faces on each cell
    svtkm::cont::ArrayHandle<svtkm::IdComponent> numExternalFaces;
    svtkm::worklet::DispatcherMapTopology<NumExternalFacesPerStructuredCell>
      numExternalFacesDispatcher((NumExternalFacesPerStructuredCell(MinPoint, MaxPoint)));

    numExternalFacesDispatcher.Invoke(inCellSet, numExternalFaces, coordData);

    svtkm::Id numberOfExternalFaces =
      svtkm::cont::Algorithm::Reduce(numExternalFaces, 0, svtkm::Sum());

    auto scatterCellToExternalFace = BuildConnectivityStructured::MakeScatter(numExternalFaces);

    // Maps output cells to input cells. Store this for cell field mapping.
    this->CellIdMap = scatterCellToExternalFace.GetOutputToInputMap();

    numExternalFaces.ReleaseResources();

    svtkm::Id connectivitySize = 4 * numberOfExternalFaces;
    svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityStorage> faceConnectivity;
    svtkm::cont::ArrayHandle<svtkm::UInt8, ShapeStorage> faceShapes;
    svtkm::cont::ArrayHandle<svtkm::IdComponent> facePointCount;
    // Must pre allocate because worklet invocation will not have enough
    // information to.
    faceConnectivity.Allocate(connectivitySize);

    svtkm::worklet::DispatcherMapTopology<BuildConnectivityStructured>
      buildConnectivityStructuredDispatcher(BuildConnectivityStructured(MinPoint, MaxPoint),
                                            scatterCellToExternalFace);

    buildConnectivityStructuredDispatcher.Invoke(
      inCellSet,
      inCellSet,
      faceShapes,
      facePointCount,
      svtkm::cont::make_ArrayHandleGroupVec<4>(faceConnectivity),
      coordData);

    auto offsets = svtkm::cont::ConvertNumIndicesToOffsets(facePointCount);

    outCellSet.Fill(inCellSet.GetNumberOfPoints(), faceShapes, faceConnectivity, offsets);
  }

  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry
  template <typename InCellSetType,
            typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage>
  SVTKM_CONT void Run(
    const InCellSetType& inCellSet,
    svtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    using PointCountArrayType = svtkm::cont::ArrayHandle<svtkm::IdComponent>;
    using ShapeArrayType = svtkm::cont::ArrayHandle<svtkm::UInt8, ShapeStorage>;
    using OffsetsArrayType = svtkm::cont::ArrayHandle<svtkm::Id, OffsetsStorage>;
    using ConnectivityArrayType = svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityStorage>;

    //Create a worklet to map the number of faces to each cell
    svtkm::cont::ArrayHandle<svtkm::IdComponent> facesPerCell;
    svtkm::worklet::DispatcherMapTopology<NumFacesPerCell> numFacesDispatcher;

    numFacesDispatcher.Invoke(inCellSet, facesPerCell);

    svtkm::worklet::ScatterCounting scatterCellToFace(facesPerCell);
    facesPerCell.ReleaseResources();

    PointCountArrayType polyDataPointCount;
    ShapeArrayType polyDataShapes;
    OffsetsArrayType polyDataOffsets;
    ConnectivityArrayType polyDataConnectivity;
    svtkm::cont::ArrayHandle<svtkm::Id> polyDataCellIdMap;
    svtkm::Id polyDataConnectivitySize = 0;
    if (this->PassPolyData)
    {
      svtkm::cont::ArrayHandle<svtkm::IdComponent> isPolyDataCell;
      svtkm::worklet::DispatcherMapTopology<IsPolyDataCell> isPolyDataCellDispatcher;

      isPolyDataCellDispatcher.Invoke(inCellSet, isPolyDataCell);

      svtkm::worklet::ScatterCounting scatterPolyDataCells(isPolyDataCell);

      isPolyDataCell.ReleaseResources();

      if (scatterPolyDataCells.GetOutputRange(inCellSet.GetNumberOfCells()) != 0)
      {
        svtkm::worklet::DispatcherMapTopology<CountPolyDataCellPoints>
          countPolyDataCellPointsDispatcher(scatterPolyDataCells);

        countPolyDataCellPointsDispatcher.Invoke(inCellSet, polyDataPointCount);

        svtkm::cont::ConvertNumIndicesToOffsets(
          polyDataPointCount, polyDataOffsets, polyDataConnectivitySize);

        svtkm::worklet::DispatcherMapTopology<PassPolyDataCells> passPolyDataCellsDispatcher(
          scatterPolyDataCells);

        polyDataConnectivity.Allocate(polyDataConnectivitySize);

        auto pdOffsetsTrim = svtkm::cont::make_ArrayHandleView(
          polyDataOffsets, 0, polyDataOffsets.GetNumberOfValues() - 1);

        passPolyDataCellsDispatcher.Invoke(
          inCellSet,
          polyDataShapes,
          svtkm::cont::make_ArrayHandleGroupVecVariable(polyDataConnectivity, pdOffsetsTrim),
          polyDataCellIdMap);
      }
    }

    if (scatterCellToFace.GetOutputRange(inCellSet.GetNumberOfCells()) == 0)
    {
      if (!polyDataConnectivitySize)
      {
        // Data has no faces. Output is empty.
        outCellSet.PrepareToAddCells(0, 0);
        outCellSet.CompleteAddingCells(inCellSet.GetNumberOfPoints());
        return;
      }
      else
      {
        // Pass only input poly data to output
        outCellSet.Fill(
          inCellSet.GetNumberOfPoints(), polyDataShapes, polyDataConnectivity, polyDataOffsets);
        this->CellIdMap = polyDataCellIdMap;
        return;
      }
    }

    svtkm::cont::ArrayHandle<svtkm::HashType> faceHashes;
    svtkm::cont::ArrayHandle<svtkm::Id> originCells;
    svtkm::cont::ArrayHandle<svtkm::IdComponent> originFaces;
    svtkm::worklet::DispatcherMapTopology<FaceHash> faceHashDispatcher(scatterCellToFace);

    faceHashDispatcher.Invoke(inCellSet, faceHashes, originCells, originFaces);

    svtkm::worklet::Keys<svtkm::HashType> faceKeys(faceHashes);

    svtkm::cont::ArrayHandle<svtkm::IdComponent> faceOutputCount;
    svtkm::worklet::DispatcherReduceByKey<FaceCounts> faceCountDispatcher;

    faceCountDispatcher.Invoke(faceKeys, inCellSet, originCells, originFaces, faceOutputCount);

    auto scatterCullInternalFaces = NumPointsPerFace::MakeScatter(faceOutputCount);

    PointCountArrayType facePointCount;
    svtkm::worklet::DispatcherReduceByKey<NumPointsPerFace> pointsPerFaceDispatcher(
      scatterCullInternalFaces);

    pointsPerFaceDispatcher.Invoke(faceKeys, inCellSet, originCells, originFaces, facePointCount);

    ShapeArrayType faceShapes;

    OffsetsArrayType faceOffsets;
    svtkm::Id connectivitySize;
    svtkm::cont::ConvertNumIndicesToOffsets(facePointCount, faceOffsets, connectivitySize);

    ConnectivityArrayType faceConnectivity;
    // Must pre allocate because worklet invocation will not have enough
    // information to.
    faceConnectivity.Allocate(connectivitySize);

    svtkm::worklet::DispatcherReduceByKey<BuildConnectivity> buildConnectivityDispatcher(
      scatterCullInternalFaces);

    svtkm::cont::ArrayHandle<svtkm::Id> faceToCellIdMap;

    // Create a view that doesn't have the last offset:
    auto faceOffsetsTrim =
      svtkm::cont::make_ArrayHandleView(faceOffsets, 0, faceOffsets.GetNumberOfValues() - 1);

    buildConnectivityDispatcher.Invoke(
      faceKeys,
      inCellSet,
      originCells,
      originFaces,
      faceShapes,
      svtkm::cont::make_ArrayHandleGroupVecVariable(faceConnectivity, faceOffsetsTrim),
      faceToCellIdMap);

    if (!polyDataConnectivitySize)
    {
      outCellSet.Fill(inCellSet.GetNumberOfPoints(), faceShapes, faceConnectivity, faceOffsets);
      this->CellIdMap = faceToCellIdMap;
    }
    else
    {
      // Join poly data to face data output
      svtkm::cont::ArrayHandleConcatenate<ShapeArrayType, ShapeArrayType> faceShapesArray(
        faceShapes, polyDataShapes);
      ShapeArrayType joinedShapesArray;
      svtkm::cont::ArrayCopy(faceShapesArray, joinedShapesArray);

      svtkm::cont::ArrayHandleConcatenate<PointCountArrayType, PointCountArrayType> pointCountArray(
        facePointCount, polyDataPointCount);
      PointCountArrayType joinedPointCountArray;
      svtkm::cont::ArrayCopy(pointCountArray, joinedPointCountArray);

      svtkm::cont::ArrayHandleConcatenate<ConnectivityArrayType, ConnectivityArrayType>
        connectivityArray(faceConnectivity, polyDataConnectivity);
      ConnectivityArrayType joinedConnectivity;
      svtkm::cont::ArrayCopy(connectivityArray, joinedConnectivity);

      // Adjust poly data offsets array with face connectivity size before join
      auto adjustedPolyDataOffsets = svtkm::cont::make_ArrayHandleTransform(
        polyDataOffsets, BiasFunctor<svtkm::Id>(faceConnectivity.GetNumberOfValues()));

      auto offsetsArray =
        svtkm::cont::make_ArrayHandleConcatenate(faceOffsetsTrim, adjustedPolyDataOffsets);
      OffsetsArrayType joinedOffsets;
      svtkm::cont::ArrayCopy(offsetsArray, joinedOffsets);

      svtkm::cont::ArrayHandleConcatenate<svtkm::cont::ArrayHandle<svtkm::Id>,
                                         svtkm::cont::ArrayHandle<svtkm::Id>>
        cellIdMapArray(faceToCellIdMap, polyDataCellIdMap);
      svtkm::cont::ArrayHandle<svtkm::Id> joinedCellIdMap;
      svtkm::cont::ArrayCopy(cellIdMapArray, joinedCellIdMap);

      outCellSet.Fill(
        inCellSet.GetNumberOfPoints(), joinedShapesArray, joinedConnectivity, joinedOffsets);
      this->CellIdMap = joinedCellIdMap;
    }
  }

private:
  svtkm::cont::ArrayHandle<svtkm::Id> CellIdMap;
  bool PassPolyData;

}; //struct ExternalFaces
}
} //namespace svtkm::worklet

#endif //svtk_m_worklet_ExternalFaces_h
