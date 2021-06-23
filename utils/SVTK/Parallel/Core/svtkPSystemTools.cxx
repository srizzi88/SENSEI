/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPSystemTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPSystemTools.h"

#include "svtkObjectFactory.h"
#include <svtkMultiProcessController.h>
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkPSystemTools);

//----------------------------------------------------------------------------
void svtkPSystemTools::BroadcastString(std::string& str, int proc)
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();

  svtkIdType size = static_cast<svtkIdType>(str.size());
  controller->Broadcast(&size, 1, proc);

  str.resize(size);
  if (size)
  {
    controller->Broadcast(&str[0], size, proc);
  }
}

//----------------------------------------------------------------------------
std::string svtkPSystemTools::CollapseFullPath(const std::string& in_relative)
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  std::string returnString;
  if (controller->GetLocalProcessId() == 0)
  {
    returnString = svtksys::SystemTools::CollapseFullPath(in_relative, nullptr);
  }
  svtkPSystemTools::BroadcastString(returnString, 0);

  return returnString;
}

//----------------------------------------------------------------------------
std::string svtkPSystemTools::CollapseFullPath(const std::string& in_path, const char* in_base)
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  std::string returnString;
  if (controller->GetLocalProcessId() == 0)
  {
    returnString = svtksys::SystemTools::CollapseFullPath(in_path, in_base);
  }
  svtkPSystemTools::BroadcastString(returnString, 0);

  return returnString;
}

//----------------------------------------------------------------------------
bool svtkPSystemTools::FileExists(const char* filename)
{
  if (!filename)
  {
    return false;
  }
  return svtkPSystemTools::FileExists(std::string(filename));
}

//----------------------------------------------------------------------------
bool svtkPSystemTools::FileExists(const std::string& filename)
{
  if (filename.empty())
  {
    return false;
  }
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  int exists = 0;
  if (controller->GetLocalProcessId() == 0)
  {
    exists = svtksys::SystemTools::FileExists(filename);
  }
  controller->Broadcast(&exists, 1, 0);
  return exists != 0;
}

//----------------------------------------------------------------------------
bool svtkPSystemTools::FileExists(const char* filename, bool isFile)
{
  if (!filename)
  {
    return false;
  }
  return svtkPSystemTools::FileExists(std::string(filename), isFile);
}

//----------------------------------------------------------------------------
bool svtkPSystemTools::FileExists(const std::string& filename, bool isFile)
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  int exists = 0;
  if (controller->GetLocalProcessId() == 0)
  {
    exists = svtksys::SystemTools::FileExists(filename, isFile);
  }
  controller->Broadcast(&exists, 1, 0);
  return exists != 0;
}

//----------------------------------------------------------------------------
bool svtkPSystemTools::FileIsDirectory(const std::string& inName)
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  int isDirectory = 0;
  if (controller->GetLocalProcessId() == 0)
  {
    isDirectory = svtksys::SystemTools::FileIsDirectory(inName);
  }
  controller->Broadcast(&isDirectory, 1, 0);
  return isDirectory != 0;
}

//----------------------------------------------------------------------------
bool svtkPSystemTools::FindProgramPath(const char* argv0, std::string& pathOut,
  std::string& errorMsg, const char* exeName, const char* buildDir, const char* installPrefix)
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  int retVal = 1;
  if (controller->GetLocalProcessId() == 0)
  {
    retVal = static_cast<int>(svtksys::SystemTools::FindProgramPath(
      argv0, pathOut, errorMsg, exeName, buildDir, installPrefix));
  }
  controller->Broadcast(&retVal, 1, 0);
  // if the retVal on proc 0 is non-zero then only information is
  // put in pathOut. Otherwise information is put in errorMsg.
  if (retVal)
  {
    svtkPSystemTools::BroadcastString(pathOut, 0);
  }
  else
  {
    svtkPSystemTools::BroadcastString(errorMsg, 0);
  }
  return retVal != 0;
}

//----------------------------------------------------------------------------
std::string svtkPSystemTools::GetCurrentWorkingDirectory(bool collapse)
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  std::string returnString;
  if (controller->GetLocalProcessId() == 0)
  {
    returnString = svtksys::SystemTools::GetCurrentWorkingDirectory(collapse);
  }
  svtkPSystemTools::BroadcastString(returnString, 0);
  return returnString;
}

//----------------------------------------------------------------------------
std::string svtkPSystemTools::GetProgramPath(const std::string& path)
{
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  std::string programPath;
  if (controller->GetLocalProcessId() == 0)
  {
    programPath = svtksys::SystemTools::GetProgramPath(path);
  }
  svtkPSystemTools::BroadcastString(programPath, 0);

  return programPath;
}

//----------------------------------------------------------------------------
void svtkPSystemTools::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
