/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractUnstructuredGridPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractUnstructuredGridPiece.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

namespace
{

void determineMinMax(
  int piece, int numPieces, svtkIdType numCells, svtkIdType& minCell, svtkIdType& maxCell)
{
  const float fnumPieces = static_cast<float>(numPieces);
  const float fminCell = (numCells / fnumPieces) * piece;
  const float fmaxCell = fminCell + (numCells / fnumPieces);

  // round up if over N.5
  minCell = static_cast<svtkIdType>(fminCell + 0.5f);
  maxCell = static_cast<svtkIdType>(fmaxCell + 0.5f);
}
}

svtkStandardNewMacro(svtkExtractUnstructuredGridPiece);

svtkExtractUnstructuredGridPiece::svtkExtractUnstructuredGridPiece()
{
  this->CreateGhostCells = 1;
}

int svtkExtractUnstructuredGridPiece::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

int svtkExtractUnstructuredGridPiece::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // get the info object
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  return 1;
}

void svtkExtractUnstructuredGridPiece::ComputeCellTags(svtkIntArray* tags, svtkIdList* pointOwnership,
  int piece, int numPieces, svtkUnstructuredGrid* input)
{
  svtkIdType idx, ptId;

  svtkIdType numCells = input->GetNumberOfCells();

  // Clear Point ownership.  This is only necessary if we
  // Are creating ghost points.
  if (pointOwnership)
  {
    for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
    {
      pointOwnership->SetId(idx, -1);
    }
  }

  // no point on tagging cells if we have no cells
  if (numCells == 0)
  {
    return;
  }

  // Brute force division.
  // mark all we own as zero and the rest as -1
  svtkIdType minCell = 0;
  svtkIdType maxCell = 0;
  determineMinMax(piece, numPieces, numCells, minCell, maxCell);

  for (idx = 0; idx < minCell; ++idx)
  {
    tags->SetValue(idx, -1);
  }
  for (idx = minCell; idx < maxCell; ++idx)
  {
    tags->SetValue(idx, 0);
  }
  for (idx = maxCell; idx < numCells; ++idx)
  {
    tags->SetValue(idx, -1);
  }

  svtkCellArray* cells = input->GetCells();
  if (pointOwnership && cells)
  {
    auto cellIter = svtkSmartPointer<svtkCellArrayIterator>::Take(cells->NewIterator());
    for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      // Fill in point ownership mapping
      svtkIdType numCellPts;
      const svtkIdType* ids;
      cellIter->GetCurrentCell(numCellPts, ids);

      for (svtkIdType j = 0; j < numCellPts; ++j)
      {
        ptId = ids[j];
        if (pointOwnership->GetId(ptId) == -1)
        {
          pointOwnership->SetId(ptId, idx);
        }
      }
    }
  }
}

int svtkExtractUnstructuredGridPiece::RequestData(svtkInformation* svtkNotUsed(request),
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
  unsigned char* cellTypes =
    (input->GetCellTypesArray() ? input->GetCellTypesArray()->GetPointer(0) : nullptr);
  int cellType;
  svtkIntArray* cellTags;
  int ghostLevel, piece, numPieces;
  svtkIdType newCellId;
  svtkIdList* pointMap;
  svtkIdList* newCellPts = svtkIdList::New();
  svtkPoints* newPoints;
  svtkUnsignedCharArray* cellGhostLevels = nullptr;
  svtkIdList* pointOwnership = nullptr;
  svtkUnsignedCharArray* pointGhostLevels = nullptr;
  svtkIdType i, ptId, newId, numPts, numCells;
  svtkIdType* faceStream;
  svtkIdType numFaces;
  svtkIdType numFacePts;
  double* x;

  // Pipeline update piece will tell us what to generate.
  ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  outPD->CopyAllocate(pd);
  outCD->CopyAllocate(cd);

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  if (ghostLevel > 0 && this->CreateGhostCells)
  {
    cellGhostLevels = svtkUnsignedCharArray::New();
    cellGhostLevels->Allocate(numCells);
    // We may want to create point ghost levels even
    // if there are no ghost cells.  Since it cost extra,
    // and no filter really uses it, and the filter did not
    // create a point ghost level array for this case before,
    // I will leave it the way it was.
    pointOwnership = svtkIdList::New();
    pointOwnership->Allocate(numPts);
    pointGhostLevels = svtkUnsignedCharArray::New();
    pointGhostLevels->Allocate(numPts);
  }

  // Break up cells based on which piece they belong to.
  cellTags = svtkIntArray::New();
  cellTags->Allocate(input->GetNumberOfCells(), 1000);
  // Cell tags end up being 0 for cells in piece and -1 for all others.
  // Point ownership is the cell that owns the point.
  this->ComputeCellTags(cellTags, pointOwnership, piece, numPieces, input);

  // Find the layers of ghost cells.
  if (this->CreateGhostCells && ghostLevel > 0)
  {
    this->AddFirstGhostLevel(input, cellTags, piece, numPieces);
    for (i = 2; i <= ghostLevel; i++)
    {
      this->AddGhostLevel(input, cellTags, i);
    }
  }

  // Filter the cells.

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
  svtkCellArray* cells = input->GetCells();
  if (cells)
  {
    auto cellIter = svtkSmartPointer<svtkCellArrayIterator>::Take(cells->NewIterator());

    for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      const svtkIdType cellId = cellIter->GetCurrentCellId();
      svtkIdType numCellPts;
      const svtkIdType* ids;
      cellIter->GetCurrentCell(numCellPts, ids);

      cellType = cellTypes[cellId];

      if (cellTags->GetValue(cellId) != -1) // satisfied thresholding
      {
        if (cellGhostLevels)
        {
          cellGhostLevels->InsertNextValue(
            cellTags->GetValue(cellId) > 0 ? svtkDataSetAttributes::DUPLICATECELL : 0);
        }
        if (cellType != SVTK_POLYHEDRON)
        {
          for (i = 0; i < numCellPts; i++)
          {
            ptId = ids[i];
            if ((newId = pointMap->GetId(ptId)) < 0)
            {
              x = input->GetPoint(ptId);
              newId = newPoints->InsertNextPoint(x);
              if (pointGhostLevels && pointOwnership)
              {
                pointGhostLevels->InsertNextValue(
                  cellTags->GetValue(pointOwnership->GetId(ptId)) > 0
                    ? svtkDataSetAttributes::DUPLICATEPOINT
                    : 0);
              }
              pointMap->SetId(ptId, newId);
              outPD->CopyData(pd, ptId, newId);
            }
            newCellPts->InsertId(i, newId);
          }
        }
        else
        { // Polyhedron, need to process face stream.
          faceStream = input->GetFaces(cellId);
          numFaces = *faceStream++;
          newCellPts->InsertNextId(numFaces);
          for (svtkIdType face = 0; face < numFaces; ++face)
          {
            numFacePts = *faceStream++;
            newCellPts->InsertNextId(numFacePts);
            while (numFacePts-- > 0)
            {
              ptId = *faceStream++;
              if ((newId = pointMap->GetId(ptId)) < 0)
              {
                x = input->GetPoint(ptId);
                newId = newPoints->InsertNextPoint(x);
                if (pointGhostLevels && pointOwnership)
                {
                  pointGhostLevels->InsertNextValue(
                    cellTags->GetValue(pointOwnership->GetId(ptId)) > 0
                      ? svtkDataSetAttributes::DUPLICATEPOINT
                      : 0);
                }
                pointMap->SetId(ptId, newId);
                outPD->CopyData(pd, ptId, newId);
              }
              newCellPts->InsertNextId(newId);
            }
          }
        }
        newCellId = output->InsertNextCell(cellType, newCellPts);
        outCD->CopyData(cd, cellId, newCellId);
        newCellPts->Reset();
      } // satisfied thresholding
    }   // for all cells
  }     // input has cells

  // Split up points that are not used by cells,
  // and have not been assigned to any piece.
  // Count the number of unassigned points.  This is an extra pass through
  // the points, but the pieces will be better load balanced and
  // more spatially coherent.
  svtkIdType count = 0;
  svtkIdType idx;
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
  {
    if (pointMap->GetId(idx) == -1)
    {
      ++count;
    }
  }
  svtkIdType count2 = 0;
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
  {
    if (pointMap->GetId(idx) == -1)
    {
      if ((count2++ * numPieces / count) == piece)
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
  if (pointOwnership)
  {
    pointOwnership->Delete();
    pointOwnership = nullptr;
  }

  return 1;
}

void svtkExtractUnstructuredGridPiece::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");
}

void svtkExtractUnstructuredGridPiece::AddFirstGhostLevel(
  svtkUnstructuredGrid* input, svtkIntArray* cellTags, int piece, int numPieces)
{
  const svtkIdType numCells = input->GetNumberOfCells();
  svtkNew<svtkIdList> cellPointIds;
  svtkNew<svtkIdList> neighborIds;

  // for level 1 we have an optimal implementation
  // that can compute the subset of cells we need to check
  svtkIdType minCell = 0;
  svtkIdType maxCell = 0;
  determineMinMax(piece, numPieces, numCells, minCell, maxCell);
  for (svtkIdType idx = minCell; idx < maxCell; ++idx)
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
          cellTags->SetValue(neighborCellId, 1);
        }
      }
    }
  }
}

void svtkExtractUnstructuredGridPiece::AddGhostLevel(
  svtkUnstructuredGrid* input, svtkIntArray* cellTags, int level)
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
