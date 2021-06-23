/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointSource.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRandomSequence.h"

#include <cfloat>
#include <cmath>

svtkStandardNewMacro(svtkPointSource);

//---------------------------------------------------------------------------
// Specify a random sequence, or use the non-threadsafe one in svtkMath by
// default.
svtkCxxSetObjectMacro(svtkPointSource, RandomSequence, svtkRandomSequence);

//----------------------------------------------------------------------------
svtkPointSource::svtkPointSource(svtkIdType numPts)
{
  this->NumberOfPoints = (numPts > 0 ? numPts : 10);

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->Radius = 0.5;

  this->Distribution = SVTK_POINT_UNIFORM;
  this->OutputPointsPrecision = SINGLE_PRECISION;
  this->RandomSequence = nullptr;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkPointSource::~svtkPointSource()
{
  this->SetRandomSequence(nullptr);
}

//----------------------------------------------------------------------------
int svtkPointSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType i;
  double theta, rho, cosphi, sinphi, radius;
  double x[3];
  svtkPoints* newPoints;
  svtkCellArray* newVerts;

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

  newPoints->Allocate(this->NumberOfPoints);
  newVerts = svtkCellArray::New();
  newVerts->AllocateEstimate(1, this->NumberOfPoints);

  newVerts->InsertNextCell(this->NumberOfPoints);

  if (this->Distribution == SVTK_POINT_SHELL)
  { // only produce points on the surface of the sphere
    for (i = 0; i < this->NumberOfPoints; i++)
    {
      cosphi = 1 - 2 * this->Random();
      sinphi = sqrt(1 - cosphi * cosphi);
      radius = this->Radius * sinphi;
      theta = 2.0 * svtkMath::Pi() * this->Random();
      x[0] = this->Center[0] + radius * cos(theta);
      x[1] = this->Center[1] + radius * sin(theta);
      x[2] = this->Center[2] + this->Radius * cosphi;
      newVerts->InsertCellPoint(newPoints->InsertNextPoint(x));
    }
  }
  else
  { // uniform distribution throughout the sphere volume
    for (i = 0; i < this->NumberOfPoints; i++)
    {
      cosphi = 1 - 2 * this->Random();
      sinphi = sqrt(1 - cosphi * cosphi);
      rho = this->Radius * pow(this->Random(), 0.33333333);
      radius = rho * sinphi;
      theta = 2.0 * svtkMath::Pi() * this->Random();
      x[0] = this->Center[0] + radius * cos(theta);
      x[1] = this->Center[1] + radius * sin(theta);
      x[2] = this->Center[2] + rho * cosphi;
      newVerts->InsertCellPoint(newPoints->InsertNextPoint(x));
    }
  }
  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(newVerts);
  newVerts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
double svtkPointSource::Random()
{
  if (!this->RandomSequence)
  {
    return svtkMath::Random();
  }

  this->RandomSequence->Next();
  return this->RandomSequence->GetValue();
}

//----------------------------------------------------------------------------
void svtkPointSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Points: " << this->NumberOfPoints << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << ")\n";
  os << indent
     << "Distribution: " << ((this->Distribution == SVTK_POINT_SHELL) ? "Shell\n" : "Uniform\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
