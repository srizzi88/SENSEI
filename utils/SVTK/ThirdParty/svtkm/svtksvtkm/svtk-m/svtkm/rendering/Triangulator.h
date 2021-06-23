//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_Triangulator_h
#define svtk_m_rendering_Triangulator_h

#include <typeinfo>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>
namespace svtkm
{
namespace rendering
{
/// \brief Triangulator creates a minimal set of triangles from a cell set.
///
///  This class creates a array of triangle indices from both 3D and 2D
///  explicit cell sets. This list can serve as input to opengl and the
///  ray tracer scene renderers.
///
class Triangulator
{
public:
  class CountTriangles : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    SVTKM_CONT
    CountTriangles() {}
    using ControlSignature = void(CellSetIn cellset, FieldOut);
    using ExecutionSignature = void(CellShape, _2);

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagGeneric shapeType, svtkm::Id& triangles) const
    {
      if (shapeType.Id == svtkm::CELL_SHAPE_TRIANGLE)
        triangles = 1;
      else if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
        triangles = 2;
      else if (shapeType.Id == svtkm::CELL_SHAPE_TETRA)
        triangles = 4;
      else if (shapeType.Id == svtkm::CELL_SHAPE_HEXAHEDRON)
        triangles = 12;
      else if (shapeType.Id == svtkm::CELL_SHAPE_WEDGE)
        triangles = 8;
      else if (shapeType.Id == svtkm::CELL_SHAPE_PYRAMID)
        triangles = 6;
      else
        triangles = 0;
    }

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType), svtkm::Id& triangles) const
    {
      triangles = 12;
    }

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagQuad svtkmNotUsed(shapeType), svtkm::Id& triangles) const
    {
      triangles = 2;
    }
    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagWedge svtkmNotUsed(shapeType), svtkm::Id& triangles) const
    {
      triangles = 8;
    }
  }; //class CountTriangles

  template <int DIM>
  class TrianglulateStructured : public svtkm::worklet::WorkletVisitCellsWithPoints
  {

  public:
    using ControlSignature = void(CellSetIn cellset, FieldInCell, WholeArrayOut);
    using ExecutionSignature = void(IncidentElementIndices, _2, _3);
    SVTKM_CONT
    TrianglulateStructured() {}

#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
    template <typename CellNodeVecType, typename OutIndicesPortal>
    SVTKM_EXEC void operator()(const CellNodeVecType& cellIndices,
                              const svtkm::Id& cellIndex,
                              OutIndicesPortal& outputIndices) const
    {
      svtkm::Id4 triangle;
      if (DIM == 2)
      {
        const svtkm::Id triangleOffset = cellIndex * 2;
        // 0-1-2
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellIndex;
        outputIndices.Set(triangleOffset, triangle);
        // 0-3-2
        triangle[2] = cellIndices[3];
        outputIndices.Set(triangleOffset + 1, triangle);
      }
      else if (DIM == 3)
      {
        const svtkm::Id triangleOffset = cellIndex * 12;

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[5];
        triangle[0] = cellIndex;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[5];
        outputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[7];
        outputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 7, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 8, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 9, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 10, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[7];
        outputIndices.Set(triangleOffset + 11, triangle);
      }
    }
#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif
  };


  class IndicesSort : public svtkm::worklet::WorkletMapField
  {
  public:
    SVTKM_CONT
    IndicesSort() {}
    using ControlSignature = void(FieldInOut);
    using ExecutionSignature = void(_1);
    SVTKM_EXEC
    void operator()(svtkm::Id4& triangleIndices) const
    {
      // first field contains the id of the cell the
      // trianlge belongs to
      svtkm::Id temp;
      if (triangleIndices[1] > triangleIndices[3])
      {
        temp = triangleIndices[1];
        triangleIndices[1] = triangleIndices[3];
        triangleIndices[3] = temp;
      }
      if (triangleIndices[1] > triangleIndices[2])
      {
        temp = triangleIndices[1];
        triangleIndices[1] = triangleIndices[2];
        triangleIndices[2] = temp;
      }
      if (triangleIndices[2] > triangleIndices[3])
      {
        temp = triangleIndices[2];
        triangleIndices[2] = triangleIndices[3];
        triangleIndices[3] = temp;
      }
    }
  }; //class IndicesSort

  struct IndicesLessThan
  {
    SVTKM_EXEC_CONT
    bool operator()(const svtkm::Id4& a, const svtkm::Id4& b) const
    {
      if (a[1] < b[1])
        return true;
      if (a[1] > b[1])
        return false;
      if (a[2] < b[2])
        return true;
      if (a[2] > b[2])
        return false;
      if (a[3] < b[3])
        return true;
      return false;
    }
  };

  class UniqueTriangles : public svtkm::worklet::WorkletMapField
  {
  public:
    SVTKM_CONT
    UniqueTriangles() {}

    using ControlSignature = void(WholeArrayIn, WholeArrayOut);
    using ExecutionSignature = void(_1, _2, WorkIndex);

    SVTKM_EXEC
    bool IsTwin(const svtkm::Id4& a, const svtkm::Id4& b) const
    {
      return (a[1] == b[1] && a[2] == b[2] && a[3] == b[3]);
    }

    template <typename IndicesPortalType, typename OutputFlagsPortalType>
    SVTKM_EXEC void operator()(const IndicesPortalType& indices,
                              OutputFlagsPortalType& outputFlags,
                              const svtkm::Id& index) const
    {
      if (index == 0)
        return;
      //if we are a shared face, mark ourself and neighbor for destruction
      if (IsTwin(indices.Get(index), indices.Get(index - 1)))
      {
        outputFlags.Set(index, 0);
        outputFlags.Set(index - 1, 0);
      }
    }
  }; //class UniqueTriangles

  class Trianglulate : public svtkm::worklet::WorkletVisitCellsWithPoints
  {

  public:
    SVTKM_CONT
    Trianglulate() {}
    using ControlSignature = void(CellSetIn cellset, FieldInCell, WholeArrayOut);
    using ExecutionSignature = void(_2, CellShape, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& triangleOffset,
                              svtkm::CellShapeTagWedge svtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      svtkm::Id4 triangle;

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[2];
      triangle[0] = cellId;
      outputIndices.Set(triangleOffset, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[4];
      outputIndices.Set(triangleOffset + 1, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[0];
      triangle[3] = cellIndices[2];
      outputIndices.Set(triangleOffset + 2, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[2];
      triangle[3] = cellIndices[5];
      outputIndices.Set(triangleOffset + 3, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[4];
      triangle[3] = cellIndices[5];
      outputIndices.Set(triangleOffset + 4, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[2];
      outputIndices.Set(triangleOffset + 5, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[3];
      triangle[3] = cellIndices[4];
      outputIndices.Set(triangleOffset + 6, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[4];
      triangle[3] = cellIndices[1];
      outputIndices.Set(triangleOffset + 7, triangle);
    }
    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& triangleOffset,
                              svtkm::CellShapeTagQuad svtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      svtkm::Id4 triangle;


      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[2];
      triangle[0] = cellId;
      outputIndices.Set(triangleOffset, triangle);

      triangle[2] = cellIndices[3];
      outputIndices.Set(triangleOffset + 1, triangle);
    }

    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& triangleOffset,
                              svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      svtkm::Id4 triangle;

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[5];
      triangle[0] = cellId;
      outputIndices.Set(triangleOffset, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[4];
      outputIndices.Set(triangleOffset + 1, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[2];
      triangle[3] = cellIndices[6];
      outputIndices.Set(triangleOffset + 2, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[5];
      outputIndices.Set(triangleOffset + 3, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[7];
      triangle[3] = cellIndices[6];
      outputIndices.Set(triangleOffset + 4, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[2];
      outputIndices.Set(triangleOffset + 5, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[4];
      triangle[3] = cellIndices[7];
      outputIndices.Set(triangleOffset + 6, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[7];
      triangle[3] = cellIndices[3];
      outputIndices.Set(triangleOffset + 7, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[3];
      triangle[3] = cellIndices[2];
      outputIndices.Set(triangleOffset + 8, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[2];
      triangle[3] = cellIndices[1];
      outputIndices.Set(triangleOffset + 9, triangle);

      triangle[1] = cellIndices[4];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[6];
      outputIndices.Set(triangleOffset + 10, triangle);

      triangle[1] = cellIndices[4];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[7];
      outputIndices.Set(triangleOffset + 11, triangle);
    }

    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& triangleOffset,
                              svtkm::CellShapeTagGeneric shapeType,
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      svtkm::Id4 triangle;

      if (shapeType.Id == svtkm::CELL_SHAPE_TRIANGLE)
      {

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
      {

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[2] = cellIndices[3];
        outputIndices.Set(triangleOffset + 1, triangle);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_TETRA)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[1];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 3, triangle);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_HEXAHEDRON)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[5];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[5];
        outputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[7];
        outputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 7, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 8, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 9, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 10, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[7];
        outputIndices.Set(triangleOffset + 11, triangle);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_WEDGE)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[0];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[5];
        outputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[5];
        outputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 7, triangle);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_PYRAMID)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[1];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[2];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[0];
        outputIndices.Set(triangleOffset + 5, triangle);
      }
    }
  }; //class Trianglulate

public:
  SVTKM_CONT
  Triangulator() {}

  SVTKM_CONT
  void ExternalTrianlges(svtkm::cont::ArrayHandle<svtkm::Id4>& outputIndices,
                         svtkm::Id& outputTriangles)
  {
    //Eliminate unseen triangles
    svtkm::worklet::DispatcherMapField<IndicesSort> sortInvoker;
    sortInvoker.Invoke(outputIndices);

    svtkm::cont::Algorithm::Sort(outputIndices, IndicesLessThan());
    svtkm::cont::ArrayHandle<svtkm::UInt8> flags;
    flags.Allocate(outputTriangles);

    svtkm::cont::ArrayHandleConstant<svtkm::Id> one(1, outputTriangles);
    svtkm::cont::Algorithm::Copy(one, flags);
    //Unique triangles will have a flag = 1
    svtkm::worklet::DispatcherMapField<UniqueTriangles>().Invoke(outputIndices, flags);

    svtkm::cont::ArrayHandle<svtkm::Id4> subset;
    svtkm::cont::Algorithm::CopyIf(outputIndices, flags, subset);
    outputIndices = subset;
    outputTriangles = subset.GetNumberOfValues();
  }

  SVTKM_CONT
  void Run(const svtkm::cont::DynamicCellSet& cellset,
           svtkm::cont::ArrayHandle<svtkm::Id4>& outputIndices,
           svtkm::Id& outputTriangles)
  {
    bool fastPath = false;
    if (cellset.IsSameType(svtkm::cont::CellSetStructured<3>()))
    {
      //svtkm::cont::CellSetStructured<3> cellSetStructured3D =
      //  cellset.Cast<svtkm::cont::CellSetStructured<3>>();

      //raytracing::MeshConnectivityBuilder<Device> builder;
      //outputIndices = builder.ExternalTrianglesStructured(cellSetStructured3D);
      //outputTriangles = outputIndices.GetNumberOfValues();
      //fastPath = true;
      svtkm::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.Cast<svtkm::cont::CellSetStructured<3>>();
      const svtkm::Id numCells = cellSetStructured3D.GetNumberOfCells();

      svtkm::cont::ArrayHandleCounting<svtkm::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * 12);
      svtkm::worklet::DispatcherMapTopology<TrianglulateStructured<3>>(TrianglulateStructured<3>())
        .Invoke(cellSetStructured3D, cellIdxs, outputIndices);

      outputTriangles = numCells * 12;
    }
    else if (cellset.IsSameType(svtkm::cont::CellSetStructured<2>()))
    {
      svtkm::cont::CellSetStructured<2> cellSetStructured2D =
        cellset.Cast<svtkm::cont::CellSetStructured<2>>();
      const svtkm::Id numCells = cellSetStructured2D.GetNumberOfCells();

      svtkm::cont::ArrayHandleCounting<svtkm::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * 2);
      svtkm::worklet::DispatcherMapTopology<TrianglulateStructured<2>>(TrianglulateStructured<2>())
        .Invoke(cellSetStructured2D, cellIdxs, outputIndices);

      outputTriangles = numCells * 2;
      // no need to do external faces on 2D cell set
      fastPath = true;
    }
    else
    {
      svtkm::cont::ArrayHandle<svtkm::Id> trianglesPerCell;
      svtkm::worklet::DispatcherMapTopology<CountTriangles>(CountTriangles())
        .Invoke(cellset, trianglesPerCell);

      svtkm::Id totalTriangles = 0;
      totalTriangles = svtkm::cont::Algorithm::Reduce(trianglesPerCell, svtkm::Id(0));

      svtkm::cont::ArrayHandle<svtkm::Id> cellOffsets;
      svtkm::cont::Algorithm::ScanExclusive(trianglesPerCell, cellOffsets);
      outputIndices.Allocate(totalTriangles);

      svtkm::worklet::DispatcherMapTopology<Trianglulate>(Trianglulate())
        .Invoke(cellset, cellOffsets, outputIndices);

      outputTriangles = totalTriangles;
    }

    //get rid of any triagles we cannot see
    if (!fastPath)
    {
      ExternalTrianlges(outputIndices, outputTriangles);
    }
  }
}; // class Triangulator
}
} //namespace svtkm::rendering
#endif //svtk_m_rendering_Triangulator_h
