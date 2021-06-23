/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReduceTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkReduceTable.h"

#include "svtkAbstractArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"

#include <algorithm>

svtkStandardNewMacro(svtkReduceTable);
//---------------------------------------------------------------------------
svtkReduceTable::svtkReduceTable()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->IndexColumn = -1;
  this->NumericalReductionMethod = svtkReduceTable::MEAN;
  this->NonNumericalReductionMethod = svtkReduceTable::MODE;
}

//---------------------------------------------------------------------------
svtkReduceTable::~svtkReduceTable() = default;

//---------------------------------------------------------------------------
int svtkReduceTable::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->IndexColumn == -1)
  {
    svtkWarningMacro(<< "Index column not set");
    return 1;
  }

  // Get input table
  svtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  svtkTable* input = svtkTable::SafeDownCast(inputInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (this->IndexColumn < 0 || this->IndexColumn > input->GetNumberOfColumns() - 1)
  {
    svtkWarningMacro(<< "Index column exceeds bounds of input table");
    return 1;
  }

  // Get output table
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkTable* output = svtkTable::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  this->InitializeOutputTable(input, output);
  this->AccumulateIndexValues(input);

  // set the number of rows in the output table
  output->SetNumberOfRows(static_cast<svtkIdType>(this->IndexValues.size()));

  this->PopulateIndexColumn(output);

  // populate the data columns of the output table
  for (svtkIdType col = 0; col < output->GetNumberOfColumns(); ++col)
  {
    if (col == this->IndexColumn)
    {
      continue;
    }

    this->PopulateDataColumn(input, output, col);
  }

  // Clean up pipeline information
  int piece = -1;
  int npieces = -1;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }
  output->GetInformation()->Set(svtkDataObject::DATA_NUMBER_OF_PIECES(), npieces);
  output->GetInformation()->Set(svtkDataObject::DATA_PIECE_NUMBER(), piece);

  return 1;
}

//---------------------------------------------------------------------------
void svtkReduceTable::InitializeOutputTable(svtkTable* input, svtkTable* output)
{
  output->DeepCopy(input);
  for (svtkIdType row = output->GetNumberOfRows() - 1; row > -1; --row)
  {
    output->RemoveRow(row);
  }
}

//---------------------------------------------------------------------------
void svtkReduceTable::AccumulateIndexValues(svtkTable* input)
{
  for (svtkIdType row = 0; row < input->GetNumberOfRows(); ++row)
  {
    svtkVariant value = input->GetValue(row, this->IndexColumn);
    this->IndexValues.insert(value);
    std::map<svtkVariant, std::vector<svtkIdType> >::iterator itr =
      this->NewRowToOldRowsMap.find(value);
    if (itr == this->NewRowToOldRowsMap.end())
    {
      std::vector<svtkIdType> v;
      v.push_back(row);
      this->NewRowToOldRowsMap[value] = v;
    }
    else
    {
      itr->second.push_back(row);
    }
  }
}

//---------------------------------------------------------------------------
void svtkReduceTable::PopulateIndexColumn(svtkTable* output)
{
  svtkIdType row = 0;
  for (std::set<svtkVariant>::iterator itr = this->IndexValues.begin();
       itr != this->IndexValues.end(); ++itr)
  {
    output->SetValue(row, this->IndexColumn, *itr);
    ++row;
  }
}

//---------------------------------------------------------------------------
void svtkReduceTable::PopulateDataColumn(svtkTable* input, svtkTable* output, svtkIdType col)
{
  int reductionMethod = 0;

  // check if this column has a reduction method
  int columnSpecificMethod = this->GetReductionMethodForColumn(col);
  if (columnSpecificMethod != -1)
  {
    reductionMethod = columnSpecificMethod;
  }
  else
  {
    // determine whether this column contains numerical or not.
    if (input->GetValue(0, col).IsNumeric())
    {
      reductionMethod = this->NumericalReductionMethod;
    }
    else
    {
      reductionMethod = this->NonNumericalReductionMethod;
    }
  }

  for (svtkIdType row = 0; row < output->GetNumberOfRows(); ++row)
  {
    // look up the cells in the input table that should be represented by
    // this cell in the output table
    svtkVariant indexValue = output->GetValue(row, this->IndexColumn);
    std::vector<svtkIdType> oldRows = this->NewRowToOldRowsMap[indexValue];

    // special case: one-to-one mapping between input table and output table
    // (no collapse necessary)
    if (oldRows.size() == 1)
    {
      output->SetValue(row, col, input->GetValue(this->NewRowToOldRowsMap[indexValue].at(0), col));
      continue;
    }

    // otherwise, combine them appropriately & store the value in the
    // output table
    switch (reductionMethod)
    {
      case svtkReduceTable::MODE:
        this->ReduceValuesToMode(input, output, row, col, oldRows);
        break;
      case svtkReduceTable::MEDIAN:
        this->ReduceValuesToMedian(input, output, row, col, oldRows);
        break;
      case svtkReduceTable::MEAN:
      default:
        this->ReduceValuesToMean(input, output, row, col, oldRows);
        break;
    }
  }
}

//---------------------------------------------------------------------------
void svtkReduceTable::ReduceValuesToMean(
  svtkTable* input, svtkTable* output, svtkIdType row, svtkIdType col, std::vector<svtkIdType> oldRows)
{
  if (!input->GetValue(0, col).IsNumeric())
  {
    svtkErrorMacro(<< "Mean is unsupported for non-numerical data");
    return;
  }

  double mean = 0.0;
  for (std::vector<svtkIdType>::iterator itr = oldRows.begin(); itr != oldRows.end(); ++itr)
  {
    mean += input->GetValue(*itr, col).ToDouble();
  }
  mean /= oldRows.size();
  output->SetValue(row, col, svtkVariant(mean));
}

//---------------------------------------------------------------------------
void svtkReduceTable::ReduceValuesToMedian(
  svtkTable* input, svtkTable* output, svtkIdType row, svtkIdType col, std::vector<svtkIdType> oldRows)
{
  if (!input->GetValue(0, col).IsNumeric())
  {
    svtkErrorMacro(<< "Median is unsupported for non-numerical data");
    return;
  }

  // generate a vector of values
  std::vector<double> values;
  for (std::vector<svtkIdType>::iterator itr = oldRows.begin(); itr != oldRows.end(); ++itr)
  {
    values.push_back(input->GetValue(*itr, col).ToDouble());
  }

  // sort it
  std::sort(values.begin(), values.end());

  // get the median and store it in the output table
  if (values.size() % 2 == 1)
  {
    output->SetValue(row, col, svtkVariant(values.at((values.size() - 1) / 2)));
  }
  else
  {
    double d1 = values.at((values.size() - 1) / 2);
    double d2 = values.at(values.size() / 2);
    output->SetValue(row, col, svtkVariant((d1 + d2) / 2.0));
  }
}

//---------------------------------------------------------------------------
void svtkReduceTable::ReduceValuesToMode(
  svtkTable* input, svtkTable* output, svtkIdType row, svtkIdType col, std::vector<svtkIdType> oldRows)
{
  // setup a map to determine how frequently each value appears
  std::map<svtkVariant, int> modeMap;
  std::map<svtkVariant, int>::iterator mapItr;
  for (std::vector<svtkIdType>::iterator vectorItr = oldRows.begin(); vectorItr != oldRows.end();
       ++vectorItr)
  {
    svtkVariant v = input->GetValue(*vectorItr, col);
    mapItr = modeMap.find(v);
    if (mapItr == modeMap.end())
    {
      modeMap[v] = 1;
    }
    else
    {
      mapItr->second += 1;
    }
  }

  // use our map to find the mode & store it in the output table
  int maxCount = -1;
  svtkVariant mode;
  for (mapItr = modeMap.begin(); mapItr != modeMap.end(); ++mapItr)
  {
    if (mapItr->second > maxCount)
    {
      mode = mapItr->first;
      maxCount = mapItr->second;
    }
  }
  output->SetValue(row, col, mode);
}

//---------------------------------------------------------------------------
int svtkReduceTable::GetReductionMethodForColumn(svtkIdType col)
{
  std::map<svtkIdType, int>::iterator itr = this->ColumnReductionMethods.find(col);
  if (itr != this->ColumnReductionMethods.end())
  {
    return itr->second;
  }
  return -1;
}

//---------------------------------------------------------------------------
void svtkReduceTable::SetReductionMethodForColumn(svtkIdType col, int method)
{
  this->ColumnReductionMethods[col] = method;
}

//---------------------------------------------------------------------------
void svtkReduceTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IndexColumn: " << this->IndexColumn << endl;
  os << indent << "NumericalReductionMethod: " << this->NumericalReductionMethod << endl;
  os << indent << "NonNumericalReductionMethod: " << this->NonNumericalReductionMethod << endl;
}
