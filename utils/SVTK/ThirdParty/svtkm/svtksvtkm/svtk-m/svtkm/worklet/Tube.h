//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_tube_h
#define svtk_m_worklet_tube_h

#include <typeinfo>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace svtkm
{
namespace worklet
{

class Tube
{
public:
  //Helper worklet to count various things in each polyline.
  class CountSegments : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    SVTKM_CONT
    CountSegments(const bool& capping, const svtkm::Id& n)
      : Capping(capping)
      , NumSides(n)
      , NumVertsPerCell(3)
    {
    }

    using ControlSignature = void(CellSetIn,
                                  FieldOut ptsPerPolyline,
                                  FieldOut ptsPerTube,
                                  FieldOut numTubeConnIds,
                                  FieldOut linesPerPolyline);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    _2 ptsPerPolyline,
                                    _3 ptsPerTube,
                                    _4 numTubeConnIds,
                                    _5 linesPerPolyline);
    using InputDomain = _1;

    template <typename CellShapeTag>
    SVTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const svtkm::IdComponent& numPoints,
                              svtkm::Id& ptsPerPolyline,
                              svtkm::Id& ptsPerTube,
                              svtkm::Id& numTubeConnIds,
                              svtkm::Id& linesPerPolyline) const
    {
      // We only support polylines that contain 2 or more points.
      if (shapeType.Id == svtkm::CELL_SHAPE_POLY_LINE && numPoints > 1)
      {
        ptsPerPolyline = numPoints;
        ptsPerTube = this->NumSides * numPoints;
        // (two tris per segment) X (numSides) X numVertsPerCell
        numTubeConnIds = (numPoints - 1) * 2 * this->NumSides * this->NumVertsPerCell;
        linesPerPolyline = numPoints - 1;

        //Capping adds center vertex in middle of cap, plus NumSides triangles for cap.
        if (this->Capping)
        {
          ptsPerTube += 2;
          numTubeConnIds += (2 * this->NumSides * this->NumVertsPerCell);
        }
      }
      else
      {
        ptsPerPolyline = 0;
        ptsPerTube = 0;
        numTubeConnIds = 0;
        linesPerPolyline = 0;
      }
    }

  private:
    bool Capping;
    svtkm::Id NumSides;
    svtkm::Id NumVertsPerCell;
  };

  //Helper worklet to generate normals at each point in the polyline.
  class GenerateNormals : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
    static constexpr svtkm::FloatDefault vecMagnitudeEps = static_cast<svtkm::FloatDefault>(1e-3);

  public:
    SVTKM_CONT
    GenerateNormals()
      : DefaultNorm(0, 0, 1)
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn pointCoords,
                                  FieldInCell polylineOffset,
                                  WholeArrayOut newNormals);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    PointIndices ptIndices,
                                    _2 inPts,
                                    _3 polylineOffset,
                                    _4 outNormals);
    using InputDomain = _1;


    template <typename InPointsType, typename PointIndexType>
    SVTKM_EXEC svtkm::IdComponent FindValidSegment(const InPointsType& inPts,
                                                 const PointIndexType& ptIndices,
                                                 const svtkm::IdComponent& numPoints,
                                                 svtkm::IdComponent start) const
    {
      auto ps = inPts.Get(ptIndices[start]);
      svtkm::IdComponent end = start + 1;
      while (end < numPoints)
      {
        auto pe = inPts.Get(ptIndices[end]);
        if (svtkm::Magnitude(pe - ps) > 0)
          return end - 1;
        end++;
      }

      return numPoints;
    }

    template <typename CellShapeTag,
              typename PointIndexType,
              typename InPointsType,
              typename OutNormalType>
    SVTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const svtkm::IdComponent& numPoints,
                              const PointIndexType& ptIndices,
                              const InPointsType& inPts,
                              const svtkm::Id& polylineOffset,
                              OutNormalType& outNormals) const
    {
      //Ignore non-polyline and polyline with less than 2 points.
      if (shapeType.Id != svtkm::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        //The following follows the SVTK implementation in:
        //svtkPolyLine::GenerateSlidingNormals
        svtkm::Vec3f sPrev, sNext, normal, p0, p1;
        svtkm::IdComponent sNextId = FindValidSegment(inPts, ptIndices, numPoints, 0);

        if (sNextId != numPoints) // at least one valid segment
        {
          p0 = inPts.Get(ptIndices[sNextId]);
          p1 = inPts.Get(ptIndices[sNextId + 1]);
          sPrev = svtkm::Normal(p1 - p0);
        }
        else // no valid segments. Set everything to the default normal.
        {
          for (svtkm::Id i = 0; i < numPoints; i++)
            outNormals.Set(polylineOffset + i, this->DefaultNorm);
          return;
        }

        // find the next valid, non-parallel segment
        while (++sNextId < numPoints)
        {
          sNextId = FindValidSegment(inPts, ptIndices, numPoints, sNextId);
          if (sNextId != numPoints)
          {
            p0 = inPts.Get(ptIndices[sNextId]);
            p1 = inPts.Get(ptIndices[sNextId + 1]);
            sNext = svtkm::Normal(p1 - p0);

            // now the starting normal should simply be the cross product
            // in the following if statement we check for the case where
            // the two segments are parallel, in which case, continue searching
            // for the next valid segment
            auto n = svtkm::Cross(sPrev, sNext);
            if (svtkm::Magnitude(n) > vecMagnitudeEps)
            {
              normal = n;
              sPrev = sNext;
              break;
            }
          }
        }

        //only one valid segment...
        if (sNextId >= numPoints)
        {
          for (svtkm::IdComponent j = 0; j < 3; j++)
            if (sPrev[j] != 0)
            {
              normal[(j + 2) % 3] = 0;
              normal[(j + 1) % 3] = 1;
              normal[j] = -sPrev[(j + 1) % 3] / sPrev[j];
              break;
            }
        }

        svtkm::Normalize(normal);
        svtkm::Id lastNormalId = 0;
        while (++sNextId < numPoints)
        {
          sNextId = FindValidSegment(inPts, ptIndices, numPoints, sNextId);
          if (sNextId == numPoints)
            break;

          p0 = inPts.Get(ptIndices[sNextId]);
          p1 = inPts.Get(ptIndices[sNextId + 1]);
          sNext = svtkm::Normal(p1 - p0);

          auto q = svtkm::Cross(sNext, sPrev);

          if (svtkm::Magnitude(q) <= svtkm::Epsilon<svtkm::FloatDefault>()) //can't use this segment
            continue;
          svtkm::Normalize(q);

          svtkm::FloatDefault f1 = svtkm::Dot(q, normal);
          svtkm::FloatDefault f2 = 1 - (f1 * f1);
          if (f2 > 0)
            f2 = svtkm::Sqrt(f2);
          else
            f2 = 0;

          auto c = svtkm::Normal(sNext + sPrev);
          auto w = svtkm::Cross(c, q);
          c = svtkm::Cross(sPrev, q);
          if ((svtkm::Dot(normal, c) * svtkm::Dot(w, c)) < 0)
            f2 = -f2;

          for (svtkm::Id i = lastNormalId; i < sNextId; i++)
            outNormals.Set(polylineOffset + i, normal);
          lastNormalId = sNextId;
          sPrev = sNext;
          normal = (f1 * q) + (f2 * w);
        }

        for (svtkm::Id i = lastNormalId; i < numPoints; i++)
          outNormals.Set(polylineOffset + i, normal);
      }
    }

  private:
    svtkm::Vec3f DefaultNorm;
  };

  //Helper worklet to generate the tube points
  class GeneratePoints : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    SVTKM_CONT
    GeneratePoints(const bool& capping, const svtkm::Id& n, const svtkm::FloatDefault& r)
      : Capping(capping)
      , NumSides(n)
      , Radius(r)
      , Theta(2 * static_cast<svtkm::FloatDefault>(svtkm::Pi()) / static_cast<svtkm::FloatDefault>(n))
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn pointCoords,
                                  WholeArrayIn normals,
                                  FieldInCell tubePointOffsets,
                                  FieldInCell polylineOffset,
                                  WholeArrayOut newPointCoords,
                                  WholeArrayOut outPointSrcIdx);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    PointIndices ptIndices,
                                    _2 inPts,
                                    _3 inNormals,
                                    _4 tubePointOffsets,
                                    _5 polylineOffset,
                                    _6 outPts,
                                    _7 outPointSrcIdx);
    using InputDomain = _1;

    template <typename CellShapeTag,
              typename PointIndexType,
              typename InPointsType,
              typename InNormalsType,
              typename OutPointsType,
              typename OutPointSrcIdxType>
    SVTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const svtkm::IdComponent& numPoints,
                              const PointIndexType& ptIndices,
                              const InPointsType& inPts,
                              const InNormalsType& inNormals,
                              const svtkm::Id& tubePointOffsets,
                              const svtkm::Id& polylineOffset,
                              OutPointsType& outPts,
                              OutPointSrcIdxType& outPointSrcIdx) const
    {
      if (shapeType.Id != svtkm::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        svtkm::Id outIdx = tubePointOffsets;
        svtkm::Id pIdx = ptIndices[0];
        svtkm::Id pNextIdx = ptIndices[1];
        svtkm::Vec3f p = inPts.Get(pIdx);
        svtkm::Vec3f pNext = inPts.Get(pNextIdx);
        svtkm::Vec3f sNext = pNext - p;
        svtkm::Vec3f sPrev = sNext;
        for (svtkm::IdComponent j = 0; j < numPoints; j++)
        {
          if (j == 0) //first point
          {
            //Variables initialized before loop started.
          }
          else if (j == numPoints - 1) //last point
          {
            sPrev = sNext;
            p = pNext;
            pIdx = pNextIdx;
          }
          else
          {
            p = pNext;
            pIdx = pNextIdx;
            pNextIdx = ptIndices[j + 1];
            pNext = inPts.Get(pNextIdx);
            sPrev = sNext;
            sNext = pNext - p;
          }
          svtkm::Vec3f n = inNormals.Get(polylineOffset + j);

          //Coincident points.
          if (svtkm::Magnitude(sNext) <= svtkm::Epsilon<svtkm::FloatDefault>())
            this->RaiseError("Coincident points in Tube worklet.");

          svtkm::Normalize(sNext);
          auto s = (sPrev + sNext) / 2.;
          if (svtkm::Magnitude(s) <= svtkm::Epsilon<svtkm::FloatDefault>())
            s = svtkm::Cross(sPrev, n);
          svtkm::Normalize(s);

          auto w = svtkm::Cross(s, n);
          //Bad normal
          if (svtkm::Magnitude(w) <= svtkm::Epsilon<svtkm::FloatDefault>())
            this->RaiseError("Bad normal in Tube worklet.");
          svtkm::Normalize(w);

          //create orthogonal coordinate system.
          auto nP = svtkm::Cross(w, s);
          svtkm::Normalize(nP);

          //Add the start cap vertex. This is just a point at the center of the tube (on the polyline).
          if (this->Capping && j == 0)
          {
            outPts.Set(outIdx, p);
            outPointSrcIdx.Set(outIdx, pIdx);
            outIdx++;
          }

          //this only implements the 'sides share vertices' line 476
          svtkm::Vec3f normal;
          for (svtkm::IdComponent k = 0; k < this->NumSides; k++)
          {
            svtkm::FloatDefault angle = static_cast<svtkm::FloatDefault>(k) * this->Theta;
            svtkm::FloatDefault cosValue = svtkm::Cos(angle);
            svtkm::FloatDefault sinValue = svtkm::Sin(angle);
            normal = w * cosValue + nP * sinValue;
            auto newPt = p + this->Radius * normal;
            outPts.Set(outIdx, newPt);
            outPointSrcIdx.Set(outIdx, pIdx);
            outIdx++;
          }

          //Add the end cap vertex. This is just a point at the center of the tube (on the polyline).
          if (this->Capping && j == numPoints - 1)
          {
            outPts.Set(outIdx, p);
            outPointSrcIdx.Set(outIdx, pIdx);
            outIdx++;
          }
        }
      }
    }

  private:
    bool Capping;
    svtkm::Id NumSides;
    svtkm::FloatDefault Radius;
    svtkm::FloatDefault Theta;
  };

  //Helper worklet to generate the tube cells
  class GenerateCells : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    SVTKM_CONT
    GenerateCells(const bool& capping, const svtkm::Id& n)
      : Capping(capping)
      , NumSides(n)
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  FieldInCell tubePointOffsets,
                                  FieldInCell tubeConnOffsets,
                                  FieldInCell segOffset,
                                  WholeArrayOut outConnectivity,
                                  WholeArrayOut outCellSrcIdx);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    _2 tubePointOffset,
                                    _3 tubeConnOffsets,
                                    _4 segOffset,
                                    _5 outConn,
                                    _6 outCellSrcIdx);
    using InputDomain = _1;

    template <typename CellShapeTag, typename OutConnType, typename OutCellSrcIdxType>
    SVTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const svtkm::IdComponent& numPoints,
                              const svtkm::Id& tubePointOffset,
                              const svtkm::Id& tubeConnOffset,
                              const svtkm::Id& segOffset,
                              OutConnType& outConn,
                              OutCellSrcIdxType& outCellSrcIdx) const
    {
      if (shapeType.Id != svtkm::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        svtkm::Id outIdx = tubeConnOffset;
        svtkm::Id tubePtOffset = (this->Capping ? tubePointOffset + 1 : tubePointOffset);
        for (svtkm::IdComponent i = 0; i < numPoints - 1; i++)
        {
          for (svtkm::Id j = 0; j < this->NumSides; j++)
          {
            //Triangle 1: verts 0,1,2
            outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
            outConn.Set(outIdx + 1, tubePtOffset + i * this->NumSides + (j + 1) % this->NumSides);
            outConn.Set(outIdx + 2,
                        tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
            outCellSrcIdx.Set(outIdx / 3, segOffset + static_cast<svtkm::Id>(i));
            outIdx += 3;

            //Triangle 2: verts 0,2,3
            outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
            outConn.Set(outIdx + 1,
                        tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
            outConn.Set(outIdx + 2, tubePtOffset + (i + 1) * this->NumSides + j);
            outCellSrcIdx.Set(outIdx / 3, segOffset + static_cast<svtkm::Id>(i));
            outIdx += 3;
          }
        }

        if (this->Capping)
        {
          //start cap triangles
          svtkm::Id startCenterPt = 0 + tubePointOffset;
          for (svtkm::Id j = 0; j < this->NumSides; j++)
          {
            outConn.Set(outIdx + 0, startCenterPt);
            outConn.Set(outIdx + 1, startCenterPt + 1 + j);
            outConn.Set(outIdx + 2, startCenterPt + 1 + ((j + 1) % this->NumSides));
            outCellSrcIdx.Set(outIdx / 3, segOffset);
            outIdx += 3;
          }

          //end cap triangles
          svtkm::Id endCenterPt = (tubePointOffset + 1) + (numPoints * this->NumSides);
          svtkm::Id endOffsetPt = endCenterPt - this->NumSides;

          for (svtkm::Id j = 0; j < this->NumSides; j++)
          {
            outConn.Set(outIdx + 0, endCenterPt);
            outConn.Set(outIdx + 1, endOffsetPt + j);
            outConn.Set(outIdx + 2, endOffsetPt + ((j + 1) % this->NumSides));
            outCellSrcIdx.Set(outIdx / 3, segOffset + static_cast<svtkm::Id>(numPoints - 2));
            outIdx += 3;
          }
        }
      }
    }

  private:
    bool Capping;
    svtkm::Id NumSides;
  };


  class MapField : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn sourceIdx, WholeArrayIn sourceArray, FieldOut output);
    using ExecutionSignature = void(_1 sourceIdx, _2 sourceArray, _3 output);
    using InputDomain = _1;

    SVTKM_CONT
    MapField() {}

    template <typename SourceArrayType, typename T>
    SVTKM_EXEC void operator()(const svtkm::Id& sourceIdx,
                              const SourceArrayType& sourceArray,
                              T& output) const
    {
      output = sourceArray.Get(sourceIdx);
    }
  };

  SVTKM_CONT
  Tube()
    : Capping(false)
    , NumSides(0)
    , Radius(0)
  {
  }

  SVTKM_CONT
  Tube(const bool& capping, const svtkm::Id& n, const svtkm::FloatDefault& r)
    : Capping(capping)
    , NumSides(n)
    , Radius(r)

  {
  }

  SVTKM_CONT
  void SetCapping(bool v) { this->Capping = v; }
  SVTKM_CONT
  void SetNumberOfSides(svtkm::Id n) { this->NumSides = n; }
  SVTKM_CONT
  void SetRadius(svtkm::FloatDefault r) { this->Radius = r; }

  template <typename Storage>
  SVTKM_CONT void Run(const svtkm::cont::ArrayHandle<svtkm::Vec3f, Storage>& coords,
                     const svtkm::cont::DynamicCellSet& cellset,
                     svtkm::cont::ArrayHandle<svtkm::Vec3f>& newPoints,
                     svtkm::cont::CellSetSingleType<>& newCells)
  {
    using NormalsType = svtkm::cont::ArrayHandle<svtkm::Vec3f>;

    if (!cellset.IsSameType(svtkm::cont::CellSetExplicit<>()) &&
        !cellset.IsSameType(svtkm::cont::CellSetSingleType<>()))
    {
      throw svtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    }

    //Count number of polyline pts, tube pts and tube cells
    svtkm::cont::ArrayHandle<svtkm::Id> ptsPerPolyline, ptsPerTube, numTubeConnIds, segPerPolyline;
    CountSegments countSegs(this->Capping, this->NumSides);
    svtkm::worklet::DispatcherMapTopology<CountSegments> countInvoker(countSegs);
    countInvoker.Invoke(cellset, ptsPerPolyline, ptsPerTube, numTubeConnIds, segPerPolyline);

    svtkm::Id totalPolylinePts = svtkm::cont::Algorithm::Reduce(ptsPerPolyline, svtkm::Id(0));
    if (totalPolylinePts == 0)
      throw svtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    svtkm::Id totalTubePts = svtkm::cont::Algorithm::Reduce(ptsPerTube, svtkm::Id(0));
    svtkm::Id totalTubeConnIds = svtkm::cont::Algorithm::Reduce(numTubeConnIds, svtkm::Id(0));
    //All cells are triangles, so cell count is simple to compute.
    svtkm::Id totalTubeCells = totalTubeConnIds / 3;

    svtkm::cont::ArrayHandle<svtkm::Id> polylinePtOffset, tubePointOffsets, tubeConnOffsets,
      segOffset;
    svtkm::cont::Algorithm::ScanExclusive(ptsPerPolyline, polylinePtOffset);
    svtkm::cont::Algorithm::ScanExclusive(ptsPerTube, tubePointOffsets);
    svtkm::cont::Algorithm::ScanExclusive(numTubeConnIds, tubeConnOffsets);
    svtkm::cont::Algorithm::ScanExclusive(segPerPolyline, segOffset);

    //Generate normals at each point on all polylines
    NormalsType normals;
    normals.Allocate(totalPolylinePts);
    svtkm::worklet::DispatcherMapTopology<GenerateNormals> genNormalsDisp;
    genNormalsDisp.Invoke(cellset, coords, polylinePtOffset, normals);

    //Generate the tube points
    newPoints.Allocate(totalTubePts);
    this->OutputPointSourceIndex.Allocate(totalTubePts);
    GeneratePoints genPts(this->Capping, this->NumSides, this->Radius);
    svtkm::worklet::DispatcherMapTopology<GeneratePoints> genPtsDisp(genPts);
    genPtsDisp.Invoke(cellset,
                      coords,
                      normals,
                      tubePointOffsets,
                      polylinePtOffset,
                      newPoints,
                      this->OutputPointSourceIndex);

    //Generate tube cells
    svtkm::cont::ArrayHandle<svtkm::Id> newConnectivity;
    newConnectivity.Allocate(totalTubeConnIds);
    this->OutputCellSourceIndex.Allocate(totalTubeCells);
    GenerateCells genCells(this->Capping, this->NumSides);
    svtkm::worklet::DispatcherMapTopology<GenerateCells> genCellsDisp(genCells);
    genCellsDisp.Invoke(cellset,
                        tubePointOffsets,
                        tubeConnOffsets,
                        segOffset,
                        newConnectivity,
                        this->OutputCellSourceIndex);
    newCells.Fill(totalTubePts, svtkm::CELL_SHAPE_TRIANGLE, 3, newConnectivity);
  }

  template <typename T, typename StorageType>
  svtkm::cont::ArrayHandle<T> ProcessPointField(
    const svtkm::cont::ArrayHandle<T, StorageType>& input) const
  {
    svtkm::cont::ArrayHandle<T> output;
    svtkm::worklet::DispatcherMapField<MapField> mapFieldDisp;

    output.Allocate(this->OutputPointSourceIndex.GetNumberOfValues());
    mapFieldDisp.Invoke(this->OutputPointSourceIndex, input, output);
    return output;
  }

  template <typename T, typename StorageType>
  svtkm::cont::ArrayHandle<T> ProcessCellField(
    const svtkm::cont::ArrayHandle<T, StorageType>& input) const
  {
    svtkm::cont::ArrayHandle<T> output;
    svtkm::worklet::DispatcherMapField<MapField> mapFieldDisp;

    output.Allocate(this->OutputCellSourceIndex.GetNumberOfValues());
    mapFieldDisp.Invoke(this->OutputCellSourceIndex, input, output);
    return output;
  }

private:
  bool Capping;
  svtkm::Id NumSides;
  svtkm::FloatDefault Radius;
  svtkm::cont::ArrayHandle<svtkm::Id> OutputCellSourceIndex;
  svtkm::cont::ArrayHandle<svtkm::Id> OutputPointSourceIndex;
};
}
}

#endif //  svtk_m_worklet_tube_h
