/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalDelimitedTextReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTemporalDelimitedTextReader.h"

#include "svtkDataArray.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkSetGet.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"
#include <string>
#include <vector>

svtkStandardNewMacro(svtkTemporalDelimitedTextReader);

//----------------------------------------------------------------------------
svtkTemporalDelimitedTextReader::svtkTemporalDelimitedTextReader()
{
  this->DetectNumericColumnsOn();
}

//----------------------------------------------------------------------------
void svtkTemporalDelimitedTextReader::SetTimeColumnName(const std::string name)
{
  if (this->TimeColumnName.compare(name) != 0)
  {
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting TimeColumnName to "
                  << name);
    this->TimeColumnName = name;
    this->InternalModified();
  }
}

//----------------------------------------------------------------------------
void svtkTemporalDelimitedTextReader::SetTimeColumnId(const int idx)
{
  if (idx != this->TimeColumnId)
  {
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting TimeColumnId to" << idx);
    this->TimeColumnId = idx;
    this->InternalModified();
  }
}

//----------------------------------------------------------------------------
void svtkTemporalDelimitedTextReader::SetRemoveTimeStepColumn(bool rts)
{
  if (rts != this->RemoveTimeStepColumn)
  {
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting RemoveTimeStepColumn to "
                  << rts);
    this->RemoveTimeStepColumn = rts;
    this->InternalModified();
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkTemporalDelimitedTextReader::GetMTime()
{
  return std::max(this->MTime, this->InternalMTime);
}

//----------------------------------------------------------------------------
int svtkTemporalDelimitedTextReader::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (strlen(this->FieldDelimiterCharacters) == 0)
  {
    // This reader does not give any output as long as the
    // FieldDelimiterCharacters is not set by the user as we need to parse the
    // input file in the RequestInformation to set the time range (which
    // requires the FieldDelimiterCharacters).
    return 1;
  }

  if (this->MTime > this->LastReadTime)
  {
    // fill the ReadTable with the actual input
    // only if modified has been called
    this->ReadTable->Initialize();
    this->ReadData(this->ReadTable);
    this->LastReadTime = this->GetMTime();
  }

  if (!this->EnforceColumnName())
  {
    // Bad user input
    return 0;
  }

  if (this->InternalColumnName.empty())
  {
    // Output the whole input data, not temporal
    return this->Superclass::RequestInformation(request, inputVector, outputVector);
  }

  // Store each line id in the TimeMap, at the given time step
  svtkDataArray* inputColumn =
    svtkDataArray::SafeDownCast(this->ReadTable->GetColumnByName(this->InternalColumnName.c_str()));
  const svtkIdType nbRows = inputColumn->GetNumberOfTuples();
  this->TimeMap.clear();
  for (svtkIdType r = 0; r < nbRows; r++)
  {
    double v = inputColumn->GetTuple1(r);
    if (svtkMath::IsNan(v))
    {
      svtkWarningMacro("The time step indicator column has a nan value at line: " << r);
    }
    else
    {
      this->TimeMap[v].emplace_back(r);
    }
  }

  // Get the time range (first and last key of the TimeMap)
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  double timeRange[2] = { this->TimeMap.cbegin()->first, this->TimeMap.crbegin()->first };
  outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  // Get the discrete time steps from the TimeMap keys
  std::vector<double> timeStepsArray;
  timeStepsArray.reserve(this->TimeMap.size());
  for (auto mapEl : this->TimeMap)
  {
    timeStepsArray.emplace_back(mapEl.first);
  }
  const int nbTimeSteps = static_cast<int>(timeStepsArray.size());
  outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeStepsArray.data(), nbTimeSteps);

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkTemporalDelimitedTextReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (strlen(this->FieldDelimiterCharacters) == 0)
  {
    svtkErrorMacro(
      "You need to set the FieldDelimiterCharacters before requesting data with this reader");
    return 0;
  }

  if (!this->EnforceColumnName())
  {
    svtkErrorMacro("Invalid user input for the Time step indicator.");
    return 0;
  }

  if (this->InternalColumnName.empty())
  {
    // Shallow copy the internal reader's output as the time column
    // has not been set
    svtkTable* outputTable = svtkTable::GetData(outputVector, 0);
    outputTable->ShallowCopy(this->ReadTable);
    this->UpdateProgress(1);
    return 1;
  }

  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): process column "
                << this->InternalColumnName);

  // Retrieve the current time step
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  double updateTimeStep = 0;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    updateTimeStep = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  this->UpdateProgress(0.5);

  if (this->TimeMap.size())
  {
    // Generate an empty output with the same structure
    svtkTable* outputTable = svtkTable::GetData(outputVector, 0);
    svtkDataSetAttributes* outAttributes = outputTable->GetRowData();
    auto timeStepDataIt = this->TimeMap.lower_bound(updateTimeStep);
    if (timeStepDataIt == this->TimeMap.end())
    {
      // update time is too high, take last element.
      --timeStepDataIt;
    }
    svtkIdType nbRow = static_cast<svtkIdType>(timeStepDataIt->second.size());
    outAttributes->CopyAllocate(this->ReadTable->GetRowData(), nbRow);
    for (auto r : timeStepDataIt->second)
    {
      outputTable->InsertNextRow(this->ReadTable->GetRow(r));
    }

    // Get rid of the time column in the result
    if (this->RemoveTimeStepColumn)
    {
      outputTable->RemoveColumnByName(this->InternalColumnName.c_str());
    }
  }

  this->UpdateProgress(1);

  return 1;
}

//----------------------------------------------------------------------------
bool svtkTemporalDelimitedTextReader::EnforceColumnName()
{
  this->InternalColumnName = "";

  if (this->TimeColumnName.empty() && this->TimeColumnId == -1)
  {
    // No user specified input, the reader simply output the whole content of
    // the input file.
    return 1;
  }

  // Set TimeColumnName from user input
  if (this->TimeColumnId != -1)
  {
    // use id to retrieve column
    if (this->TimeColumnId >= 0 && this->TimeColumnId < this->ReadTable->GetNumberOfColumns())
    {
      this->InternalColumnName = this->ReadTable->GetColumnName(this->TimeColumnId);
    }
    else
    {
      svtkErrorMacro("Invalid column id: " << this->TimeColumnId);
      return 0;
    }
  }
  else if (!this->TimeColumnName.empty())
  {
    // use name to retrieve column
    svtkAbstractArray* arr = this->ReadTable->GetColumnByName(this->TimeColumnName.c_str());
    if (arr == nullptr)
    {
      svtkErrorMacro("Invalid column name: " << this->TimeColumnName);
      return 0;
    }
    else
    {
      // check valid array
      svtkDataArray* numArr = svtkDataArray::SafeDownCast(arr);
      if (numArr == nullptr)
      {
        svtkErrorMacro("Not a numerical column: " << this->TimeColumnName);
        return 0;
      }
      else if (numArr->GetNumberOfComponents() != 1)
      {
        svtkErrorMacro("The time column must have only one component: " << this->TimeColumnName);
        return 0;
      }
    }
    this->InternalColumnName = this->TimeColumnName;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkTemporalDelimitedTextReader::InternalModified()
{
  this->InternalMTime.Modified();
}

//----------------------------------------------------------------------------
void svtkTemporalDelimitedTextReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "TimeColumnName: " << this->TimeColumnName << endl;
  os << "TimeColumnId: " << this->TimeColumnId << endl;
  os << "RemoveTimeStepColumn: " << this->RemoveTimeStepColumn << endl;
}
