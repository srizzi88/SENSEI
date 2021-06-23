/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkSQLDatabase.cxx

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

#include "svtkTextCodec.h"

const char* svtkTextCodec::Name()
{
  return "";
}

bool svtkTextCodec::CanHandle(const char*)
{
  return false;
}

bool svtkTextCodec::IsValid(istream&)
{
  return false;
}

svtkTextCodec::~svtkTextCodec() = default;

svtkTextCodec::svtkTextCodec() = default;

namespace
{
class svtkUnicodeStringOutputIterator : public svtkTextCodec::OutputIterator
{
public:
  svtkUnicodeStringOutputIterator& operator++(int) override;
  svtkUnicodeStringOutputIterator& operator*() override;
  svtkUnicodeStringOutputIterator& operator=(const svtkUnicodeString::value_type value) override;

  svtkUnicodeStringOutputIterator(svtkUnicodeString& outputString);
  ~svtkUnicodeStringOutputIterator() override;

private:
  svtkUnicodeStringOutputIterator(const svtkUnicodeStringOutputIterator&) = delete;
  svtkUnicodeStringOutputIterator& operator=(const svtkUnicodeStringOutputIterator&) = delete;

  svtkUnicodeString& OutputString;
  unsigned int StringPosition;
};

svtkUnicodeStringOutputIterator& svtkUnicodeStringOutputIterator::operator++(int)
{
  this->StringPosition++;
  return *this;
}

svtkUnicodeStringOutputIterator& svtkUnicodeStringOutputIterator::operator*()
{
  return *this;
}

svtkUnicodeStringOutputIterator& svtkUnicodeStringOutputIterator::operator=(
  const svtkUnicodeString::value_type value)
{
  this->OutputString += value;
  return *this;
}

svtkUnicodeStringOutputIterator::svtkUnicodeStringOutputIterator(svtkUnicodeString& outputString)
  : OutputString(outputString)
  , StringPosition(0)
{
}

svtkUnicodeStringOutputIterator::~svtkUnicodeStringOutputIterator() = default;
}

svtkUnicodeString svtkTextCodec::ToUnicode(istream& InputStream)
{
  // create an output string stream
  svtkUnicodeString returnString;

  svtkUnicodeStringOutputIterator StringIterator(returnString);
  this->ToUnicode(InputStream, StringIterator);

  return returnString;
}

void svtkTextCodec::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "svtkTextCodec (" << this << ") \n";
  indent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
