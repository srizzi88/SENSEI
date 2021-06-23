/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassArrays.cxx

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

#include "svtkPassArrays.h"

#include "svtkDataObject.h"
#include "svtkDataSetAttributes.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkPassArrays);

namespace
{
// returns true if modified
typedef std::vector<std::pair<int, std::string> > ArraysType;
bool ClearArraysOfType(int type, ArraysType& arrays)
{
  bool retVal = false;
  ArraysType::iterator iter = arrays.begin();
  while (iter != arrays.end())
  {
    if (iter->first == type)
    {
      iter = arrays.erase(iter);
      retVal = true;
    }
    else
    {
      ++iter;
    }
  }
  return retVal;
}
}

class svtkPassArrays::Internals
{
public:
  ArraysType Arrays;
  std::vector<int> FieldTypes;
};

svtkPassArrays::svtkPassArrays()
{
  this->Implementation = new Internals();
  this->RemoveArrays = false;
  this->UseFieldTypes = false;
}

svtkPassArrays::~svtkPassArrays()
{
  delete this->Implementation;
}

void svtkPassArrays::AddArray(int fieldType, const char* name)
{
  if (!name)
  {
    svtkErrorMacro("name cannot be null.");
    return;
  }
  std::string n = name;
  this->Implementation->Arrays.push_back(std::make_pair(fieldType, n));
  this->Modified();
}

void svtkPassArrays::AddPointDataArray(const char* name)
{
  this->AddArray(svtkDataObject::POINT, name);
}

void svtkPassArrays::AddCellDataArray(const char* name)
{
  this->AddArray(svtkDataObject::CELL, name);
}

void svtkPassArrays::AddFieldDataArray(const char* name)
{
  this->AddArray(svtkDataObject::FIELD, name);
}

void svtkPassArrays::RemoveArray(int fieldType, const char* name)
{
  if (!name)
  {
    svtkErrorMacro("name cannot be null.");
    return;
  }
  ArraysType::iterator iter = this->Implementation->Arrays.begin();
  while (iter != this->Implementation->Arrays.end())
  {
    if (iter->first == fieldType && iter->second == name)
    {
      iter = this->Implementation->Arrays.erase(iter);
      this->Modified();
    }
    else
    {
      ++iter;
    }
  }
}

void svtkPassArrays::RemovePointDataArray(const char* name)
{
  this->RemoveArray(svtkDataObject::POINT, name);
}

void svtkPassArrays::RemoveCellDataArray(const char* name)
{
  this->RemoveArray(svtkDataObject::CELL, name);
}

void svtkPassArrays::RemoveFieldDataArray(const char* name)
{
  this->RemoveArray(svtkDataObject::FIELD, name);
}

void svtkPassArrays::ClearArrays()
{
  if (this->Implementation->Arrays.empty() == false)
  {
    this->Modified();
  }
  this->Implementation->Arrays.clear();
}

void svtkPassArrays::ClearPointDataArrays()
{
  if (ClearArraysOfType(svtkDataObject::POINT, this->Implementation->Arrays) == true)
  {
    this->Modified();
  }
}

void svtkPassArrays::ClearCellDataArrays()
{
  if (ClearArraysOfType(svtkDataObject::CELL, this->Implementation->Arrays) == true)
  {
    this->Modified();
  }
}

void svtkPassArrays::ClearFieldDataArrays()
{
  if (ClearArraysOfType(svtkDataObject::FIELD, this->Implementation->Arrays) == true)
  {
    this->Modified();
  }
}

void svtkPassArrays::AddFieldType(int fieldType)
{
  this->Implementation->FieldTypes.push_back(fieldType);
  this->Modified();
}

void svtkPassArrays::ClearFieldTypes()
{
  this->Implementation->FieldTypes.clear();
  this->Modified();
}

int svtkPassArrays::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output objects
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);

  // If we are specifying arrays to add, start with only the ghost arrays in the output
  // if they exist in the input
  if (!this->RemoveArrays)
  {
    if (this->UseFieldTypes)
    {
      for (std::vector<int>::size_type i = 0; i < this->Implementation->FieldTypes.size(); ++i)
      {
        if (svtkFieldData* outData =
              output->GetAttributesAsFieldData(this->Implementation->FieldTypes[i]))
        {
          outData->Initialize();
          if (svtkFieldData* inData =
                input->GetAttributesAsFieldData(this->Implementation->FieldTypes[i]))
          {
            outData->AddArray(inData->GetAbstractArray(svtkDataSetAttributes::GhostArrayName()));
          }
        }
      }
    }
    else
    {
      for (ArraysType::size_type i = 0; i < this->Implementation->Arrays.size(); ++i)
      {
        if (svtkFieldData* outData =
              output->GetAttributesAsFieldData(this->Implementation->Arrays[i].first))
        {
          outData->Initialize();
          if (svtkFieldData* inData =
                input->GetAttributesAsFieldData(this->Implementation->Arrays[i].first))
          {
            outData->AddArray(inData->GetAbstractArray(svtkDataSetAttributes::GhostArrayName()));
          }
        }
      }
    }
  }

  ArraysType::iterator it, itEnd;
  itEnd = this->Implementation->Arrays.end();
  for (it = this->Implementation->Arrays.begin(); it != itEnd; ++it)
  {
    if (this->UseFieldTypes)
    {
      // Make sure this is a field type we are interested in
      if (std::find(this->Implementation->FieldTypes.begin(),
            this->Implementation->FieldTypes.end(),
            it->first) == this->Implementation->FieldTypes.end())
      {
        continue;
      }
    }

    svtkFieldData* data = input->GetAttributesAsFieldData(it->first);
    svtkFieldData* outData = output->GetAttributesAsFieldData(it->first);
    if (!data)
    {
      continue;
    }
    svtkAbstractArray* arr = data->GetAbstractArray(it->second.c_str());
    if (!arr)
    {
      continue;
    }
    if (this->RemoveArrays)
    {
      outData->RemoveArray(it->second.c_str());
    }
    else
    {
      outData->AddArray(arr);

      // Preserve attribute type if applicable
      svtkDataSetAttributes* attrib = svtkDataSetAttributes::SafeDownCast(data);
      svtkDataSetAttributes* outAttrib = svtkDataSetAttributes::SafeDownCast(outData);
      if (attrib)
      {
        for (int a = 0; a < svtkDataSetAttributes::NUM_ATTRIBUTES; ++a)
        {
          if (attrib->GetAbstractAttribute(a) == arr)
          {
            outAttrib->SetActiveAttribute(it->second.c_str(), a);
          }
        }
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkPassArrays::ProcessRequest(
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
int svtkPassArrays::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    // Skip composite data sets so that executives will treat this as a simple filter
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHyperTreeGrid:");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPassArrays::RequestDataObject(
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

void svtkPassArrays::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoveArrays: " << (this->RemoveArrays ? "on" : "off") << endl;
  os << indent << "UseFieldTypes: " << (this->UseFieldTypes ? "on" : "off") << endl;
}
