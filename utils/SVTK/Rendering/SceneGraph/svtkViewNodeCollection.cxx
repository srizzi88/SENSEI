/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewNodeCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkViewNodeCollection.h"

#include "svtkCollectionIterator.h"
#include "svtkObjectFactory.h"
#include "svtkViewNode.h"

//============================================================================
svtkStandardNewMacro(svtkViewNodeCollection);

//----------------------------------------------------------------------------
void svtkViewNodeCollection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkViewNodeCollection::AddItem(svtkViewNode* a)
{
  this->svtkCollection::AddItem(a);
}

//----------------------------------------------------------------------------
svtkViewNode* svtkViewNodeCollection::GetNextItem()
{
  return static_cast<svtkViewNode*>(this->GetNextItemAsObject());
}

//----------------------------------------------------------------------------
svtkViewNode* svtkViewNodeCollection::GetNextViewNode(svtkCollectionSimpleIterator& cookie)
{
  return static_cast<svtkViewNode*>(this->GetNextItemAsObject(cookie));
}

//----------------------------------------------------------------------------
bool svtkViewNodeCollection::IsRenderablePresent(svtkObject* obj)
{
  svtkCollectionIterator* it = this->NewIterator();
  it->InitTraversal();
  bool found = false;
  while (!found && !it->IsDoneWithTraversal())
  {
    svtkViewNode* vn = svtkViewNode::SafeDownCast(it->GetCurrentObject());
    if (vn)
    {
      svtkObject* nobj = vn->GetRenderable();
      if (nobj == obj)
      {
        found = true;
      }
      it->GoToNextItem();
    }
  }
  it->Delete();

  return found;
}
