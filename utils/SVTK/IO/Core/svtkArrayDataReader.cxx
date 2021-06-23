/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkArrayDataReader.h"

#include "svtkArrayReader.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtksys/FStream.hxx"

#include <sstream>
#include <stdexcept>

svtkStandardNewMacro(svtkArrayDataReader);

svtkArrayDataReader::svtkArrayDataReader()
  : FileName(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->ReadFromInputString = false;
}

svtkArrayDataReader::~svtkArrayDataReader()
{
  this->SetFileName(nullptr);
}

void svtkArrayDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "InputString: " << this->InputString << endl;
  os << indent << "ReadFromInputString: " << (this->ReadFromInputString ? "on" : "off") << endl;
}

void svtkArrayDataReader::SetInputString(const svtkStdString& string)
{
  this->InputString = string;
  this->Modified();
}

svtkStdString svtkArrayDataReader::GetInputString()
{
  return this->InputString;
}

int svtkArrayDataReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  try
  {
    svtkArrayData* array_data = nullptr;
    if (this->ReadFromInputString)
    {
      array_data = this->Read(this->InputString);
    }
    else
    {
      if (!this->FileName)
        throw std::runtime_error("FileName not set.");

      svtksys::ifstream file(this->FileName, std::ios::binary);

      array_data = this->Read(file);
    }
    if (!array_data)
      throw std::runtime_error("Error reading svtkArrayData.");

    svtkArrayData* const output_array_data = svtkArrayData::GetData(outputVector);
    output_array_data->ShallowCopy(array_data);
    array_data->Delete();

    return 1;
  }
  catch (std::exception& e)
  {
    svtkErrorMacro(<< e.what());
  }

  return 0;
}

svtkArrayData* svtkArrayDataReader::Read(const svtkStdString& str)
{
  std::istringstream iss(str);
  return svtkArrayDataReader::Read(iss);
}

svtkArrayData* svtkArrayDataReader::Read(istream& stream)
{
  try
  {
    // Read enough of the file header to identify the type ...
    std::string header_string;
    std::getline(stream, header_string);
    std::istringstream header_buffer(header_string);

    std::string header_name;
    svtkIdType header_size;
    header_buffer >> header_name >> header_size;

    if (header_name != "svtkArrayData")
    {
      throw std::runtime_error("Not a svtkArrayData file");
    }
    if (header_size < 0)
    {
      throw std::runtime_error("Invalid number of arrays");
    }
    svtkArrayData* data = svtkArrayData::New();
    for (svtkIdType i = 0; i < header_size; ++i)
    {
      svtkArray* a = svtkArrayReader::Read(stream);
      data->AddArray(a);
      a->Delete();
    }
    return data;
  }
  catch (std::exception& e)
  {
    svtkGenericWarningMacro(<< e.what());
  }

  return nullptr;
}
