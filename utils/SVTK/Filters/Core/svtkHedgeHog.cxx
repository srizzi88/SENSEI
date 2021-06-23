/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHedgeHog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHedgeHog.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkHedgeHog);

svtkHedgeHog::svtkHedgeHog()
{
  this->ScaleFactor = 1.0;
  this->VectorMode = SVTK_USE_VECTOR;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
}

int svtkHedgeHog::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts;
  svtkPoints* newPts;
  svtkPointData* pd;
  svtkDataArray* inVectors;
  svtkDataArray* inNormals;
  svtkIdType ptId;
  int i;
  svtkIdType pts[2];
  svtkCellArray* newLines;
  double x[3], v[3];
  double newX[3];
  svtkPointData* outputPD = output->GetPointData();

  // Initialize
  //
  numPts = input->GetNumberOfPoints();
  pd = input->GetPointData();
  inVectors = pd->GetVectors();
  if (numPts < 1)
  {
    svtkErrorMacro(<< "No input data");
    return 1;
  }
  if (!inVectors && this->VectorMode == SVTK_USE_VECTOR)
  {
    svtkErrorMacro(<< "No vectors in input data");
    return 1;
  }

  inNormals = pd->GetNormals();
  if (!inNormals && this->VectorMode == SVTK_USE_NORMAL)
  {
    svtkErrorMacro(<< "No normals in input data");
    return 1;
  }
  outputPD->CopyAllocate(pd, 2 * numPts);

  newPts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      newPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPts->SetDataType(SVTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->SetNumberOfPoints(2 * numPts);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(numPts, 2);

  // Loop over all points, creating oriented line
  //
  for (ptId = 0; ptId < numPts; ptId++)
  {
    if (!(ptId % 10000)) // abort/progress
    {
      this->UpdateProgress(static_cast<double>(ptId) / numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    input->GetPoint(ptId, x);
    if (this->VectorMode == SVTK_USE_VECTOR)
    {
      inVectors->GetTuple(ptId, v);
    }
    else
    {
      inNormals->GetTuple(ptId, v);
    }
    for (i = 0; i < 3; i++)
    {
      newX[i] = x[i] + this->ScaleFactor * v[i];
    }

    pts[0] = ptId;
    pts[1] = ptId + numPts;

    newPts->SetPoint(pts[0], x);
    newPts->SetPoint(pts[1], newX);

    newLines->InsertNextCell(2, pts);

    outputPD->CopyData(pd, ptId, pts[0]);
    outputPD->CopyData(pd, ptId, pts[1]);
  }

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

int svtkHedgeHog::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

void svtkHedgeHog::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Orient Mode: "
     << (this->VectorMode == SVTK_USE_VECTOR ? "Orient by vector\n" : "Orient by normal\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
