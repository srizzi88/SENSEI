/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLoopSubdivisionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLoopSubdivisionFilter.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellIterator.h"
#include "svtkEdgeTable.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkLoopSubdivisionFilter);

static const double LoopWeights[4] = { .375, .375, .125, .125 };

//----------------------------------------------------------------------------
int svtkLoopSubdivisionFilter::GenerateSubdivisionPoints(
  svtkPolyData* inputDS, svtkIntArray* edgeData, svtkPoints* outputPts, svtkPointData* outputPD)
{
  double* weights;
  const svtkIdType* pts = nullptr;
  svtkIdType numPts, cellId, newId;
  int edgeId;
  svtkIdType npts;
  svtkIdType p1, p2;
  svtkCellArray* inputPolys = inputDS->GetPolys();
  svtkSmartPointer<svtkIdList> cellIds = svtkSmartPointer<svtkIdList>::New();
  svtkSmartPointer<svtkIdList> stencil = svtkSmartPointer<svtkIdList>::New();
  svtkSmartPointer<svtkEdgeTable> edgeTable = svtkSmartPointer<svtkEdgeTable>::New();
  svtkPoints* inputPts = inputDS->GetPoints();
  svtkPointData* inputPD = inputDS->GetPointData();

  weights = new double[256];

  // Create an edge table to keep track of which edges we've processed
  edgeTable->InitEdgeInsertion(inputDS->GetNumberOfPoints());

  // Generate even points. these are derived from the old points
  numPts = inputDS->GetNumberOfPoints();
  for (svtkIdType ptId = 0; ptId < numPts; ptId++)
  {
    if (this->GenerateEvenStencil(ptId, inputDS, stencil, weights))
    {
      this->InterpolatePosition(inputPts, outputPts, stencil, weights);
      outputPD->InterpolatePoint(inputPD, ptId, stencil, weights);
    }
    else
    {
      delete[] weights;
      return 0;
    }
  }

  // Generate odd points. These will be inserted into the new dataset
  for (cellId = 0, inputPolys->InitTraversal(); inputPolys->GetNextCell(npts, pts); cellId++)
  {
    // start with one edge
    p1 = pts[2];
    p2 = pts[0];

    for (edgeId = 0; edgeId < 3; edgeId++)
    {
      // Do we need to create a point on this edge?
      if (edgeTable->IsEdge(p1, p2) == -1)
      {
        edgeTable->InsertEdge(p1, p2);
        inputDS->GetCellEdgeNeighbors(-1, p1, p2, cellIds);
        if (cellIds->GetNumberOfIds() == 1)
        {
          // Compute new Position and PointData using the same subdivision scheme
          stencil->SetNumberOfIds(2);
          stencil->SetId(0, p1);
          stencil->SetId(1, p2);
          weights[0] = .5;
          weights[1] = .5;
        } // boundary edge
        else if (cellIds->GetNumberOfIds() == 2)
        {
          this->GenerateOddStencil(p1, p2, inputDS, stencil, weights);
        }
        else
        {
          delete[] weights;
          svtkErrorMacro("Dataset is non-manifold and cannot be subdivided. Edge shared by "
            << cellIds->GetNumberOfIds() << " cells");
          return 0;
        }
        newId = this->InterpolatePosition(inputPts, outputPts, stencil, weights);
        outputPD->InterpolatePoint(inputPD, newId, stencil, weights);
      }
      else // we have already created a point on this edge. find it
      {
        newId = this->FindEdge(inputDS, cellId, p1, p2, edgeData, cellIds);
      }
      edgeData->InsertComponent(cellId, edgeId, newId);
      p1 = p2;
      if (edgeId < 2)
      {
        p2 = pts[edgeId + 1];
      }
    } // each interior edge
  }   // each cell

  // cleanup
  delete[] weights;
  return 1;
}

//----------------------------------------------------------------------------
int svtkLoopSubdivisionFilter::GenerateEvenStencil(
  svtkIdType p1, svtkPolyData* polys, svtkIdList* stencilIds, double* weights)
{
  svtkSmartPointer<svtkIdList> cellIds = svtkSmartPointer<svtkIdList>::New();
  svtkSmartPointer<svtkIdList> ptIds = svtkSmartPointer<svtkIdList>::New();
  svtkCell* cell;

  int i;
  svtkIdType j;
  svtkIdType startCell, nextCell;
  svtkIdType p, p2;
  svtkIdType bp1, bp2;
  svtkIdType K;
  double beta, cosSQ;

  // Get the cells that use this point
  polys->GetPointCells(p1, cellIds);
  svtkIdType numCellsInLoop = cellIds->GetNumberOfIds();
  if (numCellsInLoop < 1)
  {
    svtkWarningMacro("numCellsInLoop < 1: " << numCellsInLoop);
    stencilIds->Reset();
    return 0;
  }
  // Find an edge to start with that contains p1
  polys->GetCellPoints(cellIds->GetId(0), ptIds);
  p2 = ptIds->GetId(0);
  i = 1;
  while (p1 == p2)
  {
    p2 = ptIds->GetId(i++);
  }
  polys->GetCellEdgeNeighbors(-1, p1, p2, cellIds);

  nextCell = cellIds->GetId(0);
  bp2 = -1;
  bp1 = p2;
  if (cellIds->GetNumberOfIds() == 1)
  {
    startCell = -1;
  }
  else
  {
    startCell = cellIds->GetId(1);
  }

  stencilIds->Reset();
  stencilIds->InsertNextId(p2);

  // walk around the loop counter-clockwise and get cells
  for (j = 0; j < numCellsInLoop; j++)
  {
    cell = polys->GetCell(nextCell);
    p = -1;
    for (i = 0; i < 3; i++)
    {
      if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
      {
        break;
      }
    }
    p2 = p;
    stencilIds->InsertNextId(p2);
    polys->GetCellEdgeNeighbors(nextCell, p1, p2, cellIds);
    if (cellIds->GetNumberOfIds() != 1)
    {
      bp2 = p2;
      j++;
      break;
    }
    nextCell = cellIds->GetId(0);
  }

  // now walk around the other way. this will only happen if there
  // is a boundary cell left that we have not visited
  nextCell = startCell;
  p2 = bp1;
  for (; j < numCellsInLoop && startCell != -1; j++)
  {
    cell = polys->GetCell(nextCell);
    p = -1;
    for (i = 0; i < 3; i++)
    {
      if ((p = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
      {
        break;
      }
    }
    p2 = p;
    stencilIds->InsertNextId(p2);
    polys->GetCellEdgeNeighbors(nextCell, p1, p2, cellIds);
    if (cellIds->GetNumberOfIds() != 1)
    {
      bp1 = p2;
      break;
    }
    nextCell = cellIds->GetId(0);
  }

  if (bp2 != -1) // boundary edge
  {
    stencilIds->SetNumberOfIds(3);
    stencilIds->SetId(0, bp2);
    stencilIds->SetId(1, bp1);
    stencilIds->SetId(2, p1);
    weights[0] = .125;
    weights[1] = .125;
    weights[2] = .75;
  }
  else
  {
    K = stencilIds->GetNumberOfIds();
    // Remove last id. It's a duplicate of the first
    K--;
    if (K > 3)
    {
      // Generate weights
      cosSQ = .375 + .25 * cos(2.0 * svtkMath::Pi() / (double)K);
      cosSQ = cosSQ * cosSQ;
      beta = (.625 - cosSQ) / (double)K;
    }
    else
    {
      beta = 3.0 / 16.0;
    }
    for (j = 0; j < K; j++)
    {
      weights[j] = beta;
    }
    weights[K] = 1.0 - K * beta;
    stencilIds->SetId(K, p1);
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkLoopSubdivisionFilter::GenerateOddStencil(
  svtkIdType p1, svtkIdType p2, svtkPolyData* polys, svtkIdList* stencilIds, double* weights)
{
  svtkSmartPointer<svtkIdList> cellIds = svtkSmartPointer<svtkIdList>::New();
  svtkCell* cell;
  int i;
  svtkIdType cell0, cell1;
  svtkIdType p3 = 0, p4 = 0;

  polys->GetCellEdgeNeighbors(-1, p1, p2, cellIds);
  cell0 = cellIds->GetId(0);
  cell1 = cellIds->GetId(1);

  cell = polys->GetCell(cell0);
  for (i = 0; i < 3; i++)
  {
    if ((p3 = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
    {
      break;
    }
  }
  cell = polys->GetCell(cell1);
  for (i = 0; i < 3; i++)
  {
    if ((p4 = cell->GetPointId(i)) != p1 && cell->GetPointId(i) != p2)
    {
      break;
    }
  }

  stencilIds->SetNumberOfIds(4);
  stencilIds->SetId(0, p1);
  stencilIds->SetId(1, p2);
  stencilIds->SetId(2, p3);
  stencilIds->SetId(3, p4);

  for (i = 0; i < stencilIds->GetNumberOfIds(); i++)
  {
    weights[i] = LoopWeights[i];
  }
}

//----------------------------------------------------------------------------
int svtkLoopSubdivisionFilter::RequestUpdateExtent(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int numPieces, ghostLevel;
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->Superclass::RequestUpdateExtent(request, inputVector, outputVector))
  {
    return 0;
  }

  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1 && this->NumberOfSubdivisions > 0)
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel + 1);
  }

  return 1;
}
