/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDirectory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkPDirectory.h"

#include "svtkObjectFactory.h"
#include "svtkPSystemTools.h"
#include "svtkStringArray.h"
#include <string>
#include <svtkMultiProcessController.h>
#include <svtksys/Directory.hxx>
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkPDirectory);

//----------------------------------------------------------------------------
svtkPDirectory::svtkPDirectory()
{
  this->Files = svtkStringArray::New();
}

//----------------------------------------------------------------------------
svtkPDirectory::~svtkPDirectory()
{
  this->Files->Delete();
  this->Files = nullptr;
}

//----------------------------------------------------------------------------
bool svtkPDirectory::Load(const std::string& name)
{
  this->Clear();

  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();

  long numFiles = 0;
  if (controller->GetLocalProcessId() == 0)
  {
    svtksys::Directory dir;
    if (dir.Load(name) == false)
    {
      numFiles = -1; // failure
      controller->Broadcast(&numFiles, 1, 0);
      return false;
    }

    for (unsigned long i = 0; i < dir.GetNumberOfFiles(); i++)
    {
      this->Files->InsertNextValue(dir.GetFile(i));
    }
    numFiles = static_cast<long>(dir.GetNumberOfFiles());
    controller->Broadcast(&numFiles, 1, 0);
    for (long i = 0; i < numFiles; i++)
    {
      svtkPSystemTools::BroadcastString(this->Files->GetValue(i), 0);
    }
  }
  else
  {
    controller->Broadcast(&numFiles, 1, 0);
    if (numFiles == -1)
    {
      return false;
    }
    for (long i = 0; i < numFiles; i++)
    {
      std::string str;
      svtkPSystemTools::BroadcastString(str, 0);
      this->Files->InsertNextValue(str);
    }
  }

  this->Path = name;
  return true;
}

//----------------------------------------------------------------------------
int svtkPDirectory::Open(const char* name)
{
  return static_cast<int>(this->Load(name));
}

//----------------------------------------------------------------------------
svtkIdType svtkPDirectory::GetNumberOfFiles() const
{
  return this->Files->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
const char* svtkPDirectory::GetFile(svtkIdType index) const
{
  if (index >= this->Files->GetNumberOfTuples())
  {
    return nullptr;
  }
  return this->Files->GetValue(index).c_str();
}

//----------------------------------------------------------------------------
int svtkPDirectory::FileIsDirectory(const char* name)
{
  // The svtksys::SystemTools::FileIsDirectory()
  // does not equal the following code (it probably should),
  // and it will broke KWWidgets. Reverse back to 1.30
  // return svtksys::SystemTools::FileIsDirectory(name);

  if (name == nullptr)
  {
    return 0;
  }

  int result = 0;
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();

  if (controller->GetLocalProcessId() == 0)
  {
    int absolutePath = 0;
#if defined(_WIN32)
    if (name[0] == '/' || name[0] == '\\')
    {
      absolutePath = 1;
    }
    else
    {
      for (int i = 0; name[i] != '\0'; i++)
      {
        if (name[i] == ':')
        {
          absolutePath = 1;
          break;
        }
        else if (name[i] == '/' || name[i] == '\\')
        {
          break;
        }
      }
    }
#else
    if (name[0] == '/')
    {
      absolutePath = 1;
    }
#endif

    char* fullPath;

    int n = 0;
    if (!absolutePath && !this->Path.empty())
    {
      n = static_cast<int>(this->Path.size());
    }

    int m = static_cast<int>(strlen(name));

    fullPath = new char[n + m + 2];

    if (!absolutePath && !this->Path.empty())
    {
      strcpy(fullPath, this->Path.c_str());
#if defined(_WIN32)
      if (fullPath[n - 1] != '/' && fullPath[n - 1] != '\\')
      {
#if !defined(__CYGWIN__)
        fullPath[n++] = '\\';
#else
        fullPath[n++] = '/';
#endif
      }
#else
      if (fullPath[n - 1] != '/')
      {
        fullPath[n++] = '/';
      }
#endif
    }

    strcpy(&fullPath[n], name);

    svtksys::SystemTools::Stat_t fs;
    if (svtksys::SystemTools::Stat(fullPath, &fs) == 0)
    {
#if defined(_WIN32)
      result = ((fs.st_mode & _S_IFDIR) != 0);
#else
      result = S_ISDIR(fs.st_mode);
#endif
    }

    delete[] fullPath;
  }

  controller->Broadcast(&result, 1, 0);

  return result;
}

//----------------------------------------------------------------------------
const char* svtkPDirectory::GetPath() const
{
  return this->Path.c_str();
}

//----------------------------------------------------------------------------
void svtkPDirectory::Clear()
{
  this->Path.clear();
  this->Files->Reset();
}

//----------------------------------------------------------------------------
void svtkPDirectory::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Files:  (" << this->Files << ")\n";
  if (this->Path.empty())
  {
    os << indent << "Directory not open\n";
    return;
  }

  os << indent << "Directory for: " << this->Path << "\n";
  os << indent << "Contains the following files:\n";
  indent = indent.GetNextIndent();
  for (int i = 0; i < this->Files->GetNumberOfValues(); i++)
  {
    os << indent << this->Files->GetValue(i) << "\n";
  }
}
