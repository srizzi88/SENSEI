//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_Quadralizer_h
#define svtk_m_rendering_Quadralizer_h

#include <typeinfo>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>


#define QUAD_PER_CSS 6

namespace svtkm
{
namespace rendering
{

class Quadralizer
{
public:
  class CountQuads : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    SVTKM_CONT
    CountQuads() {}
    typedef void ControlSignature(CellSetIn cellset, FieldOut);
    typedef void ExecutionSignature(CellShape, _2);

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagGeneric shapeType, svtkm::Id& quads) const
    {
      if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
        quads = 1;
      else if (shapeType.Id == CELL_SHAPE_HEXAHEDRON)
        quads = 6;
      else if (shapeType.Id == svtkm::CELL_SHAPE_WEDGE)
        quads = 3;
      else if (shapeType.Id == svtkm::CELL_SHAPE_PYRAMID)
        quads = 1;

      else
        quads = 0;
    }

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType), svtkm::Id& quads) const
    {
      quads = 6;
    }

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagQuad shapeType, svtkm::Id& quads) const
    {
      if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
        quads = 1;
      else
        quads = 0;
    }
    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagWedge svtkmNotUsed(shapeType), svtkm::Id& quads) const
    {
      quads = 3;
    }
  }; //class CountQuads

  template <int DIM>
  class SegmentedStructured : public svtkm::worklet::WorkletVisitCellsWithPoints
  {

  public:
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(IncidentElementIndices, _2, _3);
    //typedef _1 InputDomain;
    SVTKM_CONT
    SegmentedStructured() {}

#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
    template <typename CellNodeVecType, typename OutIndicesPortal>
    SVTKM_EXEC void cell2quad(svtkm::Id4 idx,
                             svtkm::Vec<Id, 5>& quad,
                             const svtkm::Id offset,
                             const CellNodeVecType& cellIndices,
                             OutIndicesPortal& outputIndices) const
    {

      quad[1] = cellIndices[svtkm::IdComponent(idx[0])];
      quad[2] = cellIndices[svtkm::IdComponent(idx[1])];
      quad[3] = cellIndices[svtkm::IdComponent(idx[2])];
      quad[4] = cellIndices[svtkm::IdComponent(idx[3])];
      outputIndices.Set(offset, quad);
    }

    template <typename CellNodeVecType, typename OutIndicesPortal>
    SVTKM_EXEC void operator()(const CellNodeVecType& cellIndices,
                              const svtkm::Id& cellIndex,
                              OutIndicesPortal& outputIndices) const
    {
      if (DIM == 2)
      {
        // Do nothing mark says
      }
      else if (DIM == 3)
      {
        svtkm::Id offset = cellIndex * QUAD_PER_CSS;
        svtkm::Vec<svtkm::Id, 5> quad;
        quad[0] = cellIndex;
        svtkm::Id4 idx;
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 5, idx[3] = 4;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 1;
        idx[1] = 2;
        idx[2] = 6;
        idx[3] = 5;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 3;
        idx[1] = 7;
        idx[2] = 6;
        idx[3] = 2;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 0;
        idx[1] = 4;
        idx[2] = 7;
        idx[3] = 3;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 0;
        idx[1] = 3;
        idx[2] = 2;
        idx[3] = 1;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 4;
        idx[1] = 5;
        idx[2] = 6;
        idx[3] = 7;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);
      }
    }
#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif
  };


  class Quadralize : public svtkm::worklet::WorkletVisitCellsWithPoints
  {

  public:
    SVTKM_CONT
    Quadralize() {}
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void cell2quad(svtkm::Id& offset,
                             const VecType& cellIndices,
                             const svtkm::Id& cellId,
                             const svtkm::Id Id0,
                             const svtkm::Id Id1,
                             const svtkm::Id Id2,
                             const svtkm::Id Id3,
                             OutputPortal& outputIndices) const
    {
      svtkm::Vec<svtkm::Id, 5> quad;
      quad[0] = cellId;
      quad[1] = static_cast<svtkm::Id>(cellIndices[svtkm::IdComponent(Id0)]);
      quad[2] = static_cast<svtkm::Id>(cellIndices[svtkm::IdComponent(Id1)]);
      quad[3] = static_cast<svtkm::Id>(cellIndices[svtkm::IdComponent(Id2)]);
      quad[4] = static_cast<svtkm::Id>(cellIndices[svtkm::IdComponent(Id3)]);
      outputIndices.Set(offset++, quad);
    }

    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& pointOffset,
                              svtkm::CellShapeTagWedge svtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      svtkm::Id offset = pointOffset;

      cell2quad(offset, cellIndices, cellId, 3, 0, 2, 5, outputIndices);
      cell2quad(offset, cellIndices, cellId, 1, 4, 5, 2, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 3, 4, 1, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& offset,
                              svtkm::CellShapeTagQuad shapeType,
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
      {
        svtkm::Vec<svtkm::Id, 5> quad;
        quad[0] = cellId;
        quad[1] = static_cast<svtkm::Id>(cellIndices[0]);
        quad[2] = static_cast<svtkm::Id>(cellIndices[1]);
        quad[3] = static_cast<svtkm::Id>(cellIndices[2]);
        quad[4] = static_cast<svtkm::Id>(cellIndices[3]);
        outputIndices.Set(offset, quad);
      }
    }

    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& pointOffset,
                              svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const

    {
      svtkm::Id offset = pointOffset;
      cell2quad(offset, cellIndices, cellId, 0, 1, 5, 4, outputIndices);
      cell2quad(offset, cellIndices, cellId, 1, 2, 6, 5, outputIndices);
      cell2quad(offset, cellIndices, cellId, 3, 7, 6, 2, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 4, 7, 3, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 3, 2, 1, outputIndices);
      cell2quad(offset, cellIndices, cellId, 4, 5, 6, 7, outputIndices);
    }

    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& pointOffset,
                              svtkm::CellShapeTagGeneric shapeType,
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {

      if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
      {
        svtkm::Vec<svtkm::Id, 5> quad;
        quad[0] = cellId;
        quad[1] = cellIndices[0];
        quad[2] = cellIndices[1];
        quad[3] = cellIndices[2];
        quad[4] = cellIndices[3];
        outputIndices.Set(pointOffset, quad);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_HEXAHEDRON)
      {
        svtkm::Id offset = pointOffset;
        cell2quad(offset, cellIndices, cellId, 0, 1, 5, 4, outputIndices);
        cell2quad(offset, cellIndices, cellId, 1, 2, 6, 5, outputIndices);
        cell2quad(offset, cellIndices, cellId, 3, 7, 6, 2, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 4, 7, 3, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 3, 2, 1, outputIndices);
        cell2quad(offset, cellIndices, cellId, 4, 5, 6, 7, outputIndices);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_WEDGE)
      {
        svtkm::Id offset = pointOffset;

        cell2quad(offset, cellIndices, cellId, 3, 0, 2, 5, outputIndices);
        cell2quad(offset, cellIndices, cellId, 1, 4, 5, 2, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 3, 4, 1, outputIndices);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_PYRAMID)
      {
        svtkm::Id offset = pointOffset;

        cell2quad(offset, cellIndices, cellId, 3, 2, 1, 0, outputIndices);
      }
    }

  }; //class Quadralize

public:
  SVTKM_CONT
  Quadralizer() {}

  SVTKM_CONT
  void Run(const svtkm::cont::DynamicCellSet& cellset,
           svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Id, 5>>& outputIndices,
           svtkm::Id& output)
  {
    if (cellset.IsSameType(svtkm::cont::CellSetStructured<3>()))
    {
      svtkm::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.Cast<svtkm::cont::CellSetStructured<3>>();
      const svtkm::Id numCells = cellSetStructured3D.GetNumberOfCells();

      svtkm::cont::ArrayHandleCounting<svtkm::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * QUAD_PER_CSS);
      svtkm::worklet::DispatcherMapTopology<SegmentedStructured<3>> segInvoker;
      segInvoker.Invoke(cellSetStructured3D, cellIdxs, outputIndices);

      output = numCells * QUAD_PER_CSS;
    }
    else
    {
      svtkm::cont::ArrayHandle<svtkm::Id> quadsPerCell;
      svtkm::worklet::DispatcherMapTopology<CountQuads> countInvoker;
      countInvoker.Invoke(cellset, quadsPerCell);

      svtkm::Id total = 0;
      total = svtkm::cont::Algorithm::Reduce(quadsPerCell, svtkm::Id(0));

      svtkm::cont::ArrayHandle<svtkm::Id> cellOffsets;
      svtkm::cont::Algorithm::ScanExclusive(quadsPerCell, cellOffsets);
      outputIndices.Allocate(total);

      svtkm::worklet::DispatcherMapTopology<Quadralize> quadInvoker;
      quadInvoker.Invoke(cellset, cellOffsets, outputIndices);

      output = total;
    }
  }
};
}
}
#endif
