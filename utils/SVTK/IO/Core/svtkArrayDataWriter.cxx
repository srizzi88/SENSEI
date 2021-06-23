/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkArrayDataWriter.h"

#include "svtkArrayData.h"
#include "svtkArrayWriter.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtksys/FStream.hxx"

#include <sstream>
#include <stdexcept>

svtkStandardNewMacro(svtkArrayDataWriter);

svtkArrayDataWriter::svtkArrayDataWriter()
  : FileName(nullptr)
  , Binary(false)
  , WriteToOutputString(false)
{
}

svtkArrayDataWriter::~svtkArrayDataWriter()
{
  this->SetFileName(nullptr);
}

void svtkArrayDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Binary: " << this->Binary << endl;
  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "on" : "off") << endl;
  os << indent << "OutputString: " << this->OutputString << endl;
}

int svtkArrayDataWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkArrayData");
  return 1;
}

void svtkArrayDataWriter::WriteData()
{
  if (this->WriteToOutputString)
  {
    this->OutputString = this->Write(this->Binary > 0 ? true : false);
  }
  else
  {
    this->Write(this->FileName ? this->FileName : "", this->Binary > 0 ? true : false);
  }
}

int svtkArrayDataWriter::Write()
{
  return Superclass::Write();
}

bool svtkArrayDataWriter::Write(const svtkStdString& file_name, bool WriteBinary)
{
  svtksys::ofstream file(file_name.c_str(), std::ios::binary);
  return this->Write(file, WriteBinary);
}

bool svtkArrayDataWriter::Write(svtkArrayData* array, const svtkStdString& file_name, bool WriteBinary)
{
  svtksys::ofstream file(file_name.c_str(), std::ios::binary);
  return svtkArrayDataWriter::Write(array, file, WriteBinary);
}

bool svtkArrayDataWriter::Write(ostream& stream, bool WriteBinary)
{
  try
  {
    if (this->GetNumberOfInputConnections(0) != 1)
      throw std::runtime_error("Exactly one input required.");

    svtkArrayData* const array_data =
      svtkArrayData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
    if (!array_data)
      throw std::runtime_error("svtkArrayData input required.");

    this->Write(array_data, stream, WriteBinary);
    return true;
  }
  catch (std::exception& e)
  {
    svtkErrorMacro("caught exception: " << e.what());
  }
  return false;
}

bool svtkArrayDataWriter::Write(svtkArrayData* array_data, ostream& stream, bool WriteBinary)
{
  try
  {
    stream << "svtkArrayData " << array_data->GetNumberOfArrays() << std::endl;
    for (svtkIdType i = 0; i < array_data->GetNumberOfArrays(); ++i)
    {
      svtkArray* const array = array_data->GetArray(i);
      if (!array)
        throw std::runtime_error("Cannot serialize nullptr svtkArray.");

      svtkArrayWriter::Write(array, stream, WriteBinary);
    }
    return true;
  }
  catch (std::exception& e)
  {
    svtkGenericWarningMacro("caught exception: " << e.what());
  }
  return false;
}

svtkStdString svtkArrayDataWriter::Write(bool WriteBinary)
{
  std::ostringstream oss;
  this->Write(oss, WriteBinary);
  return oss.str();
}

svtkStdString svtkArrayDataWriter::Write(svtkArrayData* array_data, bool WriteBinary)
{
  std::ostringstream oss;
  svtkArrayDataWriter::Write(array_data, oss, WriteBinary);
  return oss.str();
}
