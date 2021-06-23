/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSplineFilter.h"

#include "svtkCardinalSpline.h"
#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkSplineFilter);
svtkCxxSetObjectMacro(svtkSplineFilter, Spline, svtkSpline);

svtkSplineFilter::svtkSplineFilter()
{
  this->Subdivide = SVTK_SUBDIVIDE_SPECIFIED;
  this->MaximumNumberOfSubdivisions = SVTK_INT_MAX;
  this->NumberOfSubdivisions = 100;
  this->Length = 0.1;
  this->GenerateTCoords = SVTK_TCOORDS_FROM_NORMALIZED_LENGTH;
  this->TextureLength = 1.0;

  this->Spline = svtkCardinalSpline::New();
  this->TCoordMap = svtkFloatArray::New();
}

svtkSplineFilter::~svtkSplineFilter()
{
  if (this->Spline)
  {
    this->Spline->Delete();
    this->Spline = nullptr;
  }

  if (this->TCoordMap)
  {
    this->TCoordMap->Delete();
    this->TCoordMap = nullptr;
  }
}

int svtkSplineFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData* pd = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkCellData* outCD = output->GetCellData();
  svtkCellArray* inLines;

  svtkPoints* inPts;
  svtkIdType numLines;
  svtkCellArray* newLines;
  svtkIdType numNewPts, numNewCells;
  svtkPoints* newPts;
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  svtkIdType offset = 0;
  svtkFloatArray* newTCoords = nullptr;
  int abort = 0;
  svtkIdType inCellId, numGenPts;
  int genTCoords = SVTK_TCOORDS_OFF;

  // Check input and initialize
  //
  svtkDebugMacro(<< "Splining polylines");

  if (!(inPts = input->GetPoints()) || inPts->GetNumberOfPoints() < 1 ||
    !(inLines = input->GetLines()) || (numLines = inLines->GetNumberOfCells()) < 1)
  {
    return 1;
  }

  if (this->Spline == nullptr)
  {
    svtkWarningMacro(<< "Need to specify a spline!");
    return 1;
  }

  // Create the geometry and topology
  numNewPts = this->NumberOfSubdivisions * numLines;
  newPts = svtkPoints::New();
  newPts->Allocate(numNewPts);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(1, numNewPts);

  // Point data
  if ((this->GenerateTCoords == SVTK_TCOORDS_FROM_SCALARS && pd->GetScalars() != nullptr) ||
    (this->GenerateTCoords == SVTK_TCOORDS_FROM_LENGTH ||
      this->GenerateTCoords == SVTK_TCOORDS_FROM_NORMALIZED_LENGTH))
  {
    genTCoords = this->GenerateTCoords;
    newTCoords = svtkFloatArray::New();
    newTCoords->SetNumberOfComponents(2);
    newTCoords->Allocate(numNewPts);
    newTCoords->SetName("TCoords");
    outPD->CopyTCoordsOff();
  }
  outPD->InterpolateAllocate(pd, numNewPts);
  this->TCoordMap->Allocate(SVTK_CELL_SIZE);

  // Copy cell data
  numNewCells = numLines;
  outCD->CopyNormalsOff();
  outCD->CopyAllocate(cd, numNewCells);

  // Set up the splines
  this->XSpline = this->Spline->NewInstance();
  this->XSpline->DeepCopy(this->Spline);
  this->YSpline = this->Spline->NewInstance();
  this->YSpline->DeepCopy(this->Spline);
  this->ZSpline = this->Spline->NewInstance();
  this->ZSpline->DeepCopy(this->Spline);

  //  Create points along each polyline.
  //
  for (inCellId = 0, inLines->InitTraversal(); inLines->GetNextCell(npts, pts) && !abort;
       inCellId++)
  {
    this->UpdateProgress(static_cast<double>(inCellId) / numLines);
    abort = this->GetAbortExecute();

    if (npts < 2)
    {
      svtkWarningMacro(<< "Less than two points in line!");
      continue; // skip tubing this polyline
    }

    // Generate the points around the polyline. The strip is not created
    // if the polyline is bad.
    //
    this->TCoordMap->Reset();
    numGenPts =
      this->GeneratePoints(offset, npts, pts, inPts, newPts, pd, outPD, genTCoords, newTCoords);
    if (!numGenPts)
    {
      // svtkWarningMacro(<< "Could not generate points!");
      continue; // skip splining
    }

    // Generate the polyline
    //
    this->GenerateLine(offset, numGenPts, inCellId, cd, outCD, newLines);

    // Compute the new offset for the next polyline
    offset += numGenPts;

  } // for all polylines

  // Update ourselves
  //
  this->TCoordMap->Initialize();

  this->XSpline->Delete();
  this->YSpline->Delete();
  this->ZSpline->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  if (newTCoords)
  {
    outPD->SetTCoords(newTCoords);
    newTCoords->Delete();
  }

  output->Squeeze();

  return 1;
}

int svtkSplineFilter::GeneratePoints(svtkIdType offset, svtkIdType npts, const svtkIdType* pts,
  svtkPoints* inPts, svtkPoints* newPts, svtkPointData* pd, svtkPointData* outPD, int genTCoords,
  svtkFloatArray* newTCoords)
{
  svtkIdType i;

  // Initialize the splines
  this->XSpline->RemoveAllPoints();
  this->YSpline->RemoveAllPoints();
  this->ZSpline->RemoveAllPoints();

  // Compute the length of the resulting spline
  double xPrev[3], x[3], length = 0.0, len, t, tc, dist;
  inPts->GetPoint(pts[0], xPrev);
  for (i = 1; i < npts; i++)
  {
    inPts->GetPoint(pts[i], x);
    len = sqrt(svtkMath::Distance2BetweenPoints(x, xPrev));
    length += len;
    xPrev[0] = x[0];
    xPrev[1] = x[1];
    xPrev[2] = x[2];
  }
  if (length <= 0.0)
  {
    return 0; // failure
  }

  // Now we insert points into the splines with the parametric coordinate
  // based on (polyline) length. We keep track of the parametric coordinates
  // of the points for later point interpolation.
  inPts->GetPoint(pts[0], xPrev);
  for (len = 0, i = 0; i < npts; i++)
  {
    inPts->GetPoint(pts[i], x);
    dist = sqrt(svtkMath::Distance2BetweenPoints(x, xPrev));
    if (i > 0 && dist == 0)
    {
      continue;
    }
    len += dist;
    t = len / length;
    this->TCoordMap->InsertValue(i, t);

    this->XSpline->AddPoint(t, x[0]);
    this->YSpline->AddPoint(t, x[1]);
    this->ZSpline->AddPoint(t, x[2]);

    xPrev[0] = x[0];
    xPrev[1] = x[1];
    xPrev[2] = x[2];
  }

  // Compute the number of subdivisions
  svtkIdType numDivs, numNewPts;
  if (this->Subdivide == SVTK_SUBDIVIDE_SPECIFIED)
  {
    numDivs = this->NumberOfSubdivisions;
  }
  else
  {
    numDivs = static_cast<int>(length / this->Length);
  }
  numDivs =
    (numDivs < 1 ? 1
                 : (numDivs > this->MaximumNumberOfSubdivisions ? this->MaximumNumberOfSubdivisions
                                                                : numDivs));

  // Now compute the new points
  numNewPts = numDivs + 1;
  svtkIdType idx;
  double s, s0 = 0.0;
  if (genTCoords == SVTK_TCOORDS_FROM_SCALARS)
  {
    s0 = pd->GetScalars()->GetTuple1(pts[0]);
  }
  double tLo = this->TCoordMap->GetValue(0);
  double tHi = this->TCoordMap->GetValue(1);
  for (idx = 0, i = 0; i < numNewPts; i++)
  {
    t = static_cast<double>(i) / numDivs;
    x[0] = this->XSpline->Evaluate(t);
    x[1] = this->YSpline->Evaluate(t);
    x[2] = this->ZSpline->Evaluate(t);
    newPts->InsertPoint(offset + i, x);

    // interpolate point data
    while (t > tHi && idx < (npts - 2))
    {
      idx++;
      tLo = this->TCoordMap->GetValue(idx);
      tHi = this->TCoordMap->GetValue(idx + 1);
    }
    tc = (t - tLo) / (tHi - tLo);
    outPD->InterpolateEdge(pd, offset + i, pts[idx], pts[idx + 1], tc);

    // generate texture coordinates if desired
    if (genTCoords != SVTK_TCOORDS_OFF)
    {
      if (genTCoords == SVTK_TCOORDS_FROM_NORMALIZED_LENGTH)
      {
        tc = t;
      }
      else if (genTCoords == SVTK_TCOORDS_FROM_LENGTH)
      {
        tc = t * length / this->TextureLength;
      }
      else if (genTCoords == SVTK_TCOORDS_FROM_SCALARS)
      {
        s = outPD->GetScalars()->GetTuple1(offset + i); // data just interpolated
        tc = (s - s0) / this->TextureLength;
      }
      newTCoords->InsertTuple2(offset + i, tc, 0.0);
    } // if generating tcoords
  }   // for all new points

  return numNewPts;
}

void svtkSplineFilter::GenerateLine(svtkIdType offset, svtkIdType npts, svtkIdType inCellId,
  svtkCellData* cd, svtkCellData* outCD, svtkCellArray* newLines)
{
  svtkIdType i, outCellId;

  outCellId = newLines->InsertNextCell(npts);
  outCD->CopyData(cd, inCellId, outCellId);
  for (i = 0; i < npts; i++)
  {
    newLines->InsertCellPoint(offset + i);
  }
}

const char* svtkSplineFilter::GetSubdivideAsString()
{
  if (this->Subdivide == SVTK_SUBDIVIDE_SPECIFIED)
  {
    return "Specified by Number of Subdivisions";
  }
  else
  {
    return "Specified by Length";
  }
}

// Return the method of generating the texture coordinates.
const char* svtkSplineFilter::GetGenerateTCoordsAsString()
{
  if (this->GenerateTCoords == SVTK_TCOORDS_OFF)
  {
    return "GenerateTCoordsOff";
  }
  else if (this->GenerateTCoords == SVTK_TCOORDS_FROM_SCALARS)
  {
    return "GenerateTCoordsFromScalar";
  }
  else if (this->GenerateTCoords == SVTK_TCOORDS_FROM_LENGTH)
  {
    return "GenerateTCoordsFromLength";
  }
  else
  {
    return "GenerateTCoordsFromNormalizedLength";
  }
}

void svtkSplineFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Subdivide: :" << this->GetSubdivideAsString() << "\n";
  os << indent << "Maximum Number of Subdivisions: " << this->MaximumNumberOfSubdivisions << "\n";
  os << indent << "Number of Subdivisions: " << this->NumberOfSubdivisions << "\n";
  os << indent << "Length: " << this->Length << "\n";
  os << indent << "Spline: " << this->Spline << "\n";
  os << indent << "Generate TCoords: " << this->GetGenerateTCoordsAsString() << endl;
  os << indent << "Texture Length: " << this->TextureLength << endl;
}
