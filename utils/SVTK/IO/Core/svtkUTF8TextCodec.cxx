/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkUTF8TextCodec.cxx

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

#include "svtkUTF8TextCodec.h"

#include "svtkObjectFactory.h"
#include "svtkTextCodecFactory.h"

#include <svtk_utf8.h>

svtkStandardNewMacro(svtkUTF8TextCodec);

bool svtkUTF8TextCodec::CanHandle(const char* testStr)
{
  if (0 == strcmp(testStr, "UTF-8"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

namespace
{
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

bool svtkUTF8TextCodec::IsValid(istream& InputStream)
{
  bool returnBool = true;
  // get the position of the stream so we can restore it when we are done
  istream::pos_type StreamPos = InputStream.tellg();

  try
  {
    testIterator junk;
    this->ToUnicode(InputStream, junk);
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

void svtkUTF8TextCodec::ToUnicode(istream& InputStream, svtkTextCodec::OutputIterator& Output)
{
  try
  {
    while (!InputStream.eof())
    {
      svtkUnicodeString::value_type CodePoint = this->NextUnicode(InputStream);
      *Output++ = CodePoint;
    }
  }
  catch (std::string& ef)
  {
    if (ef == "End of Input")
    {
      return; // we just completed the sequence...
    }
    else
    {
      throw;
    }
  }
}

svtkUnicodeString::value_type svtkUTF8TextCodec::NextUnicode(istream& InputStream)
{
  istream::char_type c[5];
  c[4] = '\0';

  unsigned int getSize = 0;
  c[getSize] = InputStream.get();
  if (InputStream.fail())
  {
    throw(std::string("End of Input"));
  }

  getSize = utf8::internal::sequence_length(c);

  if (0 == getSize)
    throw(std::string("Not enough space"));

  for (unsigned int i = 1; i < getSize; ++i)
  {
    c[i] = InputStream.get();
    if (InputStream.fail())
      throw(std::string("Not enough space"));
  }

  istream::char_type* c1 = c;

  const svtkTypeUInt32 code_point = utf8::next(c1, &c[getSize]);

  return code_point;
}

svtkUTF8TextCodec::svtkUTF8TextCodec()
  : svtkTextCodec()
{
}

svtkUTF8TextCodec::~svtkUTF8TextCodec() = default;

void svtkUTF8TextCodec::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "svtkUTF8TextCodec (" << this << ") \n";
  indent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
