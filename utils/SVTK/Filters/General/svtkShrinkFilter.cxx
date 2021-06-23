/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShrinkFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkShrinkFilter.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkShrinkFilter);

//----------------------------------------------------------------------------
svtkShrinkFilter::svtkShrinkFilter()
{
  this->ShrinkFactor = 0.5;
}

//----------------------------------------------------------------------------
svtkShrinkFilter::~svtkShrinkFilter() = default;

//----------------------------------------------------------------------------
void svtkShrinkFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}

//----------------------------------------------------------------------------
int svtkShrinkFilter::FillInputPortInformation(int, svtkInformation* info)
{
  // This filter uses the svtkDataSet cell traversal methods so it
  // suppors any data set type as input.
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkShrinkFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get input and output data.
  svtkDataSet* input = svtkDataSet::GetData(inputVector[0]);
  svtkUnstructuredGrid* output = svtkUnstructuredGrid::GetData(outputVector);

  // We are now executing this filter.
  svtkDebugMacro("Shrinking cells");

  // Skip execution if there is no input geometry.
  svtkIdType numCells = input->GetNumberOfCells();
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numCells < 1 || numPts < 1)
  {
    svtkDebugMacro("No data to shrink!");
    return 1;
  }

  // Allocate working space for new and old cell point lists.
  svtkSmartPointer<svtkIdList> ptIds = svtkSmartPointer<svtkIdList>::New();
  svtkSmartPointer<svtkIdList> newPtIds = svtkSmartPointer<svtkIdList>::New();
  ptIds->Allocate(SVTK_CELL_SIZE);
  newPtIds->Allocate(SVTK_CELL_SIZE);

  // Allocate approximately the space needed for the output cells.
  output->Allocate(numCells);

  // Allocate space for a new set of points.
  svtkSmartPointer<svtkPoints> newPts = svtkSmartPointer<svtkPoints>::New();
  newPts->Allocate(numPts * 8, numPts);

  // Allocate space for data associated with the new set of points.
  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(inPD, numPts * 8, numPts);

  // Support progress and abort.
  svtkIdType tenth = (numCells >= 10 ? numCells / 10 : 1);
  double numCellsInv = 1.0 / numCells;
  int abort = 0;

  // Point Id map.
  svtkIdType* pointMap = new svtkIdType[input->GetNumberOfPoints()];

  // Traverse all cells, obtaining node coordinates.  Compute "center"
  // of cell, then create new vertices shrunk towards center.
  for (svtkIdType cellId = 0; cellId < numCells && !abort; ++cellId)
  {
    // Get the list of points for this cell.
    input->GetCellPoints(cellId, ptIds);
    svtkIdType numIds = ptIds->GetNumberOfIds();

    // Periodically update progress and check for an abort request.
    if (cellId % tenth == 0)
    {
      this->UpdateProgress((cellId + 1) * numCellsInv);
      abort = this->GetAbortExecute();
    }

    // Compute the center of mass of the cell points.
    double center[3] = { 0, 0, 0 };
    for (svtkIdType i = 0; i < numIds; ++i)
    {
      double p[3];
      input->GetPoint(ptIds->GetId(i), p);
      for (int j = 0; j < 3; ++j)
      {
        center[j] += p[j];
      }
    }
    for (int j = 0; j < 3; ++j)
    {
      center[j] /= numIds;
    }

    // Create new points for this cell.
    newPtIds->Reset();
    for (svtkIdType i = 0; i < numIds; ++i)
    {
      // Get the old point location.
      double p[3];
      input->GetPoint(ptIds->GetId(i), p);

      // Compute the new point location.
      double newPt[3];
      for (int j = 0; j < 3; ++j)
      {
        newPt[j] = center[j] + this->ShrinkFactor * (p[j] - center[j]);
      }

      // Create the new point for this cell.
      svtkIdType newId = newPts->InsertNextPoint(newPt);

      // Copy point data from the old point.
      svtkIdType oldId = ptIds->GetId(i);
      outPD->CopyData(inPD, oldId, newId);

      pointMap[oldId] = newId;
    }

    // special handling for polyhedron cells
    if (svtkUnstructuredGrid::SafeDownCast(input) && input->GetCellType(cellId) == SVTK_POLYHEDRON)
    {
      svtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(cellId, newPtIds);
      svtkUnstructuredGrid::ConvertFaceStreamPointIds(newPtIds, pointMap);
    }
    else
    {
      for (svtkIdType i = 0; i < numIds; ++i)
      {
        newPtIds->InsertId(i, pointMap[ptIds->GetId(i)]);
      }
    }

    // Store the new cell in the output.
    output->InsertNextCell(input->GetCellType(cellId), newPtIds);
  }

  // Store the new set of points in the output.
  output->SetPoints(newPts);

  // Just pass cell data through because we still have the same number
  // and type of cells.
  output->GetCellData()->PassData(input->GetCellData());

  // Avoid keeping extra memory around.
  output->Squeeze();

  delete[] pointMap;

  return 1;
}
