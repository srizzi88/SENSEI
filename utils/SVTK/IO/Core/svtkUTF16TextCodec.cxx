/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkUTF16TextCodec.cxx

Copyright (c)
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkUTF16TextCodec.h"

#include "svtkObjectFactory.h"
#include "svtkTextCodecFactory.h"

#include <stdexcept>

svtkStandardNewMacro(svtkUTF16TextCodec);

namespace
{
//////////////////////////////////////////////////////////////////////////////
// utf16_to_unicode

svtkTypeUInt32 utf16_to_unicode_next(const bool big_endian, istream& InputStream)
{
  svtkTypeUInt8 first_byte = InputStream.get();

  if (InputStream.eof())
  {
    throw std::runtime_error("Premature end-of-sequence extracting UTF-16 code unit.");
  }
  svtkTypeUInt8 second_byte = InputStream.get();

  svtkTypeUInt32 returnCode =
    big_endian ? first_byte << 8 | second_byte : second_byte << 8 | first_byte;

  if (returnCode >= 0xd800 && returnCode <= 0xdfff)
  {
    if (InputStream.eof())
    {
      throw std::runtime_error(
        "Premature end-of-sequence extracting UTF-16 trail surrogate first byte.");
    }
    svtkTypeUInt8 third_byte = InputStream.get();

    if (InputStream.eof())
    {
      throw std::runtime_error(
        "Premature end-of-sequence extracting UTF-16 trail surrogate second byte.");
    }
    svtkTypeUInt8 fourth_byte = InputStream.get();

    const svtkTypeUInt32 second_code_unit =
      big_endian ? third_byte << 8 | fourth_byte : fourth_byte << 8 | third_byte;
    if (second_code_unit >= 0xdc00 && second_code_unit <= 0xdfff)
    {
      returnCode = svtkTypeUInt32(svtkTypeInt32(returnCode << 10) + svtkTypeInt32(second_code_unit) +
        (0x10000 - (0xd800 << 10) - 0xdc00));
    }
    else
    {
      throw std::runtime_error("Invalid UTF-16 trail surrogate.");
    }
  }
  return returnCode;
}

void utf16_to_unicode(
  const bool big_endian, istream& InputStream, svtkTextCodec::OutputIterator& output)
{
  try
  {
    while (!InputStream.eof())
    {
      const svtkTypeUInt32 code_unit = utf16_to_unicode_next(big_endian, InputStream);
      *output++ = code_unit;
    }
  }
  catch (...)
  {
    if (!InputStream.eof())
      throw;
  }
}

// iterator to use in testing validity - throws all input away.
class testIterator : public svtkTextCodec::OutputIterator
{
public:
  testIterator& operator++(int) override { return *this; }
  testIterator& operator*() override { return *this; }
  testIterator& operator=(const svtkUnicodeString::value_type) override { return *this; }

  testIterator() = default;
  ~testIterator() override = default;

private:
  testIterator(const testIterator&) = delete;
  testIterator& operator=(const testIterator&) = delete;
};

} // end anonymous namespace

svtkUTF16TextCodec::svtkUTF16TextCodec()
  : svtkTextCodec()
  , _endianExplicitlySet(false)
  , _bigEndian(true)
{
}

svtkUTF16TextCodec::~svtkUTF16TextCodec() = default;

const char* svtkUTF16TextCodec::Name()
{
  return "UTF-16";
}

bool svtkUTF16TextCodec::CanHandle(const char* NameString)
{
  if (0 == strcmp(NameString, "UTF-16"))
  {
    _endianExplicitlySet = false;
    return true;
  }
  else if (0 == strcmp(NameString, "UTF-16BE"))
  {
    SetBigEndian(true);
    return true;
  }
  else if (0 == strcmp(NameString, "UTF-16LE"))
  {
    SetBigEndian(false);
    return true;
  }
  else
  {
    return false;
  }
}

void svtkUTF16TextCodec::SetBigEndian(bool state)
{
  _endianExplicitlySet = true;
  _bigEndian = state;
}

void svtkUTF16TextCodec::FindEndianness(istream& InputStream)
{
  _endianExplicitlySet = false;

  try
  {
    istream::char_type c1, c2;
    c1 = InputStream.get();
    if (InputStream.fail())
      throw "End of Input reached while reading header.";

    c2 = InputStream.get();
    if (InputStream.fail())
      throw "End of Input reached while reading header.";

    if (static_cast<unsigned char>(c1) == 0xfe && static_cast<unsigned char>(c2) == 0xff)
    {
      _bigEndian = true;
    }

    else if (static_cast<unsigned char>(c1) == 0xff && static_cast<unsigned char>(c2) == 0xfe)
    {
      _bigEndian = false;
    }

    else
    {
      throw std::runtime_error(
        "Cannot detect UTF-16 endianness.  Try 'UTF-16BE' or 'UTF-16LE' instead.");
    }
  }
  catch (char* cstr)
  {
    throw std::runtime_error(cstr);
  }
  catch (...)
  {
    throw std::runtime_error(
      "Cannot detect UTF-16 endianness.  Try 'UTF-16BE' or 'UTF-16LE' instead.");
  }
}

bool svtkUTF16TextCodec::IsValid(istream& InputStream)
{
  bool returnBool = true;
  // get the position of the stream so we can restore it when we are
  // done
  istream::pos_type StreamPos = InputStream.tellg();

  try
  {
    if (!_endianExplicitlySet)
    {
      FindEndianness(InputStream);
    }

    testIterator junk;
    utf16_to_unicode(_bigEndian, InputStream, junk);
  }
  catch (...)
  {
    returnBool = false;
  }

  // reset the stream
  InputStream.clear();
  InputStream.seekg(StreamPos);

  return returnBool;
}

void svtkUTF16TextCodec::ToUnicode(istream& InputStream, svtkTextCodec::OutputIterator& output)
{
  if (!_endianExplicitlySet)
  {
    FindEndianness(InputStream);
  }

  utf16_to_unicode(_bigEndian, InputStream, output);
}

svtkUnicodeString::value_type svtkUTF16TextCodec::NextUnicode(istream& InputStream)
{
  return utf16_to_unicode_next(_bigEndian, InputStream);
}

void svtkUTF16TextCodec::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "svtkUTF16TextCodec (" << this << ") \n";
  indent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
