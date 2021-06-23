/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStringToNumeric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkStringToNumeric.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkUnicodeStringArray.h"
#include "svtkVariant.h"

svtkStandardNewMacro(svtkStringToNumeric);

svtkStringToNumeric::svtkStringToNumeric()
{
  this->ConvertFieldData = true;
  this->ConvertPointData = true;
  this->ConvertCellData = true;
  this->ForceDouble = false;
  this->DefaultIntegerValue = 0;
  this->DefaultDoubleValue = 0.0;
  this->TrimWhitespacePriorToNumericConversion = false;
}

svtkStringToNumeric::~svtkStringToNumeric() = default;

int svtkStringToNumeric::CountItemsToConvert(svtkFieldData* fieldData)
{
  int count = 0;
  for (int arr = 0; arr < fieldData->GetNumberOfArrays(); arr++)
  {
    svtkAbstractArray* array = fieldData->GetAbstractArray(arr);
    svtkStringArray* stringArray = svtkArrayDownCast<svtkStringArray>(array);
    svtkUnicodeStringArray* unicodeArray = svtkArrayDownCast<svtkUnicodeStringArray>(array);
    if (!stringArray && !unicodeArray)
    {
      continue;
    }
    else
    {
      count += array->GetNumberOfTuples() * array->GetNumberOfComponents();
    }
  }

  return count;
}

int svtkStringToNumeric::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output objects
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);

  svtkDataSet* outputDataSet = svtkDataSet::SafeDownCast(output);
  svtkGraph* outputGraph = svtkGraph::SafeDownCast(output);
  svtkTable* outputTable = svtkTable::SafeDownCast(output);

  // Figure out how many items we have to process
  int itemCount = 0;
  if (this->ConvertFieldData)
  {
    itemCount += this->CountItemsToConvert(output->GetFieldData());
  }
  if (outputDataSet && this->ConvertPointData)
  {
    itemCount += this->CountItemsToConvert(outputDataSet->GetPointData());
  }
  if (outputDataSet && this->ConvertCellData)
  {
    itemCount += this->CountItemsToConvert(outputDataSet->GetCellData());
  }
  if (outputGraph && this->ConvertPointData)
  {
    itemCount += this->CountItemsToConvert(outputGraph->GetVertexData());
  }
  if (outputGraph && this->ConvertCellData)
  {
    itemCount += this->CountItemsToConvert(outputGraph->GetEdgeData());
  }
  if (outputTable && this->ConvertPointData)
  {
    itemCount += this->CountItemsToConvert(outputTable->GetRowData());
  }

  this->ItemsToConvert = itemCount;
  this->ItemsConverted = 0;

  if (this->ConvertFieldData)
  {
    this->ConvertArrays(output->GetFieldData());
  }
  if (outputDataSet && this->ConvertPointData)
  {
    this->ConvertArrays(outputDataSet->GetPointData());
  }
  if (outputDataSet && this->ConvertCellData)
  {
    this->ConvertArrays(outputDataSet->GetCellData());
  }
  if (outputGraph && this->ConvertPointData)
  {
    this->ConvertArrays(outputGraph->GetVertexData());
  }
  if (outputGraph && this->ConvertCellData)
  {
    this->ConvertArrays(outputGraph->GetEdgeData());
  }
  if (outputTable && this->ConvertPointData)
  {
    this->ConvertArrays(outputTable->GetRowData());
  }

  return 1;
}

void svtkStringToNumeric::ConvertArrays(svtkFieldData* fieldData)
{
  for (int arr = 0; arr < fieldData->GetNumberOfArrays(); arr++)
  {
    svtkStringArray* stringArray =
      svtkArrayDownCast<svtkStringArray>(fieldData->GetAbstractArray(arr));
    svtkUnicodeStringArray* unicodeArray =
      svtkArrayDownCast<svtkUnicodeStringArray>(fieldData->GetAbstractArray(arr));
    if (!stringArray && !unicodeArray)
    {
      continue;
    }

    svtkIdType numTuples, numComps;
    svtkStdString arrayName;
    if (stringArray)
    {
      numTuples = stringArray->GetNumberOfTuples();
      numComps = stringArray->GetNumberOfComponents();
      arrayName = stringArray->GetName();
    }
    else
    {
      numTuples = unicodeArray->GetNumberOfTuples();
      numComps = unicodeArray->GetNumberOfComponents();
      arrayName = unicodeArray->GetName();
    }

    // Set up the output array
    svtkDoubleArray* doubleArray = svtkDoubleArray::New();
    doubleArray->SetNumberOfComponents(numComps);
    doubleArray->SetNumberOfTuples(numTuples);
    doubleArray->SetName(arrayName);

    // Set up the output array
    svtkIntArray* intArray = svtkIntArray::New();
    intArray->SetNumberOfComponents(numComps);
    intArray->SetNumberOfTuples(numTuples);
    intArray->SetName(arrayName);

    // Convert the strings to time point values
    bool allInteger = true;
    bool allNumeric = true;
    for (svtkIdType i = 0; i < numTuples * numComps; i++)
    {
      ++this->ItemsConverted;
      if (this->ItemsConverted % 100 == 0)
      {
        this->UpdateProgress(
          static_cast<double>(this->ItemsConverted) / static_cast<double>(this->ItemsToConvert));
      }

      svtkStdString str;
      if (stringArray)
      {
        str = stringArray->GetValue(i);
      }
      else
      {
        str = unicodeArray->GetValue(i).utf8_str();
      }

      if (this->TrimWhitespacePriorToNumericConversion)
      {
        size_t startPos = str.find_first_not_of(" \n\t\r");
        if (startPos == svtkStdString::npos)
        {
          str = "";
        }
        else
        {
          size_t endPos = str.find_last_not_of(" \n\t\r");
          str = str.substr(startPos, endPos - startPos + 1);
        }
      }

      bool ok;
      if (allInteger)
      {
        if (str.length() == 0)
        {
          intArray->SetValue(i, this->DefaultIntegerValue);
          doubleArray->SetValue(i, this->DefaultDoubleValue);
          continue;
        }
        int intValue = svtkVariant(str).ToInt(&ok);
        if (ok)
        {
          double doubleValue = intValue;
          intArray->SetValue(i, intValue);
          doubleArray->SetValue(i, doubleValue);
        }
        else
        {
          allInteger = false;
        }
      }
      if (!allInteger)
      {
        if (str.length() == 0)
        {
          doubleArray->SetValue(i, this->DefaultDoubleValue);
          continue;
        }
        double doubleValue = svtkVariant(str).ToDouble(&ok);
        if (!ok)
        {
          allNumeric = false;
          break;
        }
        else
        {
          doubleArray->SetValue(i, doubleValue);
        }
      }
    }
    if (allNumeric)
    {
      // Calling AddArray will replace the old array since the names match.
      // Are they all ints, and did I test anything?
      if (!this->ForceDouble && allInteger && numTuples && numComps)
      {
        fieldData->AddArray(intArray);
      }
      else
      {
        fieldData->AddArray(doubleArray);
      }
    }
    intArray->Delete();
    doubleArray->Delete();
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStringToNumeric::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkStringToNumeric::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkDataObject* output = info->Get(svtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        svtkDataObject* newOutput = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

void svtkStringToNumeric::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConvertFieldData: " << (this->ConvertFieldData ? "on" : "off") << endl;
  os << indent << "ConvertPointData: " << (this->ConvertPointData ? "on" : "off") << endl;
  os << indent << "ConvertCellData: " << (this->ConvertCellData ? "on" : "off") << endl;
  os << indent << "ForceDouble: " << (this->ForceDouble ? "on" : "off") << endl;
  os << indent << "DefaultIntegerValue: " << this->DefaultIntegerValue << endl;
  os << indent << "DefaultDoubleValue: " << this->DefaultDoubleValue << endl;
  os << indent << "TrimWhitespacePriorToNumericConversion: "
     << (this->TrimWhitespacePriorToNumericConversion ? "on" : "off") << endl;
}
