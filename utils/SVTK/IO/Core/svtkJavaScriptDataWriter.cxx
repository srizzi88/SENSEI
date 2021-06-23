/*=========================================================================

  Program:   ParaView
  Module:    svtkJavaScriptDataWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkJavaScriptDataWriter.h"

#include "svtkAlgorithm.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtksys/Encoding.hxx"
#include "svtksys/FStream.hxx"

#include <sstream>
#include <vector>

svtkStandardNewMacro(svtkJavaScriptDataWriter);
//-----------------------------------------------------------------------------
svtkJavaScriptDataWriter::svtkJavaScriptDataWriter()
{
  this->VariableName = nullptr;
  this->FileName = nullptr;
  this->IncludeFieldNames = true; // Default is to include field names
  this->OutputStream = nullptr;
  this->OutputFile = nullptr;
  this->SetVariableName("data"); // prepare the default.
}

//-----------------------------------------------------------------------------
svtkJavaScriptDataWriter::~svtkJavaScriptDataWriter()
{
  this->SetFileName(nullptr);
  this->SetVariableName(nullptr);
  this->CloseFile();
}

//-----------------------------------------------------------------------------
int svtkJavaScriptDataWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
void svtkJavaScriptDataWriter::SetOutputStream(ostream* output_stream)
{
  this->OutputStream = output_stream;
}

//-----------------------------------------------------------------------------
ostream* svtkJavaScriptDataWriter::GetOutputStream()
{
  return this->OutputStream;
}

//----------------------------------------------------------------------------
void svtkJavaScriptDataWriter::CloseFile()
{
  delete this->OutputFile;
  this->OutputFile = nullptr;
}

//-----------------------------------------------------------------------------
bool svtkJavaScriptDataWriter::OpenFile()
{
  if (!this->FileName)
  {
    svtkErrorMacro(<< "No FileName specified! Can't write!");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return false;
  }

  this->CloseFile();
  svtkDebugMacro(<< "Opening file for writing...");

  this->OutputFile = new svtksys::ofstream(this->FileName, ios::out);
  if (this->OutputFile->fail())
  {
    svtkErrorMacro(<< "Unable to open file: " << this->FileName);
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    this->CloseFile();
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
void svtkJavaScriptDataWriter::WriteData()
{
  svtkTable* input_table = svtkTable::SafeDownCast(this->GetInput());

  // Check for valid input
  if (!input_table)
  {
    svtkErrorMacro(<< "svtkJavaScriptDataWriter can only write svtkTable.");
    return;
  }

  // Check for filename
  if (this->FileName)
  {
    if (this->OpenFile())
    {
      this->WriteTable(input_table, this->OutputFile);
      this->CloseFile();
    }
  }
  else
  {
    this->WriteTable(input_table, this->OutputStream);
  }
}

//-----------------------------------------------------------------------------
void svtkJavaScriptDataWriter::WriteTable(svtkTable* table, ostream* stream_ptr)
{
  if (stream_ptr == nullptr)
  {
    if (this->FileName && this->OpenFile())
    {
      stream_ptr = this->OutputFile;
    }
  }

  if (stream_ptr != nullptr)
  {
    svtkIdType numRows = table->GetNumberOfRows();
    svtkIdType numCols = table->GetNumberOfColumns();
    svtkDataSetAttributes* dsa = table->GetRowData();

    svtkStdString rowHeader = "[";
    svtkStdString rowFooter = "],";
    if (this->IncludeFieldNames)
    {
      rowHeader = "{";
      rowFooter = "},";
    }

    // Header stuff
    if (this->VariableName)
    {
      (*stream_ptr) << "var " << this->VariableName << " = [\n";
    }
    else
    {
      (*stream_ptr) << "[";
    }

    for (svtkIdType r = 0; r < numRows; ++r)
    {
      // row header
      (*stream_ptr) << rowHeader;

      // Now for each column put out in the form
      // colname1: data1, colname2: data2, etc
      for (int c = 0; c < numCols; ++c)
      {
        if (this->IncludeFieldNames)
        {
          (*stream_ptr) << dsa->GetAbstractArray(c)->GetName() << ":";
        }

        // If the array is a string array put "" around it
        if (svtkArrayDownCast<svtkStringArray>(dsa->GetAbstractArray(c)))
        {
          (*stream_ptr) << "\"" << table->GetValue(r, c).ToString() << "\",";
        }
        else
        {
          (*stream_ptr) << table->GetValue(r, c).ToString() << ",";
        }
      }

      // row footer
      (*stream_ptr) << rowFooter;
    }

    // Footer
    (*stream_ptr) << (this->VariableName ? "];\n" : "]");
  }
}

//-----------------------------------------------------------------------------
void svtkJavaScriptDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VariableName: " << this->VariableName << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "none") << endl;
  os << indent << "IncludeFieldNames: " << (this->IncludeFieldNames ? "true" : "false") << endl;
}
