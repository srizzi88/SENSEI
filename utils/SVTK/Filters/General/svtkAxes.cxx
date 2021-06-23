/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAxes.h"

#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkAxes);

//----------------------------------------------------------------------------
// Construct with origin=(0,0,0) and scale factor=1.
svtkAxes::svtkAxes()
{
  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;

  this->ScaleFactor = 1.0;

  this->Symmetric = 0;
  this->ComputeNormals = 1;

  this->SetNumberOfInputPorts(0);
}

int svtkAxes::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int numPts = 6, numLines = 3;
  svtkPoints* newPts;
  svtkCellArray* newLines;
  svtkFloatArray* newScalars;
  svtkFloatArray* newNormals;
  double x[3], n[3];
  svtkIdType ptIds[2];

  svtkDebugMacro(<< "Creating x-y-z axes");

  newPts = svtkPoints::New();
  newPts->Allocate(numPts);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(numLines, 2);
  newScalars = svtkFloatArray::New();
  newScalars->Allocate(numPts);
  newScalars->SetName("Axes");
  newNormals = svtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPts);
  newNormals->SetName("Normals");

  //
  // Create axes
  //
  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
  {
    x[0] = this->Origin[0] - this->ScaleFactor;
  }
  n[0] = 0.0;
  n[1] = 1.0;
  n[2] = 0.0;
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.0);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0] + this->ScaleFactor;
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  ptIds[1] = newPts->InsertNextPoint(x);
  newLines->InsertNextCell(2, ptIds);
  newScalars->InsertNextValue(0.0);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
  {
    x[1] = this->Origin[1] - this->ScaleFactor;
  }
  n[0] = 0.0;
  n[1] = 0.0;
  n[2] = 1.0;
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.25);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1] + this->ScaleFactor;
  x[2] = this->Origin[2];
  ptIds[1] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.25);
  newNormals->InsertNextTuple(n);
  newLines->InsertNextCell(2, ptIds);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
  {
    x[2] = this->Origin[2] - this->ScaleFactor;
  }
  n[0] = 1.0;
  n[1] = 0.0;
  n[2] = 0.0;
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.5);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2] + this->ScaleFactor;
  ptIds[1] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.5);
  newNormals->InsertNextTuple(n);
  newLines->InsertNextCell(2, ptIds);

  //
  // Update our output and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  if (this->ComputeNormals)
  {
    output->GetPointData()->SetNormals(newNormals);
  }
  newNormals->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// This source does not know how to generate pieces yet.
int svtkAxes::ComputeDivisionExtents(svtkDataObject* svtkNotUsed(output), int idx, int numDivisions)
{
  if (idx == 0 && numDivisions == 1)
  {
    // I will give you the whole thing
    return 1;
  }
  else
  {
    // I have nothing to give you for this piece.
    return 0;
  }
}

//----------------------------------------------------------------------------
void svtkAxes::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Origin: (" << this->Origin[0] << ", " << this->Origin[1] << ", "
     << this->Origin[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Symmetric: " << this->Symmetric << "\n";
  os << indent << "ComputeNormals: " << this->ComputeNormals << "\n";
}
