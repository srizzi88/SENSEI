/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeArrays.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMergeArrays.h"

#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkMergeArrays);

//----------------------------------------------------------------------------
svtkMergeArrays::svtkMergeArrays() {}

//----------------------------------------------------------------------------
svtkMergeArrays::~svtkMergeArrays() {}

//----------------------------------------------------------------------------
bool svtkMergeArrays::GetOutputArrayName(
  svtkFieldData* arrays, const char* arrayName, int inputIndex, std::string& outputArrayName)
{
  if (arrays->GetAbstractArray(arrayName) == nullptr)
  {
    return false;
  }
  outputArrayName = std::string(arrayName) + "_input_" + std::to_string(inputIndex);
  return true;
}

//----------------------------------------------------------------------------
void svtkMergeArrays::MergeArrays(int inputIndex, svtkFieldData* inputFD, svtkFieldData* outputFD)
{
  if (inputFD == nullptr || outputFD == nullptr)
  {
    return;
  }

  std::string outputArrayName;
  int numArrays = inputFD->GetNumberOfArrays();
  for (int arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
  {
    svtkAbstractArray* array = inputFD->GetAbstractArray(arrayIdx);
    if (this->GetOutputArrayName(outputFD, array->GetName(), inputIndex, outputArrayName))
    {
      svtkAbstractArray* newArray = array->NewInstance();
      if (svtkDataArray* newDataArray = svtkDataArray::SafeDownCast(newArray))
      {
        newDataArray->ShallowCopy(svtkDataArray::SafeDownCast(array));
      }
      else
      {
        newArray->DeepCopy(array);
      }
      newArray->SetName(outputArrayName.c_str());
      outputFD->AddArray(newArray);
      newArray->FastDelete();
    }
    else
    {
      outputFD->AddArray(array);
    }
  }
}

//----------------------------------------------------------------------------
int svtkMergeArrays::MergeDataObjectFields(svtkDataObject* input, int idx, svtkDataObject* output)
{
  int checks[svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];
  for (int attr = 0; attr < svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
  {
    checks[attr] = output->GetNumberOfElements(attr) == input->GetNumberOfElements(attr) ? 0 : 1;
  }
  int globalChecks[svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];

  for (int i = 0; i < svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++i)
  {
    globalChecks[i] = checks[i];
  }

  for (int attr = 0; attr < svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
  {
    if (globalChecks[attr] == 0)
    {
      // only merge arrays when the number of elements in the input and output are the same
      this->MergeArrays(
        idx, input->GetAttributesAsFieldData(attr), output->GetAttributesAsFieldData(attr));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkMergeArrays::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int svtkMergeArrays::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int num = inputVector[0]->GetNumberOfInformationObjects();
  if (num < 1)
  {
    return 0;
  }
  // get the output info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkCompositeDataSet* cOutput = svtkCompositeDataSet::SafeDownCast(output);
  if (cOutput)
  {
    svtkCompositeDataSet* cInput = svtkCompositeDataSet::SafeDownCast(input);
    cOutput->CopyStructure(cInput);
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(cInput->NewIterator());
    iter->InitTraversal();
    for (; !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (svtkDataSet* tmpIn = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
      {
        svtkDataSet* tmpOut = tmpIn->NewInstance();
        tmpOut->ShallowCopy(tmpIn);
        cOutput->SetDataSet(iter, tmpOut);
        tmpOut->Delete();
      }
    }
  }
  else
  {
    output->ShallowCopy(input);
  }

  for (int idx = 1; idx < num; ++idx)
  {
    inInfo = inputVector[0]->GetInformationObject(idx);
    input = inInfo->Get(svtkDataObject::DATA_OBJECT());
    if (!this->MergeDataObjectFields(input, idx, output))
    {
      return 0;
    }
    svtkCompositeDataSet* cInput = svtkCompositeDataSet::SafeDownCast(input);
    if (cOutput && cInput)
    {
      svtkSmartPointer<svtkCompositeDataIterator> iter;
      iter.TakeReference(cInput->NewIterator());
      iter->InitTraversal();
      for (; !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        svtkDataObject* tmpIn = iter->GetCurrentDataObject();
        svtkDataObject* tmpOut = cOutput->GetDataSet(iter);
        if (!this->MergeDataObjectFields(tmpIn, idx, tmpOut))
        {
          return 0;
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkMergeArrays::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
