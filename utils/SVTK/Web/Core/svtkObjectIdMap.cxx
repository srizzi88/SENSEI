/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkObjectIdMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkObjectIdMap.h"

#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkWeakPointer.h"

#include <map>
#include <set>
#include <string>

struct svtkObjectIdMap::svtkInternals
{
  std::map<svtkTypeUInt32, svtkSmartPointer<svtkObject> > Object;
  std::map<svtkSmartPointer<svtkObject>, svtkTypeUInt32> GlobalId;
  std::map<std::string, svtkWeakPointer<svtkObject> > ActiveObjects;
  svtkTypeUInt32 NextAvailableId;

  svtkInternals()
    : NextAvailableId(1)
  {
  }
};

svtkStandardNewMacro(svtkObjectIdMap);
//----------------------------------------------------------------------------
svtkObjectIdMap::svtkObjectIdMap()
  : Internals(new svtkInternals())
{
}

//----------------------------------------------------------------------------
svtkObjectIdMap::~svtkObjectIdMap()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void svtkObjectIdMap::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkTypeUInt32 svtkObjectIdMap::GetGlobalId(svtkObject* obj)
{
  if (obj == nullptr)
  {
    return 0;
  }

  auto iter = this->Internals->GlobalId.find(obj);
  if (iter == this->Internals->GlobalId.end())
  {
    svtkTypeUInt32 globalId = this->Internals->NextAvailableId++;
    this->Internals->GlobalId[obj] = globalId;
    this->Internals->Object[globalId] = obj;
    return globalId;
  }
  return iter->second;
}

//----------------------------------------------------------------------------
svtkObject* svtkObjectIdMap::GetSVTKObject(svtkTypeUInt32 globalId)
{
  auto iter = this->Internals->Object.find(globalId);
  if (iter == this->Internals->Object.end())
  {
    return nullptr;
  }
  return iter->second;
}

//----------------------------------------------------------------------------
svtkTypeUInt32 svtkObjectIdMap::SetActiveObject(const char* objectType, svtkObject* obj)
{
  if (objectType)
  {
    this->Internals->ActiveObjects[objectType] = obj;
    return this->GetGlobalId(obj);
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkObject* svtkObjectIdMap::GetActiveObject(const char* objectType)
{
  if (objectType)
  {
    return this->Internals->ActiveObjects[objectType];
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkObjectIdMap::FreeObject(svtkObject* obj)
{
  auto iter = this->Internals->GlobalId.find(obj);
  if (iter != this->Internals->GlobalId.end())
  {
    this->Internals->GlobalId.erase(obj);
    this->Internals->Object.erase(iter->second);
  }
}
