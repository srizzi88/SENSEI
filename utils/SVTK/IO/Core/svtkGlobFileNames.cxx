/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlobFileNames.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGlobFileNames.h"
#include "svtkStringArray.h"

#include "svtkDebugLeaks.h"
#include "svtkObjectFactory.h"

#include <algorithm>
#include <string>
#include <vector>
#include <svtksys/Glob.hxx>
#include <svtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
svtkGlobFileNames* svtkGlobFileNames::New()
{
  SVTK_STANDARD_NEW_BODY(svtkGlobFileNames);
}

//----------------------------------------------------------------------------
svtkGlobFileNames::svtkGlobFileNames()
{
  this->Directory = nullptr;
  this->Pattern = nullptr;
  this->Recurse = 0;
  this->FileNames = svtkStringArray::New();
}

//----------------------------------------------------------------------------
svtkGlobFileNames::~svtkGlobFileNames()
{
  delete[] this->Directory;
  delete[] this->Pattern;
  this->FileNames->Delete();
  this->FileNames = nullptr;
}

//----------------------------------------------------------------------------
void svtkGlobFileNames::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Directory: " << (this->GetDirectory() ? this->GetDirectory() : " none") << "\n";
  os << indent << "Pattern: " << (this->GetPattern() ? this->GetPattern() : " none") << "\n";
  os << indent << "Recurse: " << (this->GetRecurse() ? "On\n" : "Off\n");
  os << indent << "FileNames:  (" << this->GetFileNames() << ")\n";
  indent = indent.GetNextIndent();
  for (int i = 0; i < this->FileNames->GetNumberOfValues(); i++)
  {
    os << indent << this->FileNames->GetValue(i) << "\n";
  }
}

//----------------------------------------------------------------------------
void svtkGlobFileNames::Reset()
{
  this->FileNames->Reset();
}

//----------------------------------------------------------------------------
int svtkGlobFileNames::AddFileNames(const char* pattern)
{
  this->SetPattern(pattern);

  svtksys::Glob glob;

  if (this->Recurse)
  {
    glob.RecurseOn();
  }
  else
  {
    glob.RecurseOff();
  }

  if (!this->Pattern)
  {
    svtkErrorMacro(<< "FindFileNames: pattern string is null.");

    return 0;
  }

  std::string fullPattern = this->Pattern;

  if (this->Directory && this->Directory[0] != '\0')
  {
    std::vector<std::string> components;
    svtksys::SystemTools::SplitPath(fullPattern, components);
    // If Pattern is a relative path, prepend with Directory
    if (components[0].empty())
    {
      components.insert(components.begin(), this->Directory);
      fullPattern = svtksys::SystemTools::JoinPath(components);
    }
  }

  if (!glob.FindFiles(fullPattern))
  {
    svtkErrorMacro(<< "FindFileNames: Glob action failed for \"" << fullPattern << "\"");

    return 0;
  }

  // copy the filenames from glob
  std::vector<std::string> files = glob.GetFiles();

  // sort them lexicographically
  std::sort(files.begin(), files.end());

  // add them onto the list of filenames
  for (std::vector<std::string>::const_iterator iter = files.begin(); iter != files.end(); ++iter)
  {
    this->FileNames->InsertNextValue(iter->c_str());
  }

  return 1;
}

//----------------------------------------------------------------------------
const char* svtkGlobFileNames::GetNthFileName(int index)
{
  if (index >= this->FileNames->GetNumberOfValues() || index < 0)
  {
    svtkErrorMacro(<< "Bad index for GetFileName on svtkGlobFileNames\n");
    return nullptr;
  }

  return this->FileNames->GetValue(index).c_str();
}

//----------------------------------------------------------------------------
int svtkGlobFileNames::GetNumberOfFileNames()
{
  return this->FileNames->GetNumberOfValues();
}
