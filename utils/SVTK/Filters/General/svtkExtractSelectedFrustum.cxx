/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedFrustum.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectedFrustum.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPlanes.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkTimerLog.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVoxel.h"

svtkStandardNewMacro(svtkExtractSelectedFrustum);
svtkCxxSetObjectMacro(svtkExtractSelectedFrustum, Frustum, svtkPlanes);

// set to 4 to ignore the near and far planes which are almost always passed
#define MAXPLANE 6

//----------------------------------------------------------------------------
svtkExtractSelectedFrustum::svtkExtractSelectedFrustum(svtkPlanes* f)
{
  this->SetNumberOfInputPorts(2);

  this->ShowBounds = 0;

  this->FieldType = 0;
  this->ContainingCells = 0;
  this->InsideOut = 0;

  this->NumRejects = 0;
  this->NumIsects = 0;
  this->NumAccepts = 0;

  this->ClipPoints = svtkPoints::New();
  this->ClipPoints->SetNumberOfPoints(8);
  double verts[32] = // an inside out unit cube - which selects nothing
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0 };
  this->Frustum = f;
  if (this->Frustum)
  {
    this->Frustum->Register(this);
  }
  else
  {
    this->Frustum = svtkPlanes::New();
    this->CreateFrustum(verts);
  }
}

//----------------------------------------------------------------------------
svtkExtractSelectedFrustum::~svtkExtractSelectedFrustum()
{
  this->Frustum->Delete();
  this->ClipPoints->Delete();
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
svtkMTimeType svtkExtractSelectedFrustum::GetMTime()
{
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType impFuncMTime;

  if (this->Frustum != nullptr)
  {
    impFuncMTime = this->Frustum->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//--------------------------------------------------------------------------
void svtkExtractSelectedFrustum::CreateFrustum(double verts[32])
{
  // for debugging
  for (int i = 0; i < 8; i++)
  {
    this->ClipPoints->SetPoint(i, &verts[i * 4]);
  }
  this->ClipPoints->Modified();

  svtkPoints* points = svtkPoints::New();
  points->SetNumberOfPoints(6);

  svtkDoubleArray* norms = svtkDoubleArray::New();
  norms->SetNumberOfComponents(3);
  norms->SetNumberOfTuples(6);

  // left
  this->ComputePlane(0, &verts[0 * 4], &verts[2 * 4], &verts[3 * 4], points, norms);
  // right
  this->ComputePlane(1, &verts[7 * 4], &verts[6 * 4], &verts[4 * 4], points, norms);
  // bottom
  this->ComputePlane(2, &verts[5 * 4], &verts[4 * 4], &verts[0 * 4], points, norms);
  // top
  this->ComputePlane(3, &verts[2 * 4], &verts[6 * 4], &verts[7 * 4], points, norms);
  // near
  this->ComputePlane(4, &verts[6 * 4], &verts[2 * 4], &verts[0 * 4], points, norms);
  // far
  this->ComputePlane(5, &verts[1 * 4], &verts[3 * 4], &verts[7 * 4], points, norms);

  this->Frustum->SetPoints(points);
  this->Frustum->SetNormals(norms);
  points->Delete();
  norms->Delete();
}

//--------------------------------------------------------------------------
void svtkExtractSelectedFrustum::ComputePlane(
  int idx, double v0[3], double v1[3], double v2[3], svtkPoints* points, svtkDoubleArray* norms)
{
  points->SetPoint(idx, v0[0], v0[1], v0[2]);

  double e0[3];
  e0[0] = v1[0] - v0[0];
  e0[1] = v1[1] - v0[1];
  e0[2] = v1[2] - v0[2];

  double e1[3];
  e1[0] = v2[0] - v0[0];
  e1[1] = v2[1] - v0[1];
  e1[2] = v2[2] - v0[2];

  double n[3];
  svtkMath::Cross(e0, e1, n);
  svtkMath::Normalize(n);

  norms->SetTuple(idx, n);
}

//----------------------------------------------------------------------------
// needed because parent class sets output type to input type
// and we sometimes want to change it to make an UnstructuredGrid regardless of
// input type
int svtkExtractSelectedFrustum::RequestDataObject(
  svtkInformation* req, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (input && this->ShowBounds)
  {
    svtkInformation* info = outputVector->GetInformationObject(0);
    svtkDataSet* output = svtkDataSet::GetData(info);
    if (!output || !output->IsA("svtkUnstructuredGrid"))
    {
      svtkUnstructuredGrid* newOutput = svtkUnstructuredGrid::New();
      info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
  }

  return this->Superclass::RequestDataObject(req, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkExtractSelectedFrustum::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // If we have a svtkSelection on the second input, use its frustum.
  if (this->GetNumberOfInputConnections(1) == 1)
  {
    svtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
    svtkSelection* sel = svtkSelection::SafeDownCast(selInfo->Get(svtkDataObject::DATA_OBJECT()));
    svtkSelectionNode* node = nullptr;
    if (sel->GetNumberOfNodes() == 1)
    {
      node = sel->GetNode(0);
    }
    if (node && node->GetContentType() == svtkSelectionNode::FRUSTUM)
    {
      svtkDoubleArray* corners = svtkArrayDownCast<svtkDoubleArray>(node->GetSelectionList());
      this->CreateFrustum(corners->GetPointer(0));
      if (node->GetProperties()->Has(svtkSelectionNode::INVERSE()))
      {
        this->SetInsideOut(node->GetProperties()->Get(svtkSelectionNode::INVERSE()));
      }
      if (node->GetProperties()->Has(svtkSelectionNode::FIELD_TYPE()))
      {
        this->SetFieldType(node->GetProperties()->Get(svtkSelectionNode::FIELD_TYPE()));
      }
      if (node->GetProperties()->Has(svtkSelectionNode::CONTAINING_CELLS()))
      {
        this->SetContainingCells(node->GetProperties()->Get(svtkSelectionNode::CONTAINING_CELLS()));
      }
    }
  }

  if (!this->Frustum)
  {
    // if we don't have a frustum, quietly select nothing
    return 1;
  }

  if (this->Frustum->GetNumberOfPlanes() != 6)
  {
    svtkErrorMacro(<< "Frustum must have six planes.");
    return 0;
  }

  // get the input and output
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkUnstructuredGrid* outputUG =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (this->ShowBounds && !this->PreserveTopology)
  {
    // for debugging, shows rough outline of the selection frustum
    // only valid if CreateFrustum was called
    outputUG->Allocate(1); // allocate storage for geometry/topology
    svtkLine* linesRays = svtkLine::New();
    linesRays->GetPointIds()->SetId(0, 0);
    linesRays->GetPointIds()->SetId(1, 1);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 2);
    linesRays->GetPointIds()->SetId(1, 3);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 4);
    linesRays->GetPointIds()->SetId(1, 5);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 6);
    linesRays->GetPointIds()->SetId(1, 7);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());

    linesRays->GetPointIds()->SetId(0, 0);
    linesRays->GetPointIds()->SetId(1, 2);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());

    linesRays->GetPointIds()->SetId(0, 2);
    linesRays->GetPointIds()->SetId(1, 6);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 6);
    linesRays->GetPointIds()->SetId(1, 4);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 4);
    linesRays->GetPointIds()->SetId(1, 0);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());

    linesRays->GetPointIds()->SetId(0, 1);
    linesRays->GetPointIds()->SetId(1, 3);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());

    linesRays->GetPointIds()->SetId(0, 3);
    linesRays->GetPointIds()->SetId(1, 7);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 7);
    linesRays->GetPointIds()->SetId(1, 5);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 5);
    linesRays->GetPointIds()->SetId(1, 1);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());

    outputUG->SetPoints(this->ClipPoints);
    linesRays->Delete();
    return 1;
  }

  double bounds[6];
  svtkIdType i;
  double x[3];
  input->GetBounds(bounds);
  if (!this->OverallBoundsTest(bounds))
  {
    return 1;
  }

  svtkIdType ptId, numPts, newPointId;
  svtkIdType numCells, cellId, newCellId, numCellPts;
  svtkIdType* pointMap;
  svtkCell* cell;
  svtkIdList* cellPts;
  svtkIdList* newCellPts;
  int isect;

  svtkSignedCharArray* pointInArray = svtkSignedCharArray::New();
  svtkSignedCharArray* cellInArray = svtkSignedCharArray::New();
  svtkPoints* newPts = svtkPoints::New();

  /*
  int NUMCELLS = 0;
  int NUMPTS = 0;
  cerr << "{" << endl;
  svtkTimerLog *timer = svtkTimerLog::New();
  timer->StartTimer();
  */

  svtkDataSet* outputDS = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = outputDS->GetPointData();
  svtkCellData* outputCD = outputDS->GetCellData();

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  pointMap = new svtkIdType[numPts]; // maps old point ids into new
  newCellPts = svtkIdList::New();
  newCellPts->Allocate(SVTK_CELL_SIZE);

  svtkIdTypeArray* originalCellIds = nullptr;
  svtkIdTypeArray* originalPointIds = nullptr;

  signed char flag = this->InsideOut ? 1 : -1;

  if (this->PreserveTopology)
  {
    // the output is a copy of the input, with two new arrays defined
    outputDS->ShallowCopy(input);

    pointInArray->SetNumberOfComponents(1);
    pointInArray->SetNumberOfTuples(numPts);
    for (i = 0; i < numPts; i++)
    {
      pointInArray->SetValue(i, flag);
    }
    pointInArray->SetName("svtkInsidedness");
    outputPD->AddArray(pointInArray);
    outputPD->SetScalars(pointInArray);

    cellInArray->SetNumberOfComponents(1);
    cellInArray->SetNumberOfTuples(numCells);
    for (i = 0; i < numCells; i++)
    {
      cellInArray->SetValue(i, flag);
    }
    cellInArray->SetName("svtkInsidedness");
    outputCD->AddArray(cellInArray);
    outputCD->SetScalars(cellInArray);
  }
  else
  {
    // the output is a new unstructured grid
    outputUG->Allocate(numCells / 4); // allocate storage for geometry/topology
    newPts->Allocate(numPts / 4, numPts);
    outputPD->SetCopyGlobalIds(1);
    outputPD->CopyFieldOff("svtkOriginalPointIds");
    outputPD->CopyAllocate(pd);

    if ((this->FieldType == svtkSelectionNode::CELL) || this->PreserveTopology ||
      this->ContainingCells)
    {
      outputCD->SetCopyGlobalIds(1);
      outputCD->CopyFieldOff("svtkOriginalCellIds");
      outputCD->CopyAllocate(cd);

      originalCellIds = svtkIdTypeArray::New();
      originalCellIds->SetNumberOfComponents(1);
      originalCellIds->SetName("svtkOriginalCellIds");
      outputCD->AddArray(originalCellIds);
      originalCellIds->Delete();
    }

    originalPointIds = svtkIdTypeArray::New();
    originalPointIds->SetNumberOfComponents(1);
    originalPointIds->SetName("svtkOriginalPointIds");
    outputPD->AddArray(originalPointIds);
    originalPointIds->Delete();
  }

  flag = -flag;

  svtkIdType updateInterval;

  if (this->FieldType == svtkSelectionNode::CELL)
  {
    // cell based isect test, a cell is inside if any part of it is inside the
    // frustum, a point is inside if it belongs to an inside cell, or is not
    // in any cell but is inside the frustum

    updateInterval = numCells / 1000 + 1;

    // initialize all points to say not looked at
    for (ptId = 0; ptId < numPts; ptId++)
    {
      pointMap[ptId] = -1;
    }

    /*
    timer->StopTimer();
    cerr << "  PTINIT " << timer->GetElapsedTime() << endl;
    timer->StartTimer();
    */
    // Loop over all cells to see whether they are inside.
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (!(cellId % updateInterval)) // manage progress reports
      {
        this->UpdateProgress(static_cast<double>(cellId) / numCells);
      }

      input->GetCellBounds(cellId, bounds);

      cell = input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      numCellPts = cell->GetNumberOfPoints();
      newCellPts->Reset();

      isect = this->ABoxFrustumIsect(bounds, cell);
      if ((isect == 1 && flag == 1) || (isect == 0 && flag == -1))
      {
        /*
        NUMCELLS++;
        */

        // intersects, put all of the points inside
        for (i = 0; i < numCellPts; i++)
        {
          ptId = cellPts->GetId(i);
          newPointId = pointMap[ptId];
          if (newPointId < 0)
          {
            /*
            NUMPTS++;
            */

            input->GetPoint(ptId, x);
            if (this->PreserveTopology)
            {
              pointInArray->SetValue(ptId, flag);
              newPointId = ptId;
            }
            else
            {
              newPointId = newPts->InsertNextPoint(x);
              outputPD->CopyData(pd, ptId, newPointId);
              originalPointIds->InsertNextValue(ptId);
            }
            pointMap[ptId] = newPointId;
          }
          newCellPts->InsertId(i, newPointId);
        }

        if (this->PreserveTopology)
        {
          cellInArray->SetValue(cellId, flag);
        }
        else
        {
          // special handling for polyhedron cells
          if (svtkUnstructuredGrid::SafeDownCast(input) && cell->GetCellType() == SVTK_POLYHEDRON)
          {
            newCellPts->Reset();
            svtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(cellId, newCellPts);
            svtkUnstructuredGrid::ConvertFaceStreamPointIds(newCellPts, pointMap);
          }
          newCellId = outputUG->InsertNextCell(cell->GetCellType(), newCellPts);
          outputCD->CopyData(cd, cellId, newCellId);
          originalCellIds->InsertNextValue(cellId);
        }
      }

      /*
        if (isect == -1) //complete reject, remember these points are outside
        {
        for (i=0; i < numCellPts; i++)
          {
          ptId = cellPts->GetId(i);
          pointMap[ptId] = -2;
          }
        }
      */
    } // for all cells

    /*
    timer->StopTimer();
    cerr << "  CLIN " << timer->GetElapsedTime() << endl;
    timer->StartTimer();
    cerr << "  AFTERCELL NUMPTS = " << NUMPTS << endl;
    */

    // there could be some points that are not used by any cell
    for (ptId = 0; ptId < numPts; ptId++)
    {
      if (pointMap[ptId] == -1) // point wasn't attached to a cell
      {
        input->GetPoint(ptId, x);
        if ((this->Frustum->EvaluateFunction(x) * flag) < 0.0)
        {
          /*
          NUMPTS++;
          */
          if (this->PreserveTopology)
          {
            pointInArray->SetValue(ptId, flag);
          }
          else
          {
            newPointId = newPts->InsertNextPoint(x);
            outputPD->CopyData(pd, ptId, newPointId);
            originalPointIds->InsertNextValue(ptId);
          }
        }
      }
    }
  }

  else // this->FieldType == svtkSelectionNode::POINT
  {
    // point based isect test

    updateInterval = numPts / 1000 + 1;

    // run through points and decide which ones are inside
    for (ptId = 0; ptId < numPts; ptId++)
    {

      if (!(ptId % updateInterval)) // manage progress reports
      {
        this->UpdateProgress(static_cast<double>(ptId) / numPts);
      }

      input->GetPoint(ptId, x);
      pointMap[ptId] = -1;
      if ((this->Frustum->EvaluateFunction(x) * flag) < 0.0)
      {
        /*
        NUMPTS++;
        */
        if (this->PreserveTopology)
        {
          newPointId = ptId;
          pointInArray->SetValue(ptId, flag);
        }
        else
        {
          newPointId = newPts->InsertNextPoint(x);
          outputPD->CopyData(pd, ptId, newPointId);
          originalPointIds->InsertNextValue(ptId);
        }
        pointMap[ptId] = newPointId;
      }
    }

    /*
    timer->StopTimer();
    cerr << "  PTIN " << timer->GetElapsedTime() << endl;
    timer->StartTimer();
    */

    if (this->PreserveTopology)
    {
      // we have already created a copy of the input and marked points as
      // being in or not
      if (this->ContainingCells)
      {
        // mark the cells that have at least one point inside as being in
        for (cellId = 0; cellId < numCells; cellId++)
        {
          cell = input->GetCell(cellId);
          cellPts = cell->GetPointIds();
          numCellPts = cell->GetNumberOfPoints();
          for (i = 0; i < numCellPts; i++)
          {
            ptId = cellPts->GetId(i);
            newPointId = pointMap[ptId];
            if (newPointId >= 0)
            {
              cellInArray->SetValue(cellId, flag);
              break;
            }
          }
        }
      }
    }
    else
    {
      if (this->ContainingCells)
      {
        svtkIdType* pointMap2 = new svtkIdType[numPts]; // maps old point ids into new
        memcpy(pointMap2, pointMap, numPts * sizeof(svtkIdType));

        // run through cells and accept those with any point inside
        for (cellId = 0; cellId < numCells; cellId++)
        {
          cell = input->GetCell(cellId);
          cellPts = cell->GetPointIds();
          numCellPts = cell->GetNumberOfPoints();
          newCellPts->Reset();

          isect = 0;
          for (i = 0; i < numCellPts; i++)
          {
            ptId = cellPts->GetId(i);
            newPointId = pointMap[ptId];
            if (newPointId >= 0)
            {
              isect = 1;
              break; // this cell won't be inserted
            }
          }
          if (isect)
          {
            for (i = 0; i < numCellPts; i++)
            {
              ptId = cellPts->GetId(i);
              newPointId = pointMap[ptId];
              if (newPointId < 0)
              {
                // this vertex wasn't inside
                newPointId = pointMap2[ptId];
                if (newPointId < 0)
                {
                  // we haven't encountered it before, add it and remember
                  input->GetPoint(ptId, x);
                  newPointId = newPts->InsertNextPoint(x);
                  outputPD->CopyData(pd, ptId, newPointId);
                  originalPointIds->InsertNextValue(ptId);
                  pointMap2[ptId] = newPointId;
                }
              }
              newCellPts->InsertId(i, newPointId);
            }
            // special handling for polyhedron cells
            if (svtkUnstructuredGrid::SafeDownCast(input) && cell->GetCellType() == SVTK_POLYHEDRON)
            {
              newCellPts->Reset();
              svtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(cellId, newCellPts);
              svtkUnstructuredGrid::ConvertFaceStreamPointIds(newCellPts, pointMap2);
            }
            newCellId = outputUG->InsertNextCell(cell->GetCellType(), newCellPts);
            outputCD->CopyData(cd, cellId, newCellId);
            originalCellIds->InsertNextValue(cellId);
          }
        }
        delete[] pointMap2;
      }
      else
      {
        // produce a new svtk_vertex cell for each accepted point
        for (ptId = 0; ptId < newPts->GetNumberOfPoints(); ptId++)
        {
          newCellPts->Reset();
          newCellPts->InsertId(0, ptId);
          outputUG->InsertNextCell(SVTK_VERTEX, newCellPts);
        }
      }
    }
  }

  /*
  cerr << "  REJECTS " << this->NumRejects << " ";
  this->NumRejects = 0;
  cerr << "  ACCEPTS " << this->NumAccepts << " ";
  this->NumAccepts = 0;
  cerr << "  ISECTS " << this->NumIsects << endl;
  this->NumIsects = 0;
  cerr << "  @END NUMPTS = " << NUMPTS << endl;
  cerr << "  @END NUMCELLS = " << NUMCELLS << endl;
  timer->StopTimer();
  cerr << "  " << timer->GetElapsedTime() << endl;
  timer->Delete();
  cerr << "}" << endl;
  */

  // Update ourselves and release memory
  delete[] pointMap;
  newCellPts->Delete();
  pointInArray->Delete();
  cellInArray->Delete();

  if (!this->PreserveTopology)
  {
    outputUG->SetPoints(newPts);
  }
  newPts->Delete();
  outputDS->Squeeze();

  return 1;
}

//--------------------------------------------------------------------------
int svtkExtractSelectedFrustum::OverallBoundsTest(double* bounds)
{
  svtkIdType i;
  double x[3];

  // find the near and far vertices to each plane for quick in/out tests
  for (i = 0; i < MAXPLANE; i++)
  {
    this->Frustum->GetNormals()->GetTuple(i, x);
    int xside = (x[0] > 0) ? 1 : 0;
    int yside = (x[1] > 0) ? 1 : 0;
    int zside = (x[2] > 0) ? 1 : 0;
    this->np_vertids[i][0] = (1 - xside) * 4 + (1 - yside) * 2 + (1 - zside);
    this->np_vertids[i][1] = xside * 4 + yside * 2 + zside;
  }

  svtkVoxel* vox = svtkVoxel::New();
  svtkPoints* p = vox->GetPoints();
  p->SetPoint(0, bounds[0], bounds[2], bounds[4]);
  p->SetPoint(1, bounds[1], bounds[2], bounds[4]);
  p->SetPoint(2, bounds[0], bounds[3], bounds[4]);
  p->SetPoint(3, bounds[1], bounds[3], bounds[4]);
  p->SetPoint(4, bounds[0], bounds[2], bounds[5]);
  p->SetPoint(5, bounds[1], bounds[2], bounds[5]);
  p->SetPoint(6, bounds[0], bounds[3], bounds[5]);
  p->SetPoint(7, bounds[1], bounds[3], bounds[5]);

  int rc;
  rc = this->ABoxFrustumIsect(bounds, vox);
  vox->Delete();
  return (rc > 0);
}

//--------------------------------------------------------------------------
// Intersect the cell (with its associated bounds) with the clipping frustum.
// Return 1 if at least partially inside, 0 otherwise.
// Also return a distance to the near plane.
int svtkExtractSelectedFrustum::ABoxFrustumIsect(double* bounds, svtkCell* cell)
{
  if (bounds[0] > bounds[1] || bounds[2] > bounds[3] || bounds[4] > bounds[5])
  {
    return this->IsectDegenerateCell(cell);
  }

  // convert bounds to 8 vertices
  double verts[8][3];
  verts[0][0] = bounds[0];
  verts[0][1] = bounds[2];
  verts[0][2] = bounds[4];
  verts[1][0] = bounds[0];
  verts[1][1] = bounds[2];
  verts[1][2] = bounds[5];
  verts[2][0] = bounds[0];
  verts[2][1] = bounds[3];
  verts[2][2] = bounds[4];
  verts[3][0] = bounds[0];
  verts[3][1] = bounds[3];
  verts[3][2] = bounds[5];
  verts[4][0] = bounds[1];
  verts[4][1] = bounds[2];
  verts[4][2] = bounds[4];
  verts[5][0] = bounds[1];
  verts[5][1] = bounds[2];
  verts[5][2] = bounds[5];
  verts[6][0] = bounds[1];
  verts[6][1] = bounds[3];
  verts[6][2] = bounds[4];
  verts[7][0] = bounds[1];
  verts[7][1] = bounds[3];
  verts[7][2] = bounds[5];

  int intersect = 0;

  // reject if any plane rejects the entire bbox
  for (int pid = 0; pid < MAXPLANE; pid++)
  {
    svtkPlane* plane = this->Frustum->GetPlane(pid);
    double dist;
    int nvid;
    int pvid;
    nvid = this->np_vertids[pid][0];
    dist = plane->EvaluateFunction(verts[nvid]);
    if (dist > 0.0)
    {
      /*
      this->NumRejects++;
      */
      return 0;
    }
    pvid = this->np_vertids[pid][1];
    dist = plane->EvaluateFunction(verts[pvid]);
    if (dist > 0.0)
    {
      intersect = 1;
      break;
    }
  }

  // accept if entire bbox is inside all planes
  if (!intersect)
  {
    /*
    this->NumAccepts++;
    */
    return 1;
  }

  // otherwise we have to do clipping tests to decide if actually insects
  /*
  this->NumIsects++;
  */
  svtkCell* face;
  svtkCell* edge;
  svtkPoints* pts = nullptr;
  double* vertbuffer;
  int maxedges = 16;
  // be ready to resize if we hit a polygon with many vertices
  vertbuffer = new double[3 * maxedges * 3];
  double* vlist = &vertbuffer[0 * maxedges * 3];
  double* wvlist = &vertbuffer[1 * maxedges * 3];
  double* ovlist = &vertbuffer[2 * maxedges * 3];

  int nfaces = cell->GetNumberOfFaces();
  if (nfaces < 1)
  {
    // some 2D cells have no faces, only edges
    int nedges = cell->GetNumberOfEdges();
    if (nedges < 1)
    {
      // SVTK_LINE and SVTK_POLY_LINE have no "edges" -- the cells
      // themselves are edges.  We catch them here and assemble the
      // list of vertices by hand because the code below assumes that
      // GetNumberOfEdges()==0 means a degenerate cell containing only
      // points.
      if (cell->GetCellType() == SVTK_LINE)
      {
        nedges = 2;
        svtkPoints* points = cell->GetPoints();
        points->GetPoint(0, &vlist[0 * 3]);
        points->GetPoint(1, &vlist[1 * 3]);
      }
      else if (cell->GetCellType() == SVTK_POLY_LINE)
      {
        nedges = cell->GetPointIds()->GetNumberOfIds();
        svtkPoints* points = cell->GetPoints();
        if (nedges + 4 > maxedges)
        {
          delete[] vertbuffer;
          maxedges = (nedges + 4) * 2;
          vertbuffer = new double[3 * maxedges * 3];
          vlist = &vertbuffer[0 * maxedges * 3];
          wvlist = &vertbuffer[1 * maxedges * 3];
          ovlist = &vertbuffer[2 * maxedges * 3];
        }
        for (svtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i)
        {
          points->GetPoint(i, &vlist[i * 3]);
        }
      }
      else
      {
        delete[] vertbuffer;
        return this->IsectDegenerateCell(cell);
      }
    }
    if (nedges + 4 > maxedges)
    {
      delete[] vertbuffer;
      maxedges = (nedges + 4) * 2;
      vertbuffer = new double[3 * maxedges * 3];
      vlist = &vertbuffer[0 * maxedges * 3];
      wvlist = &vertbuffer[1 * maxedges * 3];
      ovlist = &vertbuffer[2 * maxedges * 3];
    }
    edge = cell->GetEdge(0);
    if (edge)
    {
      pts = edge->GetPoints();
      pts->GetPoint(0, &vlist[0 * 3]);
      pts->GetPoint(1, &vlist[1 * 3]);
    }
    switch (cell->GetCellType())
    {
      case SVTK_PIXEL:
      {
        edge = cell->GetEdge(2);
        pts = edge->GetPoints();
        pts->GetPoint(0, &vlist[3 * 3]);
        pts->GetPoint(1, &vlist[2 * 3]);
        break;
      }
      case SVTK_QUAD:
      {
        edge = cell->GetEdge(2);
        pts = edge->GetPoints();
        pts->GetPoint(0, &vlist[2 * 3]);
        pts->GetPoint(1, &vlist[3 * 3]);
        break;
      }
      case SVTK_TRIANGLE:
      {
        edge = cell->GetEdge(1);
        pts = edge->GetPoints();
        pts->GetPoint(1, &vlist[2 * 3]);
        break;
      }
      case SVTK_LINE:
      case SVTK_POLY_LINE:
      {
        break;
      }
      default:
      {
        for (int e = 1; e < nedges - 1; e++)
        {
          edge = cell->GetEdge(e);
          pts = edge->GetPoints();
          pts->GetPoint(1, &vlist[(e + 1) * 3]); // get second point of the edge
        }
        break;
      }
    }
    if (this->FrustumClipPolygon(nedges, vlist, wvlist, ovlist))
    {
      delete[] vertbuffer;
      return 1;
    }
  }
  else
  {

    // go around edges of each face and clip to planes
    // if nothing remains at the end, then we do not intersect and reject
    for (int f = 0; f < nfaces; f++)
    {
      face = cell->GetFace(f);

      int nedges = face->GetNumberOfEdges();
      if (nedges < 1)
      {
        if (this->IsectDegenerateCell(face))
        {
          delete[] vertbuffer;
          return 1;
        }
        continue;
      }
      if (nedges + 4 > maxedges)
      {
        delete[] vertbuffer;
        maxedges = (nedges + 4) * 2;
        vertbuffer = new double[3 * maxedges * 3];
        vlist = &vertbuffer[0 * maxedges * 3];
        wvlist = &vertbuffer[1 * maxedges * 3];
        ovlist = &vertbuffer[2 * maxedges * 3];
      }
      edge = face->GetEdge(0);
      pts = edge->GetPoints();
      pts->GetPoint(0, &vlist[0 * 3]);
      pts->GetPoint(1, &vlist[1 * 3]);
      switch (face->GetCellType())
      {
        case SVTK_PIXEL:
          edge = face->GetEdge(2);
          pts = edge->GetPoints();
          pts->GetPoint(0, &vlist[3 * 3]);
          pts->GetPoint(1, &vlist[2 * 3]);
          break;
        case SVTK_QUAD:
        {
          edge = face->GetEdge(2);
          pts = edge->GetPoints();
          pts->GetPoint(0, &vlist[2 * 3]);
          pts->GetPoint(1, &vlist[3 * 3]);
          break;
        }
        case SVTK_TRIANGLE:
        {
          edge = face->GetEdge(1);
          pts = edge->GetPoints();
          pts->GetPoint(1, &vlist[2 * 3]);
          break;
        }
        case SVTK_LINE:
        {
          break;
        }
        default:
        {
          for (int e = 1; e < nedges - 1; e++)
          {
            edge = cell->GetEdge(e);
            pts = edge->GetPoints();
            pts->GetPoint(1, &vlist[(e + 1) * 3]); // get second point of the edge
          }
          break;
        }
      }
      if (this->FrustumClipPolygon(nedges, vlist, wvlist, ovlist))
      {
        delete[] vertbuffer;
        return 1;
      }
    }
  }

  delete[] vertbuffer;
  return 0;
}

//--------------------------------------------------------------------------
// handle degenerate cells by testing each point, if any in, then in
int svtkExtractSelectedFrustum::IsectDegenerateCell(svtkCell* cell)
{
  svtkIdType npts = cell->GetNumberOfPoints();
  svtkPoints* pts = cell->GetPoints();
  double x[3];
  for (svtkIdType i = 0; i < npts; i++)
  {
    pts->GetPoint(i, x);
    if (this->Frustum->EvaluateFunction(x) < 0.0)
    {
      return 1;
    }
  }
  return 0;
}

//--------------------------------------------------------------------------
// clips the polygon against the frustum
// if there is no intersection, returns 0
// if there is an intersection, returns 1
// update ovlist to contain the resulting clipped vertices
int svtkExtractSelectedFrustum::FrustumClipPolygon(
  int nverts, double* ivlist, double* wvlist, double* ovlist)
{
  int nwverts = nverts;
  memcpy(wvlist, ivlist, nverts * sizeof(double) * 3);

  int noverts = 0;
  int pid;
  for (pid = 0; pid < MAXPLANE; pid++)
  {
    noverts = 0;
    this->PlaneClipPolygon(nwverts, wvlist, pid, noverts, ovlist);
    if (noverts == 0)
    {
      return 0;
    }
    memcpy(wvlist, ovlist, noverts * sizeof(double) * 3);
    nwverts = noverts;
  }

  return 1;
}

//--------------------------------------------------------------------------
// clips a polygon against the numbered plane, resulting vertices are stored
// in ovlist, noverts
void svtkExtractSelectedFrustum::PlaneClipPolygon(
  int nverts, double* ivlist, int pid, int& noverts, double* ovlist)
{
  int vid;
  // run around the polygon and clip to this edge
  for (vid = 0; vid < nverts - 1; vid++)
  {
    this->PlaneClipEdge(&ivlist[vid * 3], &ivlist[(vid + 1) * 3], pid, noverts, ovlist);
  }
  this->PlaneClipEdge(&ivlist[(nverts - 1) * 3], &ivlist[0 * 3], pid, noverts, ovlist);
}

//--------------------------------------------------------------------------
// clips a line segment against the numbered plane.
// intersection point and the second vertex are added to overts if on or inside
void svtkExtractSelectedFrustum::PlaneClipEdge(
  double* V0, double* V1, int pid, int& noverts, double* overts)
{
  double t = 0.0;
  double ISECT[3];
  int rc = svtkPlane::IntersectWithLine(V0, V1, this->Frustum->GetNormals()->GetTuple(pid),
    this->Frustum->GetPoints()->GetPoint(pid), t, ISECT);

  if (rc)
  {
    overts[noverts * 3 + 0] = ISECT[0];
    overts[noverts * 3 + 1] = ISECT[1];
    overts[noverts * 3 + 2] = ISECT[2];
    noverts++;
  }

  svtkPlane* plane = this->Frustum->GetPlane(pid);

  if (plane->EvaluateFunction(V1) < 0.0)
  {
    overts[noverts * 3 + 0] = V1[0];
    overts[noverts * 3 + 1] = V1[1];
    overts[noverts * 3 + 2] = V1[2];
    noverts++;
  }
}

//----------------------------------------------------------------------------
void svtkExtractSelectedFrustum::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Frustum: " << static_cast<void*>(this->Frustum) << "\n";

  os << indent << "ClipPoints: " << this->ClipPoints << "\n";

  os << indent << "FieldType: " << (this->FieldType ? "On\n" : "Off\n");

  os << indent << "ContainingCells: " << (this->ContainingCells ? "On\n" : "Off\n");

  os << indent << "ShowBounds: " << (this->ShowBounds ? "On\n" : "Off\n");

  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
}
