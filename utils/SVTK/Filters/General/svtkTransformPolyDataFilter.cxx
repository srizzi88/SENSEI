/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTransformPolyDataFilter.h"

#include "svtkAbstractTransform.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLinearTransform.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkTransformPolyDataFilter);
svtkCxxSetObjectMacro(svtkTransformPolyDataFilter, Transform, svtkAbstractTransform);

svtkTransformPolyDataFilter::svtkTransformPolyDataFilter()
{
  this->Transform = nullptr;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
}

svtkTransformPolyDataFilter::~svtkTransformPolyDataFilter()
{
  this->SetTransform(nullptr);
}

int svtkTransformPolyDataFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* inPts;
  svtkPoints* newPts;
  svtkDataArray *inVectors, *inCellVectors;
  svtkFloatArray *newVectors = nullptr, *newCellVectors = nullptr;
  svtkDataArray *inNormals, *inCellNormals;
  svtkFloatArray *newNormals = nullptr, *newCellNormals = nullptr;
  svtkIdType numPts, numCells;
  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();

  svtkDebugMacro(<< "Executing polygonal transformation");

  // Check input
  //
  if (this->Transform == nullptr)
  {
    svtkErrorMacro(<< "No transform defined!");
    return 1;
  }

  inPts = input->GetPoints();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();
  inCellVectors = cd->GetVectors();
  inCellNormals = cd->GetNormals();

  if (!inPts)
  {
    svtkErrorMacro(<< "No input data");
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  newPts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->Allocate(numPts);
  if (inVectors)
  {
    newVectors = svtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3 * numPts);
    newVectors->SetName(inVectors->GetName());
  }
  if (inNormals)
  {
    newNormals = svtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3 * numPts);
    newNormals->SetName(inNormals->GetName());
  }

  this->UpdateProgress(.2);
  // Loop over all points, updating position
  //

  if (inVectors || inNormals)
  {
    this->Transform->TransformPointsNormalsVectors(
      inPts, newPts, inNormals, newNormals, inVectors, newVectors);
  }
  else
  {
    this->Transform->TransformPoints(inPts, newPts);
  }

  this->UpdateProgress(.6);

  // Can only transform cell normals/vectors if the transform
  // is linear.
  svtkLinearTransform* lt = svtkLinearTransform::SafeDownCast(this->Transform);
  if (lt)
  {
    if (inCellVectors)
    {
      newCellVectors = svtkFloatArray::New();
      newCellVectors->SetNumberOfComponents(3);
      newCellVectors->Allocate(3 * numCells);
      newCellVectors->SetName(inCellVectors->GetName());
      lt->TransformVectors(inCellVectors, newCellVectors);
    }
    if (inCellNormals)
    {
      newCellNormals = svtkFloatArray::New();
      newCellNormals->SetNumberOfComponents(3);
      newCellNormals->Allocate(3 * numCells);
      newCellNormals->SetName(inCellNormals->GetName());
      lt->TransformNormals(inCellNormals, newCellNormals);
    }
  }

  this->UpdateProgress(.8);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  output->SetPolys(input->GetPolys());
  output->SetStrips(input->GetStrips());

  if (newNormals)
  {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    outPD->CopyNormalsOff();
  }

  if (newVectors)
  {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
    outPD->CopyVectorsOff();
  }

  if (newCellNormals)
  {
    outCD->SetNormals(newCellNormals);
    newCellNormals->Delete();
    outCD->CopyNormalsOff();
  }

  if (newCellVectors)
  {
    outCD->SetVectors(newCellVectors);
    newCellVectors->Delete();
    outCD->CopyVectorsOff();
  }

  outPD->PassData(pd);
  outCD->PassData(cd);

  return 1;
}

svtkMTimeType svtkTransformPolyDataFilter::GetMTime()
{
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType transMTime;

  if (this->Transform)
  {
    transMTime = this->Transform->GetMTime();
    mTime = (transMTime > mTime ? transMTime : mTime);
  }

  return mTime;
}

void svtkTransformPolyDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Transform: " << this->Transform << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
