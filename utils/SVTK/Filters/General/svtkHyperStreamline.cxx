/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperStreamline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperStreamline.h"

#include "svtkCellArray.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkHyperStreamline);

//
// Special classes for manipulating data
//
class svtkHyperPoint
{ //;prevent man page generation
public:
  svtkHyperPoint();                                   // method sets up storage
  svtkHyperPoint& operator=(const svtkHyperPoint& hp); // for resizing

  double X[3];      // position
  svtkIdType CellId; // cell
  int SubId;        // cell sub id
  double P[3];      // parametric coords in cell
  double W[3];      // eigenvalues (sorted in decreasing value)
  double* V[3];     // pointers to eigenvectors (also sorted)
  double V0[3];     // storage for eigenvectors
  double V1[3];
  double V2[3];
  double S; // scalar value
  double D; // distance travelled so far
};

class svtkHyperArray
{ //;prevent man page generation
public:
  svtkHyperArray();
  ~svtkHyperArray() { delete[] this->Array; }
  svtkIdType GetNumberOfPoints() { return this->MaxId + 1; }
  svtkHyperPoint* GetHyperPoint(svtkIdType i) { return this->Array + i; }
  svtkHyperPoint* InsertNextHyperPoint()
  {
    if (++this->MaxId >= this->Size)
    {
      this->Resize(this->MaxId);
    }
    return this->Array + this->MaxId;
  }
  svtkHyperPoint* Resize(svtkIdType sz); // reallocates data
  void Reset() { this->MaxId = -1; }

  svtkHyperPoint* Array; // pointer to data
  svtkIdType MaxId;      // maximum index inserted thus far
  svtkIdType Size;       // allocated size of data
  svtkIdType Extend;     // grow array by this amount
  double Direction;     // integration direction
};

#define SVTK_START_FROM_POSITION 0
#define SVTK_START_FROM_LOCATION 1

svtkHyperPoint::svtkHyperPoint()
{
  // Alias V to V0,V1,V2.
  this->V[0] = this->V0;
  this->V[1] = this->V1;
  this->V[2] = this->V2;
}

svtkHyperPoint& svtkHyperPoint::operator=(const svtkHyperPoint& hp)
{
  for (int i = 0; i < 3; i++)
  {
    this->X[i] = hp.X[i];
    this->P[i] = hp.P[i];
    this->W[i] = hp.W[i];
    for (int j = 0; j < 3; j++)
    {
      this->V[j][i] = hp.V[j][i];
    }
    // Note: no need to write to V0,V1,V2 since they are written to via the V alias.
  }
  this->CellId = hp.CellId;
  this->SubId = hp.SubId;
  this->S = hp.S;
  this->D = hp.D;

  return *this;
}

svtkHyperArray::svtkHyperArray()
{
  this->MaxId = -1;
  this->Array = new svtkHyperPoint[1000];
  this->Size = 1000;
  this->Extend = 5000;
  this->Direction = SVTK_INTEGRATE_FORWARD;
}

svtkHyperPoint* svtkHyperArray::Resize(svtkIdType sz)
{
  svtkHyperPoint* newArray;
  svtkIdType newSize, i;

  if (sz >= this->Size)
  {
    newSize = this->Size + this->Extend * (((sz - this->Size) / this->Extend) + 1);
  }
  else
  {
    newSize = sz;
  }

  newArray = new svtkHyperPoint[newSize];

  for (i = 0; i < sz; i++)
  {
    newArray[i] = this->Array[i];
  }

  this->Size = newSize;
  delete[] this->Array;
  this->Array = newArray;

  return this->Array;
}

// Construct object with initial starting position (0,0,0); integration step
// length 0.2; step length 0.01; forward integration; terminal eigenvalue 0.0;
// number of sides 6; radius 0.5; and logarithmic scaling off.
svtkHyperStreamline::svtkHyperStreamline()
{
  this->StartFrom = SVTK_START_FROM_POSITION;
  this->StartPosition[0] = this->StartPosition[1] = this->StartPosition[2] = 0.0;

  this->StartCell = 0;
  this->StartSubId = 0;
  this->StartPCoords[0] = this->StartPCoords[1] = this->StartPCoords[2] = 0.5;

  this->Streamers = nullptr;

  this->MaximumPropagationDistance = 100.0;
  this->IntegrationStepLength = 0.2;
  this->StepLength = 0.01;
  this->IntegrationDirection = SVTK_INTEGRATE_FORWARD;
  this->TerminalEigenvalue = 0.0;
  this->NumberOfSides = 6;
  this->Radius = 0.5;
  this->LogScaling = 0;
  this->IntegrationEigenvector = SVTK_INTEGRATE_MAJOR_EIGENVECTOR;
}

svtkHyperStreamline::~svtkHyperStreamline()
{
  delete[] this->Streamers;
}

// Specify the start of the hyperstreamline in the cell coordinate system.
// That is, cellId and subId (if composite cell), and parametric coordinates.
void svtkHyperStreamline::SetStartLocation(svtkIdType cellId, int subId, double pcoords[3])
{
  if (cellId != this->StartCell || subId != this->StartSubId ||
    pcoords[0] != this->StartPCoords[0] || pcoords[1] != this->StartPCoords[1] ||
    pcoords[2] != this->StartPCoords[2])
  {
    this->Modified();
    this->StartFrom = SVTK_START_FROM_LOCATION;

    this->StartCell = cellId;
    this->StartSubId = subId;
    this->StartPCoords[0] = pcoords[0];
    this->StartPCoords[1] = pcoords[1];
    this->StartPCoords[2] = pcoords[2];
  }
}

// Specify the start of the hyperstreamline in the cell coordinate system.
// That is, cellId and subId (if composite cell), and parametric coordinates.
void svtkHyperStreamline::SetStartLocation(svtkIdType cellId, int subId, double r, double s, double t)
{
  double pcoords[3];
  pcoords[0] = r;
  pcoords[1] = s;
  pcoords[2] = t;

  this->SetStartLocation(cellId, subId, pcoords);
}

// Get the starting location of the hyperstreamline in the cell coordinate
// system. Returns the cell that the starting point is in.
svtkIdType svtkHyperStreamline::GetStartLocation(int& subId, double pcoords[3])
{
  subId = this->StartSubId;
  pcoords[0] = this->StartPCoords[0];
  pcoords[1] = this->StartPCoords[1];
  pcoords[2] = this->StartPCoords[2];
  return this->StartCell;
}

// Specify the start of the hyperstreamline in the global coordinate system.
// Starting from position implies that a search must be performed to find
// initial cell to start integration from.
void svtkHyperStreamline::SetStartPosition(double x[3])
{
  if (x[0] != this->StartPosition[0] || x[1] != this->StartPosition[1] ||
    x[2] != this->StartPosition[2])
  {
    this->Modified();
    this->StartFrom = SVTK_START_FROM_POSITION;

    this->StartPosition[0] = x[0];
    this->StartPosition[1] = x[1];
    this->StartPosition[2] = x[2];
  }
}

// Specify the start of the hyperstreamline in the global coordinate system.
// Starting from position implies that a search must be performed to find
// initial cell to start integration from.
void svtkHyperStreamline::SetStartPosition(double x, double y, double z)
{
  double pos[3];
  pos[0] = x;
  pos[1] = y;
  pos[2] = z;

  this->SetStartPosition(pos);
}

// Get the start position of the hyperstreamline in global x-y-z coordinates.
double* svtkHyperStreamline::GetStartPosition()
{
  return this->StartPosition;
}

// Make sure coordinate systems are consistent
static void FixVectors(double** prev, double** current, int iv, int ix, int iy)
{
  double p0[3], p1[3], p2[3];
  double v0[3], v1[3], v2[3];
  double temp[3];
  int i;

  for (i = 0; i < 3; i++)
  {
    v0[i] = current[i][iv];
    v1[i] = current[i][ix];
    v2[i] = current[i][iy];
  }

  if (prev == nullptr) // make sure coord system is right handed
  {
    svtkMath::Cross(v0, v1, temp);
    if (svtkMath::Dot(v2, temp) < 0.0)
    {
      for (i = 0; i < 3; i++)
      {
        current[i][iy] *= -1.0;
      }
    }
  }

  else // make sure vectors consistent from one point to the next
  {
    for (i = 0; i < 3; i++)
    {
      p0[i] = prev[i][iv];
      p1[i] = prev[i][ix];
      p2[i] = prev[i][iy];
    }
    if (svtkMath::Dot(p0, v0) < 0.0)
    {
      for (i = 0; i < 3; i++)
      {
        current[i][iv] *= -1.0;
      }
    }
    if (svtkMath::Dot(p1, v1) < 0.0)
    {
      for (i = 0; i < 3; i++)
      {
        current[i][ix] *= -1.0;
      }
    }
    if (svtkMath::Dot(p2, v2) < 0.0)
    {
      for (i = 0; i < 3; i++)
      {
        current[i][iy] *= -1.0;
      }
    }
  }
}

int svtkHyperStreamline::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData* pd = input->GetPointData();
  svtkDataArray* inScalars = nullptr;
  svtkDataArray* inTensors = nullptr;
  double tensor[9];
  svtkHyperPoint* sNext = nullptr;
  svtkHyperPoint* sPtr = nullptr;
  int i, j, k, ptId, subId, iv, ix, iy;
  svtkCell* cell = nullptr;
  double ev[3], xNext[3];
  double d, step, dir, tol2, p[3];
  double* w = nullptr;
  double dist2;
  double closestPoint[3];
  double *m[3], *v[3];
  double m0[3], m1[3], m2[3];
  double v0[3], v1[3], v2[3];
  svtkDataArray* cellTensors = nullptr;
  svtkDataArray* cellScalars = nullptr;
  // set up working matrices
  v[0] = v0;
  v[1] = v1;
  v[2] = v2;
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;

  svtkDebugMacro(<< "Generating hyperstreamline(s)");
  this->NumberOfStreamers = 0;

  if (!(inTensors = pd->GetTensors()))
  {
    svtkErrorMacro(<< "No tensor data defined!");
    return 1;
  }
  w = new double[input->GetMaxCellSize()];

  inScalars = pd->GetScalars();

  cellTensors = svtkDataArray::CreateDataArray(inTensors->GetDataType());
  int numComp;
  if (inTensors)
  {
    numComp = inTensors->GetNumberOfComponents();
    cellTensors->SetNumberOfComponents(numComp);
    cellTensors->SetNumberOfTuples(SVTK_CELL_SIZE);
  }
  if (inScalars)
  {
    cellScalars = svtkDataArray::CreateDataArray(inScalars->GetDataType());
    numComp = inScalars->GetNumberOfComponents();
    cellScalars->SetNumberOfComponents(numComp);
    cellScalars->SetNumberOfTuples(SVTK_CELL_SIZE);
  }

  tol2 = input->GetLength() / 1000.0;
  tol2 = tol2 * tol2;
  iv = this->IntegrationEigenvector;
  ix = (iv + 1) % 3;
  iy = (iv + 2) % 3;
  //
  // Create starting points
  //
  this->NumberOfStreamers = 1;

  if (this->IntegrationDirection == SVTK_INTEGRATE_BOTH_DIRECTIONS)
  {
    this->NumberOfStreamers *= 2;
  }

  this->Streamers = new svtkHyperArray[this->NumberOfStreamers];

  if (this->StartFrom == SVTK_START_FROM_POSITION)
  {
    sPtr = this->Streamers[0].InsertNextHyperPoint();
    for (i = 0; i < 3; i++)
    {
      sPtr->X[i] = this->StartPosition[i];
    }
    sPtr->CellId =
      input->FindCell(this->StartPosition, nullptr, (-1), 0.0, sPtr->SubId, sPtr->P, w);
  }

  else // SVTK_START_FROM_LOCATION
  {
    sPtr = this->Streamers[0].InsertNextHyperPoint();
    cell = input->GetCell(sPtr->CellId);
    cell->EvaluateLocation(sPtr->SubId, sPtr->P, sPtr->X, w);
  }
  //
  // Finish initializing each hyperstreamline
  //
  this->Streamers[0].Direction = 1.0;
  sPtr = this->Streamers[0].GetHyperPoint(0);
  sPtr->D = 0.0;
  if (sPtr->CellId >= 0) // starting point in dataset
  {
    cell = input->GetCell(sPtr->CellId);
    cell->EvaluateLocation(sPtr->SubId, sPtr->P, xNext, w);

    inTensors->GetTuples(cell->PointIds, cellTensors);

    // interpolate tensor, compute eigenfunctions
    for (j = 0; j < 3; j++)
    {
      for (i = 0; i < 3; i++)
      {
        m[i][j] = 0.0;
      }
    }
    for (k = 0; k < cell->GetNumberOfPoints(); k++)
    {
      cellTensors->GetTuple(k, tensor);
      if (cellTensors->GetNumberOfComponents() == 6)
      {
        svtkMath::TensorFromSymmetricTensor(tensor);
      }
      for (j = 0; j < 3; j++)
      {
        for (i = 0; i < 3; i++)
        {
          m[i][j] += tensor[i + 3 * j] * w[k];
        }
      }
    }

    svtkMath::Jacobi(m, sPtr->W, sPtr->V);
    FixVectors(nullptr, sPtr->V, iv, ix, iy);

    if (inScalars)
    {
      inScalars->GetTuples(cell->PointIds, cellScalars);
      for (sPtr->S = 0, i = 0; i < cell->GetNumberOfPoints(); i++)
      {
        sPtr->S += cellScalars->GetTuple(i)[0] * w[i];
      }
    }

    if (this->IntegrationDirection == SVTK_INTEGRATE_BOTH_DIRECTIONS)
    {
      this->Streamers[1].Direction = -1.0;
      sNext = this->Streamers[1].InsertNextHyperPoint();
      *sNext = *sPtr;
    }
    else if (this->IntegrationDirection == SVTK_INTEGRATE_BACKWARD)
    {
      this->Streamers[0].Direction = -1.0;
    }
  } // for hyperstreamline in dataset
  //
  // For each hyperstreamline, integrate in appropriate direction (using RK2).
  //
  for (ptId = 0; ptId < this->NumberOfStreamers; ptId++)
  {
    // get starting step
    sPtr = this->Streamers[ptId].GetHyperPoint(0);
    if (sPtr->CellId < 0)
    {
      continue;
    }

    dir = this->Streamers[ptId].Direction;
    cell = input->GetCell(sPtr->CellId);
    cell->EvaluateLocation(sPtr->SubId, sPtr->P, xNext, w);
    step = this->IntegrationStepLength * sqrt(cell->GetLength2());
    inTensors->GetTuples(cell->PointIds, cellTensors);
    if (inScalars)
    {
      inScalars->GetTuples(cell->PointIds, cellScalars);
    }

    // integrate until distance has been exceeded
    while (sPtr->CellId >= 0 && fabs(sPtr->W[0]) > this->TerminalEigenvalue &&
      sPtr->D < this->MaximumPropagationDistance)
    {

      // compute updated position using this step (Euler integration)
      for (i = 0; i < 3; i++)
      {
        xNext[i] = sPtr->X[i] + dir * step * sPtr->V[i][iv];
      }

      // compute updated position using updated step
      //
      // one potential bug here to be fixed as cell->EvaluatePosition() may return
      // 1: xNext inside  the current cell
      // 0: xNext outside the current cell
      //-1: numerical error occurs
      // In case of  0, input->FindCell() needs to be called to justify
      // subsequent tensor interpolation and Jacob computation.
      // In case of -1, the while() loop needs to be broken to avoid uncertainties
      //
      cell->EvaluatePosition(xNext, closestPoint, subId, p, dist2, w);

      // interpolate tensor
      for (j = 0; j < 3; j++)
      {
        for (i = 0; i < 3; i++)
        {
          m[i][j] = 0.0;
        }
      }
      for (k = 0; k < cell->GetNumberOfPoints(); k++)
      {
        cellTensors->GetTuple(k, tensor);
        if (cellTensors->GetNumberOfComponents() == 6)
        {
          svtkMath::TensorFromSymmetricTensor(tensor);
        }
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            m[i][j] += tensor[i + 3 * j] * w[k];
          }
        }
      }

      svtkMath::Jacobi(m, ev, v);
      FixVectors(sPtr->V, v, iv, ix, iy);

      // now compute final position
      for (i = 0; i < 3; i++)
      {
        xNext[i] = sPtr->X[i] + dir * (step / 2.0) * (sPtr->V[i][iv] + v[i][iv]);
      }

      // get the safe handle to sPtr in case the svtkHyperPoint array is resized.
      // A resize operation usually changes the address of the memory block.
      // This safe handle prevents sPtr from being a broken / wild pointer
      // that might be indirectly caused through InsertNextHyperPoint()
      svtkIdType sPtrId = this->Streamers[ptId].GetNumberOfPoints() - 1;

      // now feel free to insert a new svtkHyperPoint
      sNext = this->Streamers[ptId].InsertNextHyperPoint();

      // make sure sPtr points to the target in a possibly-resized memory block
      sPtr = this->Streamers[ptId].GetHyperPoint(sPtrId);

      int evalResult =
        cell->EvaluatePosition(xNext, closestPoint, sNext->SubId, sNext->P, dist2, w);

      if (evalResult == 1)
      { // integration still in cell
        for (i = 0; i < 3; i++)
        {
          sNext->X[i] = closestPoint[i];
        }
        sNext->CellId = sPtr->CellId;
        sNext->SubId = sPtr->SubId;
      }
      else if (evalResult == 0)
      { // integration has passed out of cell
        sNext->CellId = input->FindCell(xNext, cell, sPtr->CellId, tol2, sNext->SubId, sNext->P, w);
        if (sNext->CellId >= 0) // make sure not out of dataset
        {
          for (i = 0; i < 3; i++)
          {
            sNext->X[i] = xNext[i];
          }
          cell = input->GetCell(sNext->CellId);
          inTensors->GetTuples(cell->PointIds, cellTensors);
          if (inScalars)
          {
            inScalars->GetTuples(cell->PointIds, cellScalars);
          }
          step = this->IntegrationStepLength * sqrt(cell->GetLength2());
        }
      }
      else
      { // evalResult = -1: numerical error occurs, rarely but possibly
        // All returned values are invalid and should be ignored
        // and the segment "if ( sNext->CellId >= 0 ) {...}" will be skipped
        sNext->CellId = -1;
      }

      if (sNext->CellId >= 0)
      {
        cell->EvaluateLocation(sNext->SubId, sNext->P, xNext, w);
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            m[i][j] = 0.0;
          }
        }
        for (k = 0; k < cell->GetNumberOfPoints(); k++)
        {
          cellTensors->GetTuple(k, tensor);
          if (cellTensors->GetNumberOfComponents() == 6)
          {
            svtkMath::TensorFromSymmetricTensor(tensor);
          }
          for (j = 0; j < 3; j++)
          {
            for (i = 0; i < 3; i++)
            {
              m[i][j] += tensor[i + 3 * j] * w[k];
            }
          }
        }

        svtkMath::Jacobi(m, sNext->W, sNext->V);
        FixVectors(sPtr->V, sNext->V, iv, ix, iy);

        if (inScalars)
        {
          for (sNext->S = 0.0, i = 0; i < cell->GetNumberOfPoints(); i++)
          {
            sNext->S += cellScalars->GetTuple(i)[0] * w[i];
          }
        }
        d = sqrt(svtkMath::Distance2BetweenPoints(sPtr->X, sNext->X));
        sNext->D = sPtr->D + d;
      }
      else
      { // follow-up for evalResult = -1: to enable the next line of code
        sNext = sPtr;
      }

      sPtr = sNext;

    } // for elapsed time

  } // for each hyperstreamline

  int retval = this->BuildTube(input, output);

  delete[] w;
  cellTensors->Delete();
  if (cellScalars)
  {
    cellScalars->Delete();
  }

  return retval;
}

int svtkHyperStreamline::BuildTube(svtkDataSet* input, svtkPolyData* output)
{
  svtkHyperPoint *sPrev, *sPtr;
  svtkPoints* newPts;
  svtkFloatArray* newVectors;
  svtkFloatArray* newNormals;
  svtkFloatArray* newScalars = nullptr;
  svtkCellArray* newStrips;
  svtkIdType i, npts, ptOffset = 0;
  int ptId, j, id, k, i1, i2;
  double dOffset, x[3], v[3], s, r, r1[3], r2[3], stepLength;
  double xT[3], sFactor, normal[3], w[3];
  double theta = 2.0 * svtkMath::Pi() / this->NumberOfSides;
  svtkPointData* outPD;
  int iv, ix, iy;
  svtkIdType numIntPts;
  //
  // Initialize
  //
  svtkDebugMacro(<< "Creating hyperstreamline tube");
  if (this->NumberOfStreamers <= 0)
  {
    return 0;
  }

  stepLength = input->GetLength() * this->StepLength;
  outPD = output->GetPointData();

  iv = this->IntegrationEigenvector;
  ix = (iv + 1) % 3;
  iy = (iv + 2) % 3;
  //
  // Allocate
  //
  newPts = svtkPoints::New();
  newPts->Allocate(2500);
  if (input->GetPointData()->GetScalars())
  {
    newScalars = svtkFloatArray::New();
    newScalars->Allocate(2500);
  }
  newVectors = svtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->Allocate(7500);
  newNormals = svtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(7500);
  newStrips = svtkCellArray::New();
  newStrips->AllocateEstimate(3 * this->NumberOfStreamers, SVTK_CELL_SIZE);
  //
  // Loop over all hyperstreamlines generating points
  //
  for (ptId = 0; ptId < this->NumberOfStreamers; ptId++)
  {
    if ((numIntPts = this->Streamers[ptId].GetNumberOfPoints()) < 2)
    {
      continue;
    }
    sPrev = this->Streamers[ptId].GetHyperPoint(0);
    sPtr = this->Streamers[ptId].GetHyperPoint(1);

    // compute scale factor
    i = (sPrev->W[ix] > sPrev->W[iy] ? ix : iy);
    if (sPrev->W[i] == 0.0)
    {
      sFactor = 1.0;
    }
    else
    {
      sFactor = this->Radius / sPrev->W[i];
    }

    if (numIntPts == 2 && sPtr->CellId < 0)
    {
      continue;
    }

    dOffset = sPrev->D;

    for (npts = 0, i = 1; i < numIntPts && sPtr->CellId >= 0;
         i++, sPrev = sPtr, sPtr = this->Streamers[ptId].GetHyperPoint(i))
    {
      //
      // Bracket steps and construct tube points
      //
      while (dOffset >= sPrev->D && dOffset < sPtr->D)
      {
        r = (dOffset - sPrev->D) / (sPtr->D - sPrev->D);

        for (j = 0; j < 3; j++) // compute point in center of tube
        {
          x[j] = sPrev->X[j] + r * (sPtr->X[j] - sPrev->X[j]);
          v[j] = sPrev->V[j][iv] + r * (sPtr->V[j][iv] - sPrev->V[j][iv]);
          r1[j] = sPrev->V[j][ix] + r * (sPtr->V[j][ix] - sPrev->V[j][ix]);
          r2[j] = sPrev->V[j][iy] + r * (sPtr->V[j][iy] - sPrev->V[j][iy]);
          w[j] = sPrev->W[j] + r * (sPtr->W[j] - sPrev->W[j]);
        }

        // construct points around tube
        for (k = 0; k < this->NumberOfSides; k++)
        {
          for (j = 0; j < 3; j++)
          {
            normal[j] = w[ix] * r1[j] * cos(k * theta) + w[iy] * r2[j] * sin(k * theta);
            xT[j] = x[j] + sFactor * normal[j];
          }
          id = newPts->InsertNextPoint(xT);
          newVectors->InsertTuple(id, v);
          svtkMath::Normalize(normal);
          newNormals->InsertTuple(id, normal);
        }

        if (newScalars) // add scalars around tube
        {
          s = sPrev->S + r * (sPtr->S - sPrev->S);
          for (k = 0; k < this->NumberOfSides; k++)
          {
            newScalars->InsertNextTuple(&s);
          }
        }

        npts++;
        dOffset += stepLength;

      } // while
    }   // for this hyperstreamline

    //
    // Generate the strips for this hyperstreamline
    //
    for (k = 0; k < this->NumberOfSides; k++)
    {
      i1 = (k + 1) % this->NumberOfSides;
      newStrips->InsertNextCell(npts * 2);
      for (i = 0; i < npts; i++)
      {
        // make sure strip definition consistent with normals
        if (this->Streamers[ptId].Direction > 0.0)
        {
          i2 = i * this->NumberOfSides;
        }
        else
        {
          i2 = (npts - i - 1) * this->NumberOfSides;
        }
        newStrips->InsertCellPoint(ptOffset + i2 + k);
        newStrips->InsertCellPoint(ptOffset + i2 + i1);
      }
    } // for all tube sides

    ptOffset += this->NumberOfSides * npts;

  } // for all hyperstreamlines
  //
  // Update ourselves
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetStrips(newStrips);
  newStrips->Delete();

  if (newScalars)
  {
    int idx = outPD->AddArray(newScalars);
    outPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  outPD->SetNormals(newNormals);
  newNormals->Delete();

  outPD->SetVectors(newVectors);
  newVectors->Delete();

  output->Squeeze();

  return 1;
}

int svtkHyperStreamline::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

void svtkHyperStreamline::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->StartFrom == SVTK_START_FROM_POSITION)
  {
    os << indent << "Starting Position: (" << this->StartPosition[0] << ","
       << this->StartPosition[1] << ", " << this->StartPosition[2] << ")\n";
  }
  else
  {
    os << indent << "Starting Location:\n\tCell: " << this->StartCell
       << "\n\tSubId: " << this->StartSubId << "\n\tP.Coordinates: (" << this->StartPCoords[0]
       << ", " << this->StartPCoords[1] << ", " << this->StartPCoords[2] << ")\n";
  }

  os << indent << "Maximum Propagation Distance: " << this->MaximumPropagationDistance << "\n";

  if (this->IntegrationDirection == SVTK_INTEGRATE_FORWARD)
  {
    os << indent << "Integration Direction: FORWARD\n";
  }
  else if (this->IntegrationDirection == SVTK_INTEGRATE_BACKWARD)
  {
    os << indent << "Integration Direction: BACKWARD\n";
  }
  else
  {
    os << indent << "Integration Direction: FORWARD & BACKWARD\n";
  }

  os << indent << "Integration Step Length: " << this->IntegrationStepLength << "\n";
  os << indent << "Step Length: " << this->StepLength << "\n";

  os << indent << "Terminal Eigenvalue: " << this->TerminalEigenvalue << "\n";

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Number Of Sides: " << this->NumberOfSides << "\n";
  os << indent << "Logarithmic Scaling: " << (this->LogScaling ? "On\n" : "Off\n");

  if (this->IntegrationEigenvector == 0)
  {
    os << indent << "Integrate Along Major Eigenvector\n";
  }
  else if (this->IntegrationEigenvector == 1)
  {
    os << indent << "Integrate Along Medium Eigenvector\n";
  }
  else
  {
    os << indent << "Integrate Along Minor Eigenvector\n";
  }
}
