/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayCalculator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkArrayCalculator.h"

#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkFunctionParser.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPolyData.h"
#include "svtkTable.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkArrayCalculator);

svtkArrayCalculator::svtkArrayCalculator()
{
  this->FunctionParser = svtkFunctionParser::New();
  this->Function = nullptr;
  this->ResultArrayName = nullptr;
  this->SetResultArrayName("resultArray");
  this->ScalarArrayNames = nullptr;
  this->VectorArrayNames = nullptr;
  this->ScalarVariableNames = nullptr;
  this->VectorVariableNames = nullptr;
  this->NumberOfScalarArrays = 0;
  this->NumberOfVectorArrays = 0;
  this->AttributeType = DEFAULT_ATTRIBUTE_TYPE;
  this->SelectedScalarComponents = nullptr;
  this->SelectedVectorComponents = nullptr;
  this->CoordinateScalarVariableNames = nullptr;
  this->CoordinateVectorVariableNames = nullptr;
  this->NumberOfCoordinateScalarArrays = 0;
  this->NumberOfCoordinateVectorArrays = 0;
  this->SelectedCoordinateScalarComponents = nullptr;
  this->SelectedCoordinateVectorComponents = nullptr;
  this->CoordinateResults = 0;
  this->ResultNormals = false;
  this->ResultTCoords = false;
  this->ReplaceInvalidValues = 0;
  this->ReplacementValue = 0.0;
  this->IgnoreMissingArrays = false;
  this->ResultArrayType = SVTK_DOUBLE;
}

svtkArrayCalculator::~svtkArrayCalculator()
{
  int i;

  this->FunctionParser->Delete();
  this->FunctionParser = nullptr;

  delete[] this->Function;
  this->Function = nullptr;

  delete[] this->ResultArrayName;
  this->ResultArrayName = nullptr;

  if (this->ScalarArrayNames)
  {
    for (i = 0; i < this->NumberOfScalarArrays; i++)
    {
      delete[] this->ScalarArrayNames[i];
      this->ScalarArrayNames[i] = nullptr;
    }
    delete[] this->ScalarArrayNames;
    this->ScalarArrayNames = nullptr;
  }

  if (this->VectorArrayNames)
  {
    for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
      delete[] this->VectorArrayNames[i];
      this->VectorArrayNames[i] = nullptr;
    }
    delete[] this->VectorArrayNames;
    this->VectorArrayNames = nullptr;
  }

  if (this->ScalarVariableNames)
  {
    for (i = 0; i < this->NumberOfScalarArrays; i++)
    {
      delete[] this->ScalarVariableNames[i];
      this->ScalarVariableNames[i] = nullptr;
    }
    delete[] this->ScalarVariableNames;
    this->ScalarVariableNames = nullptr;
  }

  if (this->VectorVariableNames)
  {
    for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
      delete[] this->VectorVariableNames[i];
      this->VectorVariableNames[i] = nullptr;
    }
    delete[] this->VectorVariableNames;
    this->VectorVariableNames = nullptr;
  }

  delete[] this->SelectedScalarComponents;
  this->SelectedScalarComponents = nullptr;

  if (this->SelectedVectorComponents)
  {
    for (i = 0; i < this->NumberOfVectorArrays; i++)
    {
      delete[] this->SelectedVectorComponents[i];
      this->SelectedVectorComponents[i] = nullptr;
    }
    delete[] this->SelectedVectorComponents;
    this->SelectedVectorComponents = nullptr;
  }

  if (this->CoordinateScalarVariableNames)
  {
    for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
    {
      delete[] this->CoordinateScalarVariableNames[i];
      this->CoordinateScalarVariableNames[i] = nullptr;
    }
    delete[] this->CoordinateScalarVariableNames;
    this->CoordinateScalarVariableNames = nullptr;
  }

  if (this->CoordinateVectorVariableNames)
  {
    for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
    {
      delete[] this->CoordinateVectorVariableNames[i];
      this->CoordinateVectorVariableNames[i] = nullptr;
    }
    delete[] this->CoordinateVectorVariableNames;
    this->CoordinateVectorVariableNames = nullptr;
  }

  delete[] this->SelectedCoordinateScalarComponents;
  this->SelectedCoordinateScalarComponents = nullptr;

  if (this->SelectedCoordinateVectorComponents)
  {
    for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
    {
      delete[] this->SelectedCoordinateVectorComponents[i];
      this->SelectedCoordinateVectorComponents[i] = nullptr;
    }
    delete[] this->SelectedCoordinateVectorComponents;
    this->SelectedCoordinateVectorComponents = nullptr;
  }
}

int svtkArrayCalculator::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

void svtkArrayCalculator::SetResultArrayName(const char* name)
{
  if (name == nullptr || *name == '\0')
  {
    svtkErrorMacro("The result array must have a name.");
    return;
  }
  if (this->ResultArrayName != nullptr && strcmp(this->ResultArrayName, name) == 0)
  {
    return;
  }
  this->Modified();

  delete[] this->ResultArrayName;
  this->ResultArrayName = new char[strlen(name) + 1];
  strcpy(this->ResultArrayName, name);
}

enum ResultType
{
  SCALAR_RESULT,
  VECTOR_RESULT
} resultType = SCALAR_RESULT;

int svtkArrayCalculator::ProcessDataObject(svtkDataObject* input, svtkDataObject* output)
{
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
  svtkGraph* graphInput = svtkGraph::SafeDownCast(input);
  svtkPointSet* psOutput = svtkPointSet::SafeDownCast(output);

  int attributeType = this->GetAttributeTypeFromInput(input);

  svtkDataSetAttributes* inFD = input->GetAttributes(attributeType);
  svtkDataSetAttributes* outFD = output->GetAttributes(attributeType);
  svtkIdType numTuples = input->GetNumberOfElements(attributeType);

  if (numTuples < 1)
  {
    svtkDebugMacro("Empty data.");
    return 1;
  }

  svtkDataArray* currentArray = nullptr;

  // Tell the parser about scalar arrays
  for (int i = 0; i < this->NumberOfScalarArrays; i++)
  {
    currentArray = inFD->GetArray(this->ScalarArrayNames[i]);
    if (currentArray)
    {
      if (currentArray->GetNumberOfComponents() > this->SelectedScalarComponents[i])
      {
        this->FunctionParser->SetScalarVariableValue(this->ScalarVariableNames[i],
          currentArray->GetComponent(0, this->SelectedScalarComponents[i]));
      }
      else
      {
        svtkErrorMacro(
          "Array " << this->ScalarArrayNames[i] << " does not contain the selected component.");
        return 1;
      }
    }
    else if (this->IgnoreMissingArrays)
    {
      // Add a dummy value with the variable name. We'll skip it if the variable is
      // actually needed when collecting the arrays needed for evaluation later on.
      this->FunctionParser->SetScalarVariableValue(this->ScalarVariableNames[i], 0.0);
    }
    else if (inFD->GetAbstractArray(this->ScalarArrayNames[i]) == nullptr) // We ignore string array
    {
      svtkErrorMacro("Invalid array name: " << this->ScalarArrayNames[i]);
      return 1;
    }
  }

  // Tell the parser about vector arrays
  for (int i = 0; i < this->NumberOfVectorArrays; i++)
  {
    currentArray = inFD->GetArray(this->VectorArrayNames[i]);
    if (currentArray)
    {
      if ((currentArray->GetNumberOfComponents() > this->SelectedVectorComponents[i][0]) &&
        (currentArray->GetNumberOfComponents() > this->SelectedVectorComponents[i][1]) &&
        (currentArray->GetNumberOfComponents() > this->SelectedVectorComponents[i][2]))
      {
        this->FunctionParser->SetVectorVariableValue(this->VectorVariableNames[i],
          currentArray->GetComponent(0, this->SelectedVectorComponents[i][0]),
          currentArray->GetComponent(0, this->SelectedVectorComponents[i][1]),
          currentArray->GetComponent(0, this->SelectedVectorComponents[i][2]));
      }
      else
      {
        svtkErrorMacro("Array " << this->VectorArrayNames[i]
                               << " does not contain one of the selected components.");
        return 1;
      }
    }
    else if (this->IgnoreMissingArrays)
    {
      // Add a dummy value with the variable name. We'll skip it if the variable is
      // actually needed when collecting the arrays needed for evaluation later on.
      this->FunctionParser->SetVectorVariableValue(this->VectorVariableNames[i], 0.0, 0.0, 0.0);
    }
    else if (inFD->GetAbstractArray(this->VectorArrayNames[i]) == nullptr) // We ignore string array
    {
      svtkErrorMacro("Invalid array name: " << this->VectorArrayNames[i]);
      return 1;
    }
  }

  // Tell the parser about the coordinate arrays
  if (attributeType == svtkDataObject::POINT || attributeType == svtkDataObject::VERTEX)
  {
    for (int i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
    {
      double* pt = nullptr;
      if (dsInput)
      {
        pt = dsInput->GetPoint(0);
      }
      else
      {
        pt = graphInput->GetPoint(0);
      }
      this->FunctionParser->SetScalarVariableValue(
        this->CoordinateScalarVariableNames[i], pt[this->SelectedCoordinateScalarComponents[i]]);
    }

    for (int i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
    {
      double* pt = nullptr;
      if (dsInput)
      {
        pt = dsInput->GetPoint(0);
      }
      else
      {
        pt = graphInput->GetPoint(0);
      }
      this->FunctionParser->SetVectorVariableValue(this->CoordinateVectorVariableNames[i],
        pt[this->SelectedCoordinateVectorComponents[i][0]],
        pt[this->SelectedCoordinateVectorComponents[i][1]],
        pt[this->SelectedCoordinateVectorComponents[i][2]]);
    }
  }

  if (!this->Function || strlen(this->Function) == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }
  else if (this->FunctionParser->IsScalarResult())
  {
    resultType = SCALAR_RESULT;
  }
  else if (this->FunctionParser->IsVectorResult())
  {
    resultType = VECTOR_RESULT;
  }
  else
  {
    output->ShallowCopy(input);
    // Error occurred in svtkFunctionParser.
    svtkWarningMacro(
      "An error occurred when parsing the calculator's function.  See previous errors.");
    return 1;
  }

  if (resultType == SCALAR_RESULT && this->ResultNormals)
  {
    svtkWarningMacro("ResultNormals specified but output is scalar");
  }

  svtkMolecule* moleculeInput = svtkMolecule::SafeDownCast(input);
  if (moleculeInput && attributeType == svtkDataObject::VERTEX &&
    strcmp(this->ResultArrayName, moleculeInput->GetAtomicNumberArrayName()) == 0)
  {
    svtkErrorMacro("Cannot override atomic numbers array");
    return 1;
  }

  if (moleculeInput && attributeType == svtkDataObject::EDGE &&
    strcmp(this->ResultArrayName, moleculeInput->GetBondOrdersArrayName()) == 0)
  {
    svtkErrorMacro("Cannot override bond orders array");
    return 1;
  }

  svtkSmartPointer<svtkPoints> resultPoints;
  svtkSmartPointer<svtkDataArray> resultArray;
  if (resultType == VECTOR_RESULT && CoordinateResults != 0 &&
    (psOutput || svtkGraph::SafeDownCast(output)))
  {
    resultPoints = svtkSmartPointer<svtkPoints>::New();
    resultPoints->SetNumberOfPoints(numTuples);
    resultArray = resultPoints->GetData();
  }
  else if (CoordinateResults != 0)
  {
    if (resultType != VECTOR_RESULT)
    {
      svtkErrorMacro("Coordinate output specified, "
                    "but there are no vector results");
    }
    else if (!psOutput)
    {
      svtkErrorMacro("Coordinate output specified, "
                    "but output is not polydata or unstructured grid");
    }
    return 1;
  }
  else
  {
    resultArray.TakeReference(
      svtkArrayDownCast<svtkDataArray>(svtkAbstractArray::CreateArray(this->ResultArrayType)));
  }

  if (resultType == SCALAR_RESULT)
  {
    resultArray->SetNumberOfComponents(1);
    resultArray->SetNumberOfTuples(numTuples);
    double scalarResult = this->FunctionParser->GetScalarResult();
    resultArray->SetTuple(0, &scalarResult);
  }
  else
  {
    resultArray->Allocate(numTuples * 3);
    resultArray->SetNumberOfComponents(3);
    resultArray->SetNumberOfTuples(numTuples);
    resultArray->SetTuple(0, this->FunctionParser->GetVectorResult());
  }

  // Save array pointers to avoid looking them up for each tuple.
  std::vector<svtkDataArray*> scalarArrays(this->NumberOfScalarArrays);
  std::vector<svtkDataArray*> vectorArrays(this->NumberOfVectorArrays);
  std::vector<int> scalarArrayIndicies(this->NumberOfScalarArrays);
  std::vector<int> vectorArrayIndicies(this->NumberOfVectorArrays);

  for (int cc = 0; cc < this->NumberOfScalarArrays; cc++)
  {
    int idx = this->FunctionParser->GetScalarVariableIndex(this->ScalarVariableNames[cc]);
    if (idx >= 0)
    {
      bool needed = this->FunctionParser->GetScalarVariableNeeded(idx);
      auto array = inFD->GetArray(this->ScalarArrayNames[cc]);
      if (needed && array)
      {
        scalarArrays[cc] = array;
        scalarArrayIndicies[cc] = idx;
      }
      else if (needed)
      {
        // Skip this dataset altogether. This is an array specifically requested to be available
        // as a variable by the user of this class that does not exist on this dataset.
        return 1;
      }
    }
  }

  for (int cc = 0; cc < this->NumberOfVectorArrays; cc++)
  {
    int idx = this->FunctionParser->GetVectorVariableIndex(this->VectorVariableNames[cc]);
    if (idx >= 0)
    {
      bool needed = this->FunctionParser->GetVectorVariableNeeded(idx);
      auto array = inFD->GetArray(this->VectorArrayNames[cc]);
      if (needed && array)
      {
        vectorArrays[cc] = array;
        vectorArrayIndicies[cc] = idx;
      }
      else if (needed)
      {
        // Skip this dataset altogether. This is an array specifically requested to be available
        // as a variable by the user of this class that does not exist on this dataset.
        return 1;
      }
    }
  }

  for (svtkIdType i = 1; i < numTuples; i++)
  {
    for (int j = 0; j < this->NumberOfScalarArrays; j++)
    {
      if ((currentArray = scalarArrays[j]))
      {
        this->FunctionParser->SetScalarVariableValue(
          scalarArrayIndicies[j], currentArray->GetComponent(i, this->SelectedScalarComponents[j]));
      }
    }
    for (int j = 0; j < this->NumberOfVectorArrays; j++)
    {
      if ((currentArray = vectorArrays[j]))
      {
        this->FunctionParser->SetVectorVariableValue(vectorArrayIndicies[j],
          currentArray->GetComponent(i, this->SelectedVectorComponents[j][0]),
          currentArray->GetComponent(i, this->SelectedVectorComponents[j][1]),
          currentArray->GetComponent(i, this->SelectedVectorComponents[j][2]));
      }
    }
    if (attributeType == svtkDataObject::POINT || attributeType == svtkDataObject::VERTEX)
    {
      double* pt = nullptr;
      if (dsInput)
      {
        pt = dsInput->GetPoint(i);
      }
      else
      {
        pt = graphInput->GetPoint(i);
      }
      for (int j = 0; j < this->NumberOfCoordinateScalarArrays; j++)
      {
        this->FunctionParser->SetScalarVariableValue(
          j + this->NumberOfScalarArrays, pt[this->SelectedCoordinateScalarComponents[j]]);
      }
      for (int j = 0; j < this->NumberOfCoordinateVectorArrays; j++)
      {
        this->FunctionParser->SetVectorVariableValue(j + this->NumberOfVectorArrays,
          pt[this->SelectedCoordinateVectorComponents[j][0]],
          pt[this->SelectedCoordinateVectorComponents[j][1]],
          pt[this->SelectedCoordinateVectorComponents[j][2]]);
      }
    }
    if (resultType == SCALAR_RESULT)
    {
      double scalarResult = this->FunctionParser->GetScalarResult();
      resultArray->SetTuple(i, &scalarResult);
    }
    else
    {
      resultArray->SetTuple(i, this->FunctionParser->GetVectorResult());
    }
  }

  output->ShallowCopy(input);
  if (resultPoints)
  {
    if (psOutput)
    {
      if (attributeType == svtkDataObject::CELL)
      {
        svtkPolyData* pd = svtkPolyData::SafeDownCast(psOutput);
        svtkUnstructuredGrid* ug = svtkUnstructuredGrid::SafeDownCast(psOutput);
        if (pd)
        {
          pd->Reset();
          pd->AllocateEstimate(numTuples, 1, 0, 0, 0, 0, 0, 0);
          for (svtkIdType i = 1; i < numTuples; i++)
          {
            pd->InsertNextCell(SVTK_VERTEX, 1, &i);
          }
        }
        else if (ug)
        {
          ug->Reset();
          ug->Allocate(numTuples);
          for (svtkIdType i = 1; i < numTuples; i++)
          {
            ug->InsertNextCell(SVTK_VERTEX, 1, &i);
          }
        }
      }
      psOutput->SetPoints(resultPoints);
    }
  }

  if (this->ResultTCoords || this->ResultNormals || !this->CoordinateResults)
  {
    resultArray->SetName(this->ResultArrayName);
    outFD->AddArray(resultArray);
    if (resultType == SCALAR_RESULT)
    {
      if (this->ResultTCoords)
      {
        outFD->SetActiveTCoords(this->ResultArrayName);
      }
      else
      {
        outFD->SetActiveScalars(this->ResultArrayName);
      }
    }
    else
    {
      if (this->ResultTCoords || this->ResultNormals)
      {
        if (this->ResultTCoords)
        {
          outFD->SetActiveTCoords(this->ResultArrayName);
        }
        if (this->ResultNormals)
        {
          outFD->SetActiveNormals(this->ResultArrayName);
        }
      }
      else
      {
        outFD->SetActiveVectors(this->ResultArrayName);
      }
    }
  }

  return 1;
}

int svtkArrayCalculator::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  this->FunctionParser->SetReplaceInvalidValues(this->ReplaceInvalidValues);
  this->FunctionParser->SetReplacementValue(this->ReplacementValue);

  svtkCompositeDataSet* inputCD = svtkCompositeDataSet::GetData(inputVector[0], 0);
  svtkCompositeDataSet* outputCD = svtkCompositeDataSet::SafeDownCast(output);
  if (inputCD && outputCD)
  {
    int success = 1;

    // Copy the output structure
    outputCD->CopyStructure(inputCD);

    svtkSmartPointer<svtkCompositeDataIterator> cdIter;
    cdIter.TakeReference(inputCD->NewIterator());
    cdIter->SkipEmptyNodesOn();
    for (cdIter->InitTraversal(); !cdIter->IsDoneWithTraversal(); cdIter->GoToNextItem())
    {
      svtkDataObject* inputDataObject = cdIter->GetCurrentDataObject();
      svtkDataObject* outputDataObject = inputDataObject->NewInstance();
      outputDataObject->DeepCopy(inputDataObject);
      outputCD->SetDataSet(cdIter, outputDataObject);
      outputDataObject->FastDelete();

      success *= this->ProcessDataObject(inputDataObject, outputDataObject);
    }

    return success;
  }

  // Not a composite data set.
  return this->ProcessDataObject(input, output);
}

int svtkArrayCalculator::GetAttributeTypeFromInput(svtkDataObject* input)
{
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
  svtkGraph* graphInput = svtkGraph::SafeDownCast(input);

  int attribute = this->AttributeType;
  if (attribute == DEFAULT_ATTRIBUTE_TYPE)
  {
    if (dsInput)
    {
      attribute = svtkDataObject::POINT;
    }
    else if (graphInput)
    {
      attribute = svtkDataObject::VERTEX;
    }
    else
    {
      attribute = svtkDataObject::ROW;
    }
  }

  return attribute;
}

#ifndef SVTK_LEGACY_REMOVE
void svtkArrayCalculator::SetAttributeMode(int mode)
{
  SVTK_LEGACY_REPLACED_BODY(
    svtkArrayCalculator::SetAttributeMode, "SVTK 8.1", svtkArrayCalculator::SetAttributeType);
  switch (mode)
  {
    default:
    case SVTK_ATTRIBUTE_MODE_DEFAULT:
      this->SetAttributeType(DEFAULT_ATTRIBUTE_TYPE);
      break;
    case SVTK_ATTRIBUTE_MODE_USE_POINT_DATA:
      this->SetAttributeType(svtkDataObject::POINT);
      break;
    case SVTK_ATTRIBUTE_MODE_USE_CELL_DATA:
      this->SetAttributeType(svtkDataObject::CELL);
      break;
    case SVTK_ATTRIBUTE_MODE_USE_VERTEX_DATA:
      this->SetAttributeType(svtkDataObject::VERTEX);
      break;
    case SVTK_ATTRIBUTE_MODE_USE_EDGE_DATA:
      this->SetAttributeType(svtkDataObject::EDGE);
      break;
  }
}

int svtkArrayCalculator::GetAttributeMode()
{
  SVTK_LEGACY_REPLACED_BODY(
    svtkArrayCalculator::GetAttributeMode, "SVTK 8.1", svtkArrayCalculator::GetAttributeType);
  switch (this->AttributeType)
  {
    default:
    case svtkDataObject::ROW: // old version didn't handle row data, just return default
    case DEFAULT_ATTRIBUTE_TYPE:
      return SVTK_ATTRIBUTE_MODE_DEFAULT;
    case svtkDataObject::POINT:
      return SVTK_ATTRIBUTE_MODE_USE_POINT_DATA;
    case svtkDataObject::CELL:
      return SVTK_ATTRIBUTE_MODE_USE_CELL_DATA;
    case svtkDataObject::VERTEX:
      return SVTK_ATTRIBUTE_MODE_USE_VERTEX_DATA;
    case svtkDataObject::EDGE:
      return SVTK_ATTRIBUTE_MODE_USE_EDGE_DATA;
  }
}
#endif

void svtkArrayCalculator::SetFunction(const char* function)
{
  if (this->Function && function && strcmp(this->Function, function) == 0)
  {
    return;
  }

  this->Modified();

  delete[] this->Function;
  this->Function = nullptr;

  if (function)
  {
    this->Function = new char[strlen(function) + 1];
    strcpy(this->Function, function);
    this->FunctionParser->SetFunction(this->Function);
  }
}

void svtkArrayCalculator::AddScalarArrayName(const char* arrayName, int component)
{
  if (!arrayName)
  {
    return;
  }

  int i;
  for (i = 0; i < this->NumberOfScalarArrays; i++)
  {
    if (strcmp(arrayName, this->ScalarVariableNames[i]) == 0 &&
      strcmp(arrayName, this->ScalarArrayNames[i]) == 0 &&
      this->SelectedScalarComponents[i] == component)
    {
      // Already have this variable/array/components so return.
      return;
    }
  }

  char** arrayNames = new char*[this->NumberOfScalarArrays];
  char** varNames = new char*[this->NumberOfScalarArrays];
  int* tempComponents = new int[this->NumberOfScalarArrays];

  for (i = 0; i < this->NumberOfScalarArrays; i++)
  {
    arrayNames[i] = new char[strlen(this->ScalarArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->ScalarArrayNames[i]);
    delete[] this->ScalarArrayNames[i];
    this->ScalarArrayNames[i] = nullptr;
    varNames[i] = new char[strlen(this->ScalarVariableNames[i]) + 1];
    strcpy(varNames[i], this->ScalarVariableNames[i]);
    delete[] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = nullptr;
    tempComponents[i] = this->SelectedScalarComponents[i];
  }
  delete[] this->ScalarArrayNames;
  this->ScalarArrayNames = nullptr;

  delete[] this->ScalarVariableNames;
  this->ScalarVariableNames = nullptr;

  delete[] this->SelectedScalarComponents;
  this->SelectedScalarComponents = nullptr;

  this->ScalarArrayNames = new char*[this->NumberOfScalarArrays + 1];
  this->ScalarVariableNames = new char*[this->NumberOfScalarArrays + 1];
  this->SelectedScalarComponents = new int[this->NumberOfScalarArrays + 1];

  for (i = 0; i < this->NumberOfScalarArrays; i++)
  {
    this->ScalarArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->ScalarArrayNames[i], arrayNames[i]);
    delete[] arrayNames[i];
    arrayNames[i] = nullptr;
    this->ScalarVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->ScalarVariableNames[i], varNames[i]);
    delete[] varNames[i];
    varNames[i] = nullptr;
    this->SelectedScalarComponents[i] = tempComponents[i];
  }
  delete[] arrayNames;
  delete[] varNames;
  delete[] tempComponents;

  this->ScalarArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->ScalarArrayNames[i], arrayName);
  this->ScalarVariableNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->ScalarVariableNames[i], arrayName);
  this->SelectedScalarComponents[i] = component;

  this->NumberOfScalarArrays++;
}

void svtkArrayCalculator::AddVectorArrayName(
  const char* arrayName, int component0, int component1, int component2)
{
  if (!arrayName)
  {
    return;
  }

  int i;
  for (i = 0; i < this->NumberOfVectorArrays; i++)
  {
    if (strcmp(arrayName, this->VectorVariableNames[i]) == 0 &&
      strcmp(arrayName, this->VectorArrayNames[i]) == 0 &&
      this->SelectedVectorComponents[i][0] == component0 &&
      this->SelectedVectorComponents[i][1] == component1 &&
      this->SelectedVectorComponents[i][2] == component2)
    {
      // Already have this variable/array/components so return.
      return;
    }
  }

  char** arrayNames = new char*[this->NumberOfVectorArrays];
  char** varNames = new char*[this->NumberOfVectorArrays];
  int** tempComponents = new int*[this->NumberOfVectorArrays];

  for (i = 0; i < this->NumberOfVectorArrays; i++)
  {
    arrayNames[i] = new char[strlen(this->VectorArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->VectorArrayNames[i]);
    delete[] this->VectorArrayNames[i];
    this->VectorArrayNames[i] = nullptr;
    varNames[i] = new char[strlen(this->VectorVariableNames[i]) + 1];
    strcpy(varNames[i], this->VectorVariableNames[i]);
    delete[] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = nullptr;
    tempComponents[i] = new int[3];
    tempComponents[i][0] = this->SelectedVectorComponents[i][0];
    tempComponents[i][1] = this->SelectedVectorComponents[i][1];
    tempComponents[i][2] = this->SelectedVectorComponents[i][2];
    delete[] this->SelectedVectorComponents[i];
    this->SelectedVectorComponents[i] = nullptr;
  }

  delete[] this->VectorArrayNames;
  this->VectorArrayNames = nullptr;

  delete[] this->VectorVariableNames;
  this->VectorVariableNames = nullptr;

  delete[] this->SelectedVectorComponents;
  this->SelectedVectorComponents = nullptr;

  this->VectorArrayNames = new char*[this->NumberOfVectorArrays + 1];
  this->VectorVariableNames = new char*[this->NumberOfVectorArrays + 1];
  this->SelectedVectorComponents = new int*[this->NumberOfVectorArrays + 1];

  for (i = 0; i < this->NumberOfVectorArrays; i++)
  {
    this->VectorArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->VectorArrayNames[i], arrayNames[i]);
    delete[] arrayNames[i];
    arrayNames[i] = nullptr;
    this->VectorVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->VectorVariableNames[i], varNames[i]);
    delete[] varNames[i];
    varNames[i] = nullptr;
    this->SelectedVectorComponents[i] = new int[3];
    this->SelectedVectorComponents[i][0] = component0;
    this->SelectedVectorComponents[i][1] = component1;
    this->SelectedVectorComponents[i][2] = component2;
    delete[] tempComponents[i];
    tempComponents[i] = nullptr;
  }
  delete[] arrayNames;
  delete[] varNames;
  delete[] tempComponents;

  this->VectorArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->VectorArrayNames[i], arrayName);
  this->VectorVariableNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->VectorVariableNames[i], arrayName);
  this->SelectedVectorComponents[i] = new int[3];
  this->SelectedVectorComponents[i][0] = component0;
  this->SelectedVectorComponents[i][1] = component1;
  this->SelectedVectorComponents[i][2] = component2;

  this->NumberOfVectorArrays++;
}

void svtkArrayCalculator::AddScalarVariable(
  const char* variableName, const char* arrayName, int component)
{
  if (!variableName || !arrayName)
  {
    return;
  }

  int i;
  for (i = 0; i < this->NumberOfScalarArrays; i++)
  {
    if (strcmp(variableName, this->ScalarVariableNames[i]) == 0 &&
      strcmp(arrayName, this->ScalarArrayNames[i]) == 0 &&
      this->SelectedScalarComponents[i] == component)
    {
      // Already have this variable/array/components so return.
      return;
    }
  }

  char** arrayNames = new char*[this->NumberOfScalarArrays];
  char** varNames = new char*[this->NumberOfScalarArrays];
  int* tempComponents = new int[this->NumberOfScalarArrays];

  for (i = 0; i < this->NumberOfScalarArrays; i++)
  {
    arrayNames[i] = new char[strlen(this->ScalarArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->ScalarArrayNames[i]);
    delete[] this->ScalarArrayNames[i];
    this->ScalarArrayNames[i] = nullptr;
    varNames[i] = new char[strlen(this->ScalarVariableNames[i]) + 1];
    strcpy(varNames[i], this->ScalarVariableNames[i]);
    delete[] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = nullptr;
    tempComponents[i] = this->SelectedScalarComponents[i];
  }
  delete[] this->ScalarArrayNames;
  this->ScalarArrayNames = nullptr;

  delete[] this->ScalarVariableNames;
  this->ScalarVariableNames = nullptr;

  delete[] this->SelectedScalarComponents;
  this->SelectedScalarComponents = nullptr;

  this->ScalarArrayNames = new char*[this->NumberOfScalarArrays + 1];
  this->ScalarVariableNames = new char*[this->NumberOfScalarArrays + 1];
  this->SelectedScalarComponents = new int[this->NumberOfScalarArrays + 1];

  for (i = 0; i < this->NumberOfScalarArrays; i++)
  {
    this->ScalarArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->ScalarArrayNames[i], arrayNames[i]);
    delete[] arrayNames[i];
    arrayNames[i] = nullptr;
    this->ScalarVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->ScalarVariableNames[i], varNames[i]);
    delete[] varNames[i];
    varNames[i] = nullptr;
    this->SelectedScalarComponents[i] = tempComponents[i];
  }
  delete[] arrayNames;
  delete[] varNames;
  delete[] tempComponents;

  this->ScalarArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->ScalarArrayNames[i], arrayName);
  this->ScalarVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->ScalarVariableNames[i], variableName);
  this->SelectedScalarComponents[i] = component;

  this->NumberOfScalarArrays++;
}

void svtkArrayCalculator::AddVectorVariable(
  const char* variableName, const char* arrayName, int component0, int component1, int component2)
{
  if (!variableName || !arrayName)
  {
    return;
  }

  int i;
  for (i = 0; i < this->NumberOfVectorArrays; i++)
  {
    if (strcmp(variableName, this->VectorVariableNames[i]) == 0 &&
      strcmp(arrayName, this->VectorArrayNames[i]) == 0 &&
      this->SelectedVectorComponents[i][0] == component0 &&
      this->SelectedVectorComponents[i][1] == component1 &&
      this->SelectedVectorComponents[i][2] == component2)
    {
      // Already have this variable/array/components so return.
      return;
    }
  }

  char** arrayNames = new char*[this->NumberOfVectorArrays];
  char** varNames = new char*[this->NumberOfVectorArrays];
  int** tempComponents = new int*[this->NumberOfVectorArrays];

  for (i = 0; i < this->NumberOfVectorArrays; i++)
  {
    arrayNames[i] = new char[strlen(this->VectorArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->VectorArrayNames[i]);
    delete[] this->VectorArrayNames[i];
    this->VectorArrayNames[i] = nullptr;
    varNames[i] = new char[strlen(this->VectorVariableNames[i]) + 1];
    strcpy(varNames[i], this->VectorVariableNames[i]);
    delete[] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = nullptr;
    tempComponents[i] = new int[3];
    tempComponents[i][0] = this->SelectedVectorComponents[i][0];
    tempComponents[i][1] = this->SelectedVectorComponents[i][1];
    tempComponents[i][2] = this->SelectedVectorComponents[i][2];
    delete[] this->SelectedVectorComponents[i];
    this->SelectedVectorComponents[i] = nullptr;
  }

  delete[] this->VectorArrayNames;
  this->VectorArrayNames = nullptr;

  delete[] this->VectorVariableNames;
  this->VectorVariableNames = nullptr;

  delete[] this->SelectedVectorComponents;
  this->SelectedVectorComponents = nullptr;

  this->VectorArrayNames = new char*[this->NumberOfVectorArrays + 1];
  this->VectorVariableNames = new char*[this->NumberOfVectorArrays + 1];
  this->SelectedVectorComponents = new int*[this->NumberOfVectorArrays + 1];

  for (i = 0; i < this->NumberOfVectorArrays; i++)
  {
    this->VectorArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->VectorArrayNames[i], arrayNames[i]);
    delete[] arrayNames[i];
    arrayNames[i] = nullptr;
    this->VectorVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->VectorVariableNames[i], varNames[i]);
    delete[] varNames[i];
    varNames[i] = nullptr;
    this->SelectedVectorComponents[i] = new int[3];
    this->SelectedVectorComponents[i][0] = tempComponents[i][0];
    this->SelectedVectorComponents[i][1] = tempComponents[i][1];
    this->SelectedVectorComponents[i][2] = tempComponents[i][2];
    delete[] tempComponents[i];
    tempComponents[i] = nullptr;
  }
  delete[] arrayNames;
  delete[] varNames;
  delete[] tempComponents;

  this->VectorArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->VectorArrayNames[i], arrayName);
  this->VectorVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->VectorVariableNames[i], variableName);
  this->SelectedVectorComponents[i] = new int[3];
  this->SelectedVectorComponents[i][0] = component0;
  this->SelectedVectorComponents[i][1] = component1;
  this->SelectedVectorComponents[i][2] = component2;

  this->NumberOfVectorArrays++;
}

void svtkArrayCalculator::AddCoordinateScalarVariable(const char* variableName, int component)
{
  int i;
  char** varNames = new char*[this->NumberOfCoordinateScalarArrays];
  int* tempComponents = new int[this->NumberOfCoordinateScalarArrays];

  for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
  {
    varNames[i] = new char[strlen(this->CoordinateScalarVariableNames[i]) + 1];
    strcpy(varNames[i], this->CoordinateScalarVariableNames[i]);
    delete[] this->CoordinateScalarVariableNames[i];
    this->CoordinateScalarVariableNames[i] = nullptr;
    tempComponents[i] = this->SelectedCoordinateScalarComponents[i];
  }

  delete[] this->CoordinateScalarVariableNames;
  this->CoordinateScalarVariableNames = nullptr;

  delete[] this->SelectedCoordinateScalarComponents;
  this->SelectedCoordinateScalarComponents = nullptr;

  this->CoordinateScalarVariableNames = new char*[this->NumberOfCoordinateScalarArrays + 1];
  this->SelectedCoordinateScalarComponents = new int[this->NumberOfCoordinateScalarArrays + 1];

  for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
  {
    this->CoordinateScalarVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->CoordinateScalarVariableNames[i], varNames[i]);
    delete[] varNames[i];
    varNames[i] = nullptr;
    this->SelectedCoordinateScalarComponents[i] = tempComponents[i];
  }
  delete[] varNames;
  delete[] tempComponents;

  this->CoordinateScalarVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->CoordinateScalarVariableNames[i], variableName);
  this->SelectedCoordinateScalarComponents[i] = component;

  this->NumberOfCoordinateScalarArrays++;
}

void svtkArrayCalculator::AddCoordinateVectorVariable(
  const char* variableName, int component0, int component1, int component2)
{
  int i;
  char** varNames = new char*[this->NumberOfCoordinateVectorArrays];
  int** tempComponents = new int*[this->NumberOfCoordinateVectorArrays];

  for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
  {
    varNames[i] = new char[strlen(this->CoordinateVectorVariableNames[i]) + 1];
    strcpy(varNames[i], this->CoordinateVectorVariableNames[i]);
    delete[] this->CoordinateVectorVariableNames[i];
    this->CoordinateVectorVariableNames[i] = nullptr;
    tempComponents[i] = new int[3];
    tempComponents[i][0] = this->SelectedCoordinateVectorComponents[i][0];
    tempComponents[i][1] = this->SelectedCoordinateVectorComponents[i][1];
    tempComponents[i][2] = this->SelectedCoordinateVectorComponents[i][2];
    delete[] this->SelectedCoordinateVectorComponents[i];
    this->SelectedCoordinateVectorComponents[i] = nullptr;
  }

  delete[] this->CoordinateVectorVariableNames;
  this->CoordinateVectorVariableNames = nullptr;

  delete[] this->SelectedCoordinateVectorComponents;
  this->SelectedCoordinateVectorComponents = nullptr;

  this->CoordinateVectorVariableNames = new char*[this->NumberOfCoordinateVectorArrays + 1];
  this->SelectedCoordinateVectorComponents = new int*[this->NumberOfCoordinateVectorArrays + 1];

  for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
  {
    this->CoordinateVectorVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->CoordinateVectorVariableNames[i], varNames[i]);
    delete[] varNames[i];
    varNames[i] = nullptr;
    this->SelectedCoordinateVectorComponents[i] = new int[3];
    this->SelectedCoordinateVectorComponents[i][0] = tempComponents[i][0];
    this->SelectedCoordinateVectorComponents[i][1] = tempComponents[i][1];
    this->SelectedCoordinateVectorComponents[i][2] = tempComponents[i][2];
    delete[] tempComponents[i];
    tempComponents[i] = nullptr;
  }
  delete[] varNames;
  delete[] tempComponents;

  this->CoordinateVectorVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->CoordinateVectorVariableNames[i], variableName);

  this->SelectedCoordinateVectorComponents[i] = new int[3];
  this->SelectedCoordinateVectorComponents[i][0] = component0;
  this->SelectedCoordinateVectorComponents[i][1] = component1;
  this->SelectedCoordinateVectorComponents[i][2] = component2;

  this->NumberOfCoordinateVectorArrays++;
}

#ifndef SVTK_LEGACY_REMOVE
const char* svtkArrayCalculator::GetAttributeModeAsString()
{
  SVTK_LEGACY_REPLACED_BODY(svtkArrayCalculator::GetAttributeModeAsString, "SVTK 8.1",
    svtkArrayCalculator::GetAttributeTypeAsString);
  int attributeType = this->GetAttributeType();
  if (attributeType == svtkDataObject::POINT)
  {
    return "UsePointData";
  }
  else if (attributeType == svtkDataObject::CELL)
  {
    return "UseCellData";
  }
  else if (attributeType == svtkDataObject::VERTEX)
  {
    return "UseVertexData";
  }
  else if (attributeType == svtkDataObject::EDGE)
  {
    return "UseEdgeData";
  }
  else
  {
    return "Default";
  }
}
#endif

const char* svtkArrayCalculator::GetAttributeTypeAsString()
{
  switch (this->AttributeType)
  {
    default:
    case DEFAULT_ATTRIBUTE_TYPE:
      return "Default";
    case svtkDataObject::POINT:
      return "UsePointData";
    case svtkDataObject::CELL:
      return "UseCellData";
    case svtkDataObject::VERTEX:
      return "UseVertexData";
    case svtkDataObject::EDGE:
      return "UseEdgeData";
    case svtkDataObject::ROW:
      return "UseRowData";
  }
}

void svtkArrayCalculator::RemoveScalarVariables()
{
  int i;

  for (i = 0; i < this->NumberOfScalarArrays; i++)
  {
    delete[] this->ScalarArrayNames[i];
    this->ScalarArrayNames[i] = nullptr;
    delete[] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = nullptr;
  }
  if (this->NumberOfScalarArrays > 0)
  {
    delete[] this->ScalarArrayNames;
    this->ScalarArrayNames = nullptr;
    delete[] this->ScalarVariableNames;
    this->ScalarVariableNames = nullptr;
    delete[] this->SelectedScalarComponents;
    this->SelectedScalarComponents = nullptr;
  }
  this->NumberOfScalarArrays = 0;

  this->FunctionParser->RemoveScalarVariables();
}

void svtkArrayCalculator::RemoveVectorVariables()
{
  int i;

  for (i = 0; i < this->NumberOfVectorArrays; i++)
  {
    delete[] this->VectorArrayNames[i];
    this->VectorArrayNames[i] = nullptr;
    delete[] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = nullptr;
    delete[] this->SelectedVectorComponents[i];
    this->SelectedVectorComponents[i] = nullptr;
  }
  if (this->NumberOfVectorArrays > 0)
  {
    delete[] this->VectorArrayNames;
    this->VectorArrayNames = nullptr;
    delete[] this->VectorVariableNames;
    this->VectorVariableNames = nullptr;
    delete[] this->SelectedVectorComponents;
    this->SelectedVectorComponents = nullptr;
  }
  this->NumberOfVectorArrays = 0;

  this->FunctionParser->RemoveVectorVariables();
}

void svtkArrayCalculator::RemoveCoordinateScalarVariables()
{
  int i;

  for (i = 0; i < this->NumberOfCoordinateScalarArrays; i++)
  {
    delete[] this->CoordinateScalarVariableNames[i];
    this->CoordinateScalarVariableNames[i] = nullptr;
  }
  if (this->NumberOfCoordinateScalarArrays > 0)
  {
    delete[] this->CoordinateScalarVariableNames;
    this->CoordinateScalarVariableNames = nullptr;
    delete[] this->SelectedCoordinateScalarComponents;
    this->SelectedCoordinateScalarComponents = nullptr;
  }
  this->NumberOfCoordinateScalarArrays = 0;

  this->FunctionParser->RemoveScalarVariables();
}

void svtkArrayCalculator::RemoveCoordinateVectorVariables()
{
  int i;

  for (i = 0; i < this->NumberOfCoordinateVectorArrays; i++)
  {
    delete[] this->CoordinateVectorVariableNames[i];
    this->CoordinateVectorVariableNames[i] = nullptr;
    delete[] this->SelectedCoordinateVectorComponents[i];
    this->SelectedCoordinateVectorComponents[i] = nullptr;
  }
  if (this->NumberOfVectorArrays > 0)
  {
    delete[] this->CoordinateVectorVariableNames;
    this->CoordinateVectorVariableNames = nullptr;
    delete[] this->SelectedCoordinateVectorComponents;
    this->SelectedCoordinateVectorComponents = nullptr;
  }
  this->NumberOfCoordinateVectorArrays = 0;

  this->FunctionParser->RemoveVectorVariables();
}

void svtkArrayCalculator::RemoveAllVariables()
{
  this->RemoveScalarVariables();
  this->RemoveVectorVariables();
  this->RemoveCoordinateScalarVariables();
  this->RemoveCoordinateVectorVariables();
}

char* svtkArrayCalculator::GetScalarArrayName(int i)
{
  if (i < this->NumberOfScalarArrays)
  {
    return this->ScalarArrayNames[i];
  }
  return nullptr;
}

char* svtkArrayCalculator::GetVectorArrayName(int i)
{
  if (i < this->NumberOfVectorArrays)
  {
    return this->VectorArrayNames[i];
  }
  return nullptr;
}

char* svtkArrayCalculator::GetScalarVariableName(int i)
{
  if (i < this->NumberOfScalarArrays)
  {
    return this->ScalarVariableNames[i];
  }
  return nullptr;
}

char* svtkArrayCalculator::GetVectorVariableName(int i)
{
  if (i < this->NumberOfVectorArrays)
  {
    return this->VectorVariableNames[i];
  }
  return nullptr;
}

int svtkArrayCalculator::GetSelectedScalarComponent(int i)
{
  if (i < this->NumberOfScalarArrays)
  {
    return this->SelectedScalarComponents[i];
  }
  return -1;
}

int* svtkArrayCalculator::GetSelectedVectorComponents(int i)
{
  if (i < this->NumberOfVectorArrays)
  {
    return this->SelectedVectorComponents[i];
  }
  return nullptr;
}

svtkDataSet* svtkArrayCalculator::GetDataSetOutput()
{
  return svtkDataSet::SafeDownCast(this->GetOutput());
}

void svtkArrayCalculator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Function: " << (this->Function ? this->Function : "(none)") << endl;
  os << indent
     << "Result Array Name: " << (this->ResultArrayName ? this->ResultArrayName : "(none)") << endl;

  os << indent << "Result Array Type: " << svtkImageScalarTypeNameMacro(this->ResultArrayType)
     << endl;

  os << indent << "Coordinate Results: " << this->CoordinateResults << endl;
  os << indent << "Attribute Type: " << this->GetAttributeTypeAsString() << endl;
  os << indent << "Number Of Scalar Arrays: " << this->NumberOfScalarArrays << endl;
  os << indent << "Number Of Vector Arrays: " << this->NumberOfVectorArrays << endl;
  os << indent << "Number Of Coordinate Scalar Arrays: " << this->NumberOfCoordinateScalarArrays
     << endl;
  os << indent << "Number Of Coordinate Vector Arrays: " << this->NumberOfCoordinateVectorArrays
     << endl;
  os << indent << "Replace Invalid Values: " << (this->ReplaceInvalidValues ? "On" : "Off") << endl;
  os << indent << "Replacement Value: " << this->ReplacementValue << endl;
}
