/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelectPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSelectPolyData.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkGarbageCollector.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkTriangleFilter.h"
#include "svtkTriangleStrip.h"

svtkStandardNewMacro(svtkSelectPolyData);

svtkCxxSetObjectMacro(svtkSelectPolyData, Loop, svtkPoints);

// Description:
// Instantiate object with InsideOut turned off.
svtkSelectPolyData::svtkSelectPolyData()
{
  this->GenerateSelectionScalars = 0;
  this->InsideOut = 0;
  this->Loop = nullptr;
  this->SelectionMode = SVTK_INSIDE_SMALLEST_REGION;
  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;
  this->GenerateUnselectedOutput = 0;

  this->SetNumberOfOutputPorts(3);

  svtkPolyData* output2 = svtkPolyData::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();

  svtkPolyData* output3 = svtkPolyData::New();
  this->GetExecutive()->SetOutputData(2, output3);
  output3->Delete();
}

//----------------------------------------------------------------------------
svtkSelectPolyData::~svtkSelectPolyData()
{
  if (this->Loop)
  {
    this->Loop->Delete();
  }
}

//----------------------------------------------------------------------------
svtkPolyData* svtkSelectPolyData::GetUnselectedOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
  {
    return nullptr;
  }

  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
svtkPolyData* svtkSelectPolyData::GetSelectionEdges()
{
  if (this->GetNumberOfOutputPorts() < 3)
  {
    return nullptr;
  }

  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(2));
}

//----------------------------------------------------------------------------
int svtkSelectPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts, numLoopPts;
  svtkPolyData* triMesh;
  svtkPointData *inPD, *outPD = output->GetPointData();
  svtkCellData *inCD, *outCD = output->GetCellData();
  svtkIdType closest = 0, numPolys, i, j;
  int k;
  svtkIdList *loopIds, *edgeIds, *neighbors;
  double x[3], xLoop[3], closestDist2, dist2, t;
  double x0[3], x1[3], vec[3], dir[3], neiX[3];
  svtkCellArray* inPolys;
  svtkPoints* inPts;
  int numNei;
  svtkIdType id, currentId = 0, nextId, pt1, pt2, numCells, neiId;
  svtkIdType* cells;
  const svtkIdType* pts;
  svtkIdType npts;
  svtkIdType numMeshLoopPts;
  svtkIdType prevId;
  svtkIdType ncells;
  int mark, s1, s2, val;

  // Initialize and check data
  svtkDebugMacro(<< "Selecting data...");

  this->GetUnselectedOutput()->Initialize();
  this->GetSelectionEdges()->Initialize();

  if ((numPts = input->GetNumberOfPoints()) < 1)
  {
    svtkErrorMacro("Input contains no points");
    return 1;
  }

  if (this->Loop == nullptr || (numLoopPts = this->Loop->GetNumberOfPoints()) < 3)
  {
    svtkErrorMacro("Please define a loop with at least three points");
    return 1;
  }

  // Okay, now we build unstructured representation. Make sure we're
  // working with triangles.
  svtkTriangleFilter* tf = svtkTriangleFilter::New();
  tf->SetInputData(input);
  tf->PassLinesOff();
  tf->PassVertsOff();
  tf->Update();
  triMesh = tf->GetOutput();
  triMesh->Register(this);
  tf->Delete();
  inPD = triMesh->GetPointData();
  inCD = triMesh->GetCellData();

  numPts = triMesh->GetNumberOfPoints();
  inPts = triMesh->GetPoints();
  inPolys = triMesh->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  if (numPolys < 1)
  {
    svtkErrorMacro("This filter operates on surface primitives");
    tf->Delete();
    triMesh->UnRegister(this);
    return 1;
  }

  this->Mesh = svtkPolyData::New();
  this->Mesh->SetPoints(inPts); // inPts
  this->Mesh->SetPolys(inPolys);
  this->Mesh->BuildLinks(); // to do neighborhood searching
  numCells = this->Mesh->GetNumberOfCells();

  // First thing to do is find the closest mesh points to the loop
  // points. This creates a list of point ids.
  loopIds = svtkIdList::New();
  loopIds->SetNumberOfIds(numLoopPts);

  for (i = 0; i < numLoopPts; i++)
  {
    this->Loop->GetPoint(i, xLoop);
    closest = -1;
    closestDist2 = SVTK_DOUBLE_MAX;

    for (j = 0; j < numPts; j++)
    {
      inPts->GetPoint(j, x);

      dist2 = svtkMath::Distance2BetweenPoints(x, xLoop);
      if (dist2 < closestDist2)
      {
        closest = j;
        closestDist2 = dist2;
      }
    } // for all input points

    loopIds->SetId(i, closest);
  } // for all loop points

  // Now that we've got point ids, we build the loop. Start with the
  // first two points in the loop (which define a line), and find the
  // mesh edge that is directed along the line, and whose
  // end point is closest to the line. Continue until loop closes in on
  // itself.
  edgeIds = svtkIdList::New();
  edgeIds->Allocate(numLoopPts * 10, 1000);
  neighbors = svtkIdList::New();
  neighbors->Allocate(10000);
  edgeIds->InsertNextId(loopIds->GetId(0));

  for (i = 0; i < numLoopPts; i++)
  {
    currentId = loopIds->GetId(i);
    nextId = loopIds->GetId((i + 1) % numLoopPts);
    prevId = (-1);
    inPts->GetPoint(currentId, x);
    inPts->GetPoint(currentId, x0);
    inPts->GetPoint(nextId, x1);
    for (j = 0; j < 3; j++)
    {
      vec[j] = x1[j] - x0[j];
    }

    // track edge
    for (id = currentId; id != nextId;)
    {
      this->GetPointNeighbors(id, neighbors); // points connected by edge
      numNei = neighbors->GetNumberOfIds();
      closest = -1;
      closestDist2 = SVTK_DOUBLE_MAX;
      for (j = 0; j < numNei; j++)
      {
        neiId = neighbors->GetId(j);
        if (neiId == nextId)
        {
          closest = neiId;
          break;
        }
        else
        {
          inPts->GetPoint(neiId, neiX);
          for (k = 0; k < 3; k++)
          {
            dir[k] = neiX[k] - x[k];
          }
          if (neiId != prevId && svtkMath::Dot(dir, vec) > 0.0) // candidate
          {
            dist2 = svtkLine::DistanceToLine(neiX, x0, x1);
            if (dist2 < closestDist2)
            {
              closest = neiId;
              closestDist2 = dist2;
            }
          } // in direction of line
        }
      } // for all neighbors

      if (closest < 0)
      {
        svtkErrorMacro(<< "Can't follow edge");
        triMesh->UnRegister(this);
        this->Mesh->Delete();
        neighbors->Delete();
        edgeIds->Delete();
        loopIds->Delete();
        return 1;
      }
      else
      {
        edgeIds->InsertNextId(closest);
        prevId = id;
        id = closest;
        inPts->GetPoint(id, x);
      }
    } // for tracking edge
  }   // for all edges of loop

  // mainly for debugging
  numMeshLoopPts = edgeIds->GetNumberOfIds();
  svtkCellArray* selectionEdges = svtkCellArray::New();
  selectionEdges->AllocateEstimate(1, numMeshLoopPts);
  selectionEdges->InsertNextCell(numMeshLoopPts);
  for (i = 0; i < numMeshLoopPts; i++)
  {
    selectionEdges->InsertCellPoint(edgeIds->GetId(i));
  }
  this->GetSelectionEdges()->SetPoints(inPts);
  this->GetSelectionEdges()->SetLines(selectionEdges);
  selectionEdges->Delete();

  // Phew...we've defined loop, now want to do a fill so we can extract the
  // inside from the outside. Depending on GenerateSelectionScalars flag,
  // we either set the distance of the points to the selection loop as
  // zero (GenerateSelectionScalarsOff) or we evaluate the distance of these
  // points to the lines. (Later we'll use a connected traversal to compute
  // the distances to the remaining points.)

  // Next, prepare to mark off inside/outside and on boundary of loop.
  // Mark the boundary of the loop using point marks. Also initialize
  // the advancing front (used to mark traversal/compute scalars).
  // Prepare to compute the advancing front
  svtkIntArray* cellMarks = svtkIntArray::New();
  cellMarks->SetNumberOfValues(numCells);
  for (i = 0; i < numCells; i++) // mark unvisited
  {
    cellMarks->SetValue(i, SVTK_INT_MAX);
  }
  svtkIntArray* pointMarks = svtkIntArray::New();
  pointMarks->SetNumberOfValues(numPts);
  for (i = 0; i < numPts; i++) // mark unvisited
  {
    pointMarks->SetValue(i, SVTK_INT_MAX);
  }

  svtkIdList *currentFront = svtkIdList::New(), *tmpFront;
  svtkIdList* nextFront = svtkIdList::New();
  for (i = 0; i < numMeshLoopPts; i++)
  {
    id = edgeIds->GetId(i);
    pointMarks->SetValue(id, 0); // marks the start of the front
    currentFront->InsertNextId(id);
  }

  // Traverse the front as long as we can. We're basically computing a
  // topological distance.
  int maxFront = 0;
  svtkIdType maxFrontCell = (-1);
  int currentFrontNumber = 1, numPtsInFront;
  while ((numPtsInFront = currentFront->GetNumberOfIds()))
  {
    for (i = 0; i < numPtsInFront; i++)
    {
      id = currentFront->GetId(i);
      this->Mesh->GetPointCells(id, ncells, cells);
      for (j = 0; j < ncells; j++)
      {
        id = cells[j];
        if (cellMarks->GetValue(id) == SVTK_INT_MAX)
        {
          if (currentFrontNumber > maxFront)
          {
            maxFrontCell = id;
          }
          cellMarks->SetValue(id, currentFrontNumber);
          this->Mesh->GetCellPoints(id, npts, pts);
          for (k = 0; k < npts; k++)
          {
            if (pointMarks->GetValue(pts[k]) == SVTK_INT_MAX)
            {
              pointMarks->SetValue(pts[k], 1);
              nextFront->InsertNextId(pts[k]);
            }
          }
        }
      } // for cells surrounding point
    }   // all points in front

    currentFrontNumber++;
    tmpFront = currentFront;
    currentFront = nextFront;
    nextFront = tmpFront;
    nextFront->Reset();
  } // while still advancing

  // Okay, now one of the regions is filled with negative values. This fill
  // operation assumes that everything is connected.
  if (this->SelectionMode == SVTK_INSIDE_CLOSEST_POINT_REGION)
  { // find closest point and use as a seed
    for (closestDist2 = SVTK_DOUBLE_MAX, j = 0; j < numPts; j++)
    {
      inPts->GetPoint(j, x);

      dist2 = svtkMath::Distance2BetweenPoints(x, this->ClosestPoint);
      // get closest point not on the boundary
      if (dist2 < closestDist2 && pointMarks->GetValue(j) != 0)
      {
        closest = j;
        closestDist2 = dist2;
      }
    } // for all input points
    this->Mesh->GetPointCells(closest, ncells, cells);
  }

  // We do the fill as a moving front. This is an alternative to recursion. The
  // fill negates one region of the mesh on one side of the loop.
  currentFront->Reset();
  nextFront->Reset();
  currentFront->InsertNextId(maxFrontCell);
  svtkIdType numCellsInFront;

  cellMarks->SetValue(maxFrontCell, -1);

  while ((numCellsInFront = currentFront->GetNumberOfIds()) > 0)
  {
    for (i = 0; i < numCellsInFront; i++)
    {
      id = currentFront->GetId(i);

      this->Mesh->GetCellPoints(id, npts, pts);
      for (j = 0; j < 3; j++)
      {
        pt1 = pts[j];
        pt2 = pts[(j + 1) % 3];
        s1 = pointMarks->GetValue(pt1);
        s2 = pointMarks->GetValue(pt2);

        if (s1 != 0)
        {
          pointMarks->SetValue(pt1, -1);
        }

        if (!(s1 == 0 && s2 == 0))
        {
          this->Mesh->GetCellEdgeNeighbors(id, pt1, pt2, neighbors);
          numNei = neighbors->GetNumberOfIds();
          for (k = 0; k < numNei; k++)
          {
            neiId = neighbors->GetId(k);
            val = cellMarks->GetValue(neiId);
            if (val != -1) //-1 is what we're filling with
            {
              cellMarks->SetValue(neiId, -1);
              nextFront->InsertNextId(neiId);
            }
          }
        } // if can cross boundary
      }   // for all edges of cell
    }     // all cells in front

    tmpFront = currentFront;
    currentFront = nextFront;
    nextFront = tmpFront;
    nextFront->Reset();
  } // while still advancing

  // Now may have to invert fill value depending on what we want to extract
  if (this->SelectionMode == SVTK_INSIDE_SMALLEST_REGION)
  {
    for (i = 0; i < numCells; i++)
    {
      mark = cellMarks->GetValue(i);
      cellMarks->SetValue(i, -mark);
    }
    for (i = 0; i < numPts; i++)
    {
      mark = pointMarks->GetValue(i);
      pointMarks->SetValue(i, -mark);
    }
  }

  // If generating selection scalars, we now have to modify the scalars to
  // approximate a distance function. Otherwise, we can create the output.
  if (!this->GenerateSelectionScalars)
  { // spit out all the negative cells
    svtkCellArray* newPolys = svtkCellArray::New();
    newPolys->AllocateEstimate(numCells / 2, numCells / 2);
    for (i = 0; i < numCells; i++)
    {
      if ((cellMarks->GetValue(i) < 0) || (cellMarks->GetValue(i) > 0 && this->InsideOut))
      {
        this->Mesh->GetCellPoints(i, npts, pts);
        newPolys->InsertNextCell(npts, pts);
      }
    }
    output->SetPoints(inPts);
    output->SetPolys(newPolys);
    outPD->PassData(inPD);
    newPolys->Delete();

    if (this->GenerateUnselectedOutput)
    {
      svtkCellArray* unPolys = svtkCellArray::New();
      unPolys->AllocateEstimate(numCells / 2, numCells / 2);
      for (i = 0; i < numCells; i++)
      {
        if ((cellMarks->GetValue(i) >= 0) || this->InsideOut)
        {
          this->Mesh->GetCellPoints(i, npts, pts);
          unPolys->InsertNextCell(npts, pts);
        }
      }
      this->GetUnselectedOutput()->SetPoints(inPts);
      this->GetUnselectedOutput()->SetPolys(unPolys);
      this->GetUnselectedOutput()->GetPointData()->PassData(inPD);
      unPolys->Delete();
    }
  }
  else // modify scalars to generate selection scalars
  {
    svtkFloatArray* selectionScalars = svtkFloatArray::New();
    selectionScalars->SetNumberOfTuples(numPts);

    // compute distance to lines. Really this should be computed based on
    // the connected fill distance.
    for (j = 0; j < numPts; j++) // compute minimum distance to loop
    {
      if (pointMarks->GetValue(j) != 0)
      {
        inPts->GetPoint(j, x);
        for (closestDist2 = SVTK_DOUBLE_MAX, i = 0; i < numLoopPts; i++)
        {
          this->Loop->GetPoint(i, x0);
          this->Loop->GetPoint((i + 1) % numLoopPts, x1);
          dist2 = svtkLine::DistanceToLine(x, x0, x1, t, xLoop);
          if (dist2 < closestDist2)
          {
            closestDist2 = dist2;
          }
        } // for all loop edges
        closestDist2 = sqrt((double)closestDist2);
        selectionScalars->SetComponent(j, 0, closestDist2 * pointMarks->GetValue(j));
      }
    }

    // Now, determine the sign of those points "on the boundary" to give a
    // better approximation to the scalar field.
    for (j = 0; j < numMeshLoopPts; j++)
    {
      id = edgeIds->GetId(j);
      inPts->GetPoint(id, x);
      for (closestDist2 = SVTK_DOUBLE_MAX, i = 0; i < numLoopPts; i++)
      {
        this->Loop->GetPoint(i, x0);
        this->Loop->GetPoint((i + 1) % numLoopPts, x1);
        dist2 = svtkLine::DistanceToLine(x, x0, x1, t, xLoop);
        if (dist2 < closestDist2)
        {
          closestDist2 = dist2;
          neiX[0] = xLoop[0];
          neiX[1] = xLoop[1];
          neiX[2] = xLoop[2];
        }
      } // for all loop edges
      closestDist2 = sqrt((double)closestDist2);

      // find neighbor not on boundary and compare negative/positive values
      // to see what makes the most sense
      this->GetPointNeighbors(id, neighbors);
      numNei = neighbors->GetNumberOfIds();
      for (dist2 = 0.0, i = 0; i < numNei; i++)
      {
        neiId = neighbors->GetId(i);
        if (pointMarks->GetValue(neiId) != 0) // find the furthest away
        {
          if (fabs(selectionScalars->GetComponent(neiId, 0)) > dist2)
          {
            currentId = neiId;
            dist2 = fabs(selectionScalars->GetComponent(neiId, 0));
          }
        }
      }

      inPts->GetPoint(currentId, x0);
      if (svtkMath::Distance2BetweenPoints(x0, x) < svtkMath::Distance2BetweenPoints(x0, neiX))
      {
        closestDist2 = closestDist2 * pointMarks->GetValue(currentId);
      }
      else
      {
        closestDist2 = -closestDist2 * pointMarks->GetValue(currentId);
      }

      selectionScalars->SetComponent(id, 0, closestDist2);
    } // for all boundary points

    output->CopyStructure(this->Mesh); // pass geometry/topology unchanged
    int idx = outPD->AddArray(selectionScalars);
    outPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    outPD->CopyScalarsOff();
    outPD->PassData(inPD);
    outCD->PassData(inCD);
    selectionScalars->Delete();
  }

  // Clean up and update output
  triMesh->UnRegister(this);
  this->Mesh->Delete();
  neighbors->Delete();
  edgeIds->Delete();
  loopIds->Delete();
  cellMarks->Delete();
  pointMarks->Delete();
  currentFront->Delete();
  nextFront->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkSelectPolyData::GetPointNeighbors(svtkIdType ptId, svtkIdList* nei)
{
  svtkIdType ncells;
  int i, j;
  svtkIdType* cells;
  const svtkIdType* pts;
  svtkIdType npts;

  nei->Reset();
  this->Mesh->GetPointCells(ptId, ncells, cells);
  for (i = 0; i < ncells; i++)
  {
    this->Mesh->GetCellPoints(cells[i], npts, pts);
    for (j = 0; j < 3; j++)
    {
      if (pts[j] != ptId)
      {
        nei->InsertUniqueId(pts[j]);
      }
    }
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkSelectPolyData::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Loop != nullptr)
  {
    time = this->Loop->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkSelectPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "Generate Unselected Output: " << (this->GenerateUnselectedOutput ? "On\n" : "Off\n");

  os << indent << "Inside Mode: ";
  os << this->GetSelectionModeAsString() << "\n";

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", " << this->ClosestPoint[1]
     << ", " << this->ClosestPoint[2] << ")\n";

  os << indent
     << "Generate Selection Scalars: " << (this->GenerateSelectionScalars ? "On\n" : "Off\n");

  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");

  if (this->Loop)
  {
    os << indent << "Loop of " << this->Loop->GetNumberOfPoints() << "points defined\n";
  }
  else
  {
    os << indent << "Loop not defined\n";
  }
}
