/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkASCIITextCodec.cxx

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

#include "svtkASCIITextCodec.h"

#include "svtkObjectFactory.h"
#include "svtkTextCodecFactory.h"

#include <stdexcept>

svtkStandardNewMacro(svtkASCIITextCodec);

const char* svtkASCIITextCodec::Name()
{
  return "US-ASCII";
}

bool svtkASCIITextCodec::CanHandle(const char* NameStr)
{
  if (0 == strcmp(NameStr, "US-ASCII") || 0 == strcmp(NameStr, "ASCII"))
    return true;
  else
    return false;
}

bool svtkASCIITextCodec::IsValid(istream& InputStream)
{
  bool returnBool = true;

  // get the position of the stream so we can restore it when we are done
  istream::pos_type StreamPos = InputStream.tellg();

  // check the code points for non-ascii characters
  while (!InputStream.eof())
  {
    svtkTypeUInt32 CodePoint = InputStream.get();

    if (!InputStream.eof() && CodePoint > 0x7f)
    {
      returnBool = false;
      break;
    }
  }

  // reset the stream
  InputStream.clear();
  InputStream.seekg(StreamPos);

  return returnBool;
}

void svtkASCIITextCodec::ToUnicode(istream& InputStream, svtkTextCodec::OutputIterator& output)
{
  while (!InputStream.eof())
  {
    svtkTypeUInt32 CodePoint = InputStream.get();

    if (!InputStream.eof())
    {
      if (CodePoint > 0x7f)
        throw std::runtime_error("Detected a character that isn't valid US-ASCII.");

      *output++ = CodePoint;
    }
  }
}

svtkUnicodeString::value_type svtkASCIITextCodec::NextUnicode(istream& InputStream)
{
  svtkTypeUInt32 CodePoint = InputStream.get();

  if (!InputStream.eof())
  {
    if (CodePoint > 0x7f)
      throw std::runtime_error("Detected a character that isn't valid US-ASCII.");

    return CodePoint;
  }
  else
    return 0;
}

svtkASCIITextCodec::svtkASCIITextCodec()
  : svtkTextCodec()
{
}

svtkASCIITextCodec::~svtkASCIITextCodec() = default;

void svtkASCIITextCodec::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "svtkASCIITextCodec (" << this << ") \n";
  indent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
