/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoundedPointSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBoundedPointSource.h"

#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkBoundedPointSource);

//----------------------------------------------------------------------------
svtkBoundedPointSource::svtkBoundedPointSource()
{
  this->NumberOfPoints = 100;

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;

  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->ProduceCellOutput = false;

  this->ProduceRandomScalars = false;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int svtkBoundedPointSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType ptId;
  double x[3];

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

  // Generate the points
  newPoints->SetNumberOfPoints(this->NumberOfPoints);
  double xmin = (this->Bounds[0] < this->Bounds[1] ? this->Bounds[0] : this->Bounds[1]);
  double xmax = (this->Bounds[0] < this->Bounds[1] ? this->Bounds[1] : this->Bounds[0]);
  double ymin = (this->Bounds[2] < this->Bounds[3] ? this->Bounds[2] : this->Bounds[3]);
  double ymax = (this->Bounds[2] < this->Bounds[3] ? this->Bounds[3] : this->Bounds[2]);
  double zmin = (this->Bounds[4] < this->Bounds[5] ? this->Bounds[4] : this->Bounds[5]);
  double zmax = (this->Bounds[4] < this->Bounds[5] ? this->Bounds[5] : this->Bounds[4]);

  svtkMath* math = svtkMath::New();
  for (ptId = 0; ptId < this->NumberOfPoints; ptId++)
  {
    x[0] = math->Random(xmin, xmax);
    x[1] = math->Random(ymin, ymax);
    x[2] = math->Random(zmin, zmax);
    newPoints->SetPoint(ptId, x);
  }
  output->SetPoints(newPoints);
  newPoints->Delete();

  // Generate the scalars if requested
  if (this->ProduceRandomScalars)
  {
    svtkFloatArray* scalars = svtkFloatArray::New();
    scalars->SetName("RandomScalars");
    scalars->SetNumberOfTuples(this->NumberOfPoints);
    float* s = static_cast<float*>(scalars->GetVoidPointer(0));
    double sMin =
      (this->ScalarRange[0] < this->ScalarRange[1] ? this->ScalarRange[0] : this->ScalarRange[1]);
    double sMax =
      (this->ScalarRange[0] < this->ScalarRange[1] ? this->ScalarRange[1] : this->ScalarRange[0]);
    for (ptId = 0; ptId < this->NumberOfPoints; ptId++)
    {
      *s++ = math->Random(sMin, sMax);
    }
    output->GetPointData()->SetScalars(scalars);
    scalars->Delete();
  }

  // Generate the vertices if requested
  if (this->ProduceCellOutput)
  {
    svtkCellArray* newVerts = svtkCellArray::New();
    newVerts->AllocateEstimate(1, this->NumberOfPoints);
    newVerts->InsertNextCell(this->NumberOfPoints);
    for (ptId = 0; ptId < this->NumberOfPoints; ptId++)
    {
      newVerts->InsertCellPoint(ptId);
    }
    output->SetVerts(newVerts);
    newVerts->Delete();
  }

  math->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void svtkBoundedPointSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Points: " << this->NumberOfPoints << "\n";

  for (int i = 0; i < 6; i++)
  {
    os << indent << "Bounds[" << i << "]: " << this->Bounds[i] << "\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";

  os << indent << "Produce Cell Output: " << (this->ProduceCellOutput ? "On\n" : "Off\n");

  os << indent << "Produce Random Scalars: " << (this->ProduceRandomScalars ? "On\n" : "Off\n");
  os << indent << "Scalar Range (" << this->ScalarRange[0] << "," << this->ScalarRange[1] << ")\n";
}
