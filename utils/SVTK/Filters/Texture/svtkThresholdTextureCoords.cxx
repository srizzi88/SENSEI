/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThresholdTextureCoords.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkThresholdTextureCoords.h"

#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkThresholdTextureCoords);

// Construct with lower threshold=0, upper threshold=1, threshold
// function=upper, and texture dimension = 2. The "out" texture coordinate
// is (0.25,0,0); the "in" texture coordinate is (0.75,0,0).
svtkThresholdTextureCoords::svtkThresholdTextureCoords()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->TextureDimension = 2;

  this->ThresholdFunction = &svtkThresholdTextureCoords::Upper;

  this->OutTextureCoord[0] = 0.25;
  this->OutTextureCoord[1] = 0.0;
  this->OutTextureCoord[2] = 0.0;

  this->InTextureCoord[0] = 0.75;
  this->InTextureCoord[1] = 0.0;
  this->InTextureCoord[2] = 0.0;
}

// Criterion is cells whose scalars are less than lower threshold.
void svtkThresholdTextureCoords::ThresholdByLower(double lower)
{
  if (this->LowerThreshold != lower)
  {
    this->LowerThreshold = lower;
    this->ThresholdFunction = &svtkThresholdTextureCoords::Lower;
    this->Modified();
  }
}

// Criterion is cells whose scalars are less than upper threshold.
void svtkThresholdTextureCoords::ThresholdByUpper(double upper)
{
  if (this->UpperThreshold != upper)
  {
    this->UpperThreshold = upper;
    this->ThresholdFunction = &svtkThresholdTextureCoords::Upper;
    this->Modified();
  }
}

// Criterion is cells whose scalars are between lower and upper thresholds.
void svtkThresholdTextureCoords::ThresholdBetween(double lower, double upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
  {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->ThresholdFunction = &svtkThresholdTextureCoords::Between;
    this->Modified();
  }
}

int svtkThresholdTextureCoords::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts;
  svtkFloatArray* newTCoords;
  svtkIdType ptId;
  svtkDataArray* inScalars;

  svtkDebugMacro(<< "Executing texture threshold filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (!(inScalars = input->GetPointData()->GetScalars()))
  {
    svtkErrorMacro(<< "No scalar data to texture threshold");
    return 1;
  }

  numPts = input->GetNumberOfPoints();
  newTCoords = svtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2 * this->TextureDimension);

  // Check that the scalars of each point satisfy the threshold criterion
  for (ptId = 0; ptId < numPts; ptId++)
  {
    if ((this->*(this->ThresholdFunction))(inScalars->GetComponent(ptId, 0)))
    {
      newTCoords->InsertTuple(ptId, this->InTextureCoord);
    }
    else // doesn't satisfy criterion
    {
      newTCoords->InsertTuple(ptId, this->OutTextureCoord);
    }

  } // for all points

  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  return 1;
}

void svtkThresholdTextureCoords::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ThresholdFunction == &svtkThresholdTextureCoords::Upper)
  {
    os << indent << "Threshold By Upper\n";
  }

  else if (this->ThresholdFunction == &svtkThresholdTextureCoords::Lower)
  {
    os << indent << "Threshold By Lower\n";
  }

  else if (this->ThresholdFunction == &svtkThresholdTextureCoords::Between)
  {
    os << indent << "Threshold Between\n";
  }

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";
  os << indent << "Texture Dimension: " << this->TextureDimension << "\n";

  os << indent << "Out Texture Coordinate: (" << this->OutTextureCoord[0] << ", "
     << this->OutTextureCoord[1] << ", " << this->OutTextureCoord[2] << ")\n";

  os << indent << "In Texture Coordinate: (" << this->InTextureCoord[0] << ", "
     << this->InTextureCoord[1] << ", " << this->InTextureCoord[2] << ")\n";
}
