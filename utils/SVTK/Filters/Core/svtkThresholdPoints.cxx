/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThresholdPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkThresholdPoints.h"

#include "svtkCellArray.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkThresholdPoints);

//----------------------------------------------------------------------------
// Construct with lower threshold=0, upper threshold=1, and threshold
// function=upper.
svtkThresholdPoints::svtkThresholdPoints()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->ThresholdFunction = &svtkThresholdPoints::Upper;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
// Criterion is cells whose scalars are less than lower threshold.
void svtkThresholdPoints::ThresholdByLower(double lower)
{
  int isModified = 0;

  if (this->ThresholdFunction != &svtkThresholdPoints::Lower)
  {
    this->ThresholdFunction = &svtkThresholdPoints::Lower;
    isModified = 1;
  }

  if (this->LowerThreshold != lower)
  {
    this->LowerThreshold = lower;
    isModified = 1;
  }

  if (isModified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Criterion is cells whose scalars are less than upper threshold.
void svtkThresholdPoints::ThresholdByUpper(double upper)
{
  int isModified = 0;

  if (this->ThresholdFunction != &svtkThresholdPoints::Upper)
  {
    this->ThresholdFunction = &svtkThresholdPoints::Upper;
    isModified = 1;
  }

  if (this->UpperThreshold != upper)
  {
    this->UpperThreshold = upper;
    isModified = 1;
  }

  if (isModified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Criterion is cells whose scalars are between lower and upper thresholds.
void svtkThresholdPoints::ThresholdBetween(double lower, double upper)
{
  int isModified = 0;

  if (this->ThresholdFunction != &svtkThresholdPoints::Between)
  {
    this->ThresholdFunction = &svtkThresholdPoints::Between;
    isModified = 1;
  }

  if (this->LowerThreshold != lower)
  {
    this->LowerThreshold = lower;
    isModified = 1;
  }

  if (this->UpperThreshold != upper)
  {
    this->UpperThreshold = upper;
    isModified = 1;
  }

  if (isModified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkThresholdPoints::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* inScalars;
  svtkPoints* newPoints;
  svtkPointData *pd, *outPD;
  svtkCellArray* verts;
  svtkIdType ptId, numPts, pts[1];
  double x[3];

  svtkDebugMacro(<< "Executing threshold points filter");

  if (!(inScalars = this->GetInputArrayToProcess(0, inputVector)))
  {
    svtkErrorMacro(<< "No scalar data to threshold");
    return 1;
  }

  numPts = input->GetNumberOfPoints();

  if (numPts < 1)
  {
    svtkErrorMacro(<< "No points to threshold");
    return 1;
  }

  newPoints = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      newPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPoints->SetDataType(SVTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_DOUBLE);
  }

  newPoints->Allocate(numPts);
  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd);
  verts = svtkCellArray::New();
  verts->AllocateEstimate(numPts, 1);

  // Check that the scalars of each point satisfy the threshold criterion
  int abort = 0;
  svtkIdType progressInterval = numPts / 20 + 1;

  for (ptId = 0; ptId < numPts && !abort; ptId++)
  {
    if (!(ptId % progressInterval))
    {
      this->UpdateProgress((double)ptId / numPts);
      abort = this->GetAbortExecute();
    }

    if ((this->*(this->ThresholdFunction))(inScalars->GetComponent(ptId, 0)))
    {
      input->GetPoint(ptId, x);
      pts[0] = newPoints->InsertNextPoint(x);
      outPD->CopyData(pd, ptId, pts[0]);
      verts->InsertNextCell(1, pts);
    } // satisfied thresholding
  }   // for all points

  svtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points.");

  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(verts);
  verts->Delete();

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int svtkThresholdPoints::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkThresholdPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
