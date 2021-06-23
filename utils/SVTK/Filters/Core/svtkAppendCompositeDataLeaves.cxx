/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendCompositeDataLeaves.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAppendCompositeDataLeaves.h"

#include "svtkAppendFilter.h"
#include "svtkAppendPolyData.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkDataSetCollection.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkTable.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkAppendCompositeDataLeaves);

//----------------------------------------------------------------------------
svtkAppendCompositeDataLeaves::svtkAppendCompositeDataLeaves()
{
  this->AppendFieldData = 0;
}

//----------------------------------------------------------------------------
svtkAppendCompositeDataLeaves::~svtkAppendCompositeDataLeaves() = default;

//----------------------------------------------------------------------------
int svtkAppendCompositeDataLeaves::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // this filter preserves input data type.
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkCompositeDataSet* input =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkCompositeDataSet* output =
        svtkCompositeDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));

      if (!output || !output->IsA(input->GetClassName()))
      {
        svtkCompositeDataSet* newOutput = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int svtkAppendCompositeDataLeaves::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs <= 0)
  {
    // Fail silently when there are no inputs.
    return 1;
  }

  // get the output info object
  svtkCompositeDataSet* output = svtkCompositeDataSet::GetData(outputVector, 0);
  svtkCompositeDataSet* input0 = svtkCompositeDataSet::GetData(inputVector[0], 0);
  if (numInputs == 1)
  {
    // trivial case.
    output->ShallowCopy(input0);
    return 1;
  }

  // since composite structure is expected to be same on all inputs, we copy the
  // structure from the 1st input.
  output->CopyStructure(input0);

  svtkDebugMacro(<< "Appending data together");

  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(output->NewIterator());

  iter->SkipEmptyNodesOff(); // We're iterating over the output, whose leaves are all empty.
  static bool first = true;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    // Loop over all inputs at this "spot" in the composite data tree. locate
    // the first input that has a non-null data-object at this location, if any.
    svtkDataObject* obj = nullptr;
    int inputIndex;
    for (inputIndex = 0; inputIndex < numInputs && !obj; ++inputIndex)
    {
      svtkCompositeDataSet* inputX = svtkCompositeDataSet::GetData(inputVector[0], inputIndex);
      obj = inputX ? inputX->GetDataSet(iter) : nullptr;
    }

    if (obj == nullptr)
    {
      continue; // no input had a non-nullptr dataset
    }

    if (svtkUnstructuredGrid::SafeDownCast(obj))
    {
      this->AppendUnstructuredGrids(inputVector[0], inputIndex - 1, numInputs, iter, output);
    }
    else if (svtkPolyData::SafeDownCast(obj))
    {
      this->AppendPolyData(inputVector[0], inputIndex - 1, numInputs, iter, output);
    }
    else if (svtkTable* table = svtkTable::SafeDownCast(obj))
    {
      svtkTable* newTable = svtkTable::New();
      newTable->ShallowCopy(table);
      output->SetDataSet(iter, newTable);
      newTable->Delete();
    }
    else if (svtkImageData* img = svtkImageData::SafeDownCast(obj))
    {
      svtkImageData* clone = img->NewInstance();
      clone->ShallowCopy(img);
      output->SetDataSet(iter, clone);
      clone->FastDelete();
    }
    else if (svtkStructuredGrid* sg = svtkStructuredGrid::SafeDownCast(obj))
    {
      svtkStructuredGrid* clone = sg->NewInstance();
      clone->ShallowCopy(sg);
      output->SetDataSet(iter, clone);
      clone->FastDelete();
    }
    else if (svtkRectilinearGrid* rg = svtkRectilinearGrid::SafeDownCast(obj))
    {
      svtkRectilinearGrid* clone = rg->NewInstance();
      clone->ShallowCopy(rg);
      output->SetDataSet(iter, clone);
      clone->FastDelete();
    }
    else if (first)
    {
      first = false;
      svtkWarningMacro(<< "Input " << inputIndex << " was of type \"" << obj->GetClassName()
                      << "\" which is not handled\n");
    }
  }
  first = true;
  return 1;
}

//----------------------------------------------------------------------------
int svtkAppendCompositeDataLeaves::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void svtkAppendCompositeDataLeaves::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AppendFieldData: " << this->AppendFieldData << "\n";
}

//----------------------------------------------------------------------------
void svtkAppendCompositeDataLeaves::AppendUnstructuredGrids(svtkInformationVector* inputVector, int i,
  int numInputs, svtkCompositeDataIterator* iter, svtkCompositeDataSet* output)
{
  svtkNew<svtkAppendFilter> appender;

  for (int idx = i; idx < numInputs; ++idx)
  {
    svtkCompositeDataSet* icdset = svtkCompositeDataSet::GetData(inputVector, idx);
    if (icdset)
    {
      svtkUnstructuredGrid* iudset = svtkUnstructuredGrid::SafeDownCast(icdset->GetDataSet(iter));
      if (iudset)
      {
        appender->AddInputDataObject(iudset);
      }
    }
  }
  appender->Update();
  output->SetDataSet(iter, appender->GetOutputDataObject(0));
  this->AppendFieldDataArrays(inputVector, i, numInputs, iter, appender->GetOutput(0));
}

//----------------------------------------------------------------------------
void svtkAppendCompositeDataLeaves::AppendPolyData(svtkInformationVector* inputVector, int i,
  int numInputs, svtkCompositeDataIterator* iter, svtkCompositeDataSet* output)
{
  svtkNew<svtkAppendPolyData> appender;

  for (int idx = i; idx < numInputs; ++idx)
  {
    svtkCompositeDataSet* icdset = svtkCompositeDataSet::GetData(inputVector, idx);
    if (icdset)
    {
      svtkPolyData* ipdset = svtkPolyData::SafeDownCast(icdset->GetDataSet(iter));
      if (ipdset)
      {
        appender->AddInputDataObject(ipdset);
      }
    }
  }

  appender->Update();
  output->SetDataSet(iter, appender->GetOutputDataObject(0));
  this->AppendFieldDataArrays(inputVector, i, numInputs, iter, appender->GetOutput(0));
}

//----------------------------------------------------------------------------
void svtkAppendCompositeDataLeaves::AppendFieldDataArrays(svtkInformationVector* inputVector, int i,
  int numInputs, svtkCompositeDataIterator* iter, svtkDataSet* odset)
{
  if (!this->AppendFieldData)
    return;

  svtkFieldData* ofd = odset->GetFieldData();
  for (int idx = i; idx < numInputs; ++idx)
  {
    svtkCompositeDataSet* icdset = svtkCompositeDataSet::GetData(inputVector, idx);
    if (icdset)
    {
      svtkDataObject* idobj = icdset->GetDataSet(iter);
      if (idobj)
      {
        svtkFieldData* ifd = idobj->GetFieldData();
        int numArr = ifd->GetNumberOfArrays();
        for (int a = 0; a < numArr; ++a)
        {
          svtkAbstractArray* arr = ifd->GetAbstractArray(a);
          if (ofd->HasArray(arr->GetName()))
          {
            // Do something?
          }
          else
          {
            ofd->AddArray(arr);
          }
        }
      }
    }
  }
}
