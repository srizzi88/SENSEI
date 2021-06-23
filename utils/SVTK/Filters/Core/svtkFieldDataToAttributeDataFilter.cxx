/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFieldDataToAttributeDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFieldDataToAttributeDataFilter.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkFieldDataToAttributeDataFilter);

// Instantiate object with no input and no defined output.
svtkFieldDataToAttributeDataFilter::svtkFieldDataToAttributeDataFilter()
{
  int i;

  this->InputField = SVTK_DATA_OBJECT_FIELD;
  this->OutputAttributeData = SVTK_POINT_DATA;
  this->DefaultNormalize = 0;

  this->NumberOfScalarComponents = 0;
  for (i = 0; i < 4; i++)
  {
    this->ScalarArrays[i] = nullptr;
    this->ScalarArrayComponents[i] = -1; // uninitialized
    this->ScalarComponentRange[i][0] = this->ScalarComponentRange[i][1] = -1;
    this->ScalarNormalize[i] = 1; // yes, normalize
  }

  for (i = 0; i < 3; i++)
  {
    this->VectorArrays[i] = nullptr;
    this->VectorArrayComponents[i] = -1; // uninitialized
    this->VectorComponentRange[i][0] = this->VectorComponentRange[i][1] = -1;
    this->VectorNormalize[i] = 1; // yes, normalize
  }

  for (i = 0; i < 3; i++)
  {
    this->NormalArrays[i] = nullptr;
    this->NormalArrayComponents[i] = -1; // uninitialized
    this->NormalComponentRange[i][0] = this->NormalComponentRange[i][1] = -1;
    this->NormalNormalize[i] = 1; // yes, normalize
  }

  this->NumberOfTCoordComponents = 0;
  for (i = 0; i < 3; i++)
  {
    this->TCoordArrays[i] = nullptr;
    this->TCoordArrayComponents[i] = -1; // uninitialized
    this->TCoordComponentRange[i][0] = this->TCoordComponentRange[i][1] = -1;
    this->TCoordNormalize[i] = 1; // yes, normalize
  }

  for (i = 0; i < 9; i++)
  {
    this->TensorArrays[i] = nullptr;
    this->TensorArrayComponents[i] = -1; // uninitialized
    this->TensorComponentRange[i][0] = this->TensorComponentRange[i][1] = -1;
    this->TensorNormalize[i] = 1; // yes, normalize
  }
}

svtkFieldDataToAttributeDataFilter::~svtkFieldDataToAttributeDataFilter()
{
  int i;

  for (i = 0; i < 4; i++)
  {
    delete[] this->ScalarArrays[i];
  }

  for (i = 0; i < 3; i++)
  {
    delete[] this->VectorArrays[i];
  }

  for (i = 0; i < 3; i++)
  {
    delete[] this->NormalArrays[i];
  }

  for (i = 0; i < 3; i++)
  {
    delete[] this->TCoordArrays[i];
  }

  for (i = 0; i < 9; i++)
  {
    delete[] this->TensorArrays[i];
  }
}

// Stuff related to filter interface------------------------------------------
//
int svtkFieldDataToAttributeDataFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType num;
  svtkDataSetAttributes* attr;
  svtkFieldData* fd;
  svtkDebugMacro(<< "Generating attribute data from field data");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  // Pass here so that the attributes/fields can be over-written later
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  if (this->OutputAttributeData == SVTK_CELL_DATA)
  {
    attr = output->GetCellData();
    num = input->GetNumberOfCells();
  }
  else
  {
    attr = output->GetPointData();
    num = input->GetNumberOfPoints();
  }

  if (num < 1)
  {
    svtkDebugMacro(<< "No input points/cells to create attribute data for");
    return 1;
  }

  fd = nullptr;
  if (this->InputField == SVTK_DATA_OBJECT_FIELD)
  {
    fd = input->GetFieldData();
  }
  else if (this->InputField == SVTK_POINT_DATA_FIELD)
  {
    fd = input->GetPointData();
  }
  else if (this->InputField == SVTK_CELL_DATA_FIELD)
  {
    fd = input->GetCellData();
  }
  if (fd == nullptr)
  {
    svtkErrorMacro(<< "No field data available");
    return 1;
  }

  this->ConstructScalars(num, fd, attr, this->ScalarComponentRange, this->ScalarArrays,
    this->ScalarArrayComponents, this->ScalarNormalize, this->NumberOfScalarComponents);
  this->ConstructVectors(num, fd, attr, this->VectorComponentRange, this->VectorArrays,
    this->VectorArrayComponents, this->VectorNormalize);
  this->ConstructTensors(num, fd, attr, this->TensorComponentRange, this->TensorArrays,
    this->TensorArrayComponents, this->TensorNormalize);
  this->ConstructTCoords(num, fd, attr, this->TCoordComponentRange, this->TCoordArrays,
    this->TCoordArrayComponents, this->TCoordNormalize, this->NumberOfTCoordComponents);
  this->ConstructNormals(num, fd, attr, this->NormalComponentRange, this->NormalArrays,
    this->NormalArrayComponents, this->NormalNormalize);
  this->ConstructFieldData(num, attr);

  return 1;
}

void svtkFieldDataToAttributeDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input Field: ";
  if (this->InputField == SVTK_DATA_OBJECT_FIELD)
  {
    os << "DataObjectField\n";
  }
  else if (this->InputField == SVTK_POINT_DATA_FIELD)
  {
    os << "PointDataField\n";
  }
  else // if ( this->InputField == SVTK_CELL_DATA_FIELD )
  {
    os << "CellDataField\n";
  }

  os << indent << "Default Normalize: " << (this->DefaultNormalize ? "On\n" : "Off\n");

  os << indent << "Output Attribute Data: ";
  if (this->OutputAttributeData == SVTK_CELL_DATA)
  {
    os << "CellData\n";
  }
  else // if ( this->OutputAttributeData == SVTK_POINT_DATA )
  {
    os << "PointData\n";
  }
}

// Stuff related to scalars --------------------------------------------
//
void svtkFieldDataToAttributeDataFilter::SetScalarComponent(
  int comp, const char* arrayName, int arrayComp, int min, int max, int normalize)
{
  if (comp < 0 || comp > 3)
  {
    svtkErrorMacro(<< "Scalar component must be between (0,3)");
    return;
  }

  if (comp >= this->NumberOfScalarComponents)
  {
    this->NumberOfScalarComponents = comp + 1;
  }
  this->SetArrayName(this, this->ScalarArrays[comp], arrayName);
  if (this->ScalarArrayComponents[comp] != arrayComp)
  {
    this->ScalarArrayComponents[comp] = arrayComp;
    this->Modified();
  }
  if (this->ScalarComponentRange[comp][0] != min)
  {
    this->ScalarComponentRange[comp][0] = min;
    this->Modified();
  }
  if (this->ScalarComponentRange[comp][1] != max)
  {
    this->ScalarComponentRange[comp][1] = max;
    this->Modified();
  }
  if (this->ScalarNormalize[comp] != normalize)
  {
    this->ScalarNormalize[comp] = normalize;
    this->Modified();
  }
}

const char* svtkFieldDataToAttributeDataFilter::GetScalarComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarArrays[comp];
}

int svtkFieldDataToAttributeDataFilter::GetScalarComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarArrayComponents[comp];
}

int svtkFieldDataToAttributeDataFilter::GetScalarComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarComponentRange[comp][0];
}

int svtkFieldDataToAttributeDataFilter::GetScalarComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarComponentRange[comp][1];
}

int svtkFieldDataToAttributeDataFilter::GetScalarComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 3 ? 3 : comp));
  return this->ScalarNormalize[comp];
}

void svtkFieldDataToAttributeDataFilter::ConstructScalars(int num, svtkFieldData* fd,
  svtkDataSetAttributes* attr, svtkIdType componentRange[4][2], char* arrays[4], int arrayComp[4],
  int normalize[4], int numComp)
{
  int i, normalizeAny, updated = 0;
  svtkDataArray* fieldArray[4];

  if (numComp < 1)
  {
    return;
  }
  for (i = 0; i < numComp; i++)
  {
    if (arrays[i] == nullptr)
    {
      return;
    }
  }

  for (i = 0; i < numComp; i++)
  {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if (fieldArray[i] == nullptr)
    {
      svtkErrorMacro(<< "Can't find array/component requested");
      return;
    }
  }

  for (normalizeAny = i = 0; i < numComp; i++)
  {
    updated |= this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if (num != (componentRange[i][1] - componentRange[i][0] + 1))
    {
      svtkErrorMacro(<< "Number of scalars not consistent");
      return;
    }
    normalizeAny |= normalize[i];
  }

  svtkDataArray* newScalars;
  for (i = 1; i < numComp; i++) // see whether all the data is from the same array
  {
    if (fieldArray[i] != fieldArray[i - 1])
    {
      break;
    }
  }

  // see whether we can reuse the data array from the field
  if (i >= numComp && fieldArray[0]->GetNumberOfComponents() == numComp &&
    fieldArray[0]->GetNumberOfTuples() == num && !normalizeAny)
  {
    newScalars = fieldArray[0];
    newScalars->Register(nullptr);
  }
  else // have to copy data into created array
  {
    newScalars = svtkDataArray::CreateDataArray(this->GetComponentsType(numComp, fieldArray));
    newScalars->SetNumberOfTuples(num);

    for (i = 0; i < numComp; i++)
    {
      if (this->ConstructArray(newScalars, i, fieldArray[i], arrayComp[i], componentRange[i][0],
            componentRange[i][1], normalize[i]) == 0)
      {
        newScalars->Delete();
        return;
      }
    }
  }

  attr->SetScalars(newScalars);
  newScalars->Delete();
  if (updated) // reset for next execution pass
  {
    for (i = 0; i < numComp; i++)
    {
      componentRange[i][0] = componentRange[i][1] = -1;
    }
  }
}

// Stuff related to vectors --------------------------------------------
//
void svtkFieldDataToAttributeDataFilter::SetVectorComponent(
  int comp, const char* arrayName, int arrayComp, int min, int max, int normalize)
{
  if (comp < 0 || comp > 2)
  {
    svtkErrorMacro(<< "Vector component must be between (0,2)");
    return;
  }

  this->SetArrayName(this, this->VectorArrays[comp], arrayName);
  if (this->VectorArrayComponents[comp] != arrayComp)
  {
    this->VectorArrayComponents[comp] = arrayComp;
    this->Modified();
  }
  if (this->VectorComponentRange[comp][0] != min)
  {
    this->VectorComponentRange[comp][0] = min;
    this->Modified();
  }
  if (this->VectorComponentRange[comp][1] != max)
  {
    this->VectorComponentRange[comp][1] = max;
    this->Modified();
  }
  if (this->VectorNormalize[comp] != normalize)
  {
    this->VectorNormalize[comp] = normalize;
    this->Modified();
  }
}

const char* svtkFieldDataToAttributeDataFilter::GetVectorComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorArrays[comp];
}

int svtkFieldDataToAttributeDataFilter::GetVectorComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorArrayComponents[comp];
}

int svtkFieldDataToAttributeDataFilter::GetVectorComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorComponentRange[comp][0];
}

int svtkFieldDataToAttributeDataFilter::GetVectorComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorComponentRange[comp][1];
}

int svtkFieldDataToAttributeDataFilter::GetVectorComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->VectorNormalize[comp];
}

void svtkFieldDataToAttributeDataFilter::ConstructVectors(int num, svtkFieldData* fd,
  svtkDataSetAttributes* attr, svtkIdType componentRange[3][2], char* arrays[3], int arrayComp[3],
  int normalize[3])
{
  int i, updated;
  svtkDataArray* fieldArray[3];

  for (i = 0; i < 3; i++)
  {
    if (arrays[i] == nullptr)
    {
      return;
    }
  }

  for (i = 0; i < 3; i++)
  {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if (fieldArray[i] == nullptr)
    {
      svtkErrorMacro(<< "Can't find array requested");
      return;
    }
  }

  updated = this->UpdateComponentRange(fieldArray[0], componentRange[0]);
  updated |= this->UpdateComponentRange(fieldArray[1], componentRange[1]);
  updated |= this->UpdateComponentRange(fieldArray[2], componentRange[2]);

  if (num != (componentRange[0][1] - componentRange[0][0] + 1) ||
    num != (componentRange[1][1] - componentRange[1][0] + 1) ||
    num != (componentRange[2][1] - componentRange[2][0] + 1))
  {
    svtkErrorMacro(<< "Number of vectors not consistent");
    return;
  }

  svtkDataArray* newVectors;
  if (fieldArray[0]->GetNumberOfComponents() == 3 && fieldArray[0] == fieldArray[1] &&
    fieldArray[1] == fieldArray[2] && fieldArray[0]->GetNumberOfTuples() == num && !normalize[0] &&
    !normalize[1] && !normalize[2])
  {
    newVectors = fieldArray[0];
    newVectors->Register(nullptr);
  }
  else // have to copy data into created array
  {
    newVectors = svtkDataArray::CreateDataArray(this->GetComponentsType(3, fieldArray));
    newVectors->SetNumberOfComponents(3);
    newVectors->SetNumberOfTuples(num);

    for (i = 0; i < 3; i++)
    {
      if (this->ConstructArray(newVectors, i, fieldArray[i], arrayComp[i], componentRange[i][0],
            componentRange[i][1], normalize[i]) == 0)
      {
        newVectors->Delete();
        return;
      }
    }
  }

  attr->SetVectors(newVectors);
  newVectors->Delete();
  if (updated) // reset for next execution pass
  {
    for (i = 0; i < 3; i++)
    {
      componentRange[i][0] = componentRange[i][1] = -1;
    }
  }
}

// Stuff related to normals --------------------------------------------
//
void svtkFieldDataToAttributeDataFilter::SetNormalComponent(
  int comp, const char* arrayName, int arrayComp, int min, int max, int normalize)
{
  if (comp < 0 || comp > 2)
  {
    svtkErrorMacro(<< "Normal component must be between (0,2)");
    return;
  }

  this->SetArrayName(this, this->NormalArrays[comp], arrayName);
  if (this->NormalArrayComponents[comp] != arrayComp)
  {
    this->NormalArrayComponents[comp] = arrayComp;
    this->Modified();
  }
  if (this->NormalComponentRange[comp][0] != min)
  {
    this->NormalComponentRange[comp][0] = min;
    this->Modified();
  }
  if (this->NormalComponentRange[comp][1] != max)
  {
    this->NormalComponentRange[comp][1] = max;
    this->Modified();
  }
  if (this->NormalNormalize[comp] != normalize)
  {
    this->NormalNormalize[comp] = normalize;
    this->Modified();
  }
}

const char* svtkFieldDataToAttributeDataFilter::GetNormalComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalArrays[comp];
}

int svtkFieldDataToAttributeDataFilter::GetNormalComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalArrayComponents[comp];
}

int svtkFieldDataToAttributeDataFilter::GetNormalComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalComponentRange[comp][0];
}

int svtkFieldDataToAttributeDataFilter::GetNormalComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalComponentRange[comp][1];
}

int svtkFieldDataToAttributeDataFilter::GetNormalComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->NormalNormalize[comp];
}

void svtkFieldDataToAttributeDataFilter::ConstructNormals(int num, svtkFieldData* fd,
  svtkDataSetAttributes* attr, svtkIdType componentRange[3][2], char* arrays[3], int arrayComp[3],
  int normalize[3])
{
  int i, updated;
  svtkDataArray* fieldArray[3];

  for (i = 0; i < 3; i++)
  {
    if (arrays[i] == nullptr)
    {
      return;
    }
  }

  for (i = 0; i < 3; i++)
  {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if (fieldArray[i] == nullptr)
    {
      svtkErrorMacro(<< "Can't find array requested");
      return;
    }
  }

  updated = this->UpdateComponentRange(fieldArray[0], componentRange[0]);
  updated |= this->UpdateComponentRange(fieldArray[1], componentRange[1]);
  updated |= this->UpdateComponentRange(fieldArray[2], componentRange[2]);

  if (num != (componentRange[0][1] - componentRange[0][0] + 1) ||
    num != (componentRange[1][1] - componentRange[1][0] + 1) ||
    num != (componentRange[2][1] - componentRange[2][0] + 1))
  {
    svtkErrorMacro(<< "Number of normals not consistent");
    return;
  }

  svtkDataArray* newNormals;
  if (fieldArray[0]->GetNumberOfComponents() == 3 && fieldArray[0] == fieldArray[1] &&
    fieldArray[1] == fieldArray[2] && fieldArray[0]->GetNumberOfTuples() == num && !normalize[0] &&
    !normalize[1] && !normalize[2])
  {
    newNormals = fieldArray[0];
    newNormals->Register(nullptr);
  }
  else // have to copy data into created array
  {
    newNormals = svtkDataArray::CreateDataArray(this->GetComponentsType(3, fieldArray));
    newNormals->SetNumberOfComponents(3);
    newNormals->SetNumberOfTuples(num);

    for (i = 0; i < 3; i++)
    {
      if (this->ConstructArray(newNormals, i, fieldArray[i], arrayComp[i], componentRange[i][0],
            componentRange[i][1], normalize[i]) == 0)
      {
        newNormals->Delete();
        return;
      }
    }
  }

  attr->SetNormals(newNormals);
  newNormals->Delete();
  if (updated) // reset for next execution pass
  {
    for (i = 0; i < 3; i++)
    {
      componentRange[i][0] = componentRange[i][1] = -1;
    }
  }
}

// Stuff related to texture coords --------------------------------------------
//
void svtkFieldDataToAttributeDataFilter::SetTCoordComponent(
  int comp, const char* arrayName, int arrayComp, int min, int max, int normalize)
{
  if (comp < 0 || comp > 2)
  {
    svtkErrorMacro(<< "TCoord component must be between (0,2)");
    return;
  }

  if (comp >= this->NumberOfTCoordComponents)
  {
    this->NumberOfTCoordComponents = comp + 1;
  }
  this->SetArrayName(this, this->TCoordArrays[comp], arrayName);
  if (this->TCoordArrayComponents[comp] != arrayComp)
  {
    this->TCoordArrayComponents[comp] = arrayComp;
    this->Modified();
  }
  if (this->TCoordComponentRange[comp][0] != min)
  {
    this->TCoordComponentRange[comp][0] = min;
    this->Modified();
  }
  if (this->TCoordComponentRange[comp][1] != max)
  {
    this->TCoordComponentRange[comp][1] = max;
    this->Modified();
  }
  if (this->TCoordNormalize[comp] != normalize)
  {
    this->TCoordNormalize[comp] = normalize;
    this->Modified();
  }
}

const char* svtkFieldDataToAttributeDataFilter::GetTCoordComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordArrays[comp];
}

int svtkFieldDataToAttributeDataFilter::GetTCoordComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordArrayComponents[comp];
}

int svtkFieldDataToAttributeDataFilter::GetTCoordComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordComponentRange[comp][0];
}

int svtkFieldDataToAttributeDataFilter::GetTCoordComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordComponentRange[comp][1];
}

int svtkFieldDataToAttributeDataFilter::GetTCoordComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 2 ? 2 : comp));
  return this->TCoordNormalize[comp];
}

void svtkFieldDataToAttributeDataFilter::ConstructTCoords(int num, svtkFieldData* fd,
  svtkDataSetAttributes* attr, svtkIdType componentRange[3][2], char* arrays[3], int arrayComp[3],
  int normalize[3], int numComp)
{
  int i, normalizeAny, updated = 0;
  svtkDataArray* fieldArray[3];

  if (numComp < 1)
  {
    return;
  }
  for (i = 0; i < numComp; i++)
  {
    if (arrays[i] == nullptr)
    {
      return;
    }
  }

  for (normalizeAny = i = 0; i < numComp; i++)
  {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if (fieldArray[i] == nullptr)
    {
      svtkErrorMacro(<< "Can't find array/component requested");
      return;
    }
    normalizeAny |= normalize[i];
  }

  for (i = 0; i < numComp; i++)
  {
    updated |= this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if (num != (componentRange[i][1] - componentRange[i][0] + 1))
    {
      svtkErrorMacro(<< "Number of texture coords not consistent");
      return;
    }
  }

  svtkDataArray* newTCoords;
  for (i = 1; i < numComp; i++) // see whether all the data is from the same array
  {
    if (fieldArray[i] != fieldArray[i - 1])
    {
      break;
    }
  }

  // see whether we can reuse the data array from the field
  if (i >= numComp && fieldArray[0]->GetNumberOfComponents() == numComp &&
    fieldArray[0]->GetNumberOfTuples() == num && !normalizeAny)
  {
    newTCoords = fieldArray[0];
    newTCoords->Register(nullptr);
  }
  else // have to copy data into created array
  {
    newTCoords = svtkDataArray::CreateDataArray(this->GetComponentsType(numComp, fieldArray));
    newTCoords->SetNumberOfComponents(numComp);
    newTCoords->SetNumberOfTuples(num);

    for (i = 0; i < numComp; i++)
    {
      if (this->ConstructArray(newTCoords, i, fieldArray[i], arrayComp[i], componentRange[i][0],
            componentRange[i][1], normalize[i]) == 0)
      {
        newTCoords->Delete();
        return;
      }
    }
  }

  attr->SetTCoords(newTCoords);
  newTCoords->Delete();
  if (updated) // reset for next execution pass
  {
    for (i = 0; i < numComp; i++)
    {
      componentRange[i][0] = componentRange[i][1] = -1;
    }
  }
}

// Stuff related to tensors --------------------------------------------
//
void svtkFieldDataToAttributeDataFilter::SetTensorComponent(
  int comp, const char* arrayName, int arrayComp, int min, int max, int normalize)
{
  if (comp < 0 || comp > 8)
  {
    svtkErrorMacro(<< "Tensor component must be between (0,8)");
    return;
  }

  this->SetArrayName(this, this->TensorArrays[comp], arrayName);
  if (this->TensorArrayComponents[comp] != arrayComp)
  {
    this->TensorArrayComponents[comp] = arrayComp;
    this->Modified();
  }
  if (this->TensorComponentRange[comp][0] != min)
  {
    this->TensorComponentRange[comp][0] = min;
    this->Modified();
  }
  if (this->TensorComponentRange[comp][1] != max)
  {
    this->TensorComponentRange[comp][1] = max;
    this->Modified();
  }
  if (this->TensorNormalize[comp] != normalize)
  {
    this->TensorNormalize[comp] = normalize;
    this->Modified();
  }
}

const char* svtkFieldDataToAttributeDataFilter::GetTensorComponentArrayName(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorArrays[comp];
}

int svtkFieldDataToAttributeDataFilter::GetTensorComponentArrayComponent(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorArrayComponents[comp];
}

int svtkFieldDataToAttributeDataFilter::GetTensorComponentMinRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorComponentRange[comp][0];
}

int svtkFieldDataToAttributeDataFilter::GetTensorComponentMaxRange(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorComponentRange[comp][1];
}

int svtkFieldDataToAttributeDataFilter::GetTensorComponentNormalizeFlag(int comp)
{
  comp = (comp < 0 ? 0 : (comp > 8 ? 8 : comp));
  return this->TensorNormalize[comp];
}

void svtkFieldDataToAttributeDataFilter::ConstructTensors(int num, svtkFieldData* fd,
  svtkDataSetAttributes* attr, svtkIdType componentRange[9][2], char* arrays[9], int arrayComp[9],
  int normalize[9])
{
  int i, normalizeAny, updated = 0;
  int numComp = 9;
  svtkDataArray* fieldArray[9];

  // Check for symmetric tensor input
  if (arrayComp[6] == -1 || arrays[6] == nullptr)
  {
    numComp = 6;
  }

  for (i = 0; i < numComp; i++)
  {
    if (arrays[i] == nullptr)
    {
      return;
    }
  }

  for (i = 0; i < numComp; i++)
  {
    fieldArray[i] = this->GetFieldArray(fd, arrays[i], arrayComp[i]);

    if (fieldArray[i] == nullptr)
    {
      svtkErrorMacro(<< "Can't find array requested");
      return;
    }
  }

  for (normalizeAny = i = 0; i < numComp; i++)
  {
    updated |= this->UpdateComponentRange(fieldArray[i], componentRange[i]);
    if (num != (componentRange[i][1] - componentRange[i][0] + 1))
    {
      svtkErrorMacro(<< "Number of tensors not consistent");
      return;
    }
    normalizeAny |= normalize[i];
  }

  svtkDataArray* newTensors;
  for (i = 1; i < numComp; i++) // see whether all the data is from the same array
  {
    if (fieldArray[i] != fieldArray[i - 1])
    {
      break;
    }
  }

  // see whether we can reuse the data array from the field
  if (i >= numComp && fieldArray[0]->GetNumberOfComponents() == numComp &&
    fieldArray[0]->GetNumberOfTuples() == num && !normalizeAny)
  {
    newTensors = fieldArray[0];
    newTensors->Register(nullptr);
  }
  else // have to copy data into created array
  {
    newTensors = svtkDataArray::CreateDataArray(this->GetComponentsType(numComp, fieldArray));
    newTensors->SetNumberOfComponents(numComp);
    newTensors->SetNumberOfTuples(num);

    for (i = 0; i < numComp; i++)
    {
      if (this->ConstructArray(newTensors, i, fieldArray[i], arrayComp[i], componentRange[i][0],
            componentRange[i][1], normalize[i]) == 0)
      {
        newTensors->Delete();
        return;
      }
    }
  }

  attr->SetTensors(newTensors);
  newTensors->Delete();
  if (updated) // reset for next execution pass
  {
    for (i = 0; i < numComp; i++)
    {
      componentRange[i][0] = componentRange[i][1] = -1;
    }
  }
}

// Stuff related to fields --------------------------------------------
//
void svtkFieldDataToAttributeDataFilter::ConstructFieldData(
  int svtkNotUsed(num), svtkDataSetAttributes* svtkNotUsed(attr))
{
}

// Stuff related to helper methods ---------------------------------------
//
int svtkFieldDataToAttributeDataFilter::ConstructArray(svtkDataArray* da, int comp,
  svtkDataArray* fieldArray, int fieldComp, svtkIdType min, svtkIdType max, int normalize)
{
  svtkIdType i, n = max - min + 1;
  float minValue = SVTK_FLOAT_MAX;
  float maxValue = -SVTK_FLOAT_MAX;
  float compRange, compValue;

  if (fieldComp >= fieldArray->GetNumberOfComponents())
  {
    svtkGenericWarningMacro(<< "Trying to access component out of range");
    return 0;
  }

  for (i = 0; i < n; i++)
  {
    compValue = fieldArray->GetComponent(min + i, fieldComp);
    if (compValue < minValue)
    {
      minValue = compValue;
    }
    if (compValue > maxValue)
    {
      maxValue = compValue;
    }
    da->SetComponent(i, comp, compValue);
  }

  if (normalize)
  {
    compRange = maxValue - minValue;
    if (compRange != 0.0)
    {
      for (i = 0; i < n; i++)
      {
        compValue = da->GetComponent(i, comp);
        compValue = (compValue - minValue) / compRange;
        da->SetComponent(i, comp, compValue);
      }
    }
  }

  return 1;
}

int svtkFieldDataToAttributeDataFilter::GetComponentsType(int numComp, svtkDataArray** arrays)
{
  int type, mostComplexType = SVTK_VOID;

  for (int i = 0; i < numComp; i++)
  {
    type = arrays[i]->GetDataType();
    if (type > mostComplexType)
    {
      mostComplexType = type;
    }
  }

  return mostComplexType;
}

svtkDataArray* svtkFieldDataToAttributeDataFilter::GetFieldArray(
  svtkFieldData* fd, const char* name, int comp)
{
  svtkDataArray* da = nullptr;
  int numComp;
  bool found = false;

  if (name != nullptr)
  {
    svtkDataSetAttributes* dsa;
    if ((dsa = svtkDataSetAttributes::SafeDownCast(fd)))
    {
      found = true;
      if (!strcmp("PointScalars", name) || !strcmp("CellScalars", name))
      {
        da = dsa->GetScalars();
      }
      else if (!strcmp("PointVectors", name) || !strcmp("CellVectors", name))
      {
        da = dsa->GetVectors();
      }
      else if (!strcmp("PointTensors", name) || !strcmp("CellTensors", name))
      {
        da = dsa->GetTensors();
      }
      else if (!strcmp("PointNormals", name) || !strcmp("CellNormals", name))
      {
        da = dsa->GetNormals();
      }
      else if (!strcmp("PointTCoords", name) || !strcmp("CellTCoords", name))
      {
        da = dsa->GetTCoords();
      }
      else
      {
        found = false;
      }
    }
    if (!found || !da)
    {
      da = fd->GetArray(name);
    }

    if (da == nullptr)
    {
      return nullptr;
    }
    numComp = da->GetNumberOfComponents();
    if (comp < 0 || comp >= numComp)
    {
      return nullptr;
    }
    else
    {
      return da;
    }
  }

  return nullptr;
}

void svtkFieldDataToAttributeDataFilter::SetArrayName(
  svtkObject* self, char*& name, const char* newName)
{
  if (name && newName && (!strcmp(name, newName)))
  {
    return;
  }
  delete[] name;
  if (newName)
  {
    name = new char[strlen(newName) + 1];
    strcpy(name, newName);
  }
  else
  {
    name = nullptr;
  }
  self->Modified();
}

int svtkFieldDataToAttributeDataFilter::UpdateComponentRange(
  svtkDataArray* da, svtkIdType compRange[2])
{
  if (compRange[0] == -1)
  {
    compRange[0] = 0;
    compRange[1] = da->GetNumberOfTuples() - 1;
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
int svtkFieldDataToAttributeDataFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT()));
  return 1;
}
