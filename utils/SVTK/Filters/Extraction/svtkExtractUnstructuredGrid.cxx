/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractUnstructuredGrid.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkIdList.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkExtractUnstructuredGrid);

// Construct with all types of clipping turned off.
svtkExtractUnstructuredGrid::svtkExtractUnstructuredGrid()
{
  this->PointMinimum = 0;
  this->PointMaximum = SVTK_ID_MAX;

  this->CellMinimum = 0;
  this->CellMaximum = SVTK_ID_MAX;

  this->Extent[0] = -SVTK_DOUBLE_MAX;
  this->Extent[1] = SVTK_DOUBLE_MAX;
  this->Extent[2] = -SVTK_DOUBLE_MAX;
  this->Extent[3] = SVTK_DOUBLE_MAX;
  this->Extent[4] = -SVTK_DOUBLE_MAX;
  this->Extent[5] = SVTK_DOUBLE_MAX;

  this->PointClipping = 0;
  this->CellClipping = 0;
  this->ExtentClipping = 0;

  this->Merging = 0;
  this->Locator = nullptr;
}

// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void svtkExtractUnstructuredGrid::SetExtent(
  double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
  double extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void svtkExtractUnstructuredGrid::SetExtent(double extent[6])
{
  int i;

  if (extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
    extent[2] != this->Extent[2] || extent[3] != this->Extent[3] || extent[4] != this->Extent[4] ||
    extent[5] != this->Extent[5])
  {
    this->ExtentClippingOn();
    for (i = 0; i < 3; i++)
    {
      if (extent[2 * i + 1] < extent[2 * i])
      {
        extent[2 * i + 1] = extent[2 * i];
      }
      this->Extent[2 * i] = extent[2 * i];
      this->Extent[2 * i + 1] = extent[2 * i + 1];
    }
  }
}

// Extract cells and pass points and point data through. Also handles
// cell data.
int svtkExtractUnstructuredGrid::RequestData(svtkInformation* svtkNotUsed(request),
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

  svtkIdType cellId, i, newCellId;
  svtkIdType newPtId;
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkPoints *inPts = input->GetPoints(), *newPts;
  char* cellVis;
  svtkCell* cell;
  double x[3];
  svtkIdList* ptIds;
  svtkIdList* cellIds;
  svtkIdType ptId;
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  int allVisible;
  svtkIdType numIds;
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkIdType* pointMap = nullptr;

  svtkDebugMacro(<< "Executing extraction filter");

  if (numPts < 1 || numCells < 1 || !inPts)
  {
    svtkDebugMacro(<< "No data to extract!");
    return 1;
  }
  cellIds = svtkIdList::New();

  if ((!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping))
  {
    allVisible = 1;
    cellVis = nullptr;
  }
  else
  {
    allVisible = 0;
    cellVis = new char[numCells];
  }

  // Mark cells as being visible or not
  if (!allVisible)
  {
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum))
      {
        cellVis[cellId] = 0;
      }
      else
      {
        cell = input->GetCell(cellId);
        ptIds = cell->GetPointIds();
        numIds = ptIds->GetNumberOfIds();
        for (i = 0; i < numIds; i++)
        {
          ptId = ptIds->GetId(i);
          input->GetPoint(ptId, x);

          if ((this->PointClipping && (ptId < this->PointMinimum || ptId > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])))
          {
            cellVis[cellId] = 0;
            break;
          }
        }
        if (i >= numIds)
        {
          cellVis[cellId] = 1;
        }
      }
    }
  }

  // Allocate
  newPts = svtkPoints::New();
  newPts->Allocate(numPts);
  output->Allocate(numCells);
  outputPD->CopyAllocate(pd, numPts, numPts / 2);
  outputCD->CopyAllocate(cd, numCells, numCells / 2);

  if (this->Merging)
  {
    if (this->Locator == nullptr)
    {
      this->CreateDefaultLocator();
    }
    this->Locator->InitPointInsertion(newPts, input->GetBounds());
  }
  else
  {
    pointMap = new svtkIdType[numPts];
    for (i = 0; i < numPts; i++)
    {
      pointMap[i] = (-1); // initialize as unused
    }
  }

  // Traverse cells to extract geometry
  for (cellId = 0; cellId < numCells; cellId++)
  {
    if (allVisible || cellVis[cellId])
    {
      cell = input->GetCell(cellId);
      numIds = cell->PointIds->GetNumberOfIds();
      cellIds->Reset();
      if (this->Merging)
      {
        for (i = 0; i < numIds; i++)
        {
          ptId = cell->PointIds->GetId(i);
          input->GetPoint(ptId, x);
          if (this->Locator->InsertUniquePoint(x, newPtId))
          {
            outputPD->CopyData(pd, ptId, newPtId);
          }
          cellIds->InsertNextId(newPtId);
        }
      } // merging coincident points
      else
      {
        for (i = 0; i < numIds; i++)
        {
          ptId = cell->PointIds->GetId(i);
          if (pointMap[ptId] < 0)
          {
            pointMap[ptId] = newPtId = newPts->InsertNextPoint(inPts->GetPoint(ptId));
            outputPD->CopyData(pd, ptId, newPtId);
          }
          cellIds->InsertNextId(pointMap[ptId]);
        }
      } // keeping original point list

      newCellId = output->InsertNextCell(input->GetCellType(cellId), cellIds);
      outputCD->CopyData(cd, cellId, newCellId);

    } // if cell is visible
  }   // for all cells

  // Update ourselves and release memory
  output->SetPoints(newPts);
  newPts->Delete();

  svtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  if (this->Merging && this->Locator)
  {
    this->Locator->Initialize();
  }
  else
  {
    delete[] pointMap;
  }
  output->Squeeze();

  delete[] cellVis;
  cellIds->Delete();

  return 1;
}

svtkMTimeType svtkExtractUnstructuredGrid::GetMTime()
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

void svtkExtractUnstructuredGrid::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

// Specify a spatial locator for merging points. By
// default an instance of svtkMergePoints is used.
void svtkExtractUnstructuredGrid::SetLocator(svtkIncrementalPointLocator* locator)
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

void svtkExtractUnstructuredGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Minimum : " << this->PointMinimum << "\n";
  os << indent << "Point Maximum : " << this->PointMaximum << "\n";

  os << indent << "Cell Minimum : " << this->CellMinimum << "\n";
  os << indent << "Cell Maximum : " << this->CellMaximum << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->PointClipping ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->CellClipping ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->ExtentClipping ? "On\n" : "Off\n");

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}
