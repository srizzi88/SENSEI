/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractUserDefinedPiece.cxx

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

#include "svtkExtractUserDefinedPiece.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkExtractUserDefinedPiece);

svtkExtractUserDefinedPiece::svtkExtractUserDefinedPiece()
{
  this->ConstantData = nullptr;
  this->ConstantDataLen = 0;
  this->InPiece = nullptr;
}
svtkExtractUserDefinedPiece::~svtkExtractUserDefinedPiece()
{
  delete[](char*) this->ConstantData;
  this->ConstantData = nullptr;
}
void svtkExtractUserDefinedPiece::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConstantData: " << this->ConstantData << indent
     << "ConstantDataLen: " << this->ConstantDataLen << indent << "InPiece: " << this->InPiece
     << "\n";
}
void svtkExtractUserDefinedPiece::SetConstantData(void* data, int len)
{
  this->ConstantData = new char[len];
  this->ConstantDataLen = len;

  memcpy(this->ConstantData, data, len);

  this->Modified();
}

int svtkExtractUserDefinedPiece::GetConstantData(void** data)
{
  *data = this->ConstantData;
  return this->ConstantDataLen;
}

// This is exactly svtkExtractUnstructuredGridPiece::Execute(), with
// the exception that we call ComputeCellTagsWithFunction rather
// than ComputeCellTags.  If ComputeCellTags were virtual, we could
// just override it here.

int svtkExtractUserDefinedPiece::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkUnstructuredGrid* input =
    svtkUnstructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();
  svtkIntArray* cellTags;
  int ghostLevel;
  svtkIdType cellId, newCellId;
  svtkIdList *cellPts, *pointMap;
  svtkIdList* newCellPts = svtkIdList::New();
  svtkIdList* pointOwnership;
  svtkCell* cell;
  svtkPoints* newPoints;
  svtkUnsignedCharArray* cellGhostLevels = nullptr;
  svtkUnsignedCharArray* pointGhostLevels = nullptr;
  svtkIdType i, ptId, newId, numPts;
  int numCellPts;
  double* x;

  // Pipeline update piece will tell us what to generate.
  ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  outPD->CopyAllocate(pd);
  outCD->CopyAllocate(cd);

  if (ghostLevel > 0 && this->CreateGhostCells)
  {
    cellGhostLevels = svtkUnsignedCharArray::New();
    pointGhostLevels = svtkUnsignedCharArray::New();
    cellGhostLevels->Allocate(input->GetNumberOfCells());
    pointGhostLevels->Allocate(input->GetNumberOfPoints());
  }

  // Break up cells based on which piece they belong to.
  cellTags = svtkIntArray::New();
  cellTags->Allocate(input->GetNumberOfCells(), 1000);
  pointOwnership = svtkIdList::New();
  pointOwnership->Allocate(input->GetNumberOfPoints());

  // Cell tags end up being 0 for cells in piece and -1 for all others.
  // Point ownership is the cell that owns the point.

  this->ComputeCellTagsWithFunction(cellTags, pointOwnership, input);

  // Find the layers of ghost cells.
  if (this->CreateGhostCells)
  {
    for (i = 0; i < ghostLevel; i++)
    {
      this->AddGhostLevel(input, cellTags, i + 1);
    }
  }

  // Filter the cells.

  numPts = input->GetNumberOfPoints();
  output->Allocate(input->GetNumberOfCells());
  newPoints = svtkPoints::New();
  newPoints->Allocate(numPts);

  pointMap = svtkIdList::New(); // maps old point ids into new
  pointMap->SetNumberOfIds(numPts);
  for (i = 0; i < numPts; i++)
  {
    pointMap->SetId(i, -1);
  }

  // Filter the cells
  for (cellId = 0; cellId < input->GetNumberOfCells(); cellId++)
  {
    if (cellTags->GetValue(cellId) != -1) // satisfied thresholding
    {
      if (cellGhostLevels)
      {
        cellGhostLevels->InsertNextValue(
          cellTags->GetValue(cellId) > 0 ? svtkDataSetAttributes::DUPLICATECELL : 0);
      }

      cell = input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      numCellPts = cell->GetNumberOfPoints();

      for (i = 0; i < numCellPts; i++)
      {
        ptId = cellPts->GetId(i);
        if ((newId = pointMap->GetId(ptId)) < 0)
        {
          x = input->GetPoint(ptId);
          newId = newPoints->InsertNextPoint(x);
          if (pointGhostLevels)
          {
            pointGhostLevels->InsertNextValue(cellTags->GetValue(pointOwnership->GetId(ptId)) > 0
                ? svtkDataSetAttributes::DUPLICATEPOINT
                : 0);
          }
          pointMap->SetId(ptId, newId);
          outPD->CopyData(pd, ptId, newId);
        }
        newCellPts->InsertId(i, newId);
      }
      newCellId = output->InsertNextCell(cell->GetCellType(), newCellPts);
      outCD->CopyData(cd, cellId, newCellId);
      newCellPts->Reset();
    } // satisfied thresholding
  }   // for all cells

  svtkDebugMacro(<< "Extracted " << output->GetNumberOfCells() << " number of cells.");

  // now clean up / update ourselves
  pointMap->Delete();
  newCellPts->Delete();

  if (cellGhostLevels)
  {
    cellGhostLevels->SetName(svtkDataSetAttributes::GhostArrayName());
    output->GetCellData()->AddArray(cellGhostLevels);
    cellGhostLevels->Delete();
    cellGhostLevels = nullptr;
  }
  if (pointGhostLevels)
  {
    pointGhostLevels->SetName(svtkDataSetAttributes::GhostArrayName());
    output->GetPointData()->AddArray(pointGhostLevels);
    pointGhostLevels->Delete();
    pointGhostLevels = nullptr;
  }
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->Squeeze();
  cellTags->Delete();
  pointOwnership->Delete();

  return 1;
}
void svtkExtractUserDefinedPiece::ComputeCellTagsWithFunction(
  svtkIntArray* tags, svtkIdList* pointOwnership, svtkUnstructuredGrid* input)
{
  int j;
  svtkIdType idx, numCells, ptId;
  svtkIdList* cellPtIds;

  numCells = input->GetNumberOfCells();

  cellPtIds = svtkIdList::New();
  // Clear Point ownership.
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
  {
    pointOwnership->SetId(idx, -1);
  }

  // Brute force division.
  for (idx = 0; idx < numCells; ++idx)
  {
    if (this->InPiece(idx, input, this->ConstantData))
    {
      tags->SetValue(idx, 0);
    }
    else
    {
      tags->SetValue(idx, -1);
    }
    // Fill in point ownership mapping.
    input->GetCellPoints(idx, cellPtIds);
    for (j = 0; j < cellPtIds->GetNumberOfIds(); ++j)
    {
      ptId = cellPtIds->GetId(j);
      if (pointOwnership->GetId(ptId) == -1)
      {
        pointOwnership->SetId(ptId, idx);
      }
    }
  }

  cellPtIds->Delete();
}
