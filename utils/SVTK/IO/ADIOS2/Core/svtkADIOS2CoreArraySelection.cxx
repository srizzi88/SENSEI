/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3ArraySelection.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "Core/svtkADIOS2CoreArraySelection.h"
#include <iostream>

//--------------------------------------------------------------------------
void svtkADIOS2ArraySelection::AddArray(const char* name, bool status)
{
  (*this)[name] = status;
}

//--------------------------------------------------------------------------
bool svtkADIOS2ArraySelection::ArrayIsEnabled(const char* name)
{
  svtkADIOS2ArraySelection::iterator iter = this->find(name);
  if (iter != this->end())
  {
    return iter->second;
  }

  // don't know anything about this array, enable it by default.
  return true;
}

//--------------------------------------------------------------------------
bool svtkADIOS2ArraySelection::HasArray(const char* name)
{
  svtkADIOS2ArraySelection::iterator iter = this->find(name);
  return (iter != this->end());
}

//--------------------------------------------------------------------------
int svtkADIOS2ArraySelection::GetArrayStatus(const char* name)
{
  return this->ArrayIsEnabled(name) ? 1 : 0;
}

//--------------------------------------------------------------------------
void svtkADIOS2ArraySelection::SetArrayStatus(const char* name, bool status)
{
  (*this)[std::string(name)] = status;
}

//--------------------------------------------------------------------------
const char* svtkADIOS2ArraySelection::GetArrayName(int index)
{
  int cc = 0;
  for (svtkADIOS2ArraySelection::iterator iter = this->begin(); iter != this->end(); ++iter)
  {
    if (cc == index)
    {
      return iter->first.c_str();
    }
    cc++;
  }
  return nullptr;
}

//--------------------------------------------------------------------------
int svtkADIOS2ArraySelection::GetNumberOfArrays()
{
  return static_cast<int>(this->size());
}
