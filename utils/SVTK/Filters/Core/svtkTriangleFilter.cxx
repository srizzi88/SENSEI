/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTriangleFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTriangleFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkTriangleStrip.h"

svtkStandardNewMacro(svtkTriangleFilter);

int svtkTriangleFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numCells = input->GetNumberOfCells();
  svtkIdType cellNum = 0;
  svtkIdType numPts, newId;
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  int i, j;
  svtkCellData* inCD = input->GetCellData();
  svtkCellData* outCD = output->GetCellData();
  svtkIdType updateInterval;
  svtkCellArray *cells, *newCells;
  svtkPoints* inPts = input->GetPoints();

  int abort = 0;
  updateInterval = numCells / 100 + 1;
  outCD->CopyAllocate(inCD, numCells);

  // Do each of the verts, lines, polys, and strips separately
  // verts
  if (input->GetVerts()->GetNumberOfCells() > 0)
  {
    cells = input->GetVerts();
    if (this->PassVerts)
    {
      newId = output->GetNumberOfCells();
      newCells = svtkCellArray::New();
      newCells->AllocateCopy(cells);
      for (cells->InitTraversal(); cells->GetNextCell(npts, pts) && !abort; cellNum++)
      {
        if (!(cellNum % updateInterval)) // manage progress reports / early abort
        {
          this->UpdateProgress((float)cellNum / numCells);
          abort = this->GetAbortExecute();
        }
        if (npts > 1)
        {
          for (i = 0; i < npts; i++)
          {
            newCells->InsertNextCell(1, pts + i);
            outCD->CopyData(inCD, cellNum, newId++);
          }
        }
        else
        {
          newCells->InsertNextCell(1, pts);
          outCD->CopyData(inCD, cellNum, newId++);
        }
      }
      output->SetVerts(newCells);
      newCells->Delete();
    }
    else
    {
      cellNum += cells->GetNumberOfCells(); // skip over verts
    }
  }

  // lines
  if (!abort && input->GetLines()->GetNumberOfCells() > 0)
  {
    cells = input->GetLines();
    if (this->PassLines)
    {
      newId = output->GetNumberOfCells();
      newCells = svtkCellArray::New();
      newCells->AllocateCopy(cells);
      for (cells->InitTraversal(); cells->GetNextCell(npts, pts) && !abort; cellNum++)
      {
        if (!(cellNum % updateInterval)) // manage progress reports / early abort
        {
          this->UpdateProgress((float)cellNum / numCells);
          abort = this->GetAbortExecute();
        }
        if (npts > 2)
        {
          for (i = 0; i < (npts - 1); i++)
          {
            newCells->InsertNextCell(2, pts + i);
            outCD->CopyData(inCD, cellNum, newId++);
          }
        }
        else
        {
          newCells->InsertNextCell(2, pts);
          outCD->CopyData(inCD, cellNum, newId++);
        }
      } // for all lines
      output->SetLines(newCells);
      newCells->Delete();
    }
    else
    {
      cellNum += cells->GetNumberOfCells(); // skip over lines
    }
  }

  svtkCellArray* newPolys = nullptr;
  if (!abort && input->GetPolys()->GetNumberOfCells() > 0)
  {
    cells = input->GetPolys();
    newId = output->GetNumberOfCells();
    newPolys = svtkCellArray::New();
    newPolys->AllocateCopy(cells);
    output->SetPolys(newPolys);
    svtkIdList* ptIds = svtkIdList::New();
    ptIds->Allocate(SVTK_CELL_SIZE);
    int numSimplices;
    svtkPolygon* poly = svtkPolygon::New();
    svtkIdType triPts[3];

    for (cells->InitTraversal(); cells->GetNextCell(npts, pts) && !abort; cellNum++)
    {
      if (!(cellNum % updateInterval)) // manage progress reports / early abort
      {
        this->UpdateProgress((float)cellNum / numCells);
        abort = this->GetAbortExecute();
      }
      if (npts == 0)
      {
        continue;
      }
      if (npts == 3)
      {
        newPolys->InsertNextCell(3, pts);
        outCD->CopyData(inCD, cellNum, newId++);
      }
      else // triangulate polygon
      {
        // initialize polygon
        poly->PointIds->SetNumberOfIds(npts);
        poly->Points->SetNumberOfPoints(npts);
        for (i = 0; i < npts; i++)
        {
          poly->PointIds->SetId(i, pts[i]);
          poly->Points->SetPoint(i, inPts->GetPoint(pts[i]));
        }
        poly->Triangulate(ptIds);
        numPts = ptIds->GetNumberOfIds();
        numSimplices = numPts / 3;
        for (i = 0; i < numSimplices; i++)
        {
          for (j = 0; j < 3; j++)
          {
            triPts[j] = poly->PointIds->GetId(ptIds->GetId(3 * i + j));
          }
          newPolys->InsertNextCell(3, triPts);
          outCD->CopyData(inCD, cellNum, newId++);
        } // for each simplex
      }   // triangulate polygon
    }
    ptIds->Delete();
    poly->Delete();
  }

  // strips
  if (!abort && input->GetStrips()->GetNumberOfCells() > 0)
  {
    cells = input->GetStrips();
    newId = output->GetNumberOfCells();
    if (newPolys == nullptr)
    {
      newPolys = svtkCellArray::New();
      newPolys->AllocateCopy(cells);
      output->SetPolys(newPolys);
    }
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts) && !abort; cellNum++)
    {
      if (!(cellNum % updateInterval)) // manage progress reports / early abort
      {
        this->UpdateProgress((float)cellNum / numCells);
        abort = this->GetAbortExecute();
      }
      svtkTriangleStrip::DecomposeStrip(npts, pts, newPolys);
      for (i = 0; i < (npts - 2); i++)
      {
        outCD->CopyData(inCD, cellNum, newId++);
      }
    } // for all strips
  }

  if (newPolys != nullptr)
  {
    newPolys->Delete();
  }

  // Update output
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());
  output->Squeeze();

  svtkDebugMacro(<< "Converted " << input->GetNumberOfCells() << "input cells to "
                << output->GetNumberOfCells() << " output cells");

  return 1;
}

void svtkTriangleFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Pass Verts: " << (this->PassVerts ? "On\n" : "Off\n");
  os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");
}
