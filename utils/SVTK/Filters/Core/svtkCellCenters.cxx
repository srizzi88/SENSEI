/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellCenters.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSMPTools.h"

svtkStandardNewMacro(svtkCellCenters);

namespace
{

class CellCenterFunctor
{
public:
  void operator()(svtkIdType begin, svtkIdType end)
  {
    if (this->DataSet == nullptr)
    {
      return;
    }

    if (this->CellCenters == nullptr)
    {
      return;
    }

    std::vector<double> weights(this->DataSet->GetMaxCellSize());
    svtkNew<svtkGenericCell> cell;
    for (svtkIdType cellId = begin; cellId < end; ++cellId)
    {
      this->DataSet->GetCell(cellId, cell);
      double x[3] = { 0.0 };
      if (cell->GetCellType() != SVTK_EMPTY_CELL)
      {
        double pcoords[3];
        int subId = cell->GetParametricCenter(pcoords);
        cell->EvaluateLocation(subId, pcoords, x, weights.data());
      }
      else
      {
        x[0] = 0.0;
        x[1] = 0.0;
        x[2] = 0.0;
      }
      this->CellCenters->SetTypedTuple(cellId, x);
    }
  }

  svtkDataSet* DataSet;
  svtkDoubleArray* CellCenters;
};

} // end anonymous namespace

//----------------------------------------------------------------------------
void svtkCellCenters::ComputeCellCenters(svtkDataSet* dataset, svtkDoubleArray* centers)
{
  CellCenterFunctor functor;
  functor.DataSet = dataset;
  functor.CellCenters = centers;

  // Call this once one the main thread before calling on multiple threads.
  // According to the documentation for svtkDataSet::GetCell(svtkIdType, svtkGenericCell*),
  // this is required to make this call subsequently thread safe
  if (dataset->GetNumberOfCells() > 0)
  {
    svtkNew<svtkGenericCell> cell;
    dataset->GetCell(0, cell);
  }

  // Now split the work among threads.
  svtkSMPTools::For(0, dataset->GetNumberOfCells(), functor);
}

//----------------------------------------------------------------------------
// Generate points
int svtkCellCenters::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input and output
  svtkDataSet* input = svtkDataSet::GetData(inputVector[0]);
  svtkPolyData* output = svtkPolyData::GetData(outputVector);

  svtkCellData* inCD = input->GetCellData();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* outCD = output->GetCellData();
  svtkIdType numCells = input->GetNumberOfCells();

  if (numCells == 0)
  {
    svtkDebugMacro(<< "No cells to generate center points for");
    return 1;
  }

  svtkNew<svtkPoints> newPts;
  newPts->SetDataTypeToDouble();
  newPts->SetNumberOfPoints(numCells);
  svtkDoubleArray* pointArray = svtkDoubleArray::SafeDownCast(newPts->GetData());

  svtkNew<svtkIdList> pointIdList;
  pointIdList->SetNumberOfIds(numCells);

  svtkNew<svtkIdList> cellIdList;
  cellIdList->SetNumberOfIds(numCells);

  svtkCellCenters::ComputeCellCenters(input, pointArray);

  // Remove points that would have been produced by empty cells
  // This should be multithreaded someday
  bool hasEmptyCells = false;
  svtkTypeBool abort = 0;
  svtkIdType progressInterval = numCells / 10 + 1;
  svtkIdType numPoints = 0;
  for (svtkIdType cellId = 0; cellId < numCells && !abort; ++cellId)
  {
    if (!(cellId % progressInterval))
    {
      svtkDebugMacro(<< "Processing #" << cellId);
      this->UpdateProgress((0.5 * cellId / numCells) + 0.5);
      abort = this->GetAbortExecute();
    }

    if (input->GetCellType(cellId) != SVTK_EMPTY_CELL)
    {
      newPts->SetPoint(numPoints, newPts->GetPoint(cellId));
      pointIdList->SetId(numPoints, numPoints);
      cellIdList->SetId(numPoints, cellId);
      numPoints++;
    }
    else
    {
      hasEmptyCells = true;
    }
  }

  if (abort)
  {
    return 0;
  }

  newPts->Resize(numPoints);
  pointIdList->Resize(numPoints);
  cellIdList->Resize(numPoints);
  output->SetPoints(newPts);

  if (this->CopyArrays)
  {
    if (hasEmptyCells)
    {
      outPD->CopyAllocate(inCD, numPoints);
      outPD->CopyData(inCD, cellIdList, pointIdList);
    }
    else
    {
      outPD->PassData(inCD); // because number of points == number of cells
    }
  }

  if (this->VertexCells)
  {
    svtkNew<svtkIdTypeArray> iArray;
    iArray->SetNumberOfComponents(1);
    iArray->SetNumberOfTuples(numPoints * 2);
    for (svtkIdType i = 0; i < numPoints; i++)
    {
      iArray->SetValue(2 * i, 1);
      iArray->SetValue(2 * i + 1, i);
    }

    svtkNew<svtkCellArray> verts;
    verts->AllocateEstimate(numPoints, 1);
    verts->ImportLegacyFormat(iArray);
    output->SetVerts(verts);
    outCD->ShallowCopy(outPD);
  }

  output->Squeeze();
  this->UpdateProgress(1.0);
  return 1;
}

//----------------------------------------------------------------------------
int svtkCellCenters::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkCellCenters::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
  os << indent << "CopyArrays: " << (this->CopyArrays ? "On" : "Off") << endl;
}
