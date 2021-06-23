/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearExtrusionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLinearExtrusionFilter.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkLinearExtrusionFilter);

//----------------------------------------------------------------------------
// Create object with normal extrusion type, capping on, scale factor=1.0,
// vector (0,0,1), and extrusion point (0,0,0).
svtkLinearExtrusionFilter::svtkLinearExtrusionFilter()
{
  this->ExtrusionType = SVTK_NORMAL_EXTRUSION;
  this->Capping = 1;
  this->ScaleFactor = 1.0;
  this->Vector[0] = this->Vector[1] = 0.0;
  this->Vector[2] = 1.0;
  this->ExtrusionPoint[0] = this->ExtrusionPoint[1] = this->ExtrusionPoint[2] = 0.0;
}

//----------------------------------------------------------------------------
void svtkLinearExtrusionFilter::ViaNormal(double x[3], svtkIdType id, svtkDataArray* n)
{
  double normal[3];

  n->GetTuple(id, normal);
  for (svtkIdType i = 0; i < 3; i++)
  {
    x[i] = x[i] + this->ScaleFactor * normal[i];
  }
}

//----------------------------------------------------------------------------
void svtkLinearExtrusionFilter::ViaVector(
  double x[3], svtkIdType svtkNotUsed(id), svtkDataArray* svtkNotUsed(n))
{
  for (svtkIdType i = 0; i < 3; i++)
  {
    x[i] = x[i] + this->ScaleFactor * this->Vector[i];
  }
}

//----------------------------------------------------------------------------
void svtkLinearExtrusionFilter::ViaPoint(
  double x[3], svtkIdType svtkNotUsed(id), svtkDataArray* svtkNotUsed(n))
{
  for (svtkIdType i = 0; i < 3; i++)
  {
    x[i] = x[i] + this->ScaleFactor * (x[i] - this->ExtrusionPoint[i]);
  }
}

//----------------------------------------------------------------------------
int svtkLinearExtrusionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts, numCells;
  svtkPointData* pd = input->GetPointData();
  svtkDataArray* inNormals = nullptr;
  svtkPolyData* mesh;
  svtkPoints* inPts;
  svtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  svtkIdType inCellId, outCellId;
  int numEdges, dim;
  const svtkIdType* pts = nullptr;
  svtkIdType npts = 0;
  svtkIdType ptId, ncells, p1, p2;
  svtkIdType i, j;
  double x[3];
  svtkPoints* newPts;
  svtkCellArray *newLines = nullptr, *newPolys = nullptr, *newStrips;
  svtkCell* edge;
  svtkIdList *cellIds, *cellPts;
  svtkPointData* outputPD = output->GetPointData();
  // Here is a big pain about ordering of cells. (Copy CellData)
  svtkIdList* lineIds;
  svtkIdList* polyIds;
  svtkIdList* stripIds;

  // Initialize / check input
  //
  svtkDebugMacro(<< "Linearly extruding data");

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
  {
    svtkErrorMacro(<< "No data to extrude!");
    return 1;
  }

  // Decide which vector to use for extrusion
  //
  if (this->ExtrusionType == SVTK_POINT_EXTRUSION)
  {
    this->ExtrudePoint = &svtkLinearExtrusionFilter::ViaPoint;
  }
  else if (this->ExtrusionType == SVTK_NORMAL_EXTRUSION && (inNormals = pd->GetNormals()) != nullptr)
  {
    this->ExtrudePoint = &svtkLinearExtrusionFilter::ViaNormal;
    inNormals = pd->GetNormals();
  }
  else // SVTK_VECTOR_EXTRUSION
  {
    this->ExtrudePoint = &svtkLinearExtrusionFilter::ViaVector;
  }

  // Build cell data structure.
  //
  mesh = svtkPolyData::New();
  inPts = input->GetPoints();
  inVerts = input->GetVerts();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
  mesh->SetPoints(inPts);
  mesh->SetVerts(inVerts);
  mesh->SetLines(inLines);
  mesh->SetPolys(inPolys);
  mesh->SetStrips(inStrips);
  if (inPolys->GetNumberOfCells() || inStrips->GetNumberOfCells())
  {
    mesh->BuildLinks();
  }

  cellIds = svtkIdList::New();
  cellIds->Allocate(SVTK_CELL_SIZE);

  // Allocate memory for output. We don't copy normals because surface geometry
  // is modified. Copy all points - this is the usual requirement and it makes
  // creation of skirt much easier.
  //
  output->GetCellData()->CopyNormalsOff();
  output->GetCellData()->CopyAllocate(input->GetCellData(), 3 * input->GetNumberOfCells());

  outputPD->CopyNormalsOff();
  outputPD->CopyAllocate(pd, 2 * numPts);
  newPts = svtkPoints::New();
  newPts->SetNumberOfPoints(2 * numPts);
  if ((ncells = inVerts->GetNumberOfCells()) > 0)
  {
    newLines = svtkCellArray::New();
    newLines->AllocateEstimate(ncells, 2);
  }
  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells() / 10 +
    inStrips->GetNumberOfCells() / 10;
  ncells = (ncells < 100 ? 100 : ncells);
  newStrips = svtkCellArray::New();
  newStrips->AllocateEstimate(ncells, 4);

  svtkIdType progressInterval = numPts / 10 + 1;
  int abort = 0;

  // copy points
  for (ptId = 0; ptId < numPts; ptId++)
  {
    if (!(ptId % progressInterval)) // manage progress / early abort
    {
      this->UpdateProgress(0.25 * ptId / numPts);
    }

    inPts->GetPoint(ptId, x);
    newPts->SetPoint(ptId, x);
    (this->*(this->ExtrudePoint))(x, ptId, inNormals);
    newPts->SetPoint(ptId + numPts, x);
    outputPD->CopyData(pd, ptId, ptId);
    outputPD->CopyData(pd, ptId, ptId + numPts);
  }

  // We need the cellid to copy cell data. Skip points and lines.
  inCellId = outCellId = 0;
  if (input->GetVerts())
  {
    inCellId += input->GetVerts()->GetNumberOfCells();
  }
  if (input->GetLines())
  {
    inCellId += input->GetLines()->GetNumberOfCells();
  }
  // We need to keep track of input cell ids used to generate
  // output cells so that we can copy cell data at the end.
  // We do not know how many lines, polys and strips we will get
  // before hand.
  lineIds = svtkIdList::New();
  polyIds = svtkIdList::New();
  stripIds = svtkIdList::New();

  // If capping is on, copy 2D cells to output (plus create cap)
  //
  if (this->Capping)
  {
    if (inPolys->GetNumberOfCells() > 0)
    {
      newPolys = svtkCellArray::New();
      newPolys->AllocateCopy(inPolys);
      for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
      {
        newPolys->InsertNextCell(npts, pts);
        polyIds->InsertNextId(inCellId);
        newPolys->InsertNextCell(npts);
        for (i = 0; i < npts; i++)
        {
          newPolys->InsertCellPoint(pts[i] + numPts);
        }
        polyIds->InsertNextId(inCellId);
        ++inCellId;
      }
    }

    if (inStrips->GetNumberOfCells() > 0)
    {
      for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
      {
        newStrips->InsertNextCell(npts, pts);
        stripIds->InsertNextId(inCellId);
        newStrips->InsertNextCell(npts);
        for (i = 0; i < npts; i++)
        {
          newStrips->InsertCellPoint(pts[i] + numPts);
        }
        stripIds->InsertNextId(inCellId);
        ++inCellId;
      }
    }
  }
  this->UpdateProgress(0.4);

  // Loop over all polygons and triangle strips searching for boundary edges.
  // If boundary edge found, extrude triangle strip.
  //
  progressInterval = numCells / 10 + 1;
  svtkGenericCell* cell = svtkGenericCell::New();
  for (inCellId = 0; inCellId < numCells && !abort; inCellId++)
  {
    if (!(inCellId % progressInterval)) // manage progress / early abort
    {
      this->UpdateProgress(0.4 + 0.6 * inCellId / numCells);
      abort = this->GetAbortExecute();
    }

    mesh->GetCell(inCellId, cell);
    cellPts = cell->GetPointIds();

    if ((dim = cell->GetCellDimension()) == 0) // create lines from points
    {
      for (i = 0; i < cellPts->GetNumberOfIds(); i++)
      {
        newLines->InsertNextCell(2);
        ptId = cellPts->GetId(i);
        newLines->InsertCellPoint(ptId);
        newLines->InsertCellPoint(ptId + numPts);
        lineIds->InsertNextId(inCellId);
      }
    }

    else if (dim == 1) // create strips from lines
    {
      for (i = 0; i < (cellPts->GetNumberOfIds() - 1); i++)
      {
        p1 = cellPts->GetId(i);
        p2 = cellPts->GetId(i + 1);
        newStrips->InsertNextCell(4);
        newStrips->InsertCellPoint(p1);
        newStrips->InsertCellPoint(p2);
        newStrips->InsertCellPoint(p1 + numPts);
        newStrips->InsertCellPoint(p2 + numPts);
        stripIds->InsertNextId(inCellId);
      }
    }

    else if (dim == 2) // create strips from boundary edges
    {
      numEdges = cell->GetNumberOfEdges();
      for (i = 0; i < numEdges; i++)
      {
        edge = cell->GetEdge(i);
        for (j = 0; j < (edge->GetNumberOfPoints() - 1); j++)
        {
          p1 = edge->PointIds->GetId(j);
          p2 = edge->PointIds->GetId(j + 1);
          mesh->GetCellEdgeNeighbors(inCellId, p1, p2, cellIds);

          if (cellIds->GetNumberOfIds() < 1) // generate strip
          {
            newStrips->InsertNextCell(4);
            newStrips->InsertCellPoint(p1);
            newStrips->InsertCellPoint(p2);
            newStrips->InsertCellPoint(p1 + numPts);
            newStrips->InsertCellPoint(p2 + numPts);
            stripIds->InsertNextId(inCellId);
          }
        } // for each sub-edge
      }   // for each edge
    }     // for each polygon or triangle strip
  }       // for each cell
  cell->Delete();

  // Now Copy cell data.
  outCellId = 0;
  j = lineIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), lineIds->GetId(i), outCellId);
    ++outCellId;
  }
  j = polyIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), polyIds->GetId(i), outCellId);
    ++outCellId;
  }
  j = stripIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), stripIds->GetId(i), outCellId);
    ++outCellId;
  }
  lineIds->Delete();
  stripIds->Delete();
  polyIds->Delete();
  polyIds = nullptr;

  // Send data to output and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  cellIds->Delete();
  mesh->Delete();

  if (newLines)
  {
    output->SetLines(newLines);
    newLines->Delete();
  }

  if (newPolys)
  {
    output->SetPolys(newPolys);
    newPolys->Delete();
  }

  output->SetStrips(newStrips);
  newStrips->Delete();

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void svtkLinearExtrusionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ExtrusionType == SVTK_VECTOR_EXTRUSION)
  {
    os << indent << "Extrusion Type: Extrude along vector\n";
    os << indent << "Vector: (" << this->Vector[0] << ", " << this->Vector[1] << ", "
       << this->Vector[2] << ")\n";
  }
  else if (this->ExtrusionType == SVTK_NORMAL_EXTRUSION)
  {
    os << indent << "Extrusion Type: Extrude along vertex normals\n";
  }
  else // POINT_EXTRUSION
  {
    os << indent << "Extrusion Type: Extrude towards point\n";
    os << indent << "Extrusion Point: (" << this->ExtrusionPoint[0] << ", "
       << this->ExtrusionPoint[1] << ", " << this->ExtrusionPoint[2] << ")\n";
  }

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
