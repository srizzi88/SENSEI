/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayReader.cxx

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

#include "svtkArrayReader.h"

#include "svtkCommand.h"
#include "svtkDenseArray.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"
#include "svtkUnicodeString.h"
#include "svtksys/FStream.hxx"

#include <sstream>
#include <stdexcept>
#include <string>

svtkStandardNewMacro(svtkArrayReader);

namespace
{

template <typename ValueT>
void ExtractValue(istream& stream, ValueT& value)
{
  stream >> value;
}

void ExtractValue(istream& stream, svtkStdString& value)
{
  std::getline(stream, value);
  svtkStdString::size_type begin, end;
  begin = 0;
  end = value.size();
  while ((begin < end) && isspace(value[begin]))
    begin++;
  while ((begin < end) && isspace(value[end - 1]))
    end--;
  value = value.substr(begin, end);
}

void ExtractValue(istream& stream, svtkUnicodeString& value)
{
  std::string buffer;
  std::getline(stream, buffer);
  svtkStdString::size_type begin, end;
  begin = 0;
  end = buffer.size();
  while ((begin < end) && isspace(buffer[begin]))
    begin++;
  while ((begin < end) && isspace(buffer[end - 1]))
    end--;
  buffer = buffer.substr(begin, end);
  value = svtkUnicodeString::from_utf8(buffer);
}

void ReadHeader(
  istream& stream, svtkArrayExtents& extents, svtkArrayExtents::SizeT& non_null_size, svtkArray* array)
{
  if (!array)
    throw std::runtime_error("Missing array.");

  // Load the array name ...
  std::string name;
  std::getline(stream, name);
  array->SetName(name);

  // Load array extents ...
  std::string extents_string;
  std::getline(stream, extents_string);
  std::istringstream extents_buffer(extents_string);

  svtkArrayExtents::CoordinateT extent;
  std::vector<svtkArrayExtents::CoordinateT> temp_extents;
  for (extents_buffer >> extent; extents_buffer; extents_buffer >> extent)
    temp_extents.push_back(extent);

  extents.SetDimensions(0);
  while (temp_extents.size() > 1)
  {
    const svtkArrayExtents::CoordinateT begin = temp_extents.front();
    temp_extents.erase(temp_extents.begin());
    const svtkArrayExtents::CoordinateT end = temp_extents.front();
    temp_extents.erase(temp_extents.begin());
    extents.Append(svtkArrayRange(begin, end));
  }

  if (extents.GetDimensions() < 1)
    throw std::runtime_error("Array cannot have fewer than one dimension.");

  if (temp_extents.empty())
    throw std::runtime_error("Missing non null size.");

  non_null_size = temp_extents.back();

  array->Resize(extents);

  // Load dimension-labels ...
  for (svtkArrayExtents::DimensionT i = 0; i != extents.GetDimensions(); ++i)
  {
    std::string label;
    std::getline(stream, label);
    array->SetDimensionLabel(i, label);
  }
}

void ReadEndianOrderMark(istream& stream, bool& swap_endian)
{
  // Load the endian-order mark ...
  svtkTypeUInt32 endian_order = 0;
  stream.read(reinterpret_cast<char*>(&endian_order), sizeof(endian_order));

  swap_endian = endian_order == 0x12345678 ? false : true;
}

template <typename ValueT>
svtkSparseArray<ValueT>* ReadSparseArrayBinary(istream& stream)
{
  // Create the array ...
  svtkSmartPointer<svtkSparseArray<ValueT> > array = svtkSmartPointer<svtkSparseArray<ValueT> >::New();

  // Read the file header ...
  svtkArrayExtents extents;
  svtkArrayExtents::SizeT non_null_size = 0;
  bool swap_endian = false;
  ReadHeader(stream, extents, non_null_size, array);
  ReadEndianOrderMark(stream, swap_endian);

  // Read the array nullptr value ...
  ValueT null_value;
  stream.read(reinterpret_cast<char*>(&null_value), sizeof(ValueT));
  array->SetNullValue(null_value);

  // Read array coordinates ...
  array->ReserveStorage(non_null_size);

  for (svtkArray::DimensionT i = 0; i != array->GetDimensions(); ++i)
  {
    stream.read(reinterpret_cast<char*>(array->GetCoordinateStorage(i)),
      non_null_size * sizeof(svtkArray::CoordinateT));
  }

  // Read array values ...
  stream.read(reinterpret_cast<char*>(array->GetValueStorage()), non_null_size * sizeof(ValueT));

  array->Register(nullptr);
  return array;
}

template <>
svtkSparseArray<svtkStdString>* ReadSparseArrayBinary<svtkStdString>(istream& stream)
{
  // Create the array ...
  svtkSmartPointer<svtkSparseArray<svtkStdString> > array =
    svtkSmartPointer<svtkSparseArray<svtkStdString> >::New();

  // Read the file header ...
  svtkArrayExtents extents;
  svtkArrayExtents::SizeT non_null_size = 0;
  bool swap_endian = false;
  ReadHeader(stream, extents, non_null_size, array);
  ReadEndianOrderMark(stream, swap_endian);

  // Read the array nullptr value ...
  std::string null_value;
  for (int character = stream.get(); stream; character = stream.get())
  {
    if (character == 0)
    {
      array->SetNullValue(null_value);
      break;
    }
    else
    {
      null_value += static_cast<char>(character);
    }
  }

  // Read array coordinates ...
  array->ReserveStorage(non_null_size);

  for (svtkArray::DimensionT i = 0; i != array->GetDimensions(); ++i)
  {
    stream.read(reinterpret_cast<char*>(array->GetCoordinateStorage(i)),
      non_null_size * sizeof(svtkArray::CoordinateT));
  }

  // Read array values ...
  std::string buffer;
  svtkArray::SizeT n = 0;
  for (int character = stream.get(); stream; character = stream.get())
  {
    if (character == 0)
    {
      array->SetValueN(n++, buffer);
      buffer.resize(0);
    }
    else
    {
      buffer += static_cast<char>(character);
    }
  }

  array->Register(nullptr);
  return array;
}

template <>
svtkSparseArray<svtkUnicodeString>* ReadSparseArrayBinary<svtkUnicodeString>(istream& stream)
{
  // Create the array ...
  svtkSmartPointer<svtkSparseArray<svtkUnicodeString> > array =
    svtkSmartPointer<svtkSparseArray<svtkUnicodeString> >::New();

  // Read the file header ...
  svtkArrayExtents extents;
  svtkArrayExtents::SizeT non_null_size = 0;
  bool swap_endian = false;
  ReadHeader(stream, extents, non_null_size, array);
  ReadEndianOrderMark(stream, swap_endian);

  // Read the array nullptr value ...
  std::string null_value;
  for (int character = stream.get(); stream; character = stream.get())
  {
    if (character == 0)
    {
      array->SetNullValue(svtkUnicodeString::from_utf8(null_value));
      break;
    }
    else
    {
      null_value += static_cast<char>(character);
    }
  }

  // Read array coordinates ...
  array->ReserveStorage(non_null_size);

  for (svtkArray::DimensionT i = 0; i != array->GetDimensions(); ++i)
  {
    stream.read(reinterpret_cast<char*>(array->GetCoordinateStorage(i)),
      non_null_size * sizeof(svtkArray::CoordinateT));
  }

  // Read array values ...
  std::string buffer;
  svtkArray::SizeT n = 0;
  for (int character = stream.get(); stream; character = stream.get())
  {
    if (character == 0)
    {
      array->SetValueN(n++, svtkUnicodeString::from_utf8(buffer));
      buffer.resize(0);
    }
    else
    {
      buffer += static_cast<char>(character);
    }
  }

  array->Register(nullptr);
  return array;
}

template <typename ValueT>
svtkDenseArray<ValueT>* ReadDenseArrayBinary(istream& stream)
{
  // Create the array ...
  svtkSmartPointer<svtkDenseArray<ValueT> > array = svtkSmartPointer<svtkDenseArray<ValueT> >::New();

  // Read the file header ...
  svtkArrayExtents extents;
  svtkArrayExtents::SizeT non_null_size = 0;
  bool swap_endian = false;
  ReadHeader(stream, extents, non_null_size, array);
  ReadEndianOrderMark(stream, swap_endian);

  // Read array values ...
  stream.read(reinterpret_cast<char*>(array->GetStorage()), non_null_size * sizeof(ValueT));

  if (stream.eof())
    throw std::runtime_error("Premature end-of-file.");
  if (stream.bad())
    throw std::runtime_error("Error while reading file.");

  array->Register(nullptr);
  return array;
}

template <>
svtkDenseArray<svtkStdString>* ReadDenseArrayBinary<svtkStdString>(istream& stream)
{
  // Create the array ...
  svtkSmartPointer<svtkDenseArray<svtkStdString> > array =
    svtkSmartPointer<svtkDenseArray<svtkStdString> >::New();

  // Read the file header ...
  svtkArrayExtents extents;
  svtkArrayExtents::SizeT non_null_size = 0;
  bool swap_endian = false;
  ReadHeader(stream, extents, non_null_size, array);
  ReadEndianOrderMark(stream, swap_endian);

  // Read array values ...
  std::string buffer;
  svtkArray::SizeT n = 0;
  for (int character = stream.get(); stream; character = stream.get())
  {
    if (character == 0)
    {
      array->SetValueN(n++, buffer);
      buffer.resize(0);
    }
    else
    {
      buffer += static_cast<char>(character);
    }
  }

  array->Register(nullptr);
  return array;
}

template <>
svtkDenseArray<svtkUnicodeString>* ReadDenseArrayBinary<svtkUnicodeString>(istream& stream)
{
  // Create the array ...
  svtkSmartPointer<svtkDenseArray<svtkUnicodeString> > array =
    svtkSmartPointer<svtkDenseArray<svtkUnicodeString> >::New();

  // Read the file header ...
  svtkArrayExtents extents;
  svtkArrayExtents::SizeT non_null_size = 0;
  bool swap_endian = false;
  ReadHeader(stream, extents, non_null_size, array);
  ReadEndianOrderMark(stream, swap_endian);

  // Read array values ...
  std::string buffer;
  svtkArray::SizeT n = 0;
  for (int character = stream.get(); stream; character = stream.get())
  {
    if (character == 0)
    {
      array->SetValueN(n++, svtkUnicodeString::from_utf8(buffer));
      buffer.resize(0);
    }
    else
    {
      buffer += static_cast<char>(character);
    }
  }

  array->Register(nullptr);
  return array;
}

template <typename ValueT>
svtkSparseArray<ValueT>* ReadSparseArrayAscii(istream& stream)
{
  // Create the array ...
  svtkSmartPointer<svtkSparseArray<ValueT> > array = svtkSmartPointer<svtkSparseArray<ValueT> >::New();

  // Read the stream header ...
  svtkArrayExtents extents;
  svtkArrayExtents::SizeT non_null_size = 0;
  ReadHeader(stream, extents, non_null_size, array);

  if (non_null_size > extents.GetSize())
    throw std::runtime_error("Too many values for a sparse array.");

  // Read the array nullptr value ...
  std::string line_buffer;
  std::getline(stream, line_buffer);
  if (!stream)
    throw std::runtime_error("Premature end-of-stream reading nullptr value.");

  std::istringstream line_stream(line_buffer);
  ValueT null_value;
  ExtractValue(line_stream, null_value);
  if (!line_stream)
    throw std::runtime_error("Missing nullptr value.");
  array->SetNullValue(null_value);

  // Setup storage for the stream contents ...
  array->ReserveStorage(non_null_size);
  std::vector<svtkArray::CoordinateT*> coordinates(array->GetDimensions());
  for (svtkArray::DimensionT j = 0; j != array->GetDimensions(); ++j)
    coordinates[j] = array->GetCoordinateStorage(j);
  ValueT* value = array->GetValueStorage();

  // Read the stream contents ...
  svtkArray::SizeT value_count = 0;
  for (; value_count < non_null_size; ++value_count)
  {
    std::getline(stream, line_buffer);

    if (!stream)
      break;

    line_stream.clear();
    line_stream.str(line_buffer);

    for (svtkArray::DimensionT j = 0; j != array->GetDimensions(); ++j)
    {
      line_stream >> *(coordinates[j] + value_count);
      if (!extents[j].Contains(*(coordinates[j] + value_count)))
        throw std::runtime_error("Coordinate out-of-bounds.");
      if (!line_stream)
        throw std::runtime_error("Missing coordinate.");
    }

    ExtractValue(line_stream, *(value + value_count));
    if (!line_stream)
      throw std::runtime_error("Missing value.");
  }

  // Ensure we loaded enough values ...
  if (value_count != non_null_size)
    throw std::runtime_error("Stream doesn't contain enough values.");

  array->Register(nullptr);
  return array;
}

template <typename ValueT>
svtkDenseArray<ValueT>* ReadDenseArrayAscii(istream& stream)
{
  // Create the array ...
  svtkSmartPointer<svtkDenseArray<ValueT> > array = svtkSmartPointer<svtkDenseArray<ValueT> >::New();

  // Read the file header ...
  svtkArrayExtents extents;
  svtkArrayExtents::SizeT non_null_size = 0;
  ReadHeader(stream, extents, non_null_size, array);

  if (non_null_size != extents.GetSize())
    throw std::runtime_error("Incorrect number of values for a dense array.");

  // Read the file contents ...
  ValueT value;
  svtkArray::SizeT n = 0;
  svtkArrayCoordinates coordinates;
  for (; n < non_null_size; ++n)
  {
    ExtractValue(stream, value);
    if (!stream)
      break;
    extents.GetRightToLeftCoordinatesN(n, coordinates);
    array->SetValue(coordinates, value);
  }

  if (n != non_null_size)
    throw std::runtime_error("Stream doesn't contain enough values.");

  // If there is more in the stream (e.g. in svtkArrayDataReader),
  // eat the newline so the stream is ready for the next svtkArray.
  if (stream)
  {
    stream.get();
  }

  array->Register(nullptr);
  return array;
}

} // End anonymous namespace

svtkArrayReader::svtkArrayReader()
  : FileName(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->ReadFromInputString = false;
}

svtkArrayReader::~svtkArrayReader()
{
  this->SetFileName(nullptr);
}

void svtkArrayReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "InputString: " << this->InputString << endl;
  os << indent << "ReadFromInputString: " << (this->ReadFromInputString ? "on" : "off") << endl;
}

void svtkArrayReader::SetInputString(const svtkStdString& string)
{
  this->InputString = string;
  this->Modified();
}

svtkStdString svtkArrayReader::GetInputString()
{
  return this->InputString;
}

int svtkArrayReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  try
  {
    svtkArray* array = nullptr;
    if (this->ReadFromInputString)
    {
      array = this->Read(this->InputString);
    }
    else
    {
      if (!this->FileName)
        throw std::runtime_error("FileName not set.");

      svtksys::ifstream file(this->FileName, std::ios::binary);

      array = this->Read(file);
    }
    if (!array)
      throw std::runtime_error("Error reading array.");

    svtkArrayData* const array_data = svtkArrayData::GetData(outputVector);
    array_data->ClearArrays();
    array_data->AddArray(array);
    array->Delete();

    return 1;
  }
  catch (std::exception& e)
  {
    svtkErrorMacro(<< e.what());
  }

  return 0;
}

svtkArray* svtkArrayReader::Read(const svtkStdString& str)
{
  std::istringstream iss(str);
  return svtkArrayReader::Read(iss);
}

svtkArray* svtkArrayReader::Read(istream& stream)
{
  try
  {
    // Read enough of the file header to identify the type ...
    std::string header_string;
    std::getline(stream, header_string);
    std::istringstream header_buffer(header_string);

    std::string header_magic;
    std::string header_type;
    header_buffer >> header_magic >> header_type;

    // Read input file type, binary or ascii
    std::string header_file_string;
    std::string header_file_type;
    std::getline(stream, header_file_string);
    std::istringstream header_file_type_buffer(header_file_string);
    header_file_type_buffer >> header_file_type;

    bool read_binary = false;
    if (header_file_type == "binary")
    {
      read_binary = true;
    }
    else if (header_file_type != "ascii")
    {
      throw std::runtime_error("Unknown file type: " + header_file_type);
    }

    if (header_magic == "svtk-sparse-array")
    {
      if (header_type == "integer")
      {
        return (read_binary ? ReadSparseArrayBinary<svtkIdType>(stream)
                            : ReadSparseArrayAscii<svtkIdType>(stream));
      }
      else if (header_type == "double")
      {
        return (read_binary ? ReadSparseArrayBinary<double>(stream)
                            : ReadSparseArrayAscii<double>(stream));
      }
      else if (header_type == "string")
      {
        return (read_binary ? ReadSparseArrayBinary<svtkStdString>(stream)
                            : ReadSparseArrayAscii<svtkStdString>(stream));
      }
      else if (header_type == "unicode-string")
      {
        return (read_binary ? ReadSparseArrayBinary<svtkUnicodeString>(stream)
                            : ReadSparseArrayAscii<svtkUnicodeString>(stream));
      }
      else
      {
        throw std::runtime_error("Unknown array type: " + header_type);
      }
    }
    else if (header_magic == "svtk-dense-array")
    {
      if (header_type == "integer")
      {
        return (read_binary ? ReadDenseArrayBinary<svtkIdType>(stream)
                            : ReadDenseArrayAscii<svtkIdType>(stream));
      }
      else if (header_type == "double")
      {
        return (
          read_binary ? ReadDenseArrayBinary<double>(stream) : ReadDenseArrayAscii<double>(stream));
      }
      else if (header_type == "string")
      {
        return (read_binary ? ReadDenseArrayBinary<svtkStdString>(stream)
                            : ReadDenseArrayAscii<svtkStdString>(stream));
      }
      else if (header_type == "unicode-string")
      {
        return (read_binary ? ReadDenseArrayBinary<svtkUnicodeString>(stream)
                            : ReadDenseArrayAscii<svtkUnicodeString>(stream));
      }
      else
      {
        throw std::runtime_error("Unknown array type: " + header_type);
      }
    }
    else
    {
      throw std::runtime_error("Unknown file type: " + header_magic);
    }
  }
  catch (std::exception& e)
  {
    svtkGenericWarningMacro(<< e.what());
  }

  return nullptr;
}
