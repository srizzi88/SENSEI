/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInputStream.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInputStream.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkInputStream);

//----------------------------------------------------------------------------
svtkInputStream::svtkInputStream()
{
  this->Stream = nullptr;
}

//----------------------------------------------------------------------------
svtkInputStream::~svtkInputStream()
{
  this->SetStream(nullptr);
}

//----------------------------------------------------------------------------
void svtkInputStream::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Stream: " << (this->Stream ? "set" : "none") << "\n";
}

//----------------------------------------------------------------------------
void svtkInputStream::StartReading()
{
  if (!this->Stream)
  {
    svtkErrorMacro("StartReading() called with no Stream set.");
    return;
  }
  this->StreamStartPosition = this->Stream->tellg();
}

//----------------------------------------------------------------------------
void svtkInputStream::EndReading() {}

//----------------------------------------------------------------------------
int svtkInputStream::Seek(svtkTypeInt64 offset)
{
  std::streamoff off = static_cast<std::streamoff>(this->StreamStartPosition + offset);
  return (this->Stream->seekg(off, std::ios::beg) ? 1 : 0);
}

//----------------------------------------------------------------------------
size_t svtkInputStream::Read(void* data, size_t length)
{
  return this->ReadStream(static_cast<char*>(data), length);
}

//----------------------------------------------------------------------------
size_t svtkInputStream::ReadStream(char* data, size_t length)
{
  this->Stream->read(data, length);
  return this->Stream->gcount();
}
