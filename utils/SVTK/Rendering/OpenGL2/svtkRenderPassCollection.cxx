/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderPassCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRenderPassCollection.h"
#include "svtkObjectFactory.h"
#include "svtkRenderPass.h"

svtkStandardNewMacro(svtkRenderPassCollection);

// ----------------------------------------------------------------------------
// Description:
// Reentrant safe way to get an object in a collection. Just pass the
// same cookie back and forth.
svtkRenderPass* svtkRenderPassCollection::GetNextRenderPass(svtkCollectionSimpleIterator& cookie)
{
  return static_cast<svtkRenderPass*>(this->GetNextItemAsObject(cookie));
}

// ----------------------------------------------------------------------------
svtkRenderPassCollection::svtkRenderPassCollection() = default;

// ----------------------------------------------------------------------------
svtkRenderPassCollection::~svtkRenderPassCollection() = default;

// ----------------------------------------------------------------------------
// hide the standard AddItem from the user and the compiler.
void svtkRenderPassCollection::AddItem(svtkObject* o)
{
  this->svtkCollection::AddItem(o);
}

// ----------------------------------------------------------------------------
void svtkRenderPassCollection::AddItem(svtkRenderPass* a)
{
  this->svtkCollection::AddItem(a);
}

// ----------------------------------------------------------------------------
svtkRenderPass* svtkRenderPassCollection::GetNextRenderPass()
{
  return static_cast<svtkRenderPass*>(this->GetNextItemAsObject());
}

// ----------------------------------------------------------------------------
svtkRenderPass* svtkRenderPassCollection::GetLastRenderPass()
{
  return (this->Bottom) ? static_cast<svtkRenderPass*>(this->Bottom->Item) : nullptr;
}

// ----------------------------------------------------------------------------
void svtkRenderPassCollection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
