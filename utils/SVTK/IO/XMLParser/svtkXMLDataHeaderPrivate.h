/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLDataHeaderPrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkXMLDataHeaderPrivate_DoNotInclude
#error "do not include unless you know what you are doing"
#endif

#ifndef svtkXMLDataHeaderPrivate_h
#define svtkXMLDataHeaderPrivate_h

#include "svtkType.h"
#include <vector>

// Abstract interface using type svtkTypeUInt64 to access an array
// of either svtkTypeUInt32 or svtkTypeUInt64.  Shared by svtkXMLWriter
// and svtkXMLDataParser to write/read binary data headers.
class svtkXMLDataHeader
{
public:
  virtual void Resize(size_t count) = 0;
  virtual svtkTypeUInt64 Get(size_t index) const = 0;
  virtual bool Set(size_t index, svtkTypeUInt64 value) = 0;
  virtual size_t WordSize() const = 0;
  virtual size_t WordCount() const = 0;
  virtual unsigned char* Data() = 0;
  size_t DataSize() const { return this->WordCount() * this->WordSize(); }
  virtual ~svtkXMLDataHeader() {}
  static inline svtkXMLDataHeader* New(int width, size_t count);
};

template <typename T>
class svtkXMLDataHeaderImpl : public svtkXMLDataHeader
{
  std::vector<T> Header;

public:
  svtkXMLDataHeaderImpl(size_t n)
    : Header(n, 0)
  {
  }
  void Resize(size_t count) override { this->Header.resize(count, 0); }
  svtkTypeUInt64 Get(size_t index) const override { return this->Header[index]; }
  bool Set(size_t index, svtkTypeUInt64 value) override
  {
    this->Header[index] = T(value);
    return svtkTypeUInt64(this->Header[index]) == value;
  }
  size_t WordSize() const override { return sizeof(T); }
  size_t WordCount() const override { return this->Header.size(); }
  unsigned char* Data() override { return reinterpret_cast<unsigned char*>(&this->Header[0]); }
};

svtkXMLDataHeader* svtkXMLDataHeader::New(int width, size_t count)
{
  switch (width)
  {
    case 32:
      return new svtkXMLDataHeaderImpl<svtkTypeUInt32>(count);
    case 64:
      return new svtkXMLDataHeaderImpl<svtkTypeUInt64>(count);
  }
  return nullptr;
}

#endif
// SVTK-HeaderTest-Exclude: svtkXMLDataHeaderPrivate.h
