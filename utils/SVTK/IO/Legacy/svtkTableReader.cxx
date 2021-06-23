/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTableReader.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkTableReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
svtkTableReader::svtkTableReader() = default;

//----------------------------------------------------------------------------
svtkTableReader::~svtkTableReader() = default;

//----------------------------------------------------------------------------
svtkTable* svtkTableReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkTable* svtkTableReader::GetOutput(int idx)
{
  return svtkTable::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkTableReader::SetOutput(svtkTable* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//-----------------------------------------------------------------------------
int svtkTableReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkDebugMacro(<< "Reading svtk table...");

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader())
  {
    return 1;
  }

  // Read table-specific stuff
  char line[256];
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 1;
  }

  if (strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    svtkErrorMacro(<< "Unrecognized keyword: " << line);
    this->CloseSVTKFile();
    return 1;
  }

  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 1;
  }

  if (strncmp(this->LowerCase(line), "table", 5))
  {
    svtkErrorMacro(<< "Cannot read dataset type: " << line);
    this->CloseSVTKFile();
    return 1;
  }

  svtkTable* const output = svtkTable::SafeDownCast(doOutput);

  while (true)
  {
    if (!this->ReadString(line))
    {
      break;
    }

    if (!strncmp(this->LowerCase(line), "field", 5))
    {
      svtkFieldData* const field_data = this->ReadFieldData();
      output->SetFieldData(field_data);
      field_data->Delete();
      continue;
    }

    if (!strncmp(this->LowerCase(line), "row_data", 8))
    {
      svtkIdType row_count = 0;
      if (!this->Read(&row_count))
      {
        svtkErrorMacro(<< "Cannot read number of rows!");
        this->CloseSVTKFile();
        return 1;
      }

      this->ReadRowData(output, row_count);
      continue;
    }

    svtkErrorMacro(<< "Unrecognized keyword: " << line);
  }

  svtkDebugMacro(<< "Read " << output->GetNumberOfRows() << " rows in "
                << output->GetNumberOfColumns() << " columns.\n");

  this->CloseSVTKFile();

  return 1;
}

//----------------------------------------------------------------------------
int svtkTableReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTable");
  return 1;
}

//----------------------------------------------------------------------------
void svtkTableReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
