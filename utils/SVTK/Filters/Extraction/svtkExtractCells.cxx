/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkExtractCells.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPTools.h"
#include "svtkTimeStamp.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>

namespace
{
struct FastPointMap
{
  using ConstIteratorType = const svtkIdType*;

  svtkNew<svtkIdList> Map;
  svtkIdType LastInput;
  svtkIdType LastOutput;

  ConstIteratorType CBegin() const { return this->Map->GetPointer(0); }

  ConstIteratorType CEnd() const { return this->Map->GetPointer(this->Map->GetNumberOfIds()); }

  svtkIdType* Reset(svtkIdType numValues)
  {
    this->LastInput = -1;
    this->LastOutput = -1;
    this->Map->SetNumberOfIds(numValues);
    return this->Map->GetPointer(0);
  }

  // Map inputId to the new PointId. If inputId is invalid, return -1.
  svtkIdType LookUp(svtkIdType inputId)
  {
    svtkIdType outputId = -1;
    ConstIteratorType first;
    ConstIteratorType last;

    if (this->LastOutput >= 0)
    {
      // Here's the optimization: since the point ids are usually requested
      // with some locality, we can reduce the search range by caching the
      // results of the last lookup. This reduces the number of lookups and
      // improves CPU cache behavior.

      // Offset is the distance (in input space) between the last lookup and
      // the current id. Since the point map is sorted and unique, this is the
      // maximum distance that the current ID can be from the previous one.
      svtkIdType offset = inputId - this->LastInput;

      // Our search range is from the last output location
      first = this->CBegin() + this->LastOutput;
      last = first + offset;

      // Ensure these are correctly ordered (offset may be < 0):
      if (last < first)
      {
        std::swap(first, last);
      }

      // Adjust last to be past-the-end:
      ++last;

      // Clamp to map bounds:
      first = std::max(first, this->CBegin());
      last = std::min(last, this->CEnd());
    }
    else
    { // First run, use full range:
      first = this->CBegin();
      last = this->CEnd();
    }

    outputId = this->BinaryFind(first, last, inputId);
    if (outputId >= 0)
    {
      this->LastInput = inputId;
      this->LastOutput = outputId;
    }

    return outputId;
  }

private:
  // Modified version of std::lower_bound that returns as soon as a value is
  // found (rather than finding the beginning of a sequence). Returns the
  // position in the list, or -1 if not found.
  svtkIdType BinaryFind(ConstIteratorType first, ConstIteratorType last, svtkIdType val) const
  {
    svtkIdType len = last - first;

    while (len > 0)
    {
      // Select median
      svtkIdType half = len / 2;
      ConstIteratorType middle = first + half;

      const svtkIdType& mVal = *middle;
      if (mVal < val)
      { // This soup is too cold.
        first = middle;
        ++first;
        len = len - half - 1;
      }
      else if (val < mVal)
      { // This soup is too hot!
        len = half;
      }
      else
      { // This soup is juuuust right.
        return middle - this->Map->GetPointer(0);
      }
    }

    return -1;
  }
};
} // end anonymous namespace

class svtkExtractCellsSTLCloak
{
  svtkTimeStamp SortTime;

public:
  std::vector<svtkIdType> CellIds;
  std::pair<typename std::vector<svtkIdType>::const_iterator,
    typename std::vector<svtkIdType>::const_iterator>
    CellIdsRange;
  FastPointMap PointMap;

  svtkIdType Prepare(svtkIdType numInputCells, svtkExtractCells* self)
  {
    assert(numInputCells > 0);

    if (self->GetAssumeSortedAndUniqueIds() == false && (self->GetMTime() > this->SortTime))
    {
      svtkSMPTools::Sort(this->CellIds.begin(), this->CellIds.end());
      auto last = std::unique(this->CellIds.begin(), this->CellIds.end());
      this->CellIds.erase(last, this->CellIds.end());
      this->SortTime.Modified();
    }

    this->CellIdsRange =
      std::make_pair(std::lower_bound(this->CellIds.begin(), this->CellIds.end(), 0),
        std::upper_bound(this->CellIds.begin(), this->CellIds.end(), (numInputCells - 1)));
    return static_cast<svtkIdType>(
      std::distance(this->CellIdsRange.first, this->CellIdsRange.second));
  }
};

svtkStandardNewMacro(svtkExtractCells);
//----------------------------------------------------------------------------
svtkExtractCells::svtkExtractCells()
{
  this->CellList = new svtkExtractCellsSTLCloak;
}

//----------------------------------------------------------------------------
svtkExtractCells::~svtkExtractCells()
{
  delete this->CellList;
}

//----------------------------------------------------------------------------
void svtkExtractCells::SetCellList(svtkIdList* l)
{
  delete this->CellList;
  this->CellList = new svtkExtractCellsSTLCloak;
  if (l != nullptr)
  {
    this->AddCellList(l);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractCells::AddCellList(svtkIdList* l)
{
  const svtkIdType inputSize = l ? l->GetNumberOfIds() : 0;
  if (inputSize == 0)
  {
    return;
  }

  auto& cellIds = this->CellList->CellIds;
  const svtkIdType* inputBegin = l->GetPointer(0);
  const svtkIdType* inputEnd = inputBegin + inputSize;
  std::copy(inputBegin, inputEnd, std::back_inserter(cellIds));
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractCells::SetCellIds(const svtkIdType* ptr, svtkIdType numValues)
{
  delete this->CellList;
  this->CellList = new svtkExtractCellsSTLCloak;
  if (ptr != nullptr && numValues > 0)
  {
    this->AddCellIds(ptr, numValues);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractCells::AddCellIds(const svtkIdType* ptr, svtkIdType numValues)
{
  auto& cellIds = this->CellList->CellIds;
  std::copy(ptr, ptr + numValues, std::back_inserter(cellIds));
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractCells::AddCellRange(svtkIdType from, svtkIdType to)
{
  if (to < from || to < 0)
  {
    svtkWarningMacro("Bad cell range: (" << to << "," << from << ")");
    return;
  }

  // This range specification is inconsistent with C++. Left for backward
  // compatibility reasons.  Add 1 to `to` to make it consistent.
  ++to;

  auto& cellIds = this->CellList->CellIds;
  std::generate_n(std::back_inserter(cellIds), (to - from), [&from]() { return from++; });
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkExtractCells::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input and output
  svtkDataSet* input = svtkDataSet::GetData(inputVector[0]);
  svtkUnstructuredGrid* output = svtkUnstructuredGrid::GetData(outputVector);
  svtkPointData* newPD = output->GetPointData();
  svtkCellData* newCD = output->GetCellData();

  this->InputIsUgrid = ((svtkUnstructuredGrid::SafeDownCast(input)) != nullptr);

  // copy all arrays, including global ids etc.
  newPD->CopyAllOn();
  newCD->CopyAllOn();

  if (this->ExtractAllCells)
  {
    this->Copy(input, output);
    return 1;
  }

  const svtkIdType numCellsInput = input->GetNumberOfCells();
  const svtkIdType numCells = this->CellList->Prepare(numCellsInput, this);
  if (numCells == numCellsInput)
  {
    this->Copy(input, output);
    return 1;
  }

  svtkPointData* inPD = input->GetPointData();
  svtkCellData* inCD = input->GetCellData();

  if (numCells == 0)
  {
    // set up a ugrid with same data arrays as input, but
    // no points, cells or data.
    output->Allocate(1);
    output->GetPointData()->CopyAllocate(inPD, SVTK_CELL_SIZE);
    output->GetCellData()->CopyAllocate(inCD, 1);

    svtkNew<svtkPoints> pts;
    pts->SetNumberOfPoints(0);
    output->SetPoints(pts);
    return 1;
  }

  svtkIdType numPoints = this->ReMapPointIds(input);
  newPD->CopyAllocate(inPD, numPoints);
  newCD->CopyAllocate(inCD, numCells);

  svtkNew<svtkPoints> pts;
  if (svtkPointSet* inputPS = svtkPointSet::SafeDownCast(input))
  {
    // preserve input datatype
    pts->SetDataType(inputPS->GetPoints()->GetDataType());
  }
  pts->SetNumberOfPoints(numPoints);
  output->SetPoints(pts);

  // Copy points and point data:
  svtkPointSet* pointSet = svtkPointSet::SafeDownCast(input);
  if (pointSet)
  {
    // Optimize when a svtkPoints object exists in the input:
    svtkNew<svtkIdList> dstIds; // contiguous range [0, numPoints)
    dstIds->SetNumberOfIds(numPoints);
    std::iota(dstIds->GetPointer(0), dstIds->GetPointer(numPoints), 0);

    pts->InsertPoints(dstIds, this->CellList->PointMap.Map, pointSet->GetPoints());
    newPD->CopyData(inPD, this->CellList->PointMap.Map, dstIds);
  }
  else
  {
    // Slow path if we have to query the dataset:
    for (svtkIdType newId = 0; newId < numPoints; ++newId)
    {
      svtkIdType oldId = this->CellList->PointMap.Map->GetId(newId);
      pts->SetPoint(newId, input->GetPoint(oldId));
      newPD->CopyData(inPD, oldId, newId);
    }
  }

  if (this->InputIsUgrid)
  {
    this->CopyCellsUnstructuredGrid(input, output);
  }
  else
  {
    this->CopyCellsDataSet(input, output);
  }

  this->CellList->PointMap.Reset(0);
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractCells::Copy(svtkDataSet* input, svtkUnstructuredGrid* output)
{
  // If input is unstructured grid just deep copy through
  if (this->InputIsUgrid)
  {
    output->ShallowCopy(input);
    return;
  }

  svtkIdType numPoints = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  output->Allocate(numCells);

  if (auto inputPS = svtkPointSet::SafeDownCast(input))
  {
    svtkNew<svtkPoints> pts;
    pts->ShallowCopy(inputPS->GetPoints());
    output->SetPoints(pts);
  }
  else
  {
    svtkNew<svtkPoints> pts;
    pts->SetDataTypeToDouble();
    pts->SetNumberOfPoints(numPoints);

    double temp[3];
    pts->GetPoint(0, temp);

    auto array = svtkDoubleArray::SafeDownCast(pts->GetData());
    assert(array && array->GetNumberOfTuples() == numPoints);
    svtkSMPTools::For(0, numPoints, [&array, &input](svtkIdType first, svtkIdType last) {
      double coords[3];
      for (svtkIdType cc = first; cc < last; ++cc)
      {
        input->GetPoint(cc, coords);
        array->SetTypedTuple(cc, coords);
      }
    });
    output->SetPoints(pts);
  }

  svtkNew<svtkIdList> cellPoints;
  for (svtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    input->GetCellPoints(cellId, cellPoints);
    output->InsertNextCell(input->GetCellType(cellId), cellPoints);
  }
  output->Squeeze();

  // copy cell/point arrays.
  output->GetPointData()->ShallowCopy(input->GetPointData());
  output->GetCellData()->ShallowCopy(input->GetCellData());
}

//----------------------------------------------------------------------------
svtkIdType svtkExtractCells::ReMapPointIds(svtkDataSet* grid)
{
  svtkIdType totalPoints = grid->GetNumberOfPoints();
  std::vector<char> temp(totalPoints, 0);

  svtkIdType numberOfIds = 0;

  if (!this->InputIsUgrid)
  {
    svtkNew<svtkIdList> ptIds;
    const auto& range = this->CellList->CellIdsRange;
    for (auto iter = range.first; iter != range.second; ++iter)
    {
      const auto cellId = (*iter);
      grid->GetCellPoints(cellId, ptIds);

      svtkIdType npts = ptIds->GetNumberOfIds();
      svtkIdType* pts = ptIds->GetPointer(0);

      for (svtkIdType i = 0; i < npts; ++i)
      {
        svtkIdType pid = pts[i];
        if (temp[pid] == 0)
        {
          ++numberOfIds;
          temp[pid] = 1;
        }
      }
    }
  }
  else
  {
    svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::SafeDownCast(grid);
    svtkIdType maxid = ugrid->GetNumberOfCells();

    this->SubSetUGridCellArraySize = 0;
    this->SubSetUGridFacesArraySize = 0;

    const auto& range = this->CellList->CellIdsRange;
    for (auto iter = range.first; iter != range.second; ++iter)
    {
      const auto cellId = (*iter);
      if (cellId > maxid)
      {
        continue;
      }

      svtkIdType npts;
      const svtkIdType* pts;
      ugrid->GetCellPoints(cellId, npts, pts);

      this->SubSetUGridCellArraySize += (1 + npts);

      for (svtkIdType i = 0; i < npts; ++i)
      {
        svtkIdType pid = pts[i];
        if (temp[pid] == 0)
        {
          ++numberOfIds;
          temp[pid] = 1;
        }
      }

      if (ugrid->GetCellType(cellId) == SVTK_POLYHEDRON)
      {
        svtkIdType nfaces;
        const svtkIdType* ptids;
        ugrid->GetFaceStream(cellId, nfaces, ptids);
        this->SubSetUGridFacesArraySize += 1;
        for (svtkIdType j = 0; j < nfaces; j++)
        {
          svtkIdType nfpts = *ptids;
          this->SubSetUGridFacesArraySize += nfpts + 1;
          ptids += nfpts + 1;
        }
      }
    }
  }

  svtkIdType* pointMap = this->CellList->PointMap.Reset(numberOfIds);
  for (svtkIdType pid = 0; pid < totalPoints; pid++)
  {
    if (temp[pid])
    {
      (*pointMap++) = pid;
    }
  }

  return numberOfIds;
}

//----------------------------------------------------------------------------
void svtkExtractCells::CopyCellsDataSet(svtkDataSet* input, svtkUnstructuredGrid* output)
{
  const auto& range = this->CellList->CellIdsRange;
  output->Allocate(static_cast<svtkIdType>(std::distance(range.first, range.second)));

  svtkCellData* oldCD = input->GetCellData();
  svtkCellData* newCD = output->GetCellData();

  // We only create svtkOriginalCellIds for the output data set if it does not
  // exist in the input data set. If it is in the input data set then we
  // let CopyData() take care of copying it over.
  svtkIdTypeArray* origMap = nullptr;
  if (oldCD->GetArray("svtkOriginalCellIds") == nullptr)
  {
    origMap = svtkIdTypeArray::New();
    origMap->SetNumberOfComponents(1);
    origMap->SetName("svtkOriginalCellIds");
    newCD->AddArray(origMap);
    origMap->Delete();
  }

  svtkNew<svtkIdList> cellPoints;

  for (auto iter = range.first; iter != range.second; ++iter)
  {
    const auto cellId = (*iter);
    input->GetCellPoints(cellId, cellPoints);

    for (svtkIdType i = 0; i < cellPoints->GetNumberOfIds(); i++)
    {
      svtkIdType oldId = cellPoints->GetId(i);

      svtkIdType newId = this->CellList->PointMap.LookUp(oldId);
      assert("Old id exists in map." && newId >= 0);

      cellPoints->SetId(i, newId);
    }
    svtkIdType newId = output->InsertNextCell(input->GetCellType(cellId), cellPoints);

    newCD->CopyData(oldCD, cellId, newId);
    if (origMap)
    {
      origMap->InsertNextValue(cellId);
    }
  }
}

//----------------------------------------------------------------------------
void svtkExtractCells::CopyCellsUnstructuredGrid(svtkDataSet* input, svtkUnstructuredGrid* output)
{
  svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::SafeDownCast(input);
  if (ugrid == nullptr)
  {
    this->CopyCellsDataSet(input, output);
    return;
  }

  svtkCellData* oldCD = input->GetCellData();
  svtkCellData* newCD = output->GetCellData();

  // We only create svtkOriginalCellIds for the output data set if it does not
  // exist in the input data set. If it is in the input data set then we
  // let CopyData() take care of copying it over.
  svtkIdTypeArray* origMap = nullptr;
  if (oldCD->GetArray("svtkOriginalCellIds") == nullptr)
  {
    origMap = svtkIdTypeArray::New();
    origMap->SetNumberOfComponents(1);
    origMap->SetName("svtkOriginalCellIds");
    newCD->AddArray(origMap);
    origMap->Delete();
  }

  const auto& range = this->CellList->CellIdsRange;
  const auto numCells = static_cast<svtkIdType>(std::distance(range.first, range.second));

  svtkNew<svtkCellArray> cellArray; // output
  svtkNew<svtkIdTypeArray> newcells;
  newcells->SetNumberOfValues(this->SubSetUGridCellArraySize);
  svtkIdType cellArrayIdx = 0;

  svtkNew<svtkIdTypeArray> facesLocationArray;
  facesLocationArray->SetNumberOfValues(numCells);
  svtkNew<svtkIdTypeArray> facesArray;
  facesArray->SetNumberOfValues(this->SubSetUGridFacesArraySize);
  svtkNew<svtkUnsignedCharArray> typeArray;
  typeArray->SetNumberOfValues(numCells);

  svtkIdType nextCellId = 0;
  svtkIdType nextFaceId = 0;

  svtkIdType maxid = ugrid->GetNumberOfCells();
  bool havePolyhedron = false;

  for (auto iter = range.first; iter != range.second; ++iter)
  {
    const auto oldCellId = (*iter);
    if (oldCellId >= maxid)
    {
      continue;
    }

    unsigned char cellType = ugrid->GetCellType(oldCellId);
    typeArray->SetValue(nextCellId, cellType);

    svtkIdType npts;
    const svtkIdType* pts;
    ugrid->GetCellPoints(oldCellId, npts, pts);

    newcells->SetValue(cellArrayIdx++, npts);

    for (svtkIdType i = 0; i < npts; i++)
    {
      svtkIdType oldId = pts[i];
      svtkIdType newId = this->CellList->PointMap.LookUp(oldId);
      assert("Old id exists in map." && newId >= 0);
      newcells->SetValue(cellArrayIdx++, newId);
    }

    if (cellType == SVTK_POLYHEDRON)
    {
      havePolyhedron = true;
      svtkIdType nfaces;
      const svtkIdType* ptids;
      ugrid->GetFaceStream(oldCellId, nfaces, ptids);

      facesLocationArray->SetValue(nextCellId, nextFaceId);
      facesArray->SetValue(nextFaceId++, nfaces);

      for (svtkIdType j = 0; j < nfaces; j++)
      {
        svtkIdType nfpts = *ptids++;
        facesArray->SetValue(nextFaceId++, nfpts);
        for (svtkIdType i = 0; i < nfpts; i++)
        {
          svtkIdType oldId = *ptids++;
          svtkIdType newId = this->CellList->PointMap.LookUp(oldId);
          assert("Old id exists in map." && newId >= 0);
          facesArray->SetValue(nextFaceId++, newId);
        }
      }
    }
    else
    {
      facesLocationArray->SetValue(nextCellId, -1);
    }

    newCD->CopyData(oldCD, oldCellId, nextCellId);
    if (origMap)
    {
      origMap->InsertNextValue(oldCellId);
    }
    nextCellId++;
  }

  cellArray->AllocateExact(numCells, newcells->GetNumberOfValues() - numCells);
  cellArray->ImportLegacyFormat(newcells);

  if (havePolyhedron)
  {
    output->SetCells(typeArray, cellArray, facesLocationArray, facesArray);
  }
  else
  {
    output->SetCells(typeArray, cellArray, nullptr, nullptr);
  }
}

//----------------------------------------------------------------------------
int svtkExtractCells::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractCells::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ExtractAllCells: " << this->ExtractAllCells << endl;
  os << indent << "AssumeSortedAndUniqueIds: " << this->AssumeSortedAndUniqueIds << endl;
}
