/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollisionDetection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright (c) Goodwin Lawlor All rights reserved.
  BSD 3-Clause License

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include "svtkCollisionDetectionFilter.h"

#include "svtkBox.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkMatrixToLinearTransform.h"
#include "svtkOBBTree.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTransform.h"
#include "svtkTriangle.h"
#include "svtkTrivialProducer.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkCollisionDetectionFilter);

// Constructs with initial 0 values.
svtkCollisionDetectionFilter::svtkCollisionDetectionFilter()
{
  // Ask the superclass to set the number of connections.
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfInputConnections(0, 1);
  this->SetNumberOfInputConnections(1, 1);
  this->SetNumberOfOutputPorts(3);
  this->Transform[0] = nullptr;
  this->Transform[1] = nullptr;
  this->Matrix[0] = nullptr;
  this->Matrix[1] = nullptr;
  this->NumberOfBoxTests = 0;
  this->BoxTolerance = 0.0;
  this->CellTolerance = 0.0;
  this->NumberOfCellsPerNode = 2;
  this->Tree0 = svtkOBBTree::New();
  this->Tree1 = svtkOBBTree::New();
  this->GenerateScalars = 0;
  this->CollisionMode = SVTK_ALL_CONTACTS;
  this->Opacity = 1.0;
}

// Destroy any allocated memory.
svtkCollisionDetectionFilter::~svtkCollisionDetectionFilter()
{
  if (this->Tree0 != nullptr)
  {
    this->Tree0->Delete();
  }
  if (this->Tree1 != nullptr)
  {
    this->Tree1->Delete();
  }

  if (this->Matrix[0])
  {
    this->Matrix[0]->UnRegister(this);
    this->Matrix[0] = nullptr;
  }
  if (this->Matrix[1])
  {
    this->Matrix[1]->UnRegister(this);
    this->Matrix[1] = nullptr;
  }
  if (this->Transform[0])
  {
    this->Transform[0]->UnRegister(this);
    this->Transform[0] = nullptr;
  }
  if (this->Transform[1])
  {
    this->Transform[1]->UnRegister(this);
    this->Transform[1] = nullptr;
  }
}

// Description:
// Set and Get the input data...
void svtkCollisionDetectionFilter::SetInputData(int idx, svtkPolyData* input)
{
  if (2 <= idx || idx < 0)
  {
    svtkErrorMacro(<< "Index " << idx
                  << " is out of range in SetInputData. Only two inputs allowed!");
    return;
  }

  // Ask the superclass to connect the input.
  svtkSmartPointer<svtkTrivialProducer> inputProducer = svtkSmartPointer<svtkTrivialProducer>::New();
  inputProducer->SetOutput(input);
  this->SetNthInputConnection(idx, 0, input ? inputProducer->GetOutputPort() : nullptr);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkCollisionDetectionFilter::GetInputData(int idx)
{
  if (2 <= idx || idx < 0)
  {
    svtkErrorMacro(<< "Index " << idx << " is out of range in GetInput. Only two inputs allowed!");
    return nullptr;
  }

  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(idx, 0));
}

svtkIdTypeArray* svtkCollisionDetectionFilter::GetContactCells(int i)
{
  if (2 <= i || i < 0)
  {
    svtkErrorMacro(
      << "Index " << i
      << " is out of range in GetContactCells. There are only two contact cells arrays!");
    return nullptr;
  }
  return svtkIdTypeArray::SafeDownCast(this->GetOutput(i)->GetFieldData()->GetArray("ContactCells"));
}

void svtkCollisionDetectionFilter::SetTransform(int i, svtkLinearTransform* transform)
{
  if (i > 1 || i < 0)
  {
    svtkErrorMacro(<< "Index " << i
                  << " is out of range in SetTransform. Only two transforms allowed!");
    return;
  }

  if (transform == this->Transform[i])
  {
    return;
  }

  if (this->Transform[i])
  {
    this->Transform[i]->Delete();
    this->Transform[i] = nullptr;
  }

  if (this->Matrix[i])
  {
    this->Matrix[i]->Delete();
    this->Matrix[i] = nullptr;
  }

  if (transform)
  {
    this->Transform[i] = transform;
    this->Transform[i]->Register(this);
    this->Matrix[i] = transform->GetMatrix();
    this->Matrix[i]->Register(this);
  }
  this->Modified();
}

void svtkCollisionDetectionFilter::SetMatrix(int i, svtkMatrix4x4* matrix)
{
  if (i > 1 || i < 0)
  {
    svtkErrorMacro(<< "Index " << i << " is out of range in SetMatrix. Only two matrices allowed!");
    return;
  }

  if (matrix == this->Matrix[i])
  {
    return;
  }

  if (this->Transform[i])
  {
    this->Transform[i]->Delete();
    this->Transform[i] = nullptr;
  }

  if (this->Matrix[i])
  {
    this->Matrix[i]->Delete();
    this->Matrix[i] = nullptr;
  }
  svtkDebugMacro(<< "Setting matrix: " << i << " to point to " << matrix << endl);

  if (matrix)
  {
    this->Matrix[i] = matrix;
  }
  matrix->Register(this);
  svtkMatrixToLinearTransform* transform = svtkMatrixToLinearTransform::New();
  // Consistent Register and UnRegisters.
  transform->Register(this);
  transform->Delete();
  transform->SetInput(matrix);
  this->Transform[i] = transform;
  svtkDebugMacro(<< "Setting Transform " << i << " to points to: " << transform << endl);
  this->Modified();
}

svtkMatrix4x4* svtkCollisionDetectionFilter::GetMatrix(int i)
{
  if (this->Transform[i])
  {
    this->Transform[i]->Update();
  }
  return this->Matrix[i];
}

static int ComputeCollisions(
  svtkOBBNode* nodeA, svtkOBBNode* nodeB, svtkMatrix4x4* Xform, void* clientdata)
{
  // This is hard-coded for triangles but could be easily changed to allow for allow n-sided
  // polygons
  int numIdsA, numIdsB;
  svtkIdList *IdsA, *IdsB, *pointIdsA, *pointIdsB;
  svtkCellArray* cells;
  svtkIdType cellPtIds[2];
  svtkIdTypeArray *contactcells1, *contactcells2;
  svtkPoints* contactpoints;
  IdsA = nodeA->Cells;
  IdsB = nodeB->Cells;
  numIdsA = IdsA->GetNumberOfIds();
  numIdsB = IdsB->GetNumberOfIds();

  // clientdata is a pointer to this object... need to cast it as such
  svtkCollisionDetectionFilter* self = reinterpret_cast<svtkCollisionDetectionFilter*>(clientdata);

  // Turn off debugging here if its on... otherwise there's squawks every update/box test
  int DebugWasOn = 0;
  int FirstContact = 0;
  if (self->GetDebug())
  {
    self->DebugOff();
    DebugWasOn = 1;
  }
  svtkPolyData* inputA = svtkPolyData::SafeDownCast(self->GetInput(0));
  svtkPolyData* inputB = svtkPolyData::SafeDownCast(self->GetInput(1));
  contactcells1 = self->GetContactCells(0);
  contactcells2 = self->GetContactCells(1);
  contactpoints = self->GetOutput(2)->GetPoints();

  if (self->GetCollisionMode() == svtkCollisionDetectionFilter::SVTK_ALL_CONTACTS)
  {
    cells = self->GetOutput(2)->GetLines();
  }
  else
  {
    cells = self->GetOutput(2)->GetVerts();
  }

  float Tolerance = self->GetCellTolerance();
  if (self->GetCollisionMode() == svtkCollisionDetectionFilter::SVTK_FIRST_CONTACT)
  {
    FirstContact = 1;
  }
  svtkIdType cellIdA, cellIdB;

  double x1[4], x2[4], xnew[4];
  double ptsA[9], ptsB[9];
  double boundsA[6], boundsB[6];
  svtkIdType i, j, k, m, n, p, v;
  double *point, in[4], out[4];

  // Loop thru the cells/points in IdsA
  for (i = 0; i < numIdsA; i++)
  {
    cellIdA = IdsA->GetId(i);
    pointIdsA = inputA->GetCell(cellIdA)->GetPointIds();
    inputA->GetCellBounds(cellIdA, boundsA);

    // Initialize ptsA
    // It might speed things up if ptsA and ptsB only had to be an
    // array of pointers to the cell vertices, rather than allocating here.
    for (j = 0; j < 3; j++)
    {
      for (k = 0; k < 3; k++)
      {
        ptsA[j * 3 + k] = inputA->GetPoints()->GetPoint(pointIdsA->GetId(j))[k];
      }
    }

    // Loop thru each cell IdsB and test for collision
    for (m = 0; m < numIdsB; m++)
    {
      cellIdB = IdsB->GetId(m);
      pointIdsB = inputB->GetCell(cellIdB)->GetPointIds();
      inputB->GetCellBounds(cellIdB, boundsB);

      // Initialize ptsB
      for (n = 0; n < 3; n++)
      {
        point = inputB->GetPoints()->GetPoint(pointIdsB->GetId(n));
        // transform the vertex
        in[0] = point[0];
        in[1] = point[1];
        in[2] = point[2];
        in[3] = 1.0;
        Xform->MultiplyPoint(in, out);
        out[0] = out[0] / out[3];
        out[1] = out[1] / out[3];
        out[2] = out[2] / out[3];
        for (p = 0; p < 3; p++)
        {
          ptsB[n * 3 + p] = out[p];
        }
      }
      // Calculate the bounds for the xformed cell
      boundsB[0] = boundsB[2] = boundsB[4] = SVTK_DOUBLE_MAX;
      boundsB[1] = boundsB[3] = boundsB[5] = SVTK_DOUBLE_MIN;
      for (v = 0; v < 9; v = v + 3)
      {
        if (ptsB[v] < boundsB[0])
          boundsB[0] = ptsB[v];
        if (ptsB[v] > boundsB[1])
          boundsB[1] = ptsB[v];
        if (ptsB[v + 1] < boundsB[2])
          boundsB[2] = ptsB[v + 1];
        if (ptsB[v + 1] > boundsB[3])
          boundsB[3] = ptsB[v + 1];
        if (ptsB[v + 2] < boundsB[4])
          boundsB[4] = ptsB[v + 2];
        if (ptsB[v + 2] > boundsB[5])
          boundsB[5] = ptsB[v + 2];
      }
      // Test for intersection
      if (self->IntersectPolygonWithPolygon(
            3, ptsA, boundsA, 3, ptsB, boundsB, Tolerance, x1, x2, self->GetCollisionMode()))
      {
        contactcells1->InsertNextValue(cellIdA);
        contactcells2->InsertNextValue(cellIdB);
        // transform x back to "world space"
        // could speed this up by testing for identity matrix
        // and skipping the next transform.
        x1[3] = x2[3] = 1.0;
        self->GetMatrix(0)->MultiplyPoint(x1, xnew);
        xnew[0] = xnew[0] / xnew[3];
        xnew[1] = xnew[1] / xnew[3];
        xnew[2] = xnew[2] / xnew[3];
        cellPtIds[0] = contactpoints->InsertNextPoint(xnew);
        if (self->GetCollisionMode() == svtkCollisionDetectionFilter::SVTK_ALL_CONTACTS)
        {
          self->GetMatrix(0)->MultiplyPoint(x2, xnew);
          xnew[0] = xnew[0] / xnew[3];
          xnew[1] = xnew[1] / xnew[3];
          xnew[2] = xnew[2] / xnew[3];
          cellPtIds[1] = contactpoints->InsertNextPoint(xnew);
          // insert a new line
          cells->InsertNextCell(2, cellPtIds);
        }
        else
        {
          // insert a new vert
          cells->InsertNextCell(1, cellPtIds);
        }

        if (FirstContact)
        {
          // return the negative of the number of box tests to find first contact
          // this will call a halt to the proceedings
          if (DebugWasOn)
            self->DebugOn();
          return (-1 - self->GetNumberOfBoxTests());
        }
      }
    }
  }
  if (DebugWasOn)
    self->DebugOn();
  return 1;
}

// Description:
// Perform a collision detection
int svtkCollisionDetectionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkDebugMacro(<< "Beginning execution...");

  // inputs and outputs
  svtkPolyData* input[2];
  svtkPolyData* output[3];

  // copy inputs to outputs
  svtkInformation *inInfo, *outInfo;
  for (int i = 0; i < 2; i++)
  {
    inInfo = inputVector[i]->GetInformationObject(0);
    input[i] = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

    outInfo = outputVector->GetInformationObject(i);
    output[i] = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

    output[i]->CopyStructure(input[i]);
    output[i]->GetPointData()->PassData(input[i]->GetPointData());
    output[i]->GetCellData()->PassData(input[i]->GetCellData());
    output[i]->GetFieldData()->PassData(input[i]->GetFieldData());
  }

  // set up the contacts polydata output on port index 2
  outInfo = outputVector->GetInformationObject(2);
  output[2] = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPoints* contactsPoints = svtkPoints::New();
  output[2]->SetPoints(contactsPoints);
  contactsPoints->Delete();

  if (this->CollisionMode == svtkCollisionDetectionFilter::SVTK_ALL_CONTACTS)
  { // then create a lines cell array
    svtkCellArray* lines = svtkCellArray::New();
    output[2]->SetLines(lines);
    lines->Delete();
  }
  else
  { // else create a verts cell array
    svtkCellArray* verts = svtkCellArray::New();
    output[2]->SetVerts(verts);
    verts->Delete();
  }

  // Allocate arrays for the contact cells lists
  svtkSmartPointer<svtkIdTypeArray> contactcells0 = svtkSmartPointer<svtkIdTypeArray>::New();
  contactcells0->SetName("ContactCells");
  output[0]->GetFieldData()->AddArray(contactcells0);

  svtkSmartPointer<svtkIdTypeArray> contactcells1 = svtkSmartPointer<svtkIdTypeArray>::New();
  contactcells1->SetName("ContactCells");
  output[1]->GetFieldData()->AddArray(contactcells1);

  // The transformations...
  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  svtkMatrix4x4* tmpMatrix = svtkMatrix4x4::New();

  if (this->Transform[0] != nullptr || this->Transform[1] != nullptr)
  {
    svtkMatrix4x4::Invert(this->Transform[0]->GetMatrix(), tmpMatrix);
    // the sequence of multiplication is significant
    svtkMatrix4x4::Multiply4x4(tmpMatrix, this->Transform[1]->GetMatrix(), matrix);
  }
  else
  {
    svtkWarningMacro(<< "Set two transforms or two matrices");
    return 1;
  }
  this->InvokeEvent(svtkCommand::StartEvent, nullptr);

  // rebuild the obb trees... they do their own mtime checking with input data
  Tree0->SetDataSet(input[0]);
  Tree0->AutomaticOn();
  Tree0->SetNumberOfCellsPerNode(this->NumberOfCellsPerNode);
  Tree0->BuildLocator();

  Tree1->SetDataSet(input[1]);
  Tree1->AutomaticOn();
  Tree1->SetNumberOfCellsPerNode(this->NumberOfCellsPerNode);
  Tree1->BuildLocator();

  // Set the Box Tolerance
  Tree0->SetTolerance(this->BoxTolerance);
  Tree1->SetTolerance(this->BoxTolerance);

  // Do the collision detection...
  int boxTests = Tree0->IntersectWithOBBTree(Tree1, matrix, ComputeCollisions, this);

  matrix->Delete();
  tmpMatrix->Delete();

  svtkDebugMacro(<< "Collision detection finished");
  this->NumberOfBoxTests = std::abs(boxTests);

  // Generate the scalars if needed
  if (GenerateScalars)
  {

    for (int idx = 0; idx < 2; idx++)
    {
      svtkUnsignedCharArray* scalars = svtkUnsignedCharArray::New();
      output[idx]->GetCellData()->SetScalars(scalars);
      svtkIdType numCells = input[idx]->GetNumberOfCells();
      scalars->SetNumberOfComponents(4);
      scalars->SetNumberOfTuples(numCells);
      svtkIdTypeArray* contactcells = this->GetContactCells(idx);
      svtkIdType numContacts = this->GetNumberOfContacts();

      // Fill the array with blanks...
      // Maybe this should change, to alpha set to Opacity
      // regardless if there are contact or not.
      float alpha;
      if (numContacts > 0)
      {
        alpha = this->Opacity * 255.0;
      }
      else
      {
        alpha = 255.0;
      }
      float blank[4] = { 255.0, 255.0, 255.0, alpha };

      for (svtkIdType i = 0; i < numCells; i++)
      {
        scalars->SetTuple(i, blank);
      }
      // Now color the intersecting cells
      svtkLookupTable* lut = svtkLookupTable::New();
      if (numContacts > 0)
      {
        if (this->CollisionMode == SVTK_ALL_CONTACTS)
        {
          lut->SetTableRange(0, numContacts - 1);
          lut->SetNumberOfTableValues(numContacts);
        }
        else // SVTK_FIRST_CONTACT
        {
          lut->SetTableRange(0, 1);
          lut->SetNumberOfTableValues(numContacts + 1);
        }
        lut->Build();
      }
      double* RGBA;
      float RGB[4];

      for (svtkIdType id, i = 0; i < numContacts; i++)
      {
        id = contactcells->GetValue(i);
        RGBA = lut->GetTableValue(i);
        RGB[0] = 255.0 * RGBA[0];
        RGB[1] = 255.0 * RGBA[1];
        RGB[2] = 255.0 * RGBA[2];
        RGB[3] = 255.0;
        scalars->SetTuple(id, RGB);
      }
      lut->Delete();
      scalars->Delete();
      svtkDebugMacro(<< "Created scalars on output " << idx);
    }
  }
  this->InvokeEvent(svtkCommand::EndEvent, nullptr);

  return 1;
}

// Method intersects two polygons. You must supply the number of points and
// point coordinates (npts, *pts) and the bounding box (bounds) of the two
// polygons. Also supply a tolerance squared for controlling
// error. The method returns 1 if there is an intersection, and 0 if
// not. A single point of intersection x[3] is also returned if there
// is an intersection.
int svtkCollisionDetectionFilter::IntersectPolygonWithPolygon(int npts, double* pts,
  double bounds[6], int npts2, double* pts2, double bounds2[6], double tol2, double x1[3],
  double x2[3], int collisionMode)
{
  double n[3], n2[3], coords[3];
  int i, j;
  double *p1, *p2, *q1, ray[3], ray2[3];
  double t, u, v;
  double* x[2];
  int Num = 0;
  x[0] = x1;
  x[1] = x2;

  //  Intersect each edge of first polygon against second
  //
  svtkPolygon::ComputeNormal(npts2, pts2, n2);
  svtkPolygon::ComputeNormal(npts, pts, n);

  int parallel_edges = 0;
  for (i = 0; i < npts; i++)
  {
    p1 = pts + 3 * i;
    p2 = pts + 3 * ((i + 1) % npts);

    for (j = 0; j < 3; j++)
    {
      ray[j] = p2[j] - p1[j];
    }
    if (!svtkBox::IntersectBox(bounds2, p1, ray, coords, t))
    {
      continue;
    }

    if ((svtkPlane::IntersectWithLine(p1, p2, n2, pts2, t, x[Num])) == 1)
    {
      if ((npts2 == 3 && svtkTriangle::PointInTriangle(x[Num], pts2, pts2 + 3, pts2 + 6, tol2)) ||
        (npts2 > 3 && svtkPolygon::PointInPolygon(x[Num], npts2, pts2, bounds2, n2) == 1))
      {
        Num++;
        if (collisionMode != svtkCollisionDetectionFilter::SVTK_ALL_CONTACTS || Num == 2)
        {
          return 1;
        }
      }
    }
    else
    {
      // cout << "Test for overlapping" << endl;
      // test to see if cells are coplanar and overlapping...
      parallel_edges++;
      if (parallel_edges > 1) // cells are parallel then...
      {
        // cout << "cells are parallel" << endl;
        // test to see if they are coplanar
        q1 = pts2;
        for (j = 0; j < 3; j++)
        {
          ray2[j] = p1[j] - q1[j];
        }
        if (svtkMath::Dot(n, ray2) == 0.0) // cells are coplanar
        {
          // cout << "cells are coplanar" << endl;
          // test to see if coplanar cells overlap
          // ie, if one of the tris has a vertex in the other
          for (int ii = 0; ii < npts; ii++)
          {
            for (int jj = 0; jj < npts2; jj++)
            {
              if (svtkLine::Intersection(pts + 3 * ii, pts + 3 * ((ii + 1) % npts), pts2 + 3 * jj,
                    pts2 + 3 * ((jj + 1) % npts2), u, v) == 2)
              {
                // cout << "Found an overlapping one!!!" << endl;
                for (int k = 0; k < 3; k++)
                {
                  x[Num][k] =
                    pts[k + 3 * ii] + u * (pts[k + (3 * ((ii + 1) % npts))] - pts[k + 3 * ii]);
                }
                Num++;
                if (collisionMode != svtkCollisionDetectionFilter::SVTK_ALL_CONTACTS || Num == 2)
                {
                  return 1;
                }
              }
            }
          }

        } // end if cells are coplanar
      }   // end if cells are parallel
    }     // end else
  }

  //  Intersect each edge of second polygon against first
  //

  for (i = 0; i < npts2; i++)
  {
    p1 = pts2 + 3 * i;
    p2 = pts2 + 3 * ((i + 1) % npts2);

    for (j = 0; j < 3; j++)
    {
      ray[j] = p2[j] - p1[j];
    }

    if (!svtkBox::IntersectBox(bounds, p1, ray, coords, t))
    {
      continue;
    }

    if ((svtkPlane::IntersectWithLine(p1, p2, n, pts, t, x[Num])) == 1)
    {
      if ((npts == 3 && svtkTriangle::PointInTriangle(x[Num], pts, pts + 3, pts + 6, tol2)) ||
        (npts > 3 && svtkPolygon::PointInPolygon(x[Num], npts, pts, bounds, n) == 1))
      {
        Num++;
        if (collisionMode != svtkCollisionDetectionFilter::SVTK_ALL_CONTACTS || Num == 2)
        {
          return 1;
        }
      }
    }
  }

  // if we get through to here then there's no collision.
  return 0;
}

// Description:
// Make sure filter executes if transform are changed
svtkMTimeType svtkCollisionDetectionFilter::GetMTime()
{
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType transMTime, matrixMTime;

  if (this->Transform[0])
  {
    transMTime = this->Transform[0]->GetMTime();
    mTime = (transMTime > mTime ? transMTime : mTime);
  }

  if (this->Transform[1])
  {
    transMTime = this->Transform[1]->GetMTime();
    mTime = (transMTime > mTime ? transMTime : mTime);
  }

  if (this->Matrix[0])
  {
    matrixMTime = this->Matrix[0]->GetMTime();
    mTime = (matrixMTime > mTime ? matrixMTime : mTime);
  }

  if (this->Matrix[1])
  {
    matrixMTime = this->Matrix[1]->GetMTime();
    mTime = (matrixMTime > mTime ? matrixMTime : mTime);
  }
  return mTime;
}

void svtkCollisionDetectionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Box Tolerance: " << this->GetBoxTolerance() << "\n";
  os << indent << "Cell Tolerance: " << this->GetCellTolerance() << "\n";
  os << indent << "Number of cells per Node: " << this->GetNumberOfCellsPerNode() << "\n";
  os << indent << "GenerateScalars: " << (this->GetGenerateScalars() ? "On" : "Off") << "\n";
  os << indent << "Collision Mode: " << this->GetCollisionModeAsString() << "\n";
  os << indent << "Opacity: " << this->GetOpacity() << "\n";
  os << indent << "InputData 0: " << this->GetInput(0) << "\n";
  os << indent << "InputData 1: " << this->GetInput(1) << "\n";
  os << indent << "Transform 0: " << this->GetTransform(0) << "\n";
  os << indent << "Transform 1: " << this->GetTransform(1) << "\n";
  os << indent << "Matrix 0: " << this->GetMatrix(0) << "\n";
  os << indent << "Matrix 1: " << this->GetMatrix(1) << "\n";
}
