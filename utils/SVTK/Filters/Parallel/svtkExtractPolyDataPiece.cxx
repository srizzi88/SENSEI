/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPolyDataPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractPolyDataPiece.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkOBBDicer.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkExtractPolyDataPiece);

//=============================================================================
svtkExtractPolyDataPiece::svtkExtractPolyDataPiece()
{
  this->CreateGhostCells = 1;
}

//=============================================================================
int svtkExtractPolyDataPiece::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // get the info object
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//=============================================================================
void svtkExtractPolyDataPiece::ComputeCellTags(
  svtkIntArray* tags, svtkIdList* pointOwnership, int piece, int numPieces, svtkPolyData* input)
{
  svtkIdType idx, j, numCells, ptId;
  svtkIdList* cellPtIds;

  numCells = input->GetNumberOfCells();

  cellPtIds = svtkIdList::New();
  // Clear Point ownership.
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
  {
    pointOwnership->SetId(idx, -1);
  }

  // Brute force division.
  // The first N cells go to piece 0 ...
  for (idx = 0; idx < numCells; ++idx)
  {
    if ((idx * numPieces / numCells) == piece)
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

  // dicer->SetInput(input);
  // dicer->SetDiceModeToSpecifiedNumberOfPieces();
  // dicer->SetNumberOfPieces(numPieces);
  // dicer->Update();

  // intermediate->ShallowCopy(dicer->GetOutput());
  // intermediate->BuildLinks();
  // pointScalars = intermediate->GetPointData()->GetScalars();
}

//=============================================================================
int svtkExtractPolyDataPiece::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();
  svtkIntArray* cellTags;
  int ghostLevel, piece, numPieces;
  svtkIdType cellId, newCellId;
  svtkIdList *cellPts, *pointMap;
  svtkIdList* newCellPts = svtkIdList::New();
  svtkIdList* pointOwnership;
  svtkCell* cell;
  svtkPoints* newPoints;
  svtkUnsignedCharArray* cellGhostLevels = nullptr;
  svtkUnsignedCharArray* pointGhostLevels = nullptr;
  svtkIdType ptId = 0, newId, numPts, i;
  int numCellPts;
  double* x = nullptr;

  // Pipeline update piece will tell us what to generate.
  ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

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
  this->ComputeCellTags(cellTags, pointOwnership, piece, numPieces, input);

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
  output->AllocateCopy(input);
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
          (cellTags->GetValue(cellId) > 0) ? svtkDataSetAttributes::DUPLICATECELL : 0);
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

  // Split up points that are not used by cells,
  // and have not been assigned to any piece.
  // Count the number of unassigned points.  This is an extra pass through
  // the points, but the pieces will be better load balanced and
  // more spatially coherent.
  svtkIdType count = 0;
  svtkIdType idx;
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
  {
    if (pointOwnership->GetId(idx) == -1)
    {
      ++count;
    }
  }
  svtkIdType count2 = 0;
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
  {
    if (pointOwnership->GetId(idx) == -1)
    {
      if ((count2 * numPieces / count) == piece)
      {
        x = input->GetPoint(idx);
        newId = newPoints->InsertNextPoint(x);
        if (pointGhostLevels)
        {
          pointGhostLevels->InsertNextValue(0);
        }
        outPD->CopyData(pd, idx, newId);
      }
    }
  }

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

//=============================================================================
void svtkExtractPolyDataPiece::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");
}

//=============================================================================
void svtkExtractPolyDataPiece::AddGhostLevel(svtkPolyData* input, svtkIntArray* cellTags, int level)
{
  // for layers of ghost cells after the first we have to search
  // the entire input dataset. in the future we can extend this
  // function to return the list of cells that we set on our
  // level so we only have to search that subset for neighbors
  const svtkIdType numCells = input->GetNumberOfCells();
  svtkNew<svtkIdList> cellPointIds;
  svtkNew<svtkIdList> neighborIds;
  for (svtkIdType idx = 0; idx < numCells; ++idx)
  {
    if (cellTags->GetValue(idx) == level - 1)
    {
      input->GetCellPoints(idx, cellPointIds);
      const svtkIdType numCellPoints = cellPointIds->GetNumberOfIds();
      for (svtkIdType j = 0; j < numCellPoints; j++)
      {
        const svtkIdType pointId = cellPointIds->GetId(j);
        input->GetPointCells(pointId, neighborIds);

        const svtkIdType numNeighbors = neighborIds->GetNumberOfIds();
        for (svtkIdType k = 0; k < numNeighbors; ++k)
        {
          const svtkIdType neighborCellId = neighborIds->GetId(k);
          if (cellTags->GetValue(neighborCellId) == -1)
          {
            cellTags->SetValue(neighborCellId, level);
          }
        }
      }
    }
  }
}
