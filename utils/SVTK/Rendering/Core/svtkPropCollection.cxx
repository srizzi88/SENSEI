/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPropCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPropCollection.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPropCollection);

int svtkPropCollection::GetNumberOfPaths()
{
  int numPaths = 0;
  svtkProp* aProp;

  svtkCollectionSimpleIterator pit;
  for (this->InitTraversal(pit); (aProp = this->GetNextProp(pit));)
  {
    numPaths += aProp->GetNumberOfPaths();
  }
  return numPaths;
}
