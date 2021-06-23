/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkArrayWriter.h"
#include "svtkArrayData.h"
#include "svtkArrayPrint.h"
#include "svtkDenseArray.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"
#include "svtkUnicodeString.h"
#include "svtksys/FStream.hxx"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace
{

template <typename T>
inline void WriteValue(std::ostream& stream, const T& value)
{
  stream << value;
}

inline void WriteValue(std::ostream& stream, const double& value)
{
  if (std::abs(value) < std::numeric_limits<double>::min())
    stream << 0;
  else
    stream << value;
}

inline void WriteValue(std::ostream& stream, const svtkStdString& value)
{
  stream << value;
}

inline void WriteValue(std::ostream& stream, const svtkUnicodeString& value)
{
  stream << value.utf8_str();
}

void WriteHeader(const svtkStdString& array_type, const svtkStdString& type_name, svtkArray* array,
  ostream& stream, bool WriteBinary)
{
  // Serialize the array type ...
  stream << array_type << " " << type_name << "\n";

  // Serialize output format, binary or ascii
  WriteBinary ? stream << "binary"
                       << "\n"
              : stream << "ascii"
                       << "\n";

  const svtkArrayExtents extents = array->GetExtents();
  const svtkIdType dimensions = array->GetDimensions();

  // Serialize the array name ...
  stream << array->GetName() << "\n";

  // Serialize the array extents and number of non-nullptr values ...
  for (svtkIdType i = 0; i != dimensions; ++i)
    stream << extents[i].GetBegin() << " " << extents[i].GetEnd() << " ";
  stream << array->GetNonNullSize() << "\n";

  // Serialize the dimension-label for each dimension ...
  for (svtkIdType i = 0; i != dimensions; ++i)
    stream << array->GetDimensionLabel(i) << "\n";
}

void WriteEndianOrderMark(ostream& stream)
{
  // Serialize an endian-order mark ...
  const svtkTypeUInt32 endian_order = 0x12345678;
  stream.write(reinterpret_cast<const char*>(&endian_order), sizeof(endian_order));
}

template <typename ValueT>
bool WriteSparseArrayBinary(const svtkStdString& type_name, svtkArray* array, ostream& stream)
{
  svtkSparseArray<ValueT>* const concrete_array = svtkSparseArray<ValueT>::SafeDownCast(array);
  if (!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("svtk-sparse-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array nullptr value ...
  stream.write(reinterpret_cast<const char*>(&concrete_array->GetNullValue()), sizeof(ValueT));

  // Serialize the array coordinates ...
  for (svtkIdType i = 0; i != array->GetDimensions(); ++i)
  {
    stream.write(reinterpret_cast<char*>(concrete_array->GetCoordinateStorage(i)),
      concrete_array->GetNonNullSize() * sizeof(svtkIdType));
  }

  // Serialize the array values ...
  stream.write(reinterpret_cast<char*>(concrete_array->GetValueStorage()),
    concrete_array->GetNonNullSize() * sizeof(ValueT));

  return true;
}

template <>
bool WriteSparseArrayBinary<svtkStdString>(
  const svtkStdString& type_name, svtkArray* array, ostream& stream)
{
  svtkSparseArray<svtkStdString>* const concrete_array =
    svtkSparseArray<svtkStdString>::SafeDownCast(array);
  if (!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("svtk-sparse-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array nullptr value ...
  stream.write(concrete_array->GetNullValue().c_str(), concrete_array->GetNullValue().size() + 1);

  // Serialize the array coordinates ...
  for (svtkIdType i = 0; i != array->GetDimensions(); ++i)
  {
    stream.write(reinterpret_cast<char*>(concrete_array->GetCoordinateStorage(i)),
      concrete_array->GetNonNullSize() * sizeof(svtkIdType));
  }

  // Serialize the array values as a set of packed, nullptr-terminated strings ...
  const svtkIdType value_count = array->GetNonNullSize();
  for (svtkIdType n = 0; n != value_count; ++n)
  {
    const svtkStdString& value = concrete_array->GetValueN(n);

    stream.write(value.c_str(), value.size() + 1);
  }

  return true;
}

template <>
bool WriteSparseArrayBinary<svtkUnicodeString>(
  const svtkStdString& type_name, svtkArray* array, ostream& stream)
{
  svtkSparseArray<svtkUnicodeString>* const concrete_array =
    svtkSparseArray<svtkUnicodeString>::SafeDownCast(array);
  if (!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("svtk-sparse-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array nullptr value ...
  stream.write(concrete_array->GetNullValue().utf8_str(),
    strlen(concrete_array->GetNullValue().utf8_str()) + 1);

  // Serialize the array coordinates ...
  for (svtkIdType i = 0; i != array->GetDimensions(); ++i)
  {
    stream.write(reinterpret_cast<char*>(concrete_array->GetCoordinateStorage(i)),
      concrete_array->GetNonNullSize() * sizeof(svtkIdType));
  }

  // Serialize the array values as a set of packed, nullptr-terminated strings ...
  const svtkIdType value_count = array->GetNonNullSize();
  for (svtkIdType n = 0; n != value_count; ++n)
  {
    const svtkUnicodeString& value = concrete_array->GetValueN(n);

    stream.write(value.utf8_str(), strlen(value.utf8_str()) + 1);
  }

  return true;
}

template <typename ValueT>
bool WriteDenseArrayBinary(const svtkStdString& type_name, svtkArray* array, ostream& stream)
{
  svtkDenseArray<ValueT>* const concrete_array = svtkDenseArray<ValueT>::SafeDownCast(array);
  if (!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("svtk-dense-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array values ...
  stream.write(reinterpret_cast<char*>(concrete_array->GetStorage()),
    concrete_array->GetNonNullSize() * sizeof(ValueT));

  return true;
}

template <>
bool WriteDenseArrayBinary<svtkStdString>(
  const svtkStdString& type_name, svtkArray* array, ostream& stream)
{
  svtkDenseArray<svtkStdString>* const concrete_array =
    svtkDenseArray<svtkStdString>::SafeDownCast(array);
  if (!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("svtk-dense-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array values as a set of packed, nullptr-terminated strings ...
  const svtkIdType value_count = array->GetNonNullSize();
  for (svtkIdType n = 0; n != value_count; ++n)
  {
    const svtkStdString& value = concrete_array->GetValueN(n);

    stream.write(value.c_str(), value.size() + 1);
  }

  return true;
}

template <>
bool WriteDenseArrayBinary<svtkUnicodeString>(
  const svtkStdString& type_name, svtkArray* array, ostream& stream)
{
  svtkDenseArray<svtkUnicodeString>* const concrete_array =
    svtkDenseArray<svtkUnicodeString>::SafeDownCast(array);
  if (!concrete_array)
    return false;

  // Write the array header ...
  WriteHeader("svtk-dense-array", type_name, array, stream, true);
  WriteEndianOrderMark(stream);

  // Serialize the array values as a set of packed, nullptr-terminated strings ...
  const svtkIdType value_count = array->GetNonNullSize();
  for (svtkIdType n = 0; n != value_count; ++n)
  {
    const svtkUnicodeString& value = concrete_array->GetValueN(n);

    stream.write(value.utf8_str(), strlen(value.utf8_str()) + 1);
  }

  return true;
}

template <typename ValueT>
bool WriteSparseArrayAscii(const svtkStdString& type_name, svtkArray* array, ostream& stream)
{
  svtkSparseArray<ValueT>* const concrete_array = svtkSparseArray<ValueT>::SafeDownCast(array);
  if (!concrete_array)
    return false;

  // Write the header ...
  WriteHeader("svtk-sparse-array", type_name, array, stream, false);

  // Ensure that floating-point types are serialized with full precision
  if (std::numeric_limits<ValueT>::is_specialized)
    stream.precision(std::numeric_limits<ValueT>::digits10 + 1);

  // Write the array nullptr value ...
  WriteValue(stream, concrete_array->GetNullValue());
  stream << "\n";

  // Write the array contents ...
  const svtkIdType dimensions = array->GetDimensions();
  const svtkIdType non_null_size = array->GetNonNullSize();

  svtkArrayCoordinates coordinates;
  for (svtkIdType n = 0; n != non_null_size; ++n)
  {
    array->GetCoordinatesN(n, coordinates);
    for (svtkIdType i = 0; i != dimensions; ++i)
      stream << coordinates[i] << " ";
    WriteValue(stream, concrete_array->GetValueN(n));
    stream << "\n";
  }

  return true;
}

template <typename ValueT>
bool WriteDenseArrayAscii(const svtkStdString& type_name, svtkArray* array, ostream& stream)
{
  svtkDenseArray<ValueT>* const concrete_array = svtkDenseArray<ValueT>::SafeDownCast(array);
  if (!concrete_array)
    return false;

  // Write the header ...
  WriteHeader("svtk-dense-array", type_name, array, stream, false);

  // Write the array contents ...
  const svtkArrayExtents extents = array->GetExtents();

  // Ensure that floating-point types are serialized with full precision
  if (std::numeric_limits<ValueT>::is_specialized)
    stream.precision(std::numeric_limits<ValueT>::digits10 + 1);

  svtkArrayCoordinates coordinates;
  for (svtkArrayExtents::SizeT n = 0; n != extents.GetSize(); ++n)
  {
    extents.GetRightToLeftCoordinatesN(n, coordinates);
    WriteValue(stream, concrete_array->GetValue(coordinates));
    stream << "\n";
  }

  return true;
}

} // End anonymous namespace

svtkStandardNewMacro(svtkArrayWriter);

svtkArrayWriter::svtkArrayWriter()
  : FileName(nullptr)
  , Binary(false)
  , WriteToOutputString(false)
{
}

svtkArrayWriter::~svtkArrayWriter()
{
  this->SetFileName(nullptr);
}

void svtkArrayWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Binary: " << this->Binary << endl;
  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "on" : "off") << endl;
  os << indent << "OutputString: " << this->OutputString << endl;
}

int svtkArrayWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkArrayData");
  return 1;
}

void svtkArrayWriter::WriteData()
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

int svtkArrayWriter::Write()
{
  return Superclass::Write();
}

bool svtkArrayWriter::Write(const svtkStdString& file_name, bool WriteBinary)
{
  svtksys::ofstream file(file_name.c_str(), std::ios::binary);
  return this->Write(file, WriteBinary);
}

bool svtkArrayWriter::Write(svtkArray* array, const svtkStdString& file_name, bool WriteBinary)
{
  svtksys::ofstream file(file_name.c_str(), std::ios::binary);
  return svtkArrayWriter::Write(array, file, WriteBinary);
}

bool svtkArrayWriter::Write(ostream& stream, bool WriteBinary)
{
  try
  {
    if (this->GetNumberOfInputConnections(0) != 1)
      throw std::runtime_error("Exactly one input required.");

    svtkArrayData* const array_data =
      svtkArrayData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
    if (!array_data)
      throw std::runtime_error("svtkArrayData input required.");

    if (array_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("svtkArrayData with exactly one array required.");

    svtkArray* const array = array_data->GetArray(static_cast<svtkIdType>(0));
    if (!array)
      throw std::runtime_error("Cannot serialize nullptr svtkArray.");

    return this->Write(array, stream, WriteBinary);
  }
  catch (std::exception& e)
  {
    svtkErrorMacro("caught exception: " << e.what());
  }
  return false;
}

bool svtkArrayWriter::Write(svtkArray* array, ostream& stream, bool WriteBinary)
{
  try
  {
    if (!array)
      throw std::runtime_error("Cannot serialize nullptr svtkArray.");

    if (WriteBinary)
    {
      if (WriteSparseArrayBinary<svtkIdType>("integer", array, stream))
        return true;
      if (WriteSparseArrayBinary<double>("double", array, stream))
        return true;
      if (WriteSparseArrayBinary<svtkStdString>("string", array, stream))
        return true;
      if (WriteSparseArrayBinary<svtkUnicodeString>("unicode-string", array, stream))
        return true;

      if (WriteDenseArrayBinary<svtkIdType>("integer", array, stream))
        return true;
      if (WriteDenseArrayBinary<double>("double", array, stream))
        return true;
      if (WriteDenseArrayBinary<svtkStdString>("string", array, stream))
        return true;
      if (WriteDenseArrayBinary<svtkUnicodeString>("unicode-string", array, stream))
        return true;
    }
    else
    {
      if (WriteSparseArrayAscii<svtkIdType>("integer", array, stream))
        return true;
      if (WriteSparseArrayAscii<double>("double", array, stream))
        return true;
      if (WriteSparseArrayAscii<svtkStdString>("string", array, stream))
        return true;
      if (WriteSparseArrayAscii<svtkUnicodeString>("unicode-string", array, stream))
        return true;

      if (WriteDenseArrayAscii<svtkIdType>("integer", array, stream))
        return true;
      if (WriteDenseArrayAscii<double>("double", array, stream))
        return true;
      if (WriteDenseArrayAscii<svtkStdString>("string", array, stream))
        return true;
      if (WriteDenseArrayAscii<svtkUnicodeString>("unicode-string", array, stream))
        return true;
    }

    throw std::runtime_error(std::string("Unhandled array type: ") + array->GetClassName());
  }
  catch (std::exception& e)
  {
    svtkGenericWarningMacro("caught exception: " << e.what());
  }
  return false;
}

svtkStdString svtkArrayWriter::Write(bool WriteBinary)
{
  std::ostringstream oss;
  this->Write(oss, WriteBinary);
  return oss.str();
}

svtkStdString svtkArrayWriter::Write(svtkArray* array, bool WriteBinary)
{
  std::ostringstream oss;
  svtkArrayWriter::Write(array, oss, WriteBinary);
  return oss.str();
}
