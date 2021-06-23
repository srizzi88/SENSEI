/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplitByCellScalarFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSplitByCellScalarFilter.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkUnstructuredGrid.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

svtkStandardNewMacro(svtkSplitByCellScalarFilter);

//----------------------------------------------------------------------------
svtkSplitByCellScalarFilter::svtkSplitByCellScalarFilter()
{
  this->PassAllPoints = true;
  // by default process active cells scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
svtkSplitByCellScalarFilter::~svtkSplitByCellScalarFilter() = default;

//----------------------------------------------------------------------------
int svtkSplitByCellScalarFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input and output
  svtkDataSet* input = svtkDataSet::GetData(inputVector[0], 0);
  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector, 0);

  svtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);

  if (!inScalars)
  {
    svtkErrorMacro(<< "No scalar data to process.");
    return 1;
  }

  double range[2];
  inScalars->GetRange(range);

  svtkIdType nbCells = input->GetNumberOfCells();

  // Fetch the existing scalar ids
  std::map<svtkIdType, int> scalarValuesToBlockId;
  int nbBlocks = 0;
  for (svtkIdType i = 0; i < nbCells; i++)
  {
    svtkIdType v = static_cast<svtkIdType>(inScalars->GetTuple1(i));
    if (scalarValuesToBlockId.find(v) == scalarValuesToBlockId.end())
    {
      scalarValuesToBlockId[v] = nbBlocks;
      nbBlocks++;
    }
  }
  if (nbBlocks == 0)
  {
    svtkDebugMacro(<< "No block found.");
    return 1;
  }

  svtkPointData* inPD = input->GetPointData();
  svtkCellData* inCD = input->GetCellData();
  svtkPointSet* inputPS = svtkPointSet::SafeDownCast(input);
  svtkPolyData* inputPD = svtkPolyData::SafeDownCast(input);
  svtkUnstructuredGrid* inputUG = svtkUnstructuredGrid::SafeDownCast(input);

  // Create one UnstructuredGrid block per scalar ids
  std::vector<svtkPointSet*> blocks(nbBlocks);
  std::map<svtkIdType, int>::const_iterator it = scalarValuesToBlockId.begin();
  bool passAllPoints = inputPS && inputPS->GetPoints() && this->PassAllPoints;
  for (int i = 0; i < nbBlocks; i++, ++it)
  {
    svtkSmartPointer<svtkPointSet> ds;
    ds.TakeReference(inputPD ? static_cast<svtkPointSet*>(svtkPolyData::New())
                             : static_cast<svtkPointSet*>(svtkUnstructuredGrid::New()));
    if (passAllPoints)
    {
      ds->SetPoints(inputPS->GetPoints());
      ds->GetPointData()->ShallowCopy(inPD);
    }
    else
    {
      svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
      points->SetDataTypeToDouble();
      ds->SetPoints(points);
      ds->GetPointData()->CopyGlobalIdsOn();
      ds->GetPointData()->CopyAllocate(inPD);
    }
    if (inputPD)
    {
      svtkPolyData::SafeDownCast(ds)->AllocateCopy(inputPD);
    }
    ds->GetCellData()->CopyGlobalIdsOn();
    ds->GetCellData()->CopyAllocate(inCD);
    blocks[it->second] = ds;
    output->SetBlock(it->second, ds);
    std::stringstream ss;
    ss << inScalars->GetName() << "_" << it->first;
    output->GetMetaData(it->second)->Set(svtkCompositeDataSet::NAME(), ss.str().c_str());
  }

  svtkSmartPointer<svtkIdList> newCellPts = svtkSmartPointer<svtkIdList>::New();
  std::vector<std::map<svtkIdType, svtkIdType> > pointMaps(nbBlocks);

  int abortExecute = this->GetAbortExecute();
  svtkIdType progressInterval = nbCells / 100;

  // Check that the scalars of each cell satisfy the threshold criterion
  for (svtkIdType cellId = 0; cellId < nbCells && !abortExecute; cellId++)
  {
    if (cellId % progressInterval == 0)
    {
      this->UpdateProgress(static_cast<double>(cellId) / nbCells);
      abortExecute = this->GetAbortExecute();
    }
    int cellType = input->GetCellType(cellId);
    svtkIdType v = static_cast<svtkIdType>(inScalars->GetTuple1(cellId));
    int cellBlock = scalarValuesToBlockId[v];
    svtkPointSet* outDS = blocks[cellBlock];
    svtkPolyData* outPD = svtkPolyData::SafeDownCast(outDS);
    svtkUnstructuredGrid* outUG = svtkUnstructuredGrid::SafeDownCast(outDS);
    std::map<svtkIdType, svtkIdType>& pointMap = pointMaps[cellBlock];
    svtkCell* cell = input->GetCell(cellId);
    svtkIdList* cellPts = cell->GetPointIds();

    if (!passAllPoints)
    {
      svtkPointData* outPData = outDS->GetPointData();
      svtkPoints* outPoints = outDS->GetPoints();
      svtkIdType numCellPts = cellPts->GetNumberOfIds();
      newCellPts->Reset();
      for (svtkIdType i = 0; i < numCellPts; i++)
      {
        svtkIdType ptId = cellPts->GetId(i);
        std::map<svtkIdType, svtkIdType>::const_iterator ptIt = pointMap.find(ptId);
        svtkIdType newId = -1;
        if (ptIt == pointMap.end())
        {
          double x[3];
          input->GetPoint(ptId, x);
          newId = outPoints->InsertNextPoint(x);
          pointMap[ptId] = newId;
          outPData->CopyData(inPD, ptId, newId);
        }
        else
        {
          newId = ptIt->second;
        }
        newCellPts->InsertId(i, newId);
      }
    }

    svtkIdType newCellId = -1;
    // special handling for polyhedron cells
    if (inputUG && cellType == SVTK_POLYHEDRON)
    {
      inputUG->GetFaceStream(cellId, newCellPts.Get());
      if (!passAllPoints)
      {
        // ConvertFaceStreamPointIds using local point map
        svtkIdType* idPtr = newCellPts->GetPointer(0);
        svtkIdType nfaces = *idPtr++;
        for (svtkIdType i = 0; i < nfaces; i++)
        {
          svtkIdType npts = *idPtr++;
          for (svtkIdType j = 0; j < npts; j++)
          {
            *idPtr = pointMap[*idPtr];
            idPtr++;
          }
        }
      }
      newCellId = outUG->InsertNextCell(cellType, newCellPts);
      newCellPts->Reset();
    }
    else
    {
      if (outPD)
      {
        newCellId = outPD->InsertNextCell(cellType, passAllPoints ? cellPts : newCellPts.Get());
      }
      else
      {
        newCellId = outUG->InsertNextCell(cellType, passAllPoints ? cellPts : newCellPts.Get());
      }
    }
    outDS->GetCellData()->CopyData(inCD, cellId, newCellId);
  }

  for (int i = 0; i < nbBlocks; i++)
  {
    blocks[i]->Squeeze();
  }

  this->UpdateProgress(1.);
  return 1;
}

//----------------------------------------------------------------------------
int svtkSplitByCellScalarFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkSplitByCellScalarFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Pass All Points: " << (this->GetPassAllPoints() ? "On" : "Off") << std::endl;
}
