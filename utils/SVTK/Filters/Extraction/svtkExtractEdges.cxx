/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractEdges.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkEdgeTable.h"
#include "svtkGenericCell.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkExtractEdges);

//----------------------------------------------------------------------------
// Construct object.
svtkExtractEdges::svtkExtractEdges()
{
  this->Locator = nullptr;
}

//----------------------------------------------------------------------------
svtkExtractEdges::~svtkExtractEdges()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

//----------------------------------------------------------------------------
// Generate feature edges for mesh
int svtkExtractEdges::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* newPts;
  svtkCellArray* newLines;
  svtkIdType numCells, cellNum, numPts, newId;
  int edgeNum, numEdgePts, numCellEdges;
  int i, abort = 0;
  svtkIdType pts[2];
  svtkIdType pt1 = 0, pt2;
  double x[3];
  svtkEdgeTable* edgeTable;
  svtkGenericCell* cell;
  svtkCell* edge;
  svtkPointData *pd, *outPD;
  svtkCellData *cd, *outCD;

  svtkDebugMacro(<< "Executing edge extractor");

  //  Check input
  //
  numPts = input->GetNumberOfPoints();
  if ((numCells = input->GetNumberOfCells()) < 1 || numPts < 1)
  {
    return 1;
  }

  // Set up processing
  //
  edgeTable = svtkEdgeTable::New();
  edgeTable->InitEdgeInsertion(numPts);
  newPts = svtkPoints::New();
  newPts->Allocate(numPts);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(numPts * 4, 2);

  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd, numPts);

  cd = input->GetCellData();
  outCD = output->GetCellData();
  outCD->CopyAllocate(cd, numCells);

  cell = svtkGenericCell::New();
  svtkIdList *edgeIds, *HEedgeIds = svtkIdList::New();
  svtkPoints *edgePts, *HEedgePts = svtkPoints::New();

  // Get our locator for merging points
  //
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, input->GetBounds());

  // Loop over all cells, extracting non-visited edges.
  //
  svtkIdType tenth = numCells / 10 + 1;
  for (cellNum = 0; cellNum < numCells && !abort; cellNum++)
  {
    if (!(cellNum % tenth)) // manage progress reports / early abort
    {
      this->UpdateProgress(static_cast<double>(cellNum) / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(cellNum, cell);
    numCellEdges = cell->GetNumberOfEdges();
    for (edgeNum = 0; edgeNum < numCellEdges; edgeNum++)
    {
      edge = cell->GetEdge(edgeNum);
      numEdgePts = edge->GetNumberOfPoints();

      // Tessellate higher-order edges
      if (!edge->IsLinear())
      {
        edge->Triangulate(0, HEedgeIds, HEedgePts);
        edgeIds = HEedgeIds;
        edgePts = HEedgePts;

        for (i = 0; i < (edgeIds->GetNumberOfIds() / 2); i++)
        {
          pt1 = edgeIds->GetId(2 * i);
          pt2 = edgeIds->GetId(2 * i + 1);
          edgePts->GetPoint(2 * i, x);
          if (this->Locator->InsertUniquePoint(x, pts[0]))
          {
            outPD->CopyData(pd, pt1, pts[0]);
          }
          edgePts->GetPoint(2 * i + 1, x);
          if (this->Locator->InsertUniquePoint(x, pts[1]))
          {
            outPD->CopyData(pd, pt2, pts[1]);
          }
          if (edgeTable->IsEdge(pt1, pt2) == -1)
          {
            edgeTable->InsertEdge(pt1, pt2);
            newId = newLines->InsertNextCell(2, pts);
            outCD->CopyData(cd, cellNum, newId);
          }
        }
      } // if non-linear edge

      else // linear edges
      {
        edgeIds = edge->PointIds;
        edgePts = edge->Points;

        for (i = 0; i < numEdgePts; i++, pt1 = pt2, pts[0] = pts[1])
        {
          pt2 = edgeIds->GetId(i);
          edgePts->GetPoint(i, x);
          if (this->Locator->InsertUniquePoint(x, pts[1]))
          {
            outPD->CopyData(pd, pt2, pts[1]);
          }
          if (i > 0 && edgeTable->IsEdge(pt1, pt2) == -1)
          {
            edgeTable->InsertEdge(pt1, pt2);
            newId = newLines->InsertNextCell(2, pts);
            outCD->CopyData(cd, cellNum, newId);
          }
        } // if linear edge
      }
    } // for all edges of cell
  }   // for all cells

  svtkDebugMacro(<< "Created " << newLines->GetNumberOfCells() << " edges");

  //  Update ourselves.
  //
  HEedgeIds->Delete();
  HEedgePts->Delete();
  edgeTable->Delete();
  cell->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of svtkMergePoints is used.
void svtkExtractEdges::SetLocator(svtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (locator)
  {
    locator->Register(this);
  }
  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractEdges::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    svtkMergePoints* locator = svtkMergePoints::New();
    this->SetLocator(locator);
    locator->Delete();
  }
}

//----------------------------------------------------------------------------
int svtkExtractEdges::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractEdges::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkExtractEdges::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}
