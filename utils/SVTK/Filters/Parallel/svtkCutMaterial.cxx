/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCutMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCutMaterial.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCutter.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkCutMaterial);

// Instantiate object with no input and no defined output.
svtkCutMaterial::svtkCutMaterial()
{
  this->MaterialArrayName = nullptr;
  this->SetMaterialArrayName("material");
  this->Material = 0;
  this->ArrayName = nullptr;

  this->UpVector[0] = 0.0;
  this->UpVector[1] = 0.0;
  this->UpVector[2] = 1.0;

  this->MaximumPoint[0] = 0.0;
  this->MaximumPoint[1] = 0.0;
  this->MaximumPoint[2] = 0.0;

  this->CenterPoint[0] = 0.0;
  this->CenterPoint[1] = 0.0;
  this->CenterPoint[2] = 0.0;

  this->Normal[0] = 0.0;
  this->Normal[1] = 1.0;
  this->Normal[2] = 0.0;

  this->PlaneFunction = svtkPlane::New();
}

svtkCutMaterial::~svtkCutMaterial()
{
  this->PlaneFunction->Delete();
  this->PlaneFunction = nullptr;

  this->SetMaterialArrayName(nullptr);
  this->SetArrayName(nullptr);
}

int svtkCutMaterial::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkThreshold* thresh;
  svtkCutter* cutter;

  // Check to see if we have the required field arrays.
  if (this->MaterialArrayName == nullptr || this->ArrayName == nullptr)
  {
    svtkErrorMacro("Material and Array names must be set.");
    return 0;
  }

  if (input->GetCellData()->GetArray(this->MaterialArrayName) == nullptr)
  {
    svtkErrorMacro("Could not find cell array " << this->MaterialArrayName);
    return 0;
  }
  if (input->GetCellData()->GetArray(this->ArrayName) == nullptr)
  {
    svtkErrorMacro("Could not find cell array " << this->ArrayName);
    return 0;
  }

  // It would be nice to get rid of this in the future.
  thresh = svtkThreshold::New();
  thresh->SetInputData(input);
  thresh->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, this->MaterialArrayName);
  thresh->ThresholdBetween(this->Material - 0.5, this->Material + 0.5);
  thresh->Update();

  const double* bds = thresh->GetOutput()->GetBounds();
  this->CenterPoint[0] = 0.5 * (bds[0] + bds[1]);
  this->CenterPoint[1] = 0.5 * (bds[2] + bds[3]);
  this->CenterPoint[2] = 0.5 * (bds[4] + bds[5]);

  this->ComputeMaximumPoint(thresh->GetOutput());
  this->ComputeNormal();

  this->PlaneFunction->SetOrigin(this->CenterPoint);
  this->PlaneFunction->SetNormal(this->Normal);

  cutter = svtkCutter::New();
  cutter->SetInputConnection(thresh->GetOutputPort());
  cutter->SetCutFunction(this->PlaneFunction);
  cutter->SetValue(0, 0.0);
  cutter->Update();

  output->CopyStructure(cutter->GetOutput());
  output->GetPointData()->PassData(cutter->GetOutput()->GetPointData());
  output->GetCellData()->PassData(cutter->GetOutput()->GetCellData());

  cutter->Delete();
  thresh->Delete();

  return 1;
}

void svtkCutMaterial::ComputeNormal()
{
  double tmp[3];
  double mag;

  if (this->UpVector[0] == 0.0 && this->UpVector[1] == 0.0 && this->UpVector[2] == 0.0)
  {
    svtkErrorMacro("Zero magnitude UpVector.");
    this->UpVector[2] = 1.0;
  }

  tmp[0] = this->MaximumPoint[0] - this->CenterPoint[0];
  tmp[1] = this->MaximumPoint[1] - this->CenterPoint[1];
  tmp[2] = this->MaximumPoint[2] - this->CenterPoint[2];
  svtkMath::Cross(tmp, this->UpVector, this->Normal);
  mag = svtkMath::Normalize(this->Normal);
  // Rare singularity
  while (mag == 0.0)
  {
    tmp[0] = svtkMath::Random();
    tmp[1] = svtkMath::Random();
    tmp[2] = svtkMath::Random();
    svtkMath::Cross(tmp, this->UpVector, this->Normal);
    mag = svtkMath::Normalize(this->Normal);
  }
}

void svtkCutMaterial::ComputeMaximumPoint(svtkDataSet* input)
{
  svtkDataArray* data;
  svtkIdType idx, bestIdx, num;
  double comp, best;
  svtkCell* cell;

  // Find the maximum value.
  data = input->GetCellData()->GetArray(this->ArrayName);
  if (data == nullptr)
  {
    svtkErrorMacro("What happened to the array " << this->ArrayName);
    return;
  }

  num = data->GetNumberOfTuples();
  if (num <= 0)
  {
    svtkErrorMacro("No values in array " << this->ArrayName);
    return;
  }

  best = data->GetComponent(0, 0);
  bestIdx = 0;
  for (idx = 1; idx < num; ++idx)
  {
    comp = data->GetComponent(idx, 0);
    if (comp > best)
    {
      best = comp;
      bestIdx = idx;
    }
  }

  // Get the cell with the larges value.
  cell = input->GetCell(bestIdx);
  const double* bds = cell->GetBounds();
  this->MaximumPoint[0] = (bds[0] + bds[1]) * 0.5;
  this->MaximumPoint[1] = (bds[2] + bds[3]) * 0.5;
  this->MaximumPoint[2] = (bds[4] + bds[5]) * 0.5;
}

int svtkCutMaterial::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

void svtkCutMaterial::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ArrayName: ";
  if (this->ArrayName)
  {
    os << this->ArrayName << endl;
  }
  else
  {
    os << "(None)" << endl;
  }
  os << indent << "MaterialArrayName: " << this->MaterialArrayName << endl;
  os << indent << "Material: " << this->Material << endl;

  os << indent << "UpVector: " << this->UpVector[0] << ", " << this->UpVector[1] << ", "
     << this->UpVector[2] << endl;

  os << indent << "MaximumPoint: " << this->MaximumPoint[0] << ", " << this->MaximumPoint[1] << ", "
     << this->MaximumPoint[2] << endl;
  os << indent << "CenterPoint: " << this->CenterPoint[0] << ", " << this->CenterPoint[1] << ", "
     << this->CenterPoint[2] << endl;
  os << indent << "Normal: " << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << endl;
}
