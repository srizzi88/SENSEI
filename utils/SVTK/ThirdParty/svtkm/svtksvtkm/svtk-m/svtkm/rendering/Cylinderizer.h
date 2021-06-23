//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_Cylinderizer_h
#define svtk_m_rendering_Cylinderizer_h

#include <typeinfo>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#define SEG_PER_TRI 3
//CSS is CellSetStructured
#define TRI_PER_CSS 12

namespace svtkm
{
namespace rendering
{

class Cylinderizer
{
public:
  class CountSegments : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    SVTKM_CONT
    CountSegments() {}
    typedef void ControlSignature(CellSetIn cellset, FieldOut);
    typedef void ExecutionSignature(CellShape, _2);

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagGeneric shapeType, svtkm::Id& segments) const
    {
      if (shapeType.Id == svtkm::CELL_SHAPE_LINE)
        segments = 1;
      else if (shapeType.Id == svtkm::CELL_SHAPE_TRIANGLE)
        segments = 3;
      else if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
        segments = 4;
      else if (shapeType.Id == svtkm::CELL_SHAPE_TETRA)
        segments = 12;
      else if (shapeType.Id == svtkm::CELL_SHAPE_WEDGE)
        segments = 24;
      else if (shapeType.Id == svtkm::CELL_SHAPE_PYRAMID)
        segments = 18;
      else if (shapeType.Id == svtkm::CELL_SHAPE_HEXAHEDRON)
        segments = 36;
      else
        segments = 0;
    }

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType), svtkm::Id& segments) const
    {
      segments = 36;
    }

    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagQuad svtkmNotUsed(shapeType), svtkm::Id& segments) const
    {
      segments = 4;
    }
    SVTKM_EXEC
    void operator()(svtkm::CellShapeTagWedge svtkmNotUsed(shapeType), svtkm::Id& segments) const
    {
      segments = 24;
    }
  }; //class CountSegments

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
    SVTKM_EXEC void cell2seg(svtkm::Id3 idx,
                            svtkm::Vec<Id, 3>& segment,
                            const svtkm::Id offset,
                            const CellNodeVecType& cellIndices,
                            OutIndicesPortal& outputIndices) const
    {

      segment[1] = cellIndices[svtkm::IdComponent(idx[0])];
      segment[2] = cellIndices[svtkm::IdComponent(idx[1])];
      outputIndices.Set(offset, segment);

      segment[1] = cellIndices[svtkm::IdComponent(idx[1])];
      segment[2] = cellIndices[svtkm::IdComponent(idx[2])];
      outputIndices.Set(offset + 1, segment);

      segment[1] = cellIndices[svtkm::IdComponent(idx[2])];
      segment[2] = cellIndices[svtkm::IdComponent(idx[0])];
      outputIndices.Set(offset + 2, segment);
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
        svtkm::Id offset = cellIndex * TRI_PER_CSS * SEG_PER_TRI;
        svtkm::Id3 segment;
        segment[0] = cellIndex;
        svtkm::Id3 idx;
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 5;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 5;
        idx[2] = 4;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 1;
        idx[1] = 2;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 1;
        idx[1] = 6;
        idx[2] = 5;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 3;
        idx[1] = 7;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 3;
        idx[1] = 6;
        idx[2] = 2;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 4;
        idx[2] = 7;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 7;
        idx[2] = 3;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 3;
        idx[2] = 2;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 2;
        idx[2] = 1;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 4;
        idx[1] = 5;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 4;
        idx[1] = 6;
        idx[2] = 7;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
      }
    }
#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif
  };


  class Cylinderize : public svtkm::worklet::WorkletVisitCellsWithPoints
  {

  public:
    SVTKM_CONT
    Cylinderize() {}
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void tri2seg(svtkm::Id& offset,
                           const VecType& cellIndices,
                           const svtkm::Id& cellId,
                           const svtkm::Id Id0,
                           const svtkm::Id Id1,
                           const svtkm::Id Id2,
                           OutputPortal& outputIndices) const
    {
      svtkm::Id3 segment;
      segment[0] = cellId;
      segment[1] = svtkm::Id(cellIndices[svtkm::IdComponent(Id0)]);
      segment[2] = svtkm::Id(cellIndices[svtkm::IdComponent(Id1)]);
      outputIndices.Set(offset++, segment);

      segment[1] = svtkm::Id(cellIndices[svtkm::IdComponent(Id1)]);
      segment[2] = svtkm::Id(cellIndices[svtkm::IdComponent(Id2)]);
      outputIndices.Set(offset++, segment);

      segment[1] = svtkm::Id(cellIndices[svtkm::IdComponent(Id2)]);
      segment[2] = svtkm::Id(cellIndices[svtkm::IdComponent(Id0)]);
      outputIndices.Set(offset++, segment);
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
        svtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(offset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(offset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[3];
        outputIndices.Set(offset + 2, segment);

        segment[1] = cellIndices[3];
        segment[2] = cellIndices[0];
        outputIndices.Set(offset + 3, segment);
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
      tri2seg(offset, cellIndices, cellId, 0, 1, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 5, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 2, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 6, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 7, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 6, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 4, 7, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 7, 3, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 3, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
      tri2seg(offset, cellIndices, cellId, 4, 5, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 4, 6, 7, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& pointOffset,
                              svtkm::CellShapeTagWedge svtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const

    {
      svtkm::Id offset = pointOffset;
      tri2seg(offset, cellIndices, cellId, 0, 1, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 5, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 0, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 2, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 4, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 5, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 3, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    SVTKM_EXEC void operator()(const svtkm::Id& pointOffset,
                              svtkm::CellShapeTagGeneric shapeType,
                              const VecType& cellIndices,
                              const svtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {

      if (shapeType.Id == svtkm::CELL_SHAPE_LINE)
      {
        svtkm::Id3 segment;
        segment[0] = cellId;

        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_TRIANGLE)
      {
        svtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(pointOffset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[0];
        outputIndices.Set(pointOffset + 2, segment);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
      {
        svtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(pointOffset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[3];
        outputIndices.Set(pointOffset + 2, segment);

        segment[1] = cellIndices[3];
        segment[2] = cellIndices[0];
        outputIndices.Set(pointOffset + 3, segment);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_TETRA)
      {
        svtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 3, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_HEXAHEDRON)
      {
        svtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 1, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 5, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 6, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 7, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 6, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 7, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 7, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 3, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 4, 5, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 4, 6, 7, outputIndices);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_WEDGE)
      {
        svtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 1, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 5, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 0, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 2, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 4, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 5, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 3, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
      }
      if (shapeType.Id == svtkm::CELL_SHAPE_PYRAMID)
      {
        svtkm::Id offset = pointOffset;

        tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 2, 3, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 2, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 1, 0, outputIndices);
      }
    }

  }; //class cylinderize

public:
  SVTKM_CONT
  Cylinderizer() {}

  SVTKM_CONT
  void Run(const svtkm::cont::DynamicCellSet& cellset,
           svtkm::cont::ArrayHandle<svtkm::Id3>& outputIndices,
           svtkm::Id& output)
  {
    if (cellset.IsSameType(svtkm::cont::CellSetStructured<3>()))
    {
      svtkm::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.Cast<svtkm::cont::CellSetStructured<3>>();
      const svtkm::Id numCells = cellSetStructured3D.GetNumberOfCells();

      svtkm::cont::ArrayHandleCounting<svtkm::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * TRI_PER_CSS * SEG_PER_TRI);

      svtkm::worklet::DispatcherMapTopology<SegmentedStructured<3>> segInvoker;
      segInvoker.Invoke(cellSetStructured3D, cellIdxs, outputIndices);

      output = numCells * TRI_PER_CSS * SEG_PER_TRI;
    }
    else
    {
      svtkm::cont::ArrayHandle<svtkm::Id> segmentsPerCell;
      svtkm::worklet::DispatcherMapTopology<CountSegments> countInvoker;
      countInvoker.Invoke(cellset, segmentsPerCell);

      svtkm::Id total = 0;
      total = svtkm::cont::Algorithm::Reduce(segmentsPerCell, svtkm::Id(0));

      svtkm::cont::ArrayHandle<svtkm::Id> cellOffsets;
      svtkm::cont::Algorithm::ScanExclusive(segmentsPerCell, cellOffsets);
      outputIndices.Allocate(total);

      svtkm::worklet::DispatcherMapTopology<Cylinderize> cylInvoker;
      cylInvoker.Invoke(cellset, cellOffsets, outputIndices);

      output = total;
    }
  }
};
}
}
#endif
