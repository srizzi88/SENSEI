/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericMovieWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericMovieWriter.h"

#include "svtkErrorCode.h"
#include "svtkImageData.h"

//---------------------------------------------------------------------------
svtkGenericMovieWriter::svtkGenericMovieWriter()
{
  this->FileName = nullptr;
  this->Error = 0;
}

//---------------------------------------------------------------------------
svtkGenericMovieWriter::~svtkGenericMovieWriter()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void svtkGenericMovieWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Error: " << this->Error << endl;
}

//----------------------------------------------------------------------------
static const char* svtkMovieWriterErrorStrings[] = { "Unassigned Error", "Initialize Error",
  "No Input Error", "Can Not Compress Error", "Can Not Format Error", "Changed Resolution Error",
  nullptr };

const char* svtkGenericMovieWriter::GetStringFromErrorCode(unsigned long error)
{
  static unsigned long numerrors = 0;
  if (error < UserError)
  {
    return svtkErrorCode::GetStringFromErrorCode(error);
  }
  else
  {
    error -= UserError;
  }

  if (!numerrors)
  {
    while (svtkMovieWriterErrorStrings[numerrors] != nullptr)
    {
      numerrors++;
    }
  }

  if (error < numerrors)
  {
    return svtkMovieWriterErrorStrings[error];
  }
  else
  {
    return "Unknown Error";
  }
}
