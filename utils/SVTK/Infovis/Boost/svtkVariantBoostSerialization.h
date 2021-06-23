/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVariantBoostSerialization.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
/**
 * @class   svtkVariantBoostSerialization
 * @brief   Serialization support for
 * svtkVariant and svtkVariantArray using the Boost.Serialization
 * library.
 *
 *
 * The header includes the templates required to serialize the
 * svtkVariant and svtkVariantArray with the Boost.Serialization
 * library. Just including the header suffices to get serialization
 * support; no other action is needed.
 */

#ifndef svtkVariantBoostSerialization_h
#define svtkVariantBoostSerialization_h

#include "svtkSetGet.h"
#include "svtkType.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

// This include fixes header-ordering issues in Boost.Serialization
// prior to Boost 1.35.0.
#include <boost/archive/binary_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/extended_type_info_no_rtti.hpp>
#include <boost/serialization/split_free.hpp>

//----------------------------------------------------------------------------
// svtkStdString serialization code
//----------------------------------------------------------------------------
template <typename Archiver>
void serialize(Archiver& ar, svtkStdString& str, const unsigned int svtkNotUsed(version))
{
  ar& boost::serialization::base_object<std::string>(str);
}

//----------------------------------------------------------------------------
// svtkUnicodeString serialization code
//----------------------------------------------------------------------------

template <typename Archiver>
void save(Archiver& ar, const svtkUnicodeString& str, const unsigned int svtkNotUsed(version))
{
  std::string utf8(str.utf8_str());
  ar& utf8;
}

template <typename Archiver>
void load(Archiver& ar, svtkUnicodeString& str, const unsigned int svtkNotUsed(version))
{
  std::string utf8;
  ar& utf8;
  str = svtkUnicodeString::from_utf8(utf8);
}

BOOST_SERIALIZATION_SPLIT_FREE(svtkUnicodeString)

//----------------------------------------------------------------------------
// svtkVariant serialization code
//----------------------------------------------------------------------------

template <typename Archiver>
void save(Archiver& ar, const svtkVariant& variant, const unsigned int svtkNotUsed(version))
{
  if (!variant.IsValid())
  {
    char null = 0;
    ar& null;
    return;
  }

  // Output the type
  char Type = variant.GetType();
  ar& Type;

  // Output the value
#define SVTK_VARIANT_SAVE(Value, Type, Function)                                                    \
  case Value:                                                                                      \
  {                                                                                                \
    Type value = variant.Function();                                                               \
    ar& value;                                                                                     \
  }                                                                                                \
    return

  switch (Type)
  {
    SVTK_VARIANT_SAVE(SVTK_STRING, svtkStdString, ToString);
    SVTK_VARIANT_SAVE(SVTK_UNICODE_STRING, svtkUnicodeString, ToUnicodeString);
    SVTK_VARIANT_SAVE(SVTK_FLOAT, float, ToFloat);
    SVTK_VARIANT_SAVE(SVTK_DOUBLE, double, ToDouble);
    SVTK_VARIANT_SAVE(SVTK_CHAR, char, ToChar);
    SVTK_VARIANT_SAVE(SVTK_UNSIGNED_CHAR, unsigned char, ToUnsignedChar);
    SVTK_VARIANT_SAVE(SVTK_SHORT, short, ToShort);
    SVTK_VARIANT_SAVE(SVTK_UNSIGNED_SHORT, unsigned short, ToUnsignedShort);
    SVTK_VARIANT_SAVE(SVTK_INT, int, ToInt);
    SVTK_VARIANT_SAVE(SVTK_UNSIGNED_INT, unsigned int, ToUnsignedInt);
    SVTK_VARIANT_SAVE(SVTK_LONG, long, ToLong);
    SVTK_VARIANT_SAVE(SVTK_UNSIGNED_LONG, unsigned long, ToUnsignedLong);
    SVTK_VARIANT_SAVE(SVTK_LONG_LONG, long long, ToLongLong);
    SVTK_VARIANT_SAVE(SVTK_UNSIGNED_LONG_LONG, unsigned long long, ToUnsignedLongLong);
    default:
      cerr << "cannot serialize variant with type " << variant.GetType() << '\n';
  }
#undef SVTK_VARIANT_SAVE
}

template <typename Archiver>
void load(Archiver& ar, svtkVariant& variant, const unsigned int svtkNotUsed(version))
{
  char Type;
  ar& Type;

#define SVTK_VARIANT_LOAD(Value, Type)                                                              \
  case Value:                                                                                      \
  {                                                                                                \
    Type value;                                                                                    \
    ar& value;                                                                                     \
    variant = svtkVariant(value);                                                                   \
  }                                                                                                \
    return

  switch (Type)
  {
    case 0:
      variant = svtkVariant();
      return;
      SVTK_VARIANT_LOAD(SVTK_STRING, svtkStdString);
      SVTK_VARIANT_LOAD(SVTK_UNICODE_STRING, svtkUnicodeString);
      SVTK_VARIANT_LOAD(SVTK_FLOAT, float);
      SVTK_VARIANT_LOAD(SVTK_DOUBLE, double);
      SVTK_VARIANT_LOAD(SVTK_CHAR, char);
      SVTK_VARIANT_LOAD(SVTK_UNSIGNED_CHAR, unsigned char);
      SVTK_VARIANT_LOAD(SVTK_SHORT, short);
      SVTK_VARIANT_LOAD(SVTK_UNSIGNED_SHORT, unsigned short);
      SVTK_VARIANT_LOAD(SVTK_INT, int);
      SVTK_VARIANT_LOAD(SVTK_UNSIGNED_INT, unsigned int);
      SVTK_VARIANT_LOAD(SVTK_LONG, long);
      SVTK_VARIANT_LOAD(SVTK_UNSIGNED_LONG, unsigned long);
      SVTK_VARIANT_LOAD(SVTK_LONG_LONG, long long);
      SVTK_VARIANT_LOAD(SVTK_UNSIGNED_LONG_LONG, unsigned long long);
    default:
      cerr << "cannot deserialize variant with type " << static_cast<unsigned int>(Type) << '\n';
      variant = svtkVariant();
  }
#undef SVTK_VARIANT_LOAD
}

BOOST_SERIALIZATION_SPLIT_FREE(svtkVariant)

//----------------------------------------------------------------------------
// svtkVariantArray serialization code
//----------------------------------------------------------------------------

template <typename Archiver>
void save(Archiver& ar, const svtkVariantArray& c_array, const unsigned int svtkNotUsed(version))
{
  svtkVariantArray& array = const_cast<svtkVariantArray&>(c_array);

  // Array name
  svtkStdString name;
  if (array.GetName() != nullptr)
    name = array.GetName();
  ar& name;

  // Array data
  svtkIdType n = array.GetNumberOfTuples();
  ar& n;
  for (svtkIdType i = 0; i < n; ++i)
  {
    ar& array.GetValue(i);
  }
}

template <typename Archiver>
void load(Archiver& ar, svtkVariantArray& array, const unsigned int svtkNotUsed(version))
{
  // Array name
  svtkStdString name;
  ar& name;
  array.SetName(name.c_str());

  if (name.empty())
  {
    array.SetName(0);
  }
  else
  {
    array.SetName(name.c_str());
  }

  // Array data
  svtkIdType n;
  ar& n;
  array.SetNumberOfTuples(n);
  svtkVariant value;
  for (svtkIdType i = 0; i < n; ++i)
  {
    ar& value;
    array.SetValue(i, value);
  }
}

BOOST_SERIALIZATION_SPLIT_FREE(svtkVariantArray)

#endif
// SVTK-HeaderTest-Exclude: svtkVariantBoostSerialization.h
