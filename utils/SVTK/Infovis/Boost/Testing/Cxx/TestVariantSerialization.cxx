/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVariantSerialization.cxx

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
#include "svtkSmartPointer.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"
#include "svtkVariantBoostSerialization.h"

#include <sstream>
#include <string.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

int TestVariantSerialization(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int errors = 0;

  // Build a svtkVariantArray with a variety of values in it.
  svtkSmartPointer<svtkVariantArray> sourceArray = svtkSmartPointer<svtkVariantArray>::New();
  sourceArray->SetName("Values");
  sourceArray->SetNumberOfTuples(7);
  sourceArray->SetValue(0, svtkVariant('V'));
  sourceArray->SetValue(1, 3.14f);
  sourceArray->SetValue(2, 2.71);
  sourceArray->SetValue(3, "Test string");
  sourceArray->SetValue(4, 17);
  sourceArray->SetValue(5, 42l);

  // This is the first two words from the Iliad in Greek -- approximately 'Mnviv aeide'
  const svtkTypeUInt16 greek_text_utf16[] = { 0x039C, 0x03B7, 0x03BD, 0x03B9, 0x03BD, ' ', 0x03B1,
    0x03B5, 0x03B9, 0x03B4, 0x03B5, 0 };

  sourceArray->SetValue(6, svtkUnicodeString::from_utf16(greek_text_utf16));

  // Serialize the array
  std::ostringstream out_stream;
  boost::archive::text_oarchive out(out_stream);
  out << static_cast<const svtkVariantArray&>(*sourceArray);

  // De-serialize the array
  svtkSmartPointer<svtkVariantArray> sinkArray = svtkSmartPointer<svtkVariantArray>::New();
  std::istringstream in_stream(out_stream.str());
  boost::archive::text_iarchive in(in_stream);
  in >> *sinkArray;

  // Check that the arrays are the same
  if (strcmp(sourceArray->GetName(), sinkArray->GetName()))
  {
    cerr << "Sink array has name \"" << sinkArray->GetName() << "\", should be \""
         << sourceArray->GetName() << "\".\n";
    ++errors;
  }

  if (sourceArray->GetNumberOfTuples() != sinkArray->GetNumberOfTuples())
  {
    cerr << "Sink array has " << sinkArray->GetNumberOfTuples() << " of elements, should be "
         << sourceArray->GetNumberOfTuples() << ".\n";
    ++errors;
    return errors;
  }

  for (svtkIdType i = 0; i < sourceArray->GetNumberOfTuples(); ++i)
  {
    if (sourceArray->GetValue(i).GetType() != sinkArray->GetValue(i).GetType())
    {
      cerr << "Sink array value at index " << i << " has type " << sinkArray->GetValue(i).GetType()
           << ", should be" << sourceArray->GetValue(i).GetType() << ".\n";
      ++errors;
      return errors;
    }
  }

#define SVTK_VARIANT_ARRAY_DATA_CHECK(Index, Function, Kind)                                        \
  if (sourceArray->GetValue(Index).Function() != sinkArray->GetValue(Index).Function())            \
  {                                                                                                \
    cerr << Kind << " mismatch: \"" << sourceArray->GetValue(Index).Function() << "\" vs. \""      \
         << sinkArray->GetValue(Index).Function() << "\".\n";                                      \
    ++errors;                                                                                      \
  }

  SVTK_VARIANT_ARRAY_DATA_CHECK(0, ToChar, "Character");
  SVTK_VARIANT_ARRAY_DATA_CHECK(1, ToFloat, "Float");
  SVTK_VARIANT_ARRAY_DATA_CHECK(2, ToDouble, "Double");
  if (strcmp(sourceArray->GetValue(3).ToString(), sinkArray->GetValue(3).ToString()))
  {
    cerr << "String mismatch: \"" << sourceArray->GetValue(3).ToString() << "\" vs. \""
         << sinkArray->GetValue(3).ToString() << "\".\n";
    ++errors;
  }
  SVTK_VARIANT_ARRAY_DATA_CHECK(4, ToInt, "Int");
  SVTK_VARIANT_ARRAY_DATA_CHECK(5, ToLong, "Long");
#undef SVTK_VARIANT_ARRAY_DATA_CHECK

  return errors;
}
