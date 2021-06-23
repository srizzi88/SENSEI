/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActorCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActorCollection.h"

#include "svtkObjectFactory.h"
#include "svtkProperty.h"

svtkStandardNewMacro(svtkActorCollection);

void svtkActorCollection::ApplyProperties(svtkProperty* p)
{
  svtkActor* actor;

  if (p == nullptr)
  {
    return;
  }

  svtkCollectionSimpleIterator ait;
  for (this->InitTraversal(ait); (actor = this->GetNextActor(ait));)
  {
    actor->GetProperty()->DeepCopy(p);
  }
}

//----------------------------------------------------------------------------
void svtkActorCollection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
