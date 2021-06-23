/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBiomTableReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBiomTableReader.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"
#include "svtksys/FStream.hxx"

#include <algorithm>
#include <sstream>

svtkStandardNewMacro(svtkBiomTableReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
svtkBiomTableReader::svtkBiomTableReader()
{
  svtkTable* output = svtkTable::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
svtkBiomTableReader::~svtkBiomTableReader() = default;

//----------------------------------------------------------------------------
svtkTable* svtkBiomTableReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkTable* svtkBiomTableReader::GetOutput(int idx)
{
  return svtkTable::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::SetOutput(svtkTable* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int svtkBiomTableReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkDebugMacro(<< "Reading biom table...");

  if (fname.empty())
  {
    svtkErrorMacro(<< "Input filename not set");
    return 1;
  }

  svtksys::ifstream ifs(fname.c_str(), svtksys::ifstream::in);
  if (!ifs.good())
  {
    svtkErrorMacro(<< "Unable to open " << fname << " for reading");
    return 1;
  }

  ifs.seekg(0, std::ios::end);
  this->FileContents.reserve(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  this->FileContents.assign(
    (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

  this->ParseShape();
  this->ParseDataType();

  svtkNew<svtkStringArray> rowNames;
  rowNames->SetName("name");
  svtkTable* output = svtkTable::SafeDownCast(doOutput);
  output->AddColumn(rowNames);
  for (int i = 1; i < this->NumberOfColumns + 1; ++i)
  {
    switch (this->DataType)
    {
      case SVTK_INT:
      {
        svtkNew<svtkIntArray> intCol;
        output->AddColumn(intCol);
        break;
      }
      case SVTK_FLOAT:
      {
        svtkNew<svtkFloatArray> floatCol;
        output->AddColumn(floatCol);
        break;
      }
      case SVTK_STRING:
      {
        svtkNew<svtkStringArray> stringCol;
        output->AddColumn(stringCol);
        break;
      }
      default:
        break;
    }
  }
  output->SetNumberOfRows(this->NumberOfRows);

  // row names are stored in another column.  add it before the rest of the data
  this->ParseRows();

  this->ParseSparseness();
  if (this->Sparse)
  {
    this->InitializeData();
    this->ParseSparseData();
  }
  else
  {
    this->ParseDenseData();
  }
  this->ParseId();
  this->ParseColumns();

  return 1;
}

void svtkBiomTableReader::ParseShape()
{
  this->NumberOfRows = this->NumberOfColumns = -1;

  // find "shape":
  size_t pos1 = this->FileContents.find("\"shape\":");
  if (pos1 == std::string::npos)
  {
    svtkErrorMacro(<< "shape not found in input file");
    return;
  }

  // find [
  size_t pos2 = this->FileContents.find('[', pos1 + 1);
  if (pos2 == std::string::npos)
  {
    svtkErrorMacro(<< "shape field not formatted properly");
    return;
  }

  // find ,
  size_t pos3 = this->FileContents.find(',', pos2 + 1);
  if (pos3 == std::string::npos)
  {
    svtkErrorMacro(<< "shape field not formatted properly");
    return;
  }

  // find ]
  size_t pos4 = this->FileContents.find(']', pos3 + 1);
  if (pos4 == std::string::npos)
  {
    svtkErrorMacro(<< "shape field not formatted properly");
    return;
  }

  // number of rows is between "[" and ","
  this->NumberOfRows = atoi(this->FileContents.substr(pos2 + 1, pos3 - pos2).c_str());

  // number of columns is between "," and "]"
  this->NumberOfColumns = atoi(this->FileContents.substr(pos3 + 1, pos4 - pos3 - 1).c_str());
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::ParseDataType()
{
  // find "matrix_element_type":
  size_t pos1 = this->FileContents.find("\"matrix_element_type\":");
  if (pos1 == std::string::npos)
  {
    svtkErrorMacro(<< "matrix_element_type not found in input file");
    return;
  }

  // find :
  size_t pos2 = this->FileContents.find(':', pos1 + 1);
  if (pos2 == std::string::npos)
  {
    svtkErrorMacro(<< "matrix_element_type field not formatted properly");
    return;
  }

  // find first double quote
  size_t pos3 = this->FileContents.find('\"', pos2 + 1);
  if (pos3 == std::string::npos)
  {
    svtkErrorMacro(<< "matrix_element_type field not formatted properly");
    return;
  }

  // find second double quote
  size_t pos4 = this->FileContents.find('\"', pos3 + 1);
  if (pos4 == std::string::npos)
  {
    svtkErrorMacro(<< "matrix_element_type field not formatted properly");
    return;
  }

  // element type lies between these quotes
  std::string data_type = this->FileContents.substr(pos3 + 1, pos4 - pos3 - 1);
  if (strcmp(data_type.c_str(), "int") == 0)
  {
    this->DataType = SVTK_INT;
  }
  else if (strcmp(data_type.c_str(), "float") == 0)
  {
    this->DataType = SVTK_FLOAT;
  }
  else if (strcmp(data_type.c_str(), "unicode") == 0)
  {
    this->DataType = SVTK_STRING;
  }
  else
  {
    svtkErrorMacro(<< "unrecognized value found for matrix_element_type");
    this->DataType = SVTK_VOID;
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::InitializeData()
{
  switch (this->DataType)
  {
    case SVTK_INT:
    {
      int i = 0;
      svtkVariant v(i);
      this->FillData(v);
      break;
    }
    case SVTK_FLOAT:
    {
      float f = 0.0;
      svtkVariant v(f);
      this->FillData(v);
      break;
    }
    case SVTK_STRING:
    default:
    {
      std::string s;
      svtkVariant v(s);
      this->FillData(v);
      break;
    }
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::FillData(svtkVariant v)
{
  for (int row = 0; row < this->NumberOfRows; ++row)
  {
    for (int col = 1; col < this->NumberOfColumns + 1; ++col)
    {
      this->GetOutput()->SetValue(row, col, v);
    }
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::ParseSparseness()
{
  // find "matrix_type":
  size_t pos1 = this->FileContents.find("\"matrix_type\":");
  if (pos1 == std::string::npos)
  {
    svtkErrorMacro(<< "matrix_type not found in input file");
    return;
  }

  // find first double quote
  size_t pos2 = this->FileContents.find('\"', pos1 + 13);
  if (pos2 == std::string::npos)
  {
    svtkErrorMacro(<< "matrix_type field not formatted properly");
    return;
  }

  // find second double quote
  size_t pos3 = this->FileContents.find('\"', pos2 + 1);
  if (pos2 == std::string::npos)
  {
    svtkErrorMacro(<< "matrix_type field not formatted properly");
    return;
  }

  // We should find either 'sparse' or 'dense' between these quotes
  std::string matrix_type = this->FileContents.substr(pos2 + 1, pos3 - pos2 - 1);
  if (matrix_type.compare("sparse") == 0)
  {
    this->Sparse = true;
  }
  else if (matrix_type.compare("dense") == 0)
  {
    this->Sparse = false;
  }
  else
  {
    svtkErrorMacro(<< "matrix_type field not formatted properly");
    return;
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::ParseSparseData()
{
  // find "data":
  size_t pos1 = this->FileContents.find("\"data\":");
  if (pos1 == std::string::npos)
  {
    svtkErrorMacro(<< "data not found in input file");
    return;
  }

  // find first [ (beginning of matrix)
  size_t pos_start = this->FileContents.find('[', pos1) + 1;
  if (pos_start == std::string::npos)
  {
    svtkErrorMacro(<< "data field not formatted properly");
    return;
  }

  while (1)
  {
    // find [ (beginning of triplet)
    pos1 = this->FileContents.find('[', pos_start);
    if (pos1 == std::string::npos)
    {
      svtkErrorMacro(<< "data field not formatted properly");
      return;
    }
    // find first comma
    size_t pos2 = this->FileContents.find(',', pos1 + 1);
    if (pos2 == std::string::npos)
    {
      svtkErrorMacro(<< "data field not formatted properly");
      return;
    }
    // find second comma
    size_t pos3 = this->FileContents.find(',', pos2 + 1);
    if (pos3 == std::string::npos)
    {
      svtkErrorMacro(<< "data field not formatted properly");
      return;
    }
    // find ] (end of triplet)
    size_t pos4 = this->FileContents.find(']', pos3 + 1);
    if (pos4 == std::string::npos)
    {
      svtkErrorMacro(<< "data field not formatted properly");
      return;
    }
    // row is between "[" and ","
    int row = atoi(this->FileContents.substr(pos1 + 1, pos2 - pos1).c_str());

    // column is between first and second comma
    int column = 1 + atoi(this->FileContents.substr(pos2 + 1, pos3 - pos2 - 1).c_str());

    std::string value = this->FileContents.substr(pos3 + 1, pos4 - pos3 - 1);
    this->InsertValue(row, column, value);

    pos_start = pos4 + 1;
    if (strcmp(this->FileContents.substr(pos_start, 1).c_str(), ",") != 0)
    {
      return;
    }
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::ParseDenseData()
{
  // find "data":
  size_t pos1 = this->FileContents.find("\"data\":");
  if (pos1 == std::string::npos)
  {
    svtkErrorMacro(<< "data not found in input file");
    return;
  }

  // find first [ (beginning of matrix)
  size_t pos_start = this->FileContents.find('[', pos1) + 1;
  if (pos_start == std::string::npos)
  {
    svtkErrorMacro(<< "data field not formatted properly");
    return;
  }

  for (int currentRow = 0; currentRow < this->NumberOfRows; ++currentRow)
  {
    // find [ (beginning of row)
    size_t pos_row_1 = this->FileContents.find('[', pos_start);
    if (pos_row_1 == std::string::npos)
    {
      svtkErrorMacro(<< "data field not formatted properly");
      return;
    }
    int currentCol;
    for (currentCol = 1; currentCol < this->NumberOfColumns; ++currentCol)
    {
      // find next comma
      size_t pos_row_2 = this->FileContents.find(',', pos_row_1 + 1);
      if (pos_row_2 == std::string::npos)
      {
        svtkErrorMacro(<< "data field not formatted properly");
        return;
      }

      // parse data value and insert it into the table
      std::string value = this->FileContents.substr(pos_row_1 + 1, pos_row_2 - pos_row_1 - 1);
      this->InsertValue(currentRow, currentCol, value);

      pos_row_1 = pos_row_2;
    }

    // the last value of the row ends with a ] instead of a comma
    size_t pos_row_2 = this->FileContents.find(']', pos_row_1 + 1);
    if (pos_row_2 == std::string::npos)
    {
      svtkErrorMacro(<< "data field not formatted properly");
      return;
    }
    std::string value = this->FileContents.substr(pos_row_1 + 1, pos_row_2 - pos_row_1 - 1);
    this->InsertValue(currentRow, currentCol, value);
    pos_start = pos_row_2;
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::InsertValue(int row, int col, const std::string& value)
{
  std::stringstream stream;
  stream << value;

  switch (this->DataType)
  {
    case SVTK_INT:
      int i;
      if (!(stream >> i))
      {
        svtkErrorMacro(<< "error converting '" << value << " to integer");
      }
      else
      {
        svtkVariant v(i);
        this->GetOutput()->SetValue(row, col, v);
      }
      break;
    case SVTK_FLOAT:
      float f;
      if (!(stream >> f))
      {
        svtkErrorMacro(<< "error converting '" << value << " to float");
      }
      else
      {
        svtkVariant v(f);
        this->GetOutput()->SetValue(row, col, v);
      }
      break;
    case SVTK_STRING:
    default:
      svtkVariant v(value);
      this->GetOutput()->SetValue(row, col, v);
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::ParseColumns()
{
  // find "columns":
  size_t pos_start = this->FileContents.find("\"columns\":");
  if (pos_start == std::string::npos)
  {
    svtkErrorMacro(<< "columns not found in input file");
    return;
  }

  for (int currentCol = 1; currentCol < this->NumberOfColumns + 1; ++currentCol)
  {
    // find id for this column
    size_t pos_column_1 = this->FileContents.find("\"id\":", pos_start);
    if (pos_column_1 == std::string::npos)
    {
      svtkErrorMacro(<< "columns field not formatted properly");
      return;
    }
    size_t pos_column_2 = this->FileContents.find("\", \"metadata\":", pos_column_1);
    if (pos_column_2 == std::string::npos)
    {
      svtkErrorMacro(<< "columns field not formatted properly");
      return;
    }
    std::string name = this->FileContents.substr(pos_column_1 + 5, pos_column_2 - pos_column_1 - 5);
    // remove quotes
    name.erase(remove(name.begin(), name.end(), '\"'), name.end());
    // trim whitespace
    const size_t beginStr = name.find_first_not_of(" \t");
    const size_t endStr = name.find_last_not_of(" \t");
    const size_t range = endStr - beginStr + 1;
    name = name.substr(beginStr, range);
    this->GetOutput()->GetColumn(currentCol)->SetName(name.c_str());
    pos_start = pos_column_2;

    // this is where we would capture the metadata for this column
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::ParseRows()
{
  // find "rows":
  size_t pos_start = this->FileContents.find("\"rows\":");
  if (pos_start == std::string::npos)
  {
    svtkErrorMacro(<< "rows not found in input file");
    return;
  }

  for (int currentRow = 0; currentRow < this->NumberOfRows; ++currentRow)
  {
    // find id for this row
    size_t pos_row_1 = this->FileContents.find("\"id\":", pos_start);
    if (pos_row_1 == std::string::npos)
    {
      svtkErrorMacro(<< "rows field not formatted properly");
      return;
    }
    size_t pos_row_2 = this->FileContents.find("\", \"metadata\":", pos_row_1);
    if (pos_row_2 == std::string::npos)
    {
      svtkErrorMacro(<< "rows field not formatted properly");
      return;
    }
    std::string name = this->FileContents.substr(pos_row_1 + 5, pos_row_2 - pos_row_1 - 5);
    // remove quotes
    name.erase(remove(name.begin(), name.end(), '\"'), name.end());
    // trim whitespace
    const size_t beginStr = name.find_first_not_of(" \t");
    const size_t endStr = name.find_last_not_of(" \t");
    const size_t range = endStr - beginStr + 1;
    name = name.substr(beginStr, range);

    this->GetOutput()->SetValue(currentRow, 0, svtkVariant(name));
    pos_start = pos_row_2;

    // this is where we would capture the metadata for this row
  }
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::ParseId()
{
  bool done = false;
  size_t pos_start = 0;
  while (!done)
  {
    size_t pos_id = this->FileContents.find("\"id\":", pos_start);
    if (pos_id == std::string::npos)
    {
      svtkErrorMacro(<< "top-level id not found in input file");
      return;
    }

    // check that this is the toplevel id by matching preceding brackets
    std::string substring = this->FileContents.substr(0, pos_id);
    int numOpenBrackets = std::count(substring.begin(), substring.end(), '[');
    int numClosedBrackets = std::count(substring.begin(), substring.end(), ']');

    if (numOpenBrackets == numClosedBrackets)
    {
      size_t pos_comma = this->FileContents.find(',', pos_id + 1);
      if (pos_comma == std::string::npos)
      {
        svtkErrorMacro(<< "top-level id field not formatted properly");
        return;
      }
      std::string name = this->FileContents.substr(pos_id + 5, pos_comma - pos_id - 5);
      // remove trailing whitespace in our captured name
      size_t pos_id_begin = name.find_first_not_of(" \t");
      name = name.substr(pos_id_begin);
      // remove double quotes in the name
      name.erase(remove(name.begin(), name.end(), '\"'), name.end());
      done = true;
    }
    else
    {
      pos_start = pos_id + 5;
    }
  }
}

//----------------------------------------------------------------------------
int svtkBiomTableReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTable");
  return 1;
}

//----------------------------------------------------------------------------
void svtkBiomTableReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
