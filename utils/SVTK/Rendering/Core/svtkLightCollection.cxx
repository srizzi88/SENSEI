/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLightCollection.h"

#include "svtkLight.h"
#include "svtkObjectFactory.h"

#include <cmath>

svtkStandardNewMacro(svtkLightCollection);

// Add a light to the bottom of the list.
void svtkLightCollection::AddItem(svtkLight* a)
{
  this->svtkCollection::AddItem(a);
}

// Get the next light in the list. nullptr is returned when the collection is
// exhausted.
svtkLight* svtkLightCollection::GetNextItem()
{
  return static_cast<svtkLight*>(this->GetNextItemAsObject());
}

svtkLight* svtkLightCollection::GetNextLight(svtkCollectionSimpleIterator& cookie)
{
  return static_cast<svtkLight*>(this->GetNextItemAsObject(cookie));
}

//----------------------------------------------------------------------------
void svtkLightCollection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
