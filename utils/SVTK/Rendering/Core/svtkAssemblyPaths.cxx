/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssemblyPaths.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAssemblyPaths.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkAssemblyPaths);

svtkMTimeType svtkAssemblyPaths::GetMTime()
{
  svtkMTimeType mtime = this->svtkCollection::GetMTime();

  svtkAssemblyPath* path;
  for (this->InitTraversal(); (path = this->GetNextItem());)
  {
    svtkMTimeType pathMTime = path->GetMTime();
    if (pathMTime > mtime)
    {
      mtime = pathMTime;
    }
  }
  return mtime;
}
