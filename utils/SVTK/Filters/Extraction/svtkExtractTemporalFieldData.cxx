/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractTemporalFieldData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractTemporalFieldData.h"

#include "svtkCompositeDataIterator.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"

#include <vector>

class svtkExtractTemporalFieldData::svtkInternals
{
public:
  std::vector<double> TimeSteps;
};

svtkObjectFactoryNewMacro(svtkExtractTemporalFieldData);
//----------------------------------------------------------------------------
svtkExtractTemporalFieldData::svtkExtractTemporalFieldData()
{
  SVTK_LEGACY_REPLACED_BODY(
    svtkExtractTemporalFieldData, "SVTK 9.0", svtkExtractExodusGlobalTemporalVariables);

  this->Internals = new svtkExtractTemporalFieldData::svtkInternals();
  this->HandleCompositeDataBlocksIndividually = true;
}

//----------------------------------------------------------------------------
svtkExtractTemporalFieldData::~svtkExtractTemporalFieldData()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
int svtkExtractTemporalFieldData::GetNumberOfTimeSteps()
{
  return static_cast<int>(this->Internals->TimeSteps.size());
}

//----------------------------------------------------------------------------
void svtkExtractTemporalFieldData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "HandleCompositeDataBlocksIndividually: " << this->HandleCompositeDataBlocksIndividually
     << endl;
}

//----------------------------------------------------------------------------
int svtkExtractTemporalFieldData::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractTemporalFieldData::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataObject* input = svtkDataObject::GetData(inputVector[0], 0);

  if (svtkCompositeDataSet::SafeDownCast(input) && this->HandleCompositeDataBlocksIndividually)
  {
    if (svtkMultiBlockDataSet::GetData(outputVector, 0) == nullptr)
    {
      svtkNew<svtkMultiBlockDataSet> mb;
      outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), mb);
    }
  }
  else if (svtkTable::GetData(outputVector, 0) == nullptr)
  {
    svtkNew<svtkTable> table;
    outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), table);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractTemporalFieldData::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    int size = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->Internals->TimeSteps.resize(size);
    if (size > 0)
    {
      inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->Internals->TimeSteps[0]);
    }
  }
  else
  {
    this->Internals->TimeSteps.clear();
  }

  // The output of this filter does not contain a specific time, rather
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractTemporalFieldData::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->GetNumberOfTimeSteps() == 0)
  {
    svtkErrorMacro("No time steps in input data!");
    return 0;
  }

  svtkDataObject* inputDO = svtkDataObject::GetData(inputVector[0], 0);
  if (svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(inputDO))
  {
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    if (this->HandleCompositeDataBlocksIndividually)
    {
      svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector, 0);
      assert(output);
      output->CopyStructure(cd);
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        if (svtkDataSet* inputDS = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
        {
          svtkNew<svtkTable> outputBlock;
          this->CopyDataToOutput(inputDS, outputBlock);
          output->SetDataSet(iter, outputBlock);
        }
      }
    }
    else
    {
      svtkTable* output = svtkTable::GetData(outputVector, 0);
      assert(output);
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        if (svtkDataSet* inputDS = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
        {
          if (this->CopyDataToOutput(inputDS, output))
          {
            break;
          }
        }
      }
    }
  }
  else if (svtkDataSet* input = svtkDataSet::SafeDownCast(inputDO))
  {
    svtkTable* output = svtkTable::GetData(outputVector, 0);
    this->CopyDataToOutput(input, output);
  }
  else
  {
    svtkErrorMacro("Incorrect input type.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
bool svtkExtractTemporalFieldData::CopyDataToOutput(svtkDataSet* input, svtkTable* output)
{
  svtkDataSetAttributes* outRowData = output->GetRowData();
  svtkFieldData* ifd = input->GetFieldData();
  if (!ifd || !outRowData)
  {
    return false;
  }

  int numTimeSteps = this->GetNumberOfTimeSteps();
  assert(numTimeSteps > 0);
  for (svtkIdType j = 0; j < ifd->GetNumberOfArrays(); j++)
  {
    svtkDataArray* inFieldArray = ifd->GetArray(j);
    if (inFieldArray && inFieldArray->GetName() &&
      inFieldArray->GetNumberOfTuples() == numTimeSteps)
    {
      svtkDataArray* outArray = inFieldArray->NewInstance();
      outArray->ShallowCopy(inFieldArray);
      outRowData->AddArray(outArray);
      outArray->Delete();
    }
  }

  if (outRowData->GetNumberOfArrays() == 0)
  {
    return false;
  }

  // Add an array to hold the time at each step
  svtkNew<svtkDoubleArray> timeArray;
  timeArray->SetNumberOfComponents(1);
  timeArray->SetNumberOfTuples(numTimeSteps);
  if (ifd->GetArray("Time"))
  {
    timeArray->SetName("TimeData");
  }
  else
  {
    timeArray->SetName("Time");
  }
  std::copy(
    this->Internals->TimeSteps.begin(), this->Internals->TimeSteps.end(), timeArray->GetPointer(0));
  outRowData->AddArray(timeArray);
  return true;
}
