/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiProcessStream.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMultiProcessStream.h"

#include "svtkObjectFactory.h"
#include "svtkSocketCommunicator.h" // for svtkSwap8 and svtkSwap4 macros.
#include <cassert>
#include <deque>

class svtkMultiProcessStream::svtkInternals
{
public:
  typedef std::deque<unsigned char> DataType;
  DataType Data;

  enum Types
  {
    int32_value,
    uint32_value,
    char_value,
    uchar_value,
    double_value,
    float_value,
    string_value,
    int64_value,
    uint64_value,
    stream_value
  };

  void Push(const unsigned char* data, size_t length)
  {
    for (size_t cc = 0; cc < length; cc++)
    {
      this->Data.push_back(data[cc]);
    }
  }

  void Pop(unsigned char* data, size_t length)
  {
    for (size_t cc = 0; cc < length; cc++)
    {
      data[cc] = this->Data.front();
      this->Data.pop_front();
    }
  }

  void SwapBytes()
  {
    DataType::iterator iter = this->Data.begin();
    while (iter != this->Data.end())
    {
      unsigned char type = *iter;
      int wordSize = 1;
      ++iter;
      switch (type)
      {
        case int32_value:
        case uint32_value:
          wordSize = sizeof(int);
          break;

        case float_value:
          wordSize = sizeof(float);
          break;

        case double_value:
          wordSize = sizeof(double);
          break;

        case char_value:
        case uchar_value:
          wordSize = sizeof(char);
          break;

        case stream_value:
          wordSize = sizeof(int);
          break;

        case string_value:
          // We want to bitswap the string size which is an int
          wordSize = sizeof(int);
          break;
      }

      switch (wordSize)
      {
        case 1:
          break;
        case 4:
          svtkSwap4(&(*iter));
          break;
        case 8:
          svtkSwap8(&(*iter));
          break;
      }

      // In case of string we don't need to swap char values
      int nbSkip = 0;
      if (type == string_value || type == stream_value)
      {
        nbSkip = *reinterpret_cast<int*>(&*iter);
      }

      while (wordSize > 0)
      {
        ++iter;
        wordSize--;
      }

      // Skip String chars
      for (int cc = 0; cc < nbSkip; cc++)
      {
        ++iter;
      }
    }
  }
};

//----------------------------------------------------------------------------
svtkMultiProcessStream::svtkMultiProcessStream()
{
  this->Internals = new svtkMultiProcessStream::svtkInternals();
#ifdef SVTK_WORDS_BIGENDIAN
  this->Endianness = svtkMultiProcessStream::BigEndian;
#else
  this->Endianness = svtkMultiProcessStream::LittleEndian;
#endif
}

//----------------------------------------------------------------------------
svtkMultiProcessStream::~svtkMultiProcessStream()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
svtkMultiProcessStream::svtkMultiProcessStream(const svtkMultiProcessStream& other)
{
  this->Internals = new svtkMultiProcessStream::svtkInternals();
  this->Internals->Data = other.Internals->Data;
  this->Endianness = other.Endianness;
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator=(const svtkMultiProcessStream& other)
{
  this->Internals->Data = other.Internals->Data;
  this->Endianness = other.Endianness;
  return (*this);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Reset()
{
  this->Internals->Data.clear();
}

//----------------------------------------------------------------------------
int svtkMultiProcessStream::Size()
{
  return (static_cast<int>(this->Internals->Data.size()));
}

//----------------------------------------------------------------------------
bool svtkMultiProcessStream::Empty()
{
  return (this->Internals->Data.empty());
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Push(double array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(svtkInternals::double_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(reinterpret_cast<unsigned char*>(array), sizeof(double) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Push(float array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(svtkInternals::float_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(reinterpret_cast<unsigned char*>(array), sizeof(float) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Push(int array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(svtkInternals::int32_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(reinterpret_cast<unsigned char*>(array), sizeof(int) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Push(char array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(svtkInternals::char_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(reinterpret_cast<unsigned char*>(array), sizeof(char) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Push(unsigned int array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(svtkInternals::uint32_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(reinterpret_cast<unsigned char*>(array), sizeof(unsigned int) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Push(unsigned char array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(svtkInternals::uchar_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(array, size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Push(svtkTypeInt64 array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(svtkInternals::int64_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(reinterpret_cast<unsigned char*>(array), sizeof(svtkTypeUInt64) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Push(svtkTypeUInt64 array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(svtkInternals::uint64_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(reinterpret_cast<unsigned char*>(array), sizeof(svtkTypeUInt64) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Pop(double*& array, unsigned int& size)
{
  assert("pre: stream data must be double" &&
    this->Internals->Data.front() == svtkInternals::double_value);
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate array
    array = new double[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(reinterpret_cast<unsigned char*>(array), sizeof(double) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Pop(float*& array, unsigned int& size)
{
  assert(
    "pre: stream data must be float" && this->Internals->Data.front() == svtkInternals::float_value);
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate array
    array = new float[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(reinterpret_cast<unsigned char*>(array), sizeof(float) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Pop(int*& array, unsigned int& size)
{
  assert(
    "pre: stream data must be int" && this->Internals->Data.front() == svtkInternals::int32_value);
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate the array
    array = new int[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(reinterpret_cast<unsigned char*>(array), sizeof(int) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Pop(char*& array, unsigned int& size)
{
  assert("pre: stream data must be of type char" &&
    this->Internals->Data.front() == svtkInternals::char_value);
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate the array
    array = new char[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(reinterpret_cast<unsigned char*>(array), sizeof(char) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Pop(unsigned int*& array, unsigned int& size)
{
  assert("pre: stream data must be of type unsigned int" &&
    this->Internals->Data.front() == svtkInternals::uint32_value);
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate the array
    array = new unsigned int[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(reinterpret_cast<unsigned char*>(array), sizeof(unsigned int) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Pop(unsigned char*& array, unsigned int& size)
{
  assert("pre: stream data must be of type unsigned char" &&
    this->Internals->Data.front() == svtkInternals::uchar_value);
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate the array
    array = new unsigned char[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(array, size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Pop(svtkTypeInt64*& array, unsigned int& size)
{
  assert("pre: stream data must be of type svtkTypeInt64" &&
    this->Internals->Data.front() == svtkInternals::int64_value);
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate the array
    array = new svtkTypeInt64[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(reinterpret_cast<unsigned char*>(array), sizeof(svtkTypeInt64) * size);
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::Pop(svtkTypeUInt64*& array, unsigned int& size)
{
  assert("pre: stream data must be of type svtkTypeUInt64" &&
    this->Internals->Data.front() == svtkInternals::uint64_value);
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate the array
    array = new svtkTypeUInt64[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(reinterpret_cast<unsigned char*>(array), sizeof(svtkTypeUInt64) * size);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(double value)
{
  this->Internals->Data.push_back(svtkInternals::double_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(double));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(float value)
{
  this->Internals->Data.push_back(svtkInternals::float_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(float));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(int value)
{
  this->Internals->Data.push_back(svtkInternals::int32_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(int));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(char value)
{
  this->Internals->Data.push_back(svtkInternals::char_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(char));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(bool v)
{
  char value = v;
  this->Internals->Data.push_back(svtkInternals::char_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(char));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(unsigned int value)
{
  this->Internals->Data.push_back(svtkInternals::uint32_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned int));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(unsigned char value)
{
  this->Internals->Data.push_back(svtkInternals::uchar_value);
  this->Internals->Push(&value, sizeof(unsigned char));
  return (*this);
}

//-----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(svtkTypeInt64 value)
{
  this->Internals->Data.push_back(svtkInternals::int64_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(svtkTypeInt64));
  return (*this);
}

//-----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(svtkTypeUInt64 value)
{
  this->Internals->Data.push_back(svtkInternals::uint64_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(svtkTypeUInt64));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(const char* value)
{
  this->operator<<(std::string(value));
  return *this;
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(const std::string& value)
{
  // Find the real string size
  int size = static_cast<int>(value.size());

  // Set the type
  this->Internals->Data.push_back(svtkInternals::string_value);

  // Set the string size
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(int));

  // Set the string content
  for (int idx = 0; idx < size; idx++)
  {
    this->Internals->Push(reinterpret_cast<const unsigned char*>(&value[idx]), sizeof(char));
  }
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator<<(const svtkMultiProcessStream& value)
{
  unsigned int size = static_cast<unsigned int>(value.Internals->Data.size());
  size += 1;
  this->Internals->Data.push_back(svtkInternals::stream_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Data.push_back(value.Endianness);
  this->Internals->Data.insert(
    this->Internals->Data.end(), value.Internals->Data.begin(), value.Internals->Data.end());
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(svtkMultiProcessStream& value)
{
  assert(this->Internals->Data.front() == svtkInternals::stream_value);
  this->Internals->Data.pop_front();

  unsigned int size;
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

  assert(size >= 1);
  this->Internals->Pop(&value.Endianness, 1);
  size--;

  value.Internals->Data.resize(size);
  this->Internals->Pop(&value.Internals->Data[0], size);
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(double& value)
{
  assert(this->Internals->Data.front() == svtkInternals::double_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(double));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(float& value)
{
  assert(this->Internals->Data.front() == svtkInternals::float_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(float));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(int& value)
{
  // Automatically convert 64 bit values in case we are trying to transfer
  // svtkIdType with processes compiled with 32/64 values.
  if (this->Internals->Data.front() == svtkInternals::int64_value)
  {
    svtkTypeInt64 value64;
    (*this) >> value64;
    value = static_cast<int>(value64);
    return (*this);
  }
  assert(this->Internals->Data.front() == svtkInternals::int32_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(int));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(char& value)
{
  assert(this->Internals->Data.front() == svtkInternals::char_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(char));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(bool& v)
{
  char value;
  assert(this->Internals->Data.front() == svtkInternals::char_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(char));
  v = (value != 0);
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(unsigned int& value)
{
  assert(this->Internals->Data.front() == svtkInternals::uint32_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned int));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(unsigned char& value)
{
  assert(this->Internals->Data.front() == svtkInternals::uchar_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned char));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(svtkTypeInt64& value)
{
  // Automatically convert 64 bit values in case we are trying to transfer
  // svtkIdType with processes compiled with 32/64 values.
  if (this->Internals->Data.front() == svtkInternals::int32_value)
  {
    int value32;
    (*this) >> value32;
    value = value32;
    return (*this);
  }
  assert(this->Internals->Data.front() == svtkInternals::int64_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(svtkTypeInt64));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(svtkTypeUInt64& value)
{
  assert(this->Internals->Data.front() == svtkInternals::uint64_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(svtkTypeUInt64));
  return (*this);
}

//----------------------------------------------------------------------------
svtkMultiProcessStream& svtkMultiProcessStream::operator>>(std::string& value)
{
  value = "";
  assert(this->Internals->Data.front() == svtkInternals::string_value);
  this->Internals->Data.pop_front();
  int stringSize;
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&stringSize), sizeof(int));
  char c_value;
  for (int idx = 0; idx < stringSize; idx++)
  {
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&c_value), sizeof(char));
    value += c_value;
  }
  return (*this);
}

//----------------------------------------------------------------------------
std::vector<unsigned char> svtkMultiProcessStream::GetRawData() const
{
  std::vector<unsigned char> data;
  this->GetRawData(data);
  return data;
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::GetRawData(std::vector<unsigned char>& data) const
{
  data.clear();
  data.push_back(this->Endianness);
  data.resize(1 + this->Internals->Data.size());
  svtkInternals::DataType::iterator iter;
  int cc = 1;
  for (iter = this->Internals->Data.begin(); iter != this->Internals->Data.end(); ++iter, ++cc)
  {
    data[cc] = (*iter);
  }
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::SetRawData(const std::vector<unsigned char>& data)
{
  this->Internals->Data.clear();
  unsigned char endianness = data.front();
  std::vector<unsigned char>::const_iterator iter = data.begin();
  ++iter;
  this->Internals->Data.resize(data.size() - 1);
  int cc = 0;
  for (; iter != data.end(); ++iter, ++cc)
  {
    this->Internals->Data[cc] = *iter;
  }
  if (this->Endianness != endianness)
  {
    this->Internals->SwapBytes();
  }
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::GetRawData(unsigned char*& data, unsigned int& size)
{
  delete[] data;

  size = this->Size() + 1;
  data = new unsigned char[size + 1];
  assert("pre: cannot allocate raw data buffer" && (data != nullptr));

  data[0] = this->Endianness;
  svtkInternals::DataType::iterator iter = this->Internals->Data.begin();
  for (int idx = 1; iter != this->Internals->Data.end(); ++iter, ++idx)
  {
    data[idx] = *iter;
  }
}

//----------------------------------------------------------------------------
void svtkMultiProcessStream::SetRawData(const unsigned char* data, unsigned int size)
{
  this->Internals->Data.clear();
  if (size > 0)
  {
    unsigned char endianness = data[0];
    this->Internals->Data.resize(size - 1);
    int cc = 0;
    for (; cc < static_cast<int>(size - 1); cc++)
    {
      this->Internals->Data[cc] = data[cc + 1];
    }
    if (this->Endianness != endianness)
    {
      this->Internals->SwapBytes();
    }
  }
}
