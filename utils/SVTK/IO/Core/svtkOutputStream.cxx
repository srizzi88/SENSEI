/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOutputStream.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOutputStream.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkOutputStream);

//----------------------------------------------------------------------------
svtkOutputStream::svtkOutputStream()
{
  this->Stream = nullptr;
}

//----------------------------------------------------------------------------
svtkOutputStream::~svtkOutputStream()
{
  this->SetStream(nullptr);
}

//----------------------------------------------------------------------------
void svtkOutputStream::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Stream: " << (this->Stream ? "set" : "none") << "\n";
}

//----------------------------------------------------------------------------
int svtkOutputStream::StartWriting()
{
  if (!this->Stream)
  {
    svtkErrorMacro("StartWriting() called with no Stream set.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkOutputStream::EndWriting()
{
  return 1;
}

//----------------------------------------------------------------------------
int svtkOutputStream::Write(void const* data, size_t length)
{
  return this->WriteStream(static_cast<const char*>(data), length);
}

//----------------------------------------------------------------------------
int svtkOutputStream::WriteStream(const char* data, size_t length)
{
  return (this->Stream->write(data, length) ? 1 : 0);
}
