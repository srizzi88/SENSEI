/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLFileReadTester.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLFileReadTester.h"
#include "svtkObjectFactory.h"
#include "svtksys/FStream.hxx"

svtkStandardNewMacro(svtkXMLFileReadTester);

//----------------------------------------------------------------------------
svtkXMLFileReadTester::svtkXMLFileReadTester()
{
  this->FileDataType = nullptr;
  this->FileVersion = nullptr;
}

//----------------------------------------------------------------------------
svtkXMLFileReadTester::~svtkXMLFileReadTester()
{
  this->SetFileDataType(nullptr);
  this->SetFileVersion(nullptr);
}

//----------------------------------------------------------------------------
void svtkXMLFileReadTester::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileDataType: " << (this->FileDataType ? this->FileDataType : "") << "\n";
  os << indent << "FileVersion: " << (this->FileVersion ? this->FileVersion : "") << "\n";
}

//----------------------------------------------------------------------------
int svtkXMLFileReadTester::TestReadFile()
{
  if (!this->FileName)
  {
    return 0;
  }

  svtksys::ifstream inFile(this->FileName);
  if (!inFile)
  {
    return 0;
  }

  this->SetStream(&inFile);
  this->Done = 0;

  this->Parse();

  return this->Done ? 1 : 0;
}

//----------------------------------------------------------------------------
void svtkXMLFileReadTester::StartElement(const char* name, const char** atts)
{
  this->Done = 1;
  if (strcmp(name, "SVTKFile") == 0)
  {
    for (unsigned int i = 0; atts[i] && atts[i + 1]; i += 2)
    {
      if (strcmp(atts[i], "type") == 0)
      {
        this->SetFileDataType(atts[i + 1]);
      }
      else if (strcmp(atts[i], "version") == 0)
      {
        this->SetFileVersion(atts[i + 1]);
      }
    }
  }
}

//----------------------------------------------------------------------------
int svtkXMLFileReadTester::ParsingComplete()
{
  return this->Done ? 1 : 0;
}
