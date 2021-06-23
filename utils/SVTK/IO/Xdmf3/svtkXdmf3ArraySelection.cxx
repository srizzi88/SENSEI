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

#include "svtkXdmf3ArraySelection.h"

//==============================================================================
void svtkXdmf3ArraySelection::Merge(const svtkXdmf3ArraySelection& other)
{
  svtkXdmf3ArraySelection::const_iterator iter = other.begin();
  for (; iter != other.end(); ++iter)
  {
    (*this)[iter->first] = iter->second;
  }
}

//--------------------------------------------------------------------------
void svtkXdmf3ArraySelection::AddArray(const char* name, bool status)
{
  (*this)[name] = status;
}

//--------------------------------------------------------------------------
bool svtkXdmf3ArraySelection::ArrayIsEnabled(const char* name)
{
  svtkXdmf3ArraySelection::iterator iter = this->find(name);
  if (iter != this->end())
  {
    return iter->second;
  }

  // don't know anything about this array, enable it by default.
  return true;
}

//--------------------------------------------------------------------------
bool svtkXdmf3ArraySelection::HasArray(const char* name)
{
  svtkXdmf3ArraySelection::iterator iter = this->find(name);
  return (iter != this->end());
}

//--------------------------------------------------------------------------
int svtkXdmf3ArraySelection::GetArraySetting(const char* name)
{
  return this->ArrayIsEnabled(name) ? 1 : 0;
}

//--------------------------------------------------------------------------
void svtkXdmf3ArraySelection::SetArrayStatus(const char* name, bool status)
{
  this->AddArray(name, status);
}

//--------------------------------------------------------------------------
const char* svtkXdmf3ArraySelection::GetArrayName(int index)
{
  int cc = 0;
  for (svtkXdmf3ArraySelection::iterator iter = this->begin(); iter != this->end(); ++iter)
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
int svtkXdmf3ArraySelection::GetNumberOfArrays()
{
  return static_cast<int>(this->size());
}
