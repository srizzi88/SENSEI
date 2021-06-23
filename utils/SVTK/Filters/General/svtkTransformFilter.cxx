/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTransformFilter.h"

#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLinearTransform.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridToPointSet.h"
#include "svtkStructuredGrid.h"

#include "svtkNew.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkTransformFilter);
svtkCxxSetObjectMacro(svtkTransformFilter, Transform, svtkAbstractTransform);

svtkTransformFilter::svtkTransformFilter()
{
  this->Transform = nullptr;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
  this->TransformAllInputVectors = false;
}

svtkTransformFilter::~svtkTransformFilter()
{
  this->SetTransform(nullptr);
}

int svtkTransformFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

int svtkTransformFilter::RequestDataObject(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkImageData* inImage = svtkImageData::GetData(inputVector[0]);
  svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    svtkStructuredGrid* output = svtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      svtkNew<svtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

int svtkTransformFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkSmartPointer<svtkPointSet> input = svtkPointSet::GetData(inputVector[0]);
  svtkPointSet* output = svtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    svtkImageData* inImage = svtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      svtkNew<svtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      svtkNew<svtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    svtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  svtkPoints* inPts;
  svtkPoints* newPts;
  svtkDataArray *inVectors, *inCellVectors;
  svtkDataArray *newVectors = nullptr, *newCellVectors = nullptr;
  svtkDataArray *inNormals, *inCellNormals;
  svtkDataArray *newNormals = nullptr, *newCellNormals = nullptr;
  svtkIdType numPts, numCells;
  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();

  svtkDebugMacro(<< "Executing transform filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

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
    newVectors = this->CreateNewDataArray(inVectors);
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3 * numPts);
    newVectors->SetName(inVectors->GetName());
  }
  if (inNormals)
  {
    newNormals = this->CreateNewDataArray(inNormals);
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3 * numPts);
    newNormals->SetName(inNormals->GetName());
  }

  this->UpdateProgress(.2);
  // Loop over all points, updating position
  //

  int nArrays = pd->GetNumberOfArrays();
  svtkDataArray** inVrsArr = new svtkDataArray*[nArrays];
  svtkDataArray** outVrsArr = new svtkDataArray*[nArrays];
  int nInputVectors = 0;
  if (this->TransformAllInputVectors)
  {
    for (int i = 0; i < nArrays; i++)
    {
      svtkDataArray* tmpArray = pd->GetArray(i);
      if (tmpArray != inVectors && tmpArray != inNormals && tmpArray->GetNumberOfComponents() == 3)
      {
        inVrsArr[nInputVectors] = tmpArray;
        svtkDataArray* tmpOutArray = this->CreateNewDataArray(tmpArray);
        tmpOutArray->SetNumberOfComponents(3);
        tmpOutArray->Allocate(3 * numPts);
        tmpOutArray->SetName(tmpArray->GetName());
        outVrsArr[nInputVectors] = tmpOutArray;
        outPD->AddArray(tmpOutArray);
        nInputVectors++;
        tmpOutArray->Delete();
      }
    }
  }

  if (inVectors || inNormals || nInputVectors > 0)
  {
    this->Transform->TransformPointsNormalsVectors(inPts, newPts, inNormals, newNormals, inVectors,
      newVectors, nInputVectors, inVrsArr, outVrsArr);
  }
  else
  {
    this->Transform->TransformPoints(inPts, newPts);
  }

  delete[] inVrsArr;
  delete[] outVrsArr;

  this->UpdateProgress(.6);

  // Can only transform cell normals/vectors if the transform
  // is linear.
  svtkLinearTransform* lt = svtkLinearTransform::SafeDownCast(this->Transform);
  if (lt)
  {
    if (inCellVectors)
    {
      newCellVectors = this->CreateNewDataArray(inCellVectors);
      newCellVectors->SetNumberOfComponents(3);
      newCellVectors->Allocate(3 * numCells);
      newCellVectors->SetName(inCellVectors->GetName());
      lt->TransformVectors(inCellVectors, newCellVectors);
    }
    if (this->TransformAllInputVectors)
    {
      for (int i = 0; i < cd->GetNumberOfArrays(); i++)
      {
        svtkDataArray* tmpArray = cd->GetArray(i);
        if (tmpArray != inCellVectors && tmpArray != inCellNormals &&
          tmpArray->GetNumberOfComponents() == 3)
        {
          svtkDataArray* tmpOutArray = this->CreateNewDataArray(tmpArray);
          tmpOutArray->SetNumberOfComponents(3);
          tmpOutArray->Allocate(3 * numCells);
          tmpOutArray->SetName(tmpArray->GetName());
          lt->TransformVectors(tmpArray, tmpOutArray);
          outCD->AddArray(tmpOutArray);
          tmpOutArray->Delete();
        }
      }
    }
    if (inCellNormals)
    {
      newCellNormals = this->CreateNewDataArray(inCellNormals);
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
  if (this->TransformAllInputVectors)
  {
    for (int i = 0; i < pd->GetNumberOfArrays(); i++)
    {
      if (!outPD->GetArray(pd->GetAbstractArray(i)->GetName()))
      {
        outPD->AddArray(pd->GetAbstractArray(i));
        int attributeType = pd->IsArrayAnAttribute(i);
        if (attributeType >= 0 && attributeType != svtkDataSetAttributes::VECTORS &&
          attributeType != svtkDataSetAttributes::NORMALS)
        {
          outPD->SetAttribute(pd->GetAbstractArray(i), attributeType);
        }
      }
    }
    for (int i = 0; i < cd->GetNumberOfArrays(); i++)
    {
      if (!outCD->GetArray(cd->GetAbstractArray(i)->GetName()))
      {
        outCD->AddArray(cd->GetAbstractArray(i));
        int attributeType = pd->IsArrayAnAttribute(i);
        if (attributeType >= 0 && attributeType != svtkDataSetAttributes::VECTORS &&
          attributeType != svtkDataSetAttributes::NORMALS)
        {
          outPD->SetAttribute(pd->GetAbstractArray(i), attributeType);
        }
      }
    }
    // TODO does order matters ?
  }
  else
  {
    outPD->PassData(pd);
    outCD->PassData(cd);
  }

  svtkFieldData* inFD = input->GetFieldData();
  if (inFD)
  {
    svtkFieldData* outFD = output->GetFieldData();
    if (!outFD)
    {
      outFD = svtkFieldData::New();
      output->SetFieldData(outFD);
      // We can still use outFD since it's registered
      // by the output
      outFD->Delete();
    }
    outFD->PassData(inFD);
  }

  return 1;
}

svtkMTimeType svtkTransformFilter::GetMTime()
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

svtkDataArray* svtkTransformFilter::CreateNewDataArray(svtkDataArray* input)
{
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION && input != nullptr)
  {
    return input->NewInstance();
  }

  switch (this->OutputPointsPrecision)
  {
    case svtkAlgorithm::DOUBLE_PRECISION:
      return svtkDoubleArray::New();
    case svtkAlgorithm::SINGLE_PRECISION:
    default:
      return svtkFloatArray::New();
  }
}

void svtkTransformFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Transform: " << this->Transform << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
