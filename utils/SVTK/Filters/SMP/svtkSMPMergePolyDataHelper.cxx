/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSMPMergePolyDataHelper.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSMPMergePoints.h"
#include "svtkSMPTools.h"
#include "svtkSmartPointer.h"

#include <algorithm>

namespace
{

struct svtkMergePointsData
{
  svtkPolyData* Output;
  svtkSMPMergePoints* Locator;

  svtkMergePointsData(svtkPolyData* output, svtkSMPMergePoints* locator)
    : Output(output)
    , Locator(locator)
  {
  }
};

class svtkParallelMergePoints
{
public:
  svtkIdType* BucketIds;
  std::vector<svtkMergePointsData>::iterator Begin;
  std::vector<svtkMergePointsData>::iterator End;
  svtkSMPMergePoints* Merger;
  svtkIdList** IdMaps;
  svtkPointData* OutputPointData;
  svtkPointData** InputPointDatas;

  void operator()(svtkIdType begin, svtkIdType end)
  {
    // All actual work is done by svtkSMPMergePoints::Merge
    std::vector<svtkMergePointsData>::iterator itr;
    svtkPointData* outPD = this->OutputPointData;

    svtkIdType counter = 0;
    for (itr = Begin; itr != End; ++itr)
    {
      svtkIdList* idMap = this->IdMaps[counter];
      svtkPointData* inPD = this->InputPointDatas[counter++];
      for (svtkIdType i = begin; i < end; i++)
      {
        svtkIdType bucketId = BucketIds[i];
        if ((*itr).Locator->GetNumberOfIdsInBucket(bucketId) > 0)
        {
          Merger->Merge((*itr).Locator, bucketId, outPD, inPD, idMap);
        }
      }
    }
  }
};

void MergePoints(
  std::vector<svtkMergePointsData>& data, std::vector<svtkIdList*>& idMaps, svtkPolyData* outPolyData)
{
  // This merges points in parallel/

  std::vector<svtkMergePointsData>::iterator itr = data.begin();
  std::vector<svtkMergePointsData>::iterator begin = itr;
  std::vector<svtkMergePointsData>::iterator end = data.end();
  svtkPoints* outPts = (*itr).Output->GetPoints();

  // Prepare output points
  svtkIdType numPts = 0;
  while (itr != end)
  {
    numPts += (*itr).Output->GetNumberOfPoints();
    ++itr;
  }
  outPts->Resize(numPts);

  // Find non-empty buckets for best load balancing. We don't
  // want to visit bunch of empty buckets.
  svtkIdType numBuckets = (*begin).Locator->GetNumberOfBuckets();
  std::vector<svtkIdType> nonEmptyBuckets;
  std::vector<bool> bucketVisited(numBuckets, false);
  nonEmptyBuckets.reserve(numBuckets);
  for (itr = begin; itr != end; ++itr)
  {
    svtkSMPMergePoints* mp = (*itr).Locator;
    for (svtkIdType i = 0; i < numBuckets; i++)
    {
      if (mp->GetNumberOfIdsInBucket(i) > 0 && !bucketVisited[i])
      {
        nonEmptyBuckets.push_back(i);
        bucketVisited[i] = true;
      }
    }
  }

  // These id maps will later be used when merging cells.
  std::vector<svtkPointData*> pds;
  itr = begin;
  ++itr;
  while (itr != end)
  {
    pds.push_back((*itr).Output->GetPointData());
    svtkIdList* idMap = svtkIdList::New();
    idMap->Allocate((*itr).Output->GetNumberOfPoints());
    idMaps.push_back(idMap);
    ++itr;
  }

  svtkParallelMergePoints mergePoints;
  mergePoints.BucketIds = &nonEmptyBuckets[0];
  mergePoints.Merger = (*begin).Locator;
  mergePoints.OutputPointData = (*begin).Output->GetPointData();
  if (!idMaps.empty())
  {
    mergePoints.Merger->InitializeMerge();
    mergePoints.IdMaps = &idMaps[0];
    // Prepare output point data
    int numArrays = mergePoints.OutputPointData->GetNumberOfArrays();
    for (int i = 0; i < numArrays; i++)
    {
      mergePoints.OutputPointData->GetArray(i)->Resize(numPts);
    }
    mergePoints.InputPointDatas = &pds[0];

    // The first locator is what we will use to accumulate all others
    // So all iteration starts from second dataset.
    std::vector<svtkMergePointsData>::iterator second = begin;
    ++second;
    mergePoints.Begin = second;
    mergePoints.End = end;
    // Actual work
    svtkSMPTools::For(0, static_cast<svtkIdType>(nonEmptyBuckets.size()), mergePoints);
    // mergePoints.operator()(0, nonEmptyBuckets.size());

    // Fixup output sizes.
    mergePoints.Merger->FixSizeOfPointArray();
    for (int i = 0; i < numArrays; i++)
    {
      mergePoints.OutputPointData->GetArray(i)->SetNumberOfTuples(
        mergePoints.Merger->GetMaxId() + 1);
    }
  }
  outPolyData->SetPoints(mergePoints.Merger->GetPoints());
  outPolyData->GetPointData()->ShallowCopy(mergePoints.OutputPointData);
}

class svtkParallelMergeCells
{
public:
  svtkIdList* CellOffsets;
  svtkIdList* ConnOffsets;
  svtkCellArray* InCellArray;
  svtkCellArray* OutCellArray;
  svtkIdType OutputCellOffset;
  svtkIdType OutputConnOffset;
  svtkIdList* IdMap;

  struct MapCellsImpl
  {
    // Call this signature:
    template <typename InCellStateT>
    void operator()(InCellStateT& inState, svtkCellArray* outCells, svtkIdType inCellOffset,
      svtkIdType inCellOffsetEnd, svtkIdType inConnOffset, svtkIdType inConnOffsetEnd,
      svtkIdType outCellOffset, svtkIdType outConnOffset, svtkIdList* map)
    {
      outCells->Visit(*this, inState, inCellOffset, inCellOffsetEnd, inConnOffset, inConnOffsetEnd,
        outCellOffset, outConnOffset, map);
    }

    // Internal signature:
    template <typename InCellStateT, typename OutCellStateT>
    void operator()(OutCellStateT& outState, InCellStateT& inState, svtkIdType inCellOffset,
      svtkIdType inCellOffsetEnd, svtkIdType inConnOffset, svtkIdType inConnOffsetEnd,
      svtkIdType outCellOffset, svtkIdType outConnOffset, svtkIdList* map)
    {
      using InIndexType = typename InCellStateT::ValueType;
      using OutIndexType = typename OutCellStateT::ValueType;

      const auto inCell =
        svtk::DataArrayValueRange<1>(inState.GetOffsets(), inCellOffset, inCellOffsetEnd + 1);
      const auto inConn =
        svtk::DataArrayValueRange<1>(inState.GetConnectivity(), inConnOffset, inConnOffsetEnd);
      auto outCell =
        svtk::DataArrayValueRange<1>(outState.GetOffsets(), outCellOffset + inCellOffset);
      auto outConn =
        svtk::DataArrayValueRange<1>(outState.GetConnectivity(), outConnOffset + inConnOffset);

      // Copy the offsets, adding outConnOffset to adjust for existing
      // connectivity entries:
      std::transform(
        inCell.cbegin(), inCell.cend(), outCell.begin(), [&](InIndexType i) -> OutIndexType {
          return static_cast<OutIndexType>(i + outConnOffset);
        });

      // Copy the connectivities, passing them through the map:
      std::transform(
        inConn.cbegin(), inConn.cend(), outConn.begin(), [&](InIndexType i) -> OutIndexType {
          return static_cast<OutIndexType>(map->GetId(static_cast<svtkIdType>(i)));
        });
    }
  };

  void operator()(svtkIdType begin, svtkIdType end)
  {
    svtkIdType noffsets = this->CellOffsets->GetNumberOfIds();
    svtkIdList* cellOffsets = this->CellOffsets;
    svtkIdList* connOffsets = this->ConnOffsets;
    svtkCellArray* outCellArray = this->OutCellArray;
    svtkCellArray* inCellArray = this->InCellArray;
    svtkIdType outputCellOffset = this->OutputCellOffset;
    svtkIdType outputConnOffset = this->OutputConnOffset;
    svtkIdList* map = this->IdMap;

    for (svtkIdType i = begin; i < end; i++)
    {
      // Note that there may be multiple cells starting at
      // this offset. So we find the next offset and insert
      // all cells between here and there.
      svtkIdType nextCellOffset;
      svtkIdType nextConnOffset;
      if (i ==
        noffsets - 1) // This needs to be the end of the array always, not the loop counter's end
      {
        nextCellOffset = this->InCellArray->GetNumberOfCells();
        nextConnOffset = this->InCellArray->GetNumberOfConnectivityIds();
      }
      else
      {
        nextCellOffset = cellOffsets->GetId(i + 1);
        nextConnOffset = connOffsets->GetId(i + 1);
      }
      // Process all cells between the given offset and the next.
      svtkIdType cellOffset = cellOffsets->GetId(i);
      svtkIdType connOffset = connOffsets->GetId(i);

      inCellArray->Visit(MapCellsImpl{}, outCellArray, cellOffset, nextCellOffset, connOffset,
        nextConnOffset, outputCellOffset, outputConnOffset, map);
    }
  }
};

class svtkParallelCellDataCopier
{
public:
  svtkDataSetAttributes* InputCellData;
  svtkDataSetAttributes* OutputCellData;
  svtkIdType Offset;

  void operator()(svtkIdType begin, svtkIdType end)
  {
    svtkDataSetAttributes* inputCellData = this->InputCellData;
    svtkDataSetAttributes* outputCellData = this->OutputCellData;
    svtkIdType offset = this->Offset;

    for (svtkIdType i = begin; i < end; i++)
    {
      outputCellData->SetTuple(offset + i, i, inputCellData);
    }
  }
};

struct svtkMergeCellsData
{
  svtkPolyData* Output;
  svtkIdList* CellOffsets;
  svtkIdList* ConnOffsets;
  svtkCellArray* OutCellArray;

  svtkMergeCellsData(
    svtkPolyData* output, svtkIdList* cellOffsets, svtkIdList* connOffsets, svtkCellArray* cellArray)
    : Output(output)
    , CellOffsets(cellOffsets)
    , ConnOffsets(connOffsets)
    , OutCellArray(cellArray)
  {
  }
};

struct CopyCellArraysToFront
{
  // call this signature:
  template <typename OutCellArraysT>
  void operator()(OutCellArraysT& out, svtkCellArray* in)
  {
    in->Visit(*this, out);
  }

  // Internal signature:
  template <typename InCellArraysT, typename OutCellArraysT>
  void operator()(InCellArraysT& in, OutCellArraysT& out)
  {
    using InIndexType = typename InCellArraysT::ValueType;
    using OutIndexType = typename OutCellArraysT::ValueType;

    const auto inCell = svtk::DataArrayValueRange<1>(in.GetOffsets());
    const auto inConn = svtk::DataArrayValueRange<1>(in.GetConnectivity());
    auto outCell = svtk::DataArrayValueRange<1>(out.GetOffsets());
    auto outConn = svtk::DataArrayValueRange<1>(out.GetConnectivity());

    auto cast = [](InIndexType i) -> OutIndexType { return static_cast<OutIndexType>(i); };

    std::transform(inCell.cbegin(), inCell.cend(), outCell.begin(), cast);
    std::transform(inConn.cbegin(), inConn.cend(), outConn.begin(), cast);
  }
};

void MergeCells(std::vector<svtkMergeCellsData>& data, const std::vector<svtkIdList*>& idMaps,
  svtkIdType cellDataOffset, svtkCellArray* outCells)
{
  std::vector<svtkMergeCellsData>::iterator begin = data.begin();
  std::vector<svtkMergeCellsData>::iterator itr;
  std::vector<svtkMergeCellsData>::iterator second = begin;
  std::vector<svtkMergeCellsData>::iterator end = data.end();
  ++second;

  std::vector<svtkIdList*>::const_iterator mapIter = idMaps.begin();

  svtkCellArray* firstCells = (*begin).OutCellArray;

  svtkIdType outCellOffset = 0;
  svtkIdType outConnOffset = 0;
  outCellOffset += firstCells->GetNumberOfCells();
  outConnOffset += firstCells->GetNumberOfConnectivityIds();

  // Prepare output. Since there's no mapping here, do a simple copy in
  // serial:
  outCells->Visit(CopyCellArraysToFront{}, firstCells);

  svtkParallelMergeCells mergeCells;
  mergeCells.OutCellArray = outCells;

  // The first locator is what we will use to accumulate all others
  // So all iteration starts from second dataset.
  for (itr = second; itr != end; ++itr, ++mapIter)
  {
    mergeCells.CellOffsets = (*itr).CellOffsets;
    mergeCells.ConnOffsets = (*itr).ConnOffsets;
    mergeCells.InCellArray = (*itr).OutCellArray;
    mergeCells.OutputCellOffset = outCellOffset;
    mergeCells.OutputConnOffset = outConnOffset;
    mergeCells.IdMap = *mapIter;

    // First, we merge the cell arrays. This also adjust point ids.
    svtkSMPTools::For(0, mergeCells.CellOffsets->GetNumberOfIds(), mergeCells);

    outCellOffset += (*itr).OutCellArray->GetNumberOfCells();
    outConnOffset += (*itr).OutCellArray->GetNumberOfConnectivityIds();
  }

  svtkIdType outCellsOffset = cellDataOffset + (*begin).OutCellArray->GetNumberOfCells();

  // Now copy cell data in parallel
  svtkParallelCellDataCopier cellCopier;
  cellCopier.OutputCellData = (*begin).Output->GetCellData();
  int numCellArrays = cellCopier.OutputCellData->GetNumberOfArrays();
  if (numCellArrays > 0)
  {
    for (itr = second; itr != end; ++itr)
    {
      cellCopier.InputCellData = (*itr).Output->GetCellData();
      cellCopier.Offset = outCellsOffset;
      svtkCellArray* cells = (*itr).OutCellArray;

      svtkSMPTools::For(0, cells->GetNumberOfCells(), cellCopier);
      // cellCopier.operator()(0, polys->GetNumberOfCells());

      outCellsOffset += (*itr).Output->GetPolys()->GetNumberOfCells();
    }
  }
}
}

svtkPolyData* svtkSMPMergePolyDataHelper::MergePolyData(std::vector<InputData>& inputs)
{
  // First merge points

  std::vector<InputData>::iterator itr = inputs.begin();
  std::vector<InputData>::iterator begin = itr;
  std::vector<InputData>::iterator end = inputs.end();

  std::vector<svtkMergePointsData> mpData;
  while (itr != end)
  {
    mpData.push_back(svtkMergePointsData((*itr).Input, (*itr).Locator));
    ++itr;
  }

  std::vector<svtkIdList*> idMaps;
  svtkPolyData* outPolyData = svtkPolyData::New();

  MergePoints(mpData, idMaps, outPolyData);

  itr = begin;
  svtkIdType vertSize = 0;
  svtkIdType lineSize = 0;
  svtkIdType polySize = 0;
  svtkIdType numVerts = 0;
  svtkIdType numLines = 0;
  svtkIdType numPolys = 0;
  std::vector<svtkMergeCellsData> mcData;
  while (itr != end)
  {
    vertSize += (*itr).Input->GetVerts()->GetNumberOfConnectivityIds();
    lineSize += (*itr).Input->GetLines()->GetNumberOfConnectivityIds();
    polySize += (*itr).Input->GetPolys()->GetNumberOfConnectivityIds();
    numVerts += (*itr).Input->GetVerts()->GetNumberOfCells();
    numLines += (*itr).Input->GetLines()->GetNumberOfCells();
    numPolys += (*itr).Input->GetPolys()->GetNumberOfCells();
    ++itr;
  }

  svtkIdType numOutCells = numVerts + numLines + numPolys;

  svtkCellData* outCellData = (*begin).Input->GetCellData();
  int numCellArrays = outCellData->GetNumberOfArrays();
  for (int i = 0; i < numCellArrays; i++)
  {
    outCellData->GetArray(i)->Resize(numOutCells);
    outCellData->GetArray(i)->SetNumberOfTuples(numOutCells);
  }

  // Now merge each cell type. Because svtkPolyData stores each
  // cell type separately, we need to merge them separately.

  if (vertSize > 0)
  {
    svtkNew<svtkCellArray> outVerts;
    outVerts->ResizeExact(numVerts, vertSize);

    itr = begin;
    while (itr != end)
    {
      mcData.push_back(svtkMergeCellsData(
        (*itr).Input, (*itr).VertCellOffsets, (*itr).VertConnOffsets, (*itr).Input->GetVerts()));
      ++itr;
    }
    MergeCells(mcData, idMaps, 0, outVerts);

    outPolyData->SetVerts(outVerts);

    mcData.clear();
  }

  if (lineSize > 0)
  {
    svtkNew<svtkCellArray> outLines;
    outLines->ResizeExact(numLines, lineSize);

    itr = begin;
    while (itr != end)
    {
      mcData.push_back(svtkMergeCellsData(
        (*itr).Input, (*itr).LineCellOffsets, (*itr).LineConnOffsets, (*itr).Input->GetLines()));
      ++itr;
    }
    MergeCells(mcData, idMaps, vertSize, outLines);

    outPolyData->SetLines(outLines);

    mcData.clear();
  }

  if (polySize > 0)
  {
    svtkNew<svtkCellArray> outPolys;
    outPolys->ResizeExact(numPolys, polySize);

    itr = begin;
    while (itr != end)
    {
      mcData.push_back(svtkMergeCellsData(
        (*itr).Input, (*itr).PolyCellOffsets, (*itr).PolyConnOffsets, (*itr).Input->GetPolys()));
      ++itr;
    }
    MergeCells(mcData, idMaps, vertSize + lineSize, outPolys);

    outPolyData->SetPolys(outPolys);
  }

  outPolyData->GetCellData()->ShallowCopy(outCellData);

  std::vector<svtkIdList*>::iterator mapIter = idMaps.begin();
  while (mapIter != idMaps.end())
  {
    (*mapIter)->Delete();
    ++mapIter;
  }

  return outPolyData;
}
