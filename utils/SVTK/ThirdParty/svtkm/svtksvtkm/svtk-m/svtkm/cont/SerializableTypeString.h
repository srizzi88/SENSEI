//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_SerializableTypeString_h
#define svtk_m_cont_SerializableTypeString_h

#include <svtkm/Types.h>

#include <string>

namespace svtkm
{
namespace cont
{

/// \brief A traits class that gives a unique name for a type. This class
/// should be specialized for every type that has to be serialized by diy.
template <typename T>
struct SerializableTypeString
#ifdef SVTKM_DOXYGEN_ONLY
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "TypeName";
    return name;
  }
}
#endif
;

namespace internal
{

template <typename T, typename... Ts>
std::string GetVariadicSerializableTypeString(const T&, const Ts&... ts)
{
  return SerializableTypeString<T>::Get() + "," + GetVariadicSerializableTypeString(ts...);
}

template <typename T>
std::string GetVariadicSerializableTypeString(const T&)
{
  return SerializableTypeString<T>::Get();
}

} // internal

/// @cond SERIALIZATION
template <>
struct SerializableTypeString<svtkm::Int8>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "I8";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::UInt8>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "U8";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::Int16>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "I16";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::UInt16>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "U16";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::Int32>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "I32";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::UInt32>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "U32";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::Int64>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "I64";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::UInt64>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "U64";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::Float32>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "F32";
    return name;
  }
};

template <>
struct SerializableTypeString<svtkm::Float64>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "F64";
    return name;
  }
};

template <typename T, svtkm::IdComponent NumComponents>
struct SerializableTypeString<svtkm::Vec<T, NumComponents>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name =
      "V<" + SerializableTypeString<T>::Get() + "," + std::to_string(NumComponents) + ">";
    return name;
  }
};

template <typename T1, typename T2>
struct SerializableTypeString<svtkm::Pair<T1, T2>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "svtkm::Pair<" + SerializableTypeString<T1>::Get() + "," +
      SerializableTypeString<T2>::Get() + ">";
    return name;
  }
};
}
} // svtkm::cont
/// @endcond SERIALIZATION

#endif // svtk_m_cont_SerializableTypeString_h
