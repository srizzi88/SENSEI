/*=========================================================================

  Program:   ParaView
  Module:    svtkDelimitedTextWriter.cxx

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

#include "svtkDelimitedTextWriter.h"

#include "svtkAlgorithm.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtksys/FStream.hxx"

#include <sstream>
#include <vector>

svtkStandardNewMacro(svtkDelimitedTextWriter);
//-----------------------------------------------------------------------------
svtkDelimitedTextWriter::svtkDelimitedTextWriter()
{
  this->StringDelimiter = nullptr;
  this->FieldDelimiter = nullptr;
  this->UseStringDelimiter = true;
  this->SetStringDelimiter("\"");
  this->SetFieldDelimiter(",");
  this->Stream = nullptr;
  this->FileName = nullptr;
  this->WriteToOutputString = false;
  this->OutputString = nullptr;
}

//-----------------------------------------------------------------------------
svtkDelimitedTextWriter::~svtkDelimitedTextWriter()
{
  this->SetStringDelimiter(nullptr);
  this->SetFieldDelimiter(nullptr);
  this->SetFileName(nullptr);
  delete this->Stream;
  delete[] this->OutputString;
}

//-----------------------------------------------------------------------------
int svtkDelimitedTextWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
bool svtkDelimitedTextWriter::OpenStream()
{
  if (this->WriteToOutputString)
  {
    this->Stream = new std::ostringstream;
  }
  else
  {
    if (!this->FileName)
    {
      svtkErrorMacro(<< "No FileName specified! Can't write!");
      this->SetErrorCode(svtkErrorCode::NoFileNameError);
      return false;
    }

    svtkDebugMacro(<< "Opening file for writing...");

    svtksys::ofstream* fptr = new svtksys::ofstream(this->FileName, ios::out);

    if (fptr->fail())
    {
      svtkErrorMacro(<< "Unable to open file: " << this->FileName);
      this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
      delete fptr;
      return false;
    }

    this->Stream = fptr;
  }

  return true;
}

//-----------------------------------------------------------------------------
template <class iterT>
void svtkDelimitedTextWriterGetDataString(
  iterT* iter, svtkIdType tupleIndex, ostream* stream, svtkDelimitedTextWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  svtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if ((index + cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << iter->GetValue(index + cc);
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void svtkDelimitedTextWriterGetDataString(svtkArrayIteratorTemplate<svtkStdString>* iter,
  svtkIdType tupleIndex, ostream* stream, svtkDelimitedTextWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  svtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if ((index + cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << writer->GetString(iter->GetValue(index + cc));
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
svtkStdString svtkDelimitedTextWriter::GetString(svtkStdString string)
{
  if (this->UseStringDelimiter && this->StringDelimiter)
  {
    svtkStdString temp = this->StringDelimiter;
    temp += string + this->StringDelimiter;
    return temp;
  }
  return string;
}

//-----------------------------------------------------------------------------
void svtkDelimitedTextWriter::WriteData()
{
  svtkTable* rg = svtkTable::SafeDownCast(this->GetInput());
  if (rg)
  {
    this->WriteTable(rg);
  }
  else
  {
    svtkErrorMacro(<< "CSVWriter can only write svtkTable.");
  }
}

//-----------------------------------------------------------------------------
void svtkDelimitedTextWriter::WriteTable(svtkTable* table)
{
  svtkIdType numRows = table->GetNumberOfRows();
  svtkDataSetAttributes* dsa = table->GetRowData();
  if (!this->OpenStream())
  {
    return;
  }

  std::vector<svtkSmartPointer<svtkArrayIterator> > columnsIters;

  int cc;
  int numArrays = dsa->GetNumberOfArrays();
  bool first = true;
  // Write headers:
  for (cc = 0; cc < numArrays; cc++)
  {
    svtkAbstractArray* array = dsa->GetAbstractArray(cc);
    for (int comp = 0; comp < array->GetNumberOfComponents(); comp++)
    {
      if (!first)
      {
        (*this->Stream) << this->FieldDelimiter;
      }
      first = false;

      std::ostringstream array_name;
      array_name << array->GetName();
      if (array->GetNumberOfComponents() > 1)
      {
        array_name << ":" << comp;
      }
      (*this->Stream) << this->GetString(array_name.str());
    }
    svtkArrayIterator* iter = array->NewIterator();
    columnsIters.push_back(iter);
    iter->Delete();
  }
  (*this->Stream) << "\n";

  for (svtkIdType index = 0; index < numRows; index++)
  {
    first = true;
    std::vector<svtkSmartPointer<svtkArrayIterator> >::iterator iter;
    for (iter = columnsIters.begin(); iter != columnsIters.end(); ++iter)
    {
      switch ((*iter)->GetDataType())
      {
        svtkArrayIteratorTemplateMacro(svtkDelimitedTextWriterGetDataString(
          static_cast<SVTK_TT*>(iter->GetPointer()), index, this->Stream, this, &first));
        case SVTK_VARIANT:
        {
          svtkDelimitedTextWriterGetDataString(
            static_cast<svtkArrayIteratorTemplate<svtkVariant>*>(iter->GetPointer()), index,
            this->Stream, this, &first);
          break;
        }
      }
    }
    (*this->Stream) << "\n";
  }

  if (this->WriteToOutputString)
  {
    std::ostringstream* ostr = static_cast<std::ostringstream*>(this->Stream);

    delete[] this->OutputString;
    size_t strLen = ostr->str().size();
    this->OutputString = new char[strLen + 1];
    memcpy(this->OutputString, ostr->str().c_str(), strLen + 1);
  }
  delete this->Stream;
  this->Stream = nullptr;
}

//-----------------------------------------------------------------------------
char* svtkDelimitedTextWriter::RegisterAndGetOutputString()
{
  char* tmp = this->OutputString;
  this->OutputString = nullptr;

  return tmp;
}

//-----------------------------------------------------------------------------
void svtkDelimitedTextWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldDelimiter: " << (this->FieldDelimiter ? this->FieldDelimiter : "(none)")
     << endl;
  os << indent << "StringDelimiter: " << (this->StringDelimiter ? this->StringDelimiter : "(none)")
     << endl;
  os << indent << "UseStringDelimiter: " << this->UseStringDelimiter << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "none") << endl;
  os << indent << "WriteToOutputString: " << this->WriteToOutputString << endl;
}
