/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRandomAttributeGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRandomAttributeGenerator.h"

#include "svtkBitArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkLongLongArray.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkShortArray.h"
#include "svtkSmartPointer.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedLongLongArray.h"
#include "svtkUnsignedShortArray.h"

svtkStandardNewMacro(svtkRandomAttributeGenerator);

// ----------------------------------------------------------------------------
svtkRandomAttributeGenerator::svtkRandomAttributeGenerator()
{
  this->DataType = SVTK_FLOAT;
  this->NumberOfComponents = 1;
  this->NumberOfTuples = 0;
  this->MinimumComponentValue = 0.0;
  this->MaximumComponentValue = 1.0;

  this->GeneratePointScalars = 0;
  this->GeneratePointVectors = 0;
  this->GeneratePointNormals = 0;
  this->GeneratePointTCoords = 0;
  this->GeneratePointTensors = 0;
  this->GeneratePointArray = 0;

  this->GenerateCellScalars = 0;
  this->GenerateCellVectors = 0;
  this->GenerateCellNormals = 0;
  this->GenerateCellTCoords = 0;
  this->GenerateCellTensors = 0;
  this->GenerateCellArray = 0;

  this->GenerateFieldArray = 0;
  this->AttributesConstantPerBlock = false;
}

// ----------------------------------------------------------------------------
template <class T>
void GenerateRandomTuple(
  T* data, svtkIdType i, int numComp, int minComp, int maxComp, double min, double max)
{
  for (int comp = minComp; comp <= maxComp; comp++)
  {
    // Now generate a random component value
    data[i * numComp + comp] = static_cast<T>(svtkMath::Random(min, max));
  }
}

// ----------------------------------------------------------------------------
void GenerateRandomTupleBit(svtkDataArray* data, svtkIdType i, int minComp, int maxComp)
{
  for (int comp = minComp; comp <= maxComp; comp++)
  {
    // Now generate a random component value
    data->SetComponent(i, comp, svtkMath::Random(0.0, 1.0) < 0.5 ? 0 : 1);
  }
}

// ----------------------------------------------------------------------------
template <class T>
void CopyTupleFrom0(T* data, svtkIdType i, int numComp, int minComp, int maxComp)
{
  memcpy(data + i * numComp + minComp, data + minComp, (maxComp - minComp + 1) * sizeof(T));
}
// ----------------------------------------------------------------------------
void CopyTupleFrom0Bit(svtkDataArray* data, svtkIdType i, int minComp, int maxComp)
{
  for (int comp = minComp; comp <= maxComp; comp++)
  {
    data->SetComponent(i, comp, data->GetComponent(0, comp));
  }
}

// ----------------------------------------------------------------------------
// This function template creates random attributes within a given range. It is
// assumed that the input data array may have a variable number of components.
template <class T>
void svtkRandomAttributeGenerator::GenerateRandomTuples(
  T* data, svtkIdType numTuples, int numComp, int minComp, int maxComp, double min, double max)
{
  if (numTuples == 0)
    return;
  svtkIdType total = numComp * numTuples;
  svtkIdType tenth = total / 10 + 1;
  GenerateRandomTuple(data, 0, numComp, minComp, maxComp, min, max);
  for (svtkIdType i = 1; i < numTuples; i++)
  {
    // update progress and check for aborts
    if (!(i % tenth))
    {
      this->UpdateProgress(static_cast<double>(i) / total);
      if (this->GetAbortExecute())
      {
        break;
      }
    }
    if (this->AttributesConstantPerBlock)
    {
      CopyTupleFrom0(data, i, numComp, minComp, maxComp);
    }
    else
    {
      GenerateRandomTuple(data, i, numComp, minComp, maxComp, min, max);
    }
  }
}

// ----------------------------------------------------------------------------
// This method does the data type allocation and switching for various types.
svtkDataArray* svtkRandomAttributeGenerator::GenerateData(
  int dataType, svtkIdType numTuples, int numComp, int minComp, int maxComp, double min, double max)
{
  svtkDataArray* dataArray = nullptr;

  switch (dataType)
  {
    case SVTK_CHAR:
    {
      dataArray = svtkCharArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      char* data = static_cast<svtkCharArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_UNSIGNED_CHAR:
    {
      dataArray = svtkUnsignedCharArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned char* data = static_cast<svtkUnsignedCharArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_SHORT:
    {
      dataArray = svtkShortArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      short* data = static_cast<svtkShortArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_UNSIGNED_SHORT:
    {
      dataArray = svtkUnsignedShortArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned short* data = static_cast<svtkUnsignedShortArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_INT:
    {
      dataArray = svtkIntArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      int* data = static_cast<svtkIntArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_UNSIGNED_INT:
    {
      dataArray = svtkUnsignedIntArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned int* data = static_cast<svtkUnsignedIntArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_LONG:
    {
      dataArray = svtkLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      long* data = static_cast<svtkLongArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_UNSIGNED_LONG:
    {
      dataArray = svtkUnsignedLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned long* data = static_cast<svtkUnsignedLongArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_LONG_LONG:
    {
      dataArray = svtkLongLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      long long* data = static_cast<svtkLongLongArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_UNSIGNED_LONG_LONG:
    {
      dataArray = svtkUnsignedLongLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned long long* data = static_cast<svtkUnsignedLongLongArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_FLOAT:
    {
      dataArray = svtkFloatArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      float* data = static_cast<svtkFloatArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_DOUBLE:
    {
      dataArray = svtkDoubleArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      double* data = static_cast<svtkDoubleArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_ID_TYPE:
    {
      dataArray = svtkIdTypeArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      svtkIdType* data = static_cast<svtkIdTypeArray*>(dataArray)->GetPointer(0);
      this->GenerateRandomTuples(data, numTuples, numComp, minComp, maxComp, min, max);
    }
    break;
    case SVTK_BIT: // we'll do something special for bit arrays
    {
      svtkIdType total = numComp * numTuples;
      svtkIdType tenth = total / 10 + 1;
      dataArray = svtkBitArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      if (numTuples == 0)
        break;
      GenerateRandomTupleBit(dataArray, 0, minComp, maxComp);
      for (svtkIdType i = 1; i < numTuples; i++)
      {
        // update progress and check for aborts
        if (!(i % tenth))
        {
          this->UpdateProgress(static_cast<double>(i) / total);
          if (this->GetAbortExecute())
          {
            break;
          }
        }
        if (this->AttributesConstantPerBlock)
        {
          CopyTupleFrom0Bit(dataArray, i, minComp, maxComp);
        }
        else
        {
          GenerateRandomTupleBit(dataArray, i, minComp, maxComp);
        }
      }
    }
    break;

    default:
      svtkGenericWarningMacro("Cannot create random data array\n");
  }

  return dataArray;
}

// ----------------------------------------------------------------------------
int svtkRandomAttributeGenerator::RequestData(
  svtkCompositeDataSet* input, svtkCompositeDataSet* output)
{
  if (input == nullptr || output == nullptr)
  {
    return 0;
  }
  output->CopyStructure(input);

  svtkSmartPointer<svtkCompositeDataIterator> it;
  it.TakeReference(input->NewIterator());
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    svtkDataSet* inputDS = svtkDataSet::SafeDownCast(it->GetCurrentDataObject());
    svtkSmartPointer<svtkDataSet> outputDS;
    outputDS.TakeReference(inputDS->NewInstance());
    output->SetDataSet(it, outputDS);
    RequestData(inputDS, outputDS);
  }
  return 1;
}

// ----------------------------------------------------------------------------
int svtkRandomAttributeGenerator::RequestData(svtkDataSet* input, svtkDataSet* output)
{
  svtkDebugMacro(<< "Producing random attributes");
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();

  if (numPts < 1)
  {
    svtkDebugMacro(<< "No input!");
    return 1;
  }

  // Configure the output
  output->CopyStructure(input);
  output->CopyAttributes(input);

  // Produce the appropriate output
  // First the point data
  if (this->GeneratePointScalars)
  {
    svtkDataArray* ptScalars = this->GenerateData(this->DataType, numPts, this->NumberOfComponents,
      0, this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    ptScalars->SetName("RandomPointScalars");
    output->GetPointData()->SetScalars(ptScalars);
    ptScalars->Delete();
  }
  if (this->GeneratePointVectors)
  {
    svtkDataArray* ptVectors = this->GenerateData(
      this->DataType, numPts, 3, 0, 2, this->MinimumComponentValue, this->MaximumComponentValue);
    ptVectors->SetName("RandomPointVectors");
    output->GetPointData()->SetVectors(ptVectors);
    ptVectors->Delete();
  }
  if (this->GeneratePointNormals)
  {
    svtkDataArray* ptNormals = this->GenerateData(
      this->DataType, numPts, 3, 0, 2, this->MinimumComponentValue, this->MaximumComponentValue);
    double v[3];
    for (svtkIdType id = 0; id < numPts; id++)
    {
      ptNormals->GetTuple(id, v);
      svtkMath::Normalize(v);
      ptNormals->SetTuple(id, v);
    }
    output->GetPointData()->SetNormals(ptNormals);
    ptNormals->Delete();
  }
  if (this->GeneratePointTensors)
  {
    // fill in 6 components, and then shift them around to make them symmetric
    svtkDataArray* ptTensors = this->GenerateData(
      this->DataType, numPts, 9, 0, 5, this->MinimumComponentValue, this->MaximumComponentValue);
    ptTensors->SetName("RandomPointTensors");
    double t[9];
    for (svtkIdType id = 0; id < numPts; id++)
    {
      ptTensors->GetTuple(id, t);
      t[8] = t[3]; // make sure the tensor is symmetric
      t[3] = t[1];
      t[6] = t[2];
      t[7] = t[5];
      ptTensors->SetTuple(id, t);
    }
    output->GetPointData()->SetTensors(ptTensors);
    ptTensors->Delete();
  }
  if (this->GeneratePointTCoords)
  {
    int numComp = this->NumberOfComponents < 1
      ? 1
      : (this->NumberOfComponents > 3 ? 3 : this->NumberOfComponents);
    svtkDataArray* ptTCoords = this->GenerateData(this->DataType, numPts, numComp, 0,
      this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    output->GetPointData()->SetTCoords(ptTCoords);
    ptTCoords->Delete();
  }
  if (this->GeneratePointArray)
  {
    svtkDataArray* ptData = this->GenerateData(this->DataType, numPts, this->NumberOfComponents, 0,
      this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    ptData->SetName("RandomPointArray");
    output->GetPointData()->AddArray(ptData);
    ptData->Delete();
  }

  if (numCells < 1)
  {
    svtkDebugMacro(<< "No input!");
    return 1;
  }

  // Now the cell data
  if (this->GenerateCellScalars)
  {
    svtkDataArray* cellScalars =
      this->GenerateData(this->DataType, numCells, this->NumberOfComponents, 0,
        this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    cellScalars->SetName("RandomCellScalars");
    output->GetCellData()->SetScalars(cellScalars);
    cellScalars->Delete();
  }
  if (this->GenerateCellVectors)
  {
    svtkDataArray* cellVectors = this->GenerateData(
      this->DataType, numCells, 3, 0, 2, this->MinimumComponentValue, this->MaximumComponentValue);
    cellVectors->SetName("RandomCellVectors");
    output->GetCellData()->SetVectors(cellVectors);
    cellVectors->Delete();
  }
  if (this->GenerateCellNormals)
  {
    svtkDataArray* cellNormals = this->GenerateData(
      this->DataType, numCells, 3, 0, 2, this->MinimumComponentValue, this->MaximumComponentValue);
    double v[3];
    for (svtkIdType id = 0; id < numCells; id++)
    {
      cellNormals->GetTuple(id, v);
      svtkMath::Normalize(v);
      cellNormals->SetTuple(id, v);
    }
    output->GetCellData()->SetNormals(cellNormals);
    cellNormals->Delete();
  }
  if (this->GenerateCellTensors)
  {
    svtkDataArray* cellTensors = this->GenerateData(
      this->DataType, numCells, 9, 0, 5, this->MinimumComponentValue, this->MaximumComponentValue);
    cellTensors->SetName("RandomCellTensors");
    double t[9];
    for (svtkIdType id = 0; id < numCells; id++)
    {
      cellTensors->GetTuple(id, t);
      t[6] = t[1]; // make sure the tensor is symmetric
      t[7] = t[2];
      t[8] = t[4];
      cellTensors->SetTuple(id, t);
    }
    output->GetCellData()->SetTensors(cellTensors);
    cellTensors->Delete();
  }
  if (this->GenerateCellTCoords)
  {
    int numComp = this->NumberOfComponents < 1
      ? 1
      : (this->NumberOfComponents > 3 ? 3 : this->NumberOfComponents);
    svtkDataArray* cellTCoords = this->GenerateData(this->DataType, numCells, numComp, 0,
      this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    output->GetCellData()->SetTCoords(cellTCoords);
    cellTCoords->Delete();
  }
  if (this->GenerateCellArray)
  {
    svtkDataArray* cellArray = this->GenerateData(this->DataType, numCells, this->NumberOfComponents,
      0, this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    cellArray->SetName("RandomCellArray");
    output->GetCellData()->AddArray(cellArray);
    cellArray->Delete();
  }

  // Finally any field data
  if (this->GenerateFieldArray)
  {
    svtkDataArray* data =
      this->GenerateData(this->DataType, this->NumberOfTuples, this->NumberOfComponents, 0,
        this->NumberOfComponents - 1, this->MinimumComponentValue, this->MaximumComponentValue);
    data->SetName("RandomFieldArray");
    output->GetFieldData()->AddArray(data);
    data->Delete();
  }
  return 1;
}

// ----------------------------------------------------------------------------
int svtkRandomAttributeGenerator::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  if (input->IsA("svtkDataSet"))
  {
    return this->RequestData(svtkDataSet::SafeDownCast(input), svtkDataSet::SafeDownCast(output));
  }
  else
  {
    return this->RequestData(
      svtkCompositeDataSet::SafeDownCast(input), svtkCompositeDataSet::SafeDownCast(output));
  }
}

// ----------------------------------------------------------------------------
void svtkRandomAttributeGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Data Type: " << this->DataType << endl;
  os << indent << "Number of Components: " << this->NumberOfComponents << endl;
  os << indent << "Number of Tuples: " << this->NumberOfTuples << endl;
  os << indent << "Minimum Component Value: " << this->MinimumComponentValue << endl;
  os << indent << "Maximum Component Value: " << this->MaximumComponentValue << endl;

  os << indent << "Generate Point Scalars: " << (this->GeneratePointScalars ? "On\n" : "Off\n");
  os << indent << "Generate Point Vectors: " << (this->GeneratePointVectors ? "On\n" : "Off\n");
  os << indent << "Generate Point Normals: " << (this->GeneratePointNormals ? "On\n" : "Off\n");
  os << indent << "Generate Point TCoords: " << (this->GeneratePointTCoords ? "On\n" : "Off\n");
  os << indent << "Generate Point Tensors: " << (this->GeneratePointTensors ? "On\n" : "Off\n");
  os << indent << "Generate Point Array: " << (this->GeneratePointArray ? "On\n" : "Off\n");

  os << indent << "Generate Cell Scalars: " << (this->GenerateCellScalars ? "On\n" : "Off\n");
  os << indent << "Generate Cell Vectors: " << (this->GenerateCellVectors ? "On\n" : "Off\n");
  os << indent << "Generate Cell Normals: " << (this->GenerateCellNormals ? "On\n" : "Off\n");
  os << indent << "Generate Cell TCoords: " << (this->GenerateCellTCoords ? "On\n" : "Off\n");
  os << indent << "Generate Cell Tensors: " << (this->GenerateCellTensors ? "On\n" : "Off\n");
  os << indent << "Generate Cell Array: " << (this->GenerateCellArray ? "On\n" : "Off\n");

  os << indent << "Generate Field Array: " << (this->GenerateFieldArray ? "On\n" : "Off\n");
}

int svtkRandomAttributeGenerator::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}
