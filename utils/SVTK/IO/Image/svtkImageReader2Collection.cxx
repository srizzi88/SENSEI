/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReader2Collection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageReader2Collection.h"

#include "svtkImageReader2.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkImageReader2Collection);

void svtkImageReader2Collection::AddItem(svtkImageReader2* f)
{
  this->svtkCollection::AddItem(f);
}

svtkImageReader2* svtkImageReader2Collection::GetNextItem()
{
  return static_cast<svtkImageReader2*>(this->GetNextItemAsObject());
}

svtkImageReader2* svtkImageReader2Collection::GetNextImageReader2(svtkCollectionSimpleIterator& cookie)
{
  return static_cast<svtkImageReader2*>(this->GetNextItemAsObject(cookie));
}

//----------------------------------------------------------------------------
void svtkImageReader2Collection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
