/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostSplitTableField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBoostSplitTableField.h"

#include "svtkAbstractArray.h"
#include "svtkCommand.h"
#include "svtkDataSetAttributes.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>

svtkStandardNewMacro(svtkBoostSplitTableField);

/// Ecapsulates private implementation details of svtkBoostSplitTableField
class svtkBoostSplitTableField::implementation
{
public:
  typedef boost::char_separator<char> delimiter_t;
  typedef boost::tokenizer<delimiter_t> tokenizer_t;
  typedef std::vector<tokenizer_t*> tokenizers_t;

  static void GenerateRows(const tokenizers_t& tokenizers, const unsigned int column_index,
    svtkVariantArray* input_row, svtkVariantArray* output_row, svtkTable* output_table)
  {
    if (column_index == tokenizers.size())
    {
      output_table->InsertNextRow(output_row);
      return;
    }

    tokenizer_t* const tokenizer = tokenizers[column_index];
    svtkVariant input_value = input_row->GetValue(column_index);

    if (tokenizer && input_value.IsString())
    {
      const std::string value = input_value.ToString();
      tokenizer->assign(value);
      for (tokenizer_t::iterator token = tokenizer->begin(); token != tokenizer->end(); ++token)
      {
        output_row->SetValue(column_index, boost::trim_copy(*token).c_str());
        GenerateRows(tokenizers, column_index + 1, input_row, output_row, output_table);
      }
    }
    else
    {
      output_row->SetValue(column_index, input_value);
      GenerateRows(tokenizers, column_index + 1, input_row, output_row, output_table);
    }
  }
};

svtkBoostSplitTableField::svtkBoostSplitTableField()
  : Fields(svtkStringArray::New())
  , Delimiters(svtkStringArray::New())
{
}

svtkBoostSplitTableField::~svtkBoostSplitTableField()
{
  this->Delimiters->Delete();
  this->Fields->Delete();
}

void svtkBoostSplitTableField::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkBoostSplitTableField::ClearFields()
{
  this->Fields->Initialize();
  this->Delimiters->Initialize();
  this->Modified();
}

void svtkBoostSplitTableField::AddField(const char* field, const char* delimiters)
{
  assert(field);
  assert(delimiters);

  this->Fields->InsertNextValue(field);
  this->Delimiters->InsertNextValue(delimiters);

  this->Modified();
}

int svtkBoostSplitTableField::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTable* const input = svtkTable::GetData(inputVector[0]);
  svtkTable* const output = svtkTable::GetData(outputVector);

  // If the number of fields and delimiters don't match, we're done ...
  if (this->Fields->GetNumberOfValues() != this->Delimiters->GetNumberOfValues())
  {
    svtkErrorMacro("The number of fields and the number of delimiters must match");
    return 0;
  }

  // If no fields were specified, we don't do any splitting - just shallow copy ...
  if (0 == this->Fields->GetNumberOfValues())
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Setup the columns for our output table ...
  for (svtkIdType i = 0; i < input->GetNumberOfColumns(); ++i)
  {
    svtkAbstractArray* const column = input->GetColumn(i);
    svtkAbstractArray* const new_column = svtkAbstractArray::CreateArray(column->GetDataType());
    new_column->SetName(column->GetName());
    new_column->SetNumberOfComponents(column->GetNumberOfComponents());
    output->AddColumn(new_column);
    if (input->GetRowData()->GetPedigreeIds() == column)
    {
      output->GetRowData()->SetPedigreeIds(new_column);
    }
    new_column->Delete();
  }

  // Setup a tokenizer for each column that will be split ...
  implementation::tokenizers_t tokenizers;
  for (svtkIdType column = 0; column < input->GetNumberOfColumns(); ++column)
  {
    tokenizers.push_back(static_cast<implementation::tokenizer_t*>(0));

    for (svtkIdType field = 0; field < this->Fields->GetNumberOfValues(); ++field)
    {
      if (this->Fields->GetValue(field) == input->GetColumn(column)->GetName())
      {
        tokenizers[column] = new implementation::tokenizer_t(
          std::string(), implementation::delimiter_t(this->Delimiters->GetValue(field)));
        break;
      }
    }
  }

  // Iterate over each row in the input table, generating one-to-many rows in the output table ...
  svtkVariantArray* const output_row = svtkVariantArray::New();
  output_row->SetNumberOfValues(input->GetNumberOfColumns());

  for (svtkIdType i = 0; i < input->GetNumberOfRows(); ++i)
  {
    svtkVariantArray* const input_row = input->GetRow(i);
    implementation::GenerateRows(tokenizers, 0, input_row, output_row, output);

    double progress = static_cast<double>(i) / static_cast<double>(input->GetNumberOfRows());
    this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
  }

  output_row->Delete();

  // Cleanup tokenizers ...
  for (implementation::tokenizers_t::iterator tokenizer = tokenizers.begin();
       tokenizer != tokenizers.end(); ++tokenizer)
    delete *tokenizer;

  return 1;
}
