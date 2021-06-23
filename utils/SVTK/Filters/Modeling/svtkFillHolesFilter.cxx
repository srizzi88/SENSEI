/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFillHolesFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFillHolesFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkSphere.h"
#include "svtkTriangleStrip.h"

svtkStandardNewMacro(svtkFillHolesFilter);

//------------------------------------------------------------------------
svtkFillHolesFilter::svtkFillHolesFilter()
{
  this->HoleSize = 1.0;
}

//------------------------------------------------------------------------
svtkFillHolesFilter::~svtkFillHolesFilter() = default;

//------------------------------------------------------------------------
int svtkFillHolesFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();

  svtkDebugMacro(<< "Executing hole fill operation");

  // check the input, build data structures as necessary
  svtkIdType numPts;
  svtkIdType npts;
  const svtkIdType* pts;
  svtkPoints* inPts = input->GetPoints();
  svtkIdType numPolys = input->GetNumberOfPolys();
  svtkIdType numStrips = input->GetNumberOfStrips();
  if ((numPts = input->GetNumberOfPoints()) < 1 || !inPts || (numPolys < 1 && numStrips < 1))
  {
    svtkDebugMacro(<< "No input data!");
    return 1;
  }

  svtkPolyData* Mesh = svtkPolyData::New();
  Mesh->SetPoints(inPts);
  svtkCellArray *newPolys, *inPolys = input->GetPolys();
  if (numStrips > 0)
  {
    newPolys = svtkCellArray::New();
    if (numPolys > 0)
    {
      newPolys->DeepCopy(inPolys);
    }
    else
    {
      newPolys->AllocateEstimate(numStrips, 5);
    }
    svtkCellArray* inStrips = input->GetStrips();
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
    {
      svtkTriangleStrip::DecomposeStrip(npts, pts, newPolys);
    }
    Mesh->SetPolys(newPolys);
    newPolys->Delete();
  }
  else
  {
    newPolys = inPolys;
    Mesh->SetPolys(newPolys);
  }
  Mesh->BuildLinks();

  // Allocate storage for lines/points (arbitrary allocation sizes)
  //
  svtkPolyData* Lines = svtkPolyData::New();
  svtkCellArray* newLines = svtkCellArray::New();
  newLines->AllocateEstimate(numPts / 10, 1);
  Lines->SetLines(newLines);
  Lines->SetPoints(inPts);

  // grab all free edges and place them into a temporary polydata
  int abort = 0;
  svtkIdType cellId, p1, p2, numNei, i, numCells = newPolys->GetNumberOfCells();
  svtkIdType progressInterval = numCells / 20 + 1;
  svtkIdList* neighbors = svtkIdList::New();
  neighbors->Allocate(SVTK_CELL_SIZE);
  for (cellId = 0, newPolys->InitTraversal(); newPolys->GetNextCell(npts, pts) && !abort; cellId++)
  {
    if (!(cellId % progressInterval)) // manage progress / early abort
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    for (i = 0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i + 1) % npts];

      Mesh->GetCellEdgeNeighbors(cellId, p1, p2, neighbors);
      numNei = neighbors->GetNumberOfIds();

      if (numNei < 1)
      {
        newLines->InsertNextCell(2);
        newLines->InsertCellPoint(p1);
        newLines->InsertCellPoint(p2);
      }
    }
  }

  // Track all free edges and see whether polygons can be built from them.
  // For each polygon of appropriate HoleSize, triangulate the hole and
  // add to the output list of cells
  svtkIdType numHolesFilled = 0;
  numCells = newLines->GetNumberOfCells();
  svtkCellArray* newCells = nullptr;
  if (numCells >= 3) // only do the work if there are free edges
  {
    double sphere[4];
    svtkIdType startId, neiId, currentCellId, hints[2];
    hints[0] = 0;
    hints[1] = 0;
    svtkPolygon* polygon = svtkPolygon::New();
    polygon->Points->SetDataTypeToDouble();
    svtkIdList* endId = svtkIdList::New();
    endId->SetNumberOfIds(1);
    char* visited = new char[numCells];
    memset(visited, 0, numCells);
    Lines->BuildLinks(); // build the neighbor data structure
    newCells = svtkCellArray::New();
    newCells->DeepCopy(inPolys);

    for (cellId = 0; cellId < numCells && !abort; cellId++)
    {
      if (!visited[cellId])
      {
        visited[cellId] = 1;
        // Setup the polygon
        Lines->GetCellPoints(cellId, npts, pts);
        startId = pts[0];
        polygon->PointIds->Reset();
        polygon->Points->Reset();
        polygon->PointIds->InsertId(0, pts[0]);
        polygon->Points->InsertPoint(0, inPts->GetPoint(pts[0]));

        // Work around the loop and terminate when the loop ends
        endId->SetId(0, pts[1]);
        int valid = 1;
        currentCellId = cellId;
        while (startId != endId->GetId(0) && valid)
        {
          polygon->PointIds->InsertNextId(endId->GetId(0));
          polygon->Points->InsertNextPoint(inPts->GetPoint(endId->GetId(0)));
          Lines->GetCellNeighbors(currentCellId, endId, neighbors);
          if (neighbors->GetNumberOfIds() == 0)
          {
            valid = 0;
          }
          else if (neighbors->GetNumberOfIds() > 1)
          {
            // have to logically split this vertex
            valid = 0;
          }
          else
          {
            neiId = neighbors->GetId(0);
            visited[neiId] = 1;
            Lines->GetCellPoints(neiId, npts, pts);
            endId->SetId(0, (pts[0] != endId->GetId(0) ? pts[0] : pts[1]));
            currentCellId = neiId;
          }
        } // while loop connected

        // Evaluate the size of the loop and see if it is small enough
        if (valid)
        {
          svtkSphere::ComputeBoundingSphere(
            static_cast<svtkDoubleArray*>(polygon->Points->GetData())->GetPointer(0),
            polygon->PointIds->GetNumberOfIds(), sphere, hints);
          if (sphere[3] <= this->HoleSize)
          {
            // Now triangulate the loop and pass to the output
            numHolesFilled++;
            polygon->NonDegenerateTriangulate(neighbors);
            for (i = 0; i < neighbors->GetNumberOfIds(); i += 3)
            {
              newCells->InsertNextCell(3);
              newCells->InsertCellPoint(polygon->PointIds->GetId(neighbors->GetId(i)));
              newCells->InsertCellPoint(polygon->PointIds->GetId(neighbors->GetId(i + 1)));
              newCells->InsertCellPoint(polygon->PointIds->GetId(neighbors->GetId(i + 2)));
            }
          } // if hole small enough
        }   // if a valid loop
      }     // if not yet visited a line
    }       // for all lines
    polygon->Delete();
    endId->Delete();
    delete[] visited;
  } // if loops present in the input

  // Clean up
  neighbors->Delete();
  Lines->Delete();

  // No new points are created, so the points and point data can be passed
  // through to the output.
  output->SetPoints(inPts);
  outPD->PassData(pd);

  // New cells are created, so currently we do not pass the cell data.
  // It would be pretty easy to extend the existing cell data and mark
  // the new cells with special data values.
  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  if (newCells)
  {
    output->SetPolys(newCells);
    newCells->Delete();
  }
  else
  {
    output->SetPolys(inPolys);
  }
  output->SetStrips(input->GetStrips());

  Mesh->Delete();
  newLines->Delete();
  return 1;
}

//------------------------------------------------------------------------
void svtkFillHolesFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Hole Size: " << this->HoleSize << "\n";
}
