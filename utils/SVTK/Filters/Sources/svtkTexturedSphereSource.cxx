/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexturedSphereSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTexturedSphereSource.h"

#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkTexturedSphereSource);

// Construct sphere with radius=0.5 and default resolution 8 in both Phi
// and Theta directions.
svtkTexturedSphereSource::svtkTexturedSphereSource(int res)
{
  res = res < 4 ? 4 : res;
  this->Radius = 0.5;
  this->ThetaResolution = res;
  this->PhiResolution = res;
  this->Theta = 0.0;
  this->Phi = 0.0;
  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->SetNumberOfInputPorts(0);
}

int svtkTexturedSphereSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int i, j;
  int numPts;
  int numPolys;
  svtkPoints* newPoints;
  svtkFloatArray* newNormals;
  svtkFloatArray* newTCoords;
  svtkCellArray* newPolys;
  double x[3], deltaPhi, deltaTheta, phi, theta, radius, norm;
  svtkIdType pts[3];
  double tc[2];

  //
  // Set things up; allocate memory
  //

  numPts = (this->PhiResolution + 1) * (this->ThetaResolution + 1);
  // creating triangles
  numPolys = this->PhiResolution * 2 * this->ThetaResolution;

  newPoints = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(SVTK_FLOAT);
  }

  newPoints->Allocate(numPts);
  newNormals = svtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3 * numPts);
  newTCoords = svtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2 * numPts);
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(numPolys, 3);
  //
  // Create sphere
  //
  // Create intermediate points
  deltaPhi = svtkMath::Pi() / this->PhiResolution;
  deltaTheta = 2.0 * svtkMath::Pi() / this->ThetaResolution;
  for (i = 0; i <= this->ThetaResolution; i++)
  {
    theta = i * deltaTheta;
    tc[0] = theta / (2.0 * svtkMath::Pi());
    for (j = 0; j <= this->PhiResolution; j++)
    {
      phi = j * deltaPhi;
      radius = this->Radius * sin((double)phi);
      x[0] = radius * cos((double)theta);
      x[1] = radius * sin((double)theta);
      x[2] = this->Radius * cos((double)phi);
      newPoints->InsertNextPoint(x);

      if ((norm = svtkMath::Norm(x)) == 0.0)
      {
        norm = 1.0;
      }
      x[0] /= norm;
      x[1] /= norm;
      x[2] /= norm;
      newNormals->InsertNextTuple(x);

      tc[1] = 1.0 - phi / svtkMath::Pi();
      newTCoords->InsertNextTuple(tc);
    }
  }
  //
  // Generate mesh connectivity
  //
  // bands between poles
  for (i = 0; i < this->ThetaResolution; i++)
  {
    for (j = 0; j < this->PhiResolution; j++)
    {
      pts[0] = (this->PhiResolution + 1) * i + j;
      pts[1] = pts[0] + 1;
      pts[2] = ((this->PhiResolution + 1) * (i + 1) + j) + 1;
      newPolys->InsertNextCell(3, pts);

      pts[1] = pts[2];
      pts[2] = pts[1] - 1;
      newPolys->InsertNextCell(3, pts);
    }
  }
  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

void svtkTexturedSphereSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Theta Resolution: " << this->ThetaResolution << "\n";
  os << indent << "Phi Resolution: " << this->PhiResolution << "\n";
  os << indent << "Theta: " << this->Theta << "\n";
  os << indent << "Phi: " << this->Phi << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
