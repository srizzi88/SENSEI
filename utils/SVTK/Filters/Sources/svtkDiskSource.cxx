/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDiskSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDiskSource.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkDiskSource);

svtkDiskSource::svtkDiskSource()
{
  this->InnerRadius = 0.25;
  this->OuterRadius = 0.5;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;
  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->SetNumberOfInputPorts(0);
}

int svtkDiskSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPolys, numPts;
  double x[3];
  int i, j;
  svtkIdType pts[4];
  double theta, deltaRadius;
  double cosTheta, sinTheta;
  svtkPoints* newPoints;
  svtkCellArray* newPolys;

  // Set things up; allocate memory
  //
  numPts = (this->RadialResolution + 1) * (this->CircumferentialResolution + 1);
  numPolys = this->RadialResolution * this->CircumferentialResolution;
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
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(numPolys, 4);

  // Create disk
  //
  theta = 2.0 * svtkMath::Pi() / this->CircumferentialResolution;
  deltaRadius = (this->OuterRadius - this->InnerRadius) / this->RadialResolution;

  for (i = 0; i < this->CircumferentialResolution; i++)
  {
    cosTheta = cos(i * theta);
    sinTheta = sin(i * theta);
    for (j = 0; j <= this->RadialResolution; j++)
    {
      x[0] = (this->InnerRadius + j * deltaRadius) * cosTheta;
      x[1] = (this->InnerRadius + j * deltaRadius) * sinTheta;
      x[2] = 0.0;
      newPoints->InsertNextPoint(x);
    }
  }

  //  Create connectivity
  //
  for (i = 0; i < this->CircumferentialResolution; i++)
  {
    for (j = 0; j < this->RadialResolution; j++)
    {
      pts[0] = i * (this->RadialResolution + 1) + j;
      pts[1] = pts[0] + 1;
      if (i < (this->CircumferentialResolution - 1))
      {
        pts[2] = pts[1] + this->RadialResolution + 1;
      }
      else
      {
        pts[2] = j + 1;
      }
      pts[3] = pts[2] - 1;
      newPolys->InsertNextCell(4, pts);
    }
  }

  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

void svtkDiskSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
