/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArcSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkArcSource.h"

#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkArcSource);

// --------------------------------------------------------------------------
svtkArcSource::svtkArcSource(int res)
{
  // Default first point
  this->Point1[0] = 0.0;
  this->Point1[1] = 0.5;
  this->Point1[2] = 0.0;

  // Default second point
  this->Point2[0] = 0.5;
  this->Point2[1] = 0.0;
  this->Point2[2] = 0.0;

  // Default center is origin
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  // Default normal vector is unit in the positive Z direction.
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  // Default polar vector is unit in the positive X direction.
  this->PolarVector[0] = 1.0;
  this->PolarVector[1] = 0.0;
  this->PolarVector[2] = 0.0;

  // Default arc is a quarter-circle
  this->Angle = 90.;

  // Ensure resolution (number of line segments to approximate the arc)
  // is at least 1
  this->Resolution = (res < 1 ? 1 : res);

  // By default use the shortest angular sector
  // rather than its complement (a.k.a. negative coterminal)
  this->Negative = false;

  // By default use the original API (endpoints + center)
  this->UseNormalAndAngle = false;

  this->OutputPointsPrecision = SINGLE_PRECISION;

  // This is a source
  this->SetNumberOfInputPorts(0);
}

// --------------------------------------------------------------------------
int svtkArcSource::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

// --------------------------------------------------------------------------
int svtkArcSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  int numLines = this->Resolution;
  int numPts = this->Resolution + 1;
  double tc[3] = { 0.0, 0.0, 0.0 };

  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 1;
  }

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Calculate vector from origin to first point

  // Normal and angle are either specified (consistent API) or calculated (original API)
  double angle = 0.0;
  double radius = 0.5;
  double perpendicular[3];
  double v1[3];
  if (this->UseNormalAndAngle)
  {
    // Retrieve angle, which is specified with this API
    angle = svtkMath::RadiansFromDegrees(this->Angle);

    // Retrieve polar vector, which is specified with this API
    for (int i = 0; i < 3; ++i)
    {
      v1[i] = this->PolarVector[i];
    }

    // Calculate perpendicular vector with normal which is specified with this API
    svtkMath::Cross(this->Normal, this->PolarVector, perpendicular);

    // Calculate radius
    radius = svtkMath::Normalize(v1);
  }
  else // if ( this->UseNormalAndAngle )
  {
    // Compute the cross product of the two vectors.
    for (int i = 0; i < 3; ++i)
    {
      v1[i] = this->Point1[i] - this->Center[i];
    }

    double v2[3] = { this->Point2[0] - this->Center[0], this->Point2[1] - this->Center[1],
      this->Point2[2] - this->Center[2] };

    double normal[3];
    svtkMath::Cross(v1, v2, normal);
    svtkMath::Cross(normal, v1, perpendicular);
    double dotprod = svtkMath::Dot(v1, v2) / (svtkMath::Norm(v1) * svtkMath::Norm(v2));
    angle = acos(dotprod);
    if (this->Negative)
    {
      angle -= (2.0 * svtkMath::Pi());
    }

    // Calcute radius
    radius = svtkMath::Normalize(v1);
  } // else

  // Calcute angle increment
  double angleInc = angle / this->Resolution;

  // Normalize perpendicular vector
  svtkMath::Normalize(perpendicular);

  // Now create arc points and segments
  svtkPoints* newPoints = svtkPoints::New();

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
  svtkFloatArray* newTCoords = svtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2 * numPts);
  newTCoords->SetName("Texture Coordinates");
  svtkCellArray* newLines = svtkCellArray::New();
  newLines->AllocateEstimate(numLines, 2);

  double theta = 0.0;
  // Iterate over angle increments
  for (int i = 0; i <= this->Resolution; ++i, theta += angleInc)
  {
    const double cosine = cos(theta);
    const double sine = sin(theta);
    double p[3] = { this->Center[0] + cosine * radius * v1[0] + sine * radius * perpendicular[0],
      this->Center[1] + cosine * radius * v1[1] + sine * radius * perpendicular[1],
      this->Center[2] + cosine * radius * v1[2] + sine * radius * perpendicular[2] };

    tc[0] = static_cast<double>(i) / this->Resolution;
    newPoints->InsertPoint(i, p);
    newTCoords->InsertTuple(i, tc);
  }

  newLines->InsertNextCell(numPts);
  for (int k = 0; k < numPts; ++k)
  {
    newLines->InsertCellPoint(k);
  }

  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

// --------------------------------------------------------------------------
void svtkArcSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", " << this->Point1[1] << ", "
     << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", " << this->Point2[1] << ", "
     << this->Point2[2] << ")\n";

  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << ")\n";

  os << indent << "PolarVector: (" << this->PolarVector[0] << ", " << this->PolarVector[1] << ", "
     << this->PolarVector[2] << ")\n";

  os << indent << "Angle: " << this->Angle << "\n";

  os << indent << "Negative: " << this->Negative << "\n";

  os << indent << "UseNormalAndAngle: " << this->UseNormalAndAngle << "\n";

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
