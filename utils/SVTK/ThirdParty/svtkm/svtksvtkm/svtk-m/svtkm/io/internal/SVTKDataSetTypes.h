//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_internal_SVTKDataSetTypes_h
#define svtk_m_io_internal_SVTKDataSetTypes_h

#include <svtkm/Types.h>

#include <algorithm>
#include <cassert>
#include <string>

namespace svtkm
{
namespace io
{
namespace internal
{

enum DataType
{
  DTYPE_UNKNOWN = 0,
  DTYPE_BIT,
  DTYPE_UNSIGNED_CHAR,
  DTYPE_CHAR,
  DTYPE_UNSIGNED_SHORT,
  DTYPE_SHORT,
  DTYPE_UNSIGNED_INT,
  DTYPE_INT,
  DTYPE_UNSIGNED_LONG,
  DTYPE_LONG,
  DTYPE_FLOAT,
  DTYPE_DOUBLE
};

inline const char* DataTypeString(int id)
{
  static const char* strings[] = {
    "",    "bit",           "unsigned_char", "char",  "unsigned_short", "short", "unsigned_int",
    "int", "unsigned_long", "long",          "float", "double"
  };
  return strings[id];
}

inline DataType DataTypeId(const std::string& str)
{
  DataType type = DTYPE_UNKNOWN;
  for (int id = 1; id < 12; ++id)
  {
    if (str == DataTypeString(id))
    {
      type = static_cast<DataType>(id);
    }
  }

  return type;
}

struct DummyBitType
{
  // Needs to work with streams' << operator
  operator bool() const { return false; }
};

class ColorChannel8
{
public:
  ColorChannel8()
    : Data()
  {
  }
  ColorChannel8(svtkm::UInt8 val)
    : Data(val)
  {
  }
  ColorChannel8(svtkm::Float32 val)
    : Data(static_cast<svtkm::UInt8>(std::min(std::max(val, 1.0f), 0.0f) * 255))
  {
  }
  operator svtkm::Float32() const { return static_cast<svtkm::Float32>(this->Data) / 255.0f; }
  operator svtkm::UInt8() const { return this->Data; }

private:
  svtkm::UInt8 Data;
};

inline std::ostream& operator<<(std::ostream& out, const ColorChannel8& val)
{
  return out << static_cast<svtkm::Float32>(val);
}

inline std::istream& operator>>(std::istream& in, ColorChannel8& val)
{
  svtkm::Float32 fval;
  in >> fval;
  val = ColorChannel8(fval);
  return in;
}

template <typename T>
struct DataTypeName
{
  static const char* Name() { return "unknown"; }
};
template <>
struct DataTypeName<DummyBitType>
{
  static const char* Name() { return "bit"; }
};
template <>
struct DataTypeName<svtkm::Int8>
{
  static const char* Name() { return "char"; }
};
template <>
struct DataTypeName<svtkm::UInt8>
{
  static const char* Name() { return "unsigned_char"; }
};
template <>
struct DataTypeName<svtkm::Int16>
{
  static const char* Name() { return "short"; }
};
template <>
struct DataTypeName<svtkm::UInt16>
{
  static const char* Name() { return "unsigned_short"; }
};
template <>
struct DataTypeName<svtkm::Int32>
{
  static const char* Name() { return "int"; }
};
template <>
struct DataTypeName<svtkm::UInt32>
{
  static const char* Name() { return "unsigned_int"; }
};
template <>
struct DataTypeName<svtkm::Int64>
{
  static const char* Name() { return "long"; }
};
template <>
struct DataTypeName<svtkm::UInt64>
{
  static const char* Name() { return "unsigned_long"; }
};
template <>
struct DataTypeName<svtkm::Float32>
{
  static const char* Name() { return "float"; }
};
template <>
struct DataTypeName<svtkm::Float64>
{
  static const char* Name() { return "double"; }
};

template <typename T, typename Functor>
inline void SelectVecTypeAndCall(T, svtkm::IdComponent numComponents, const Functor& functor)
{
  switch (numComponents)
  {
    case 1:
      functor(T());
      break;
    case 2:
      functor(svtkm::Vec<T, 2>());
      break;
    case 3:
      functor(svtkm::Vec<T, 3>());
      break;
    case 4:
      functor(svtkm::Vec<T, 4>());
      break;
    case 9:
      functor(svtkm::Vec<T, 9>());
      break;
    default:
      functor(numComponents, T());
      break;
  }
}

template <typename Functor>
inline void SelectTypeAndCall(DataType dtype,
                              svtkm::IdComponent numComponents,
                              const Functor& functor)
{
  switch (dtype)
  {
    case DTYPE_BIT:
      SelectVecTypeAndCall(DummyBitType(), numComponents, functor);
      break;
    case DTYPE_UNSIGNED_CHAR:
      SelectVecTypeAndCall(svtkm::UInt8(), numComponents, functor);
      break;
    case DTYPE_CHAR:
      SelectVecTypeAndCall(svtkm::Int8(), numComponents, functor);
      break;
    case DTYPE_UNSIGNED_SHORT:
      SelectVecTypeAndCall(svtkm::UInt16(), numComponents, functor);
      break;
    case DTYPE_SHORT:
      SelectVecTypeAndCall(svtkm::Int16(), numComponents, functor);
      break;
    case DTYPE_UNSIGNED_INT:
      SelectVecTypeAndCall(svtkm::UInt32(), numComponents, functor);
      break;
    case DTYPE_INT:
      SelectVecTypeAndCall(svtkm::Int32(), numComponents, functor);
      break;
    case DTYPE_UNSIGNED_LONG:
      SelectVecTypeAndCall(svtkm::UInt64(), numComponents, functor);
      break;
    case DTYPE_LONG:
      SelectVecTypeAndCall(svtkm::Int64(), numComponents, functor);
      break;
    case DTYPE_FLOAT:
      SelectVecTypeAndCall(svtkm::Float32(), numComponents, functor);
      break;
    case DTYPE_DOUBLE:
      SelectVecTypeAndCall(svtkm::Float64(), numComponents, functor);
      break;
    default:
      assert(false);
  }
}
}
}
} // namespace svtkm::io::internal

#endif // svtk_m_io_internal_SVTKDataSetTypes_h
