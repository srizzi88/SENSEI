/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkViewNode.h"

#include "svtkCollectionIterator.h"
#include "svtkObjectFactory.h"
#include "svtkViewNodeCollection.h"
#include "svtkViewNodeFactory.h"

//----------------------------------------------------------------------------
const char* svtkViewNode::operation_type_strings[] = { "noop", "build", "synchronize", "render",
  nullptr };

//----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkViewNode, Children, svtkViewNodeCollection);

//----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkViewNode, MyFactory, svtkViewNodeFactory);

//----------------------------------------------------------------------------
svtkViewNode::svtkViewNode()
{
  this->Renderable = nullptr;
  this->Parent = nullptr;
  this->Children = svtkViewNodeCollection::New();
  this->PreparedNodes = svtkCollection::New();
  this->MyFactory = nullptr;

  this->RenderTime = 0;
}

//----------------------------------------------------------------------------
svtkViewNode::~svtkViewNode()
{
  this->Parent = 0;
  if (this->Children)
  {
    this->Children->Delete();
    this->Children = 0;
  }
  if (this->MyFactory)
  {
    this->MyFactory->Delete();
    this->MyFactory = 0;
  }
  if (this->PreparedNodes)
  {
    this->PreparedNodes->Delete();
    this->PreparedNodes = 0;
  }
}

//----------------------------------------------------------------------------
void svtkViewNode::SetParent(svtkViewNode* p)
{
  this->Parent = p;
}

//----------------------------------------------------------------------------
svtkViewNode* svtkViewNode::GetParent()
{
  return this->Parent;
}

//----------------------------------------------------------------------------
void svtkViewNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkViewNode::PrepareNodes()
{
  this->PreparedNodes->RemoveAllItems();
}

//----------------------------------------------------------------------------
void svtkViewNode::RemoveUnusedNodes()
{
  // remove viewnodes if their renderables are no longer present
  svtkViewNodeCollection* nodes = this->GetChildren();
  svtkCollectionIterator* nit = nodes->NewIterator();
  nit->InitTraversal();
  while (!nit->IsDoneWithTraversal())
  {
    svtkViewNode* node = svtkViewNode::SafeDownCast(nit->GetCurrentObject());
    if (node)
    {
      svtkObject* obj = node->GetRenderable();
      if (!this->PreparedNodes->IsItemPresent(obj))
      {
        nodes->RemoveItem(node);
        nit->InitTraversal(); // don't stumble over deleted node
      }
    }
    nit->GoToNextItem();
  }
  nit->Delete();

  this->PrepareNodes();
}

//----------------------------------------------------------------------------
void svtkViewNode::AddMissingNodes(svtkCollection* col)
{
  // add viewnodes for renderables that are not yet present
  svtkViewNodeCollection* nodes = this->GetChildren();
  svtkCollectionIterator* rit = col->NewIterator();
  rit->InitTraversal();
  while (!rit->IsDoneWithTraversal())
  {
    svtkObject* obj = rit->GetCurrentObject();
    if (obj)
    {
      this->PreparedNodes->AddItem(obj);
      if (!nodes->IsRenderablePresent(obj))
      {
        svtkViewNode* node = this->CreateViewNode(obj);
        if (node)
        {
          nodes->AddItem(node);
          node->SetParent(this);
          node->Delete();
        }
      }
    }
    rit->GoToNextItem();
  }
  rit->Delete();
}

//----------------------------------------------------------------------------
void svtkViewNode::AddMissingNode(svtkObject* obj)
{
  if (!obj)
  {
    return;
  }

  // add viewnodes for renderables that are not yet present
  svtkViewNodeCollection* nodes = this->GetChildren();
  this->PreparedNodes->AddItem(obj);
  if (!nodes->IsRenderablePresent(obj))
  {
    svtkViewNode* node = this->CreateViewNode(obj);
    if (node)
    {
      nodes->AddItem(node);
      node->SetParent(this);
      node->Delete();
    }
  }
}

//----------------------------------------------------------------------------
void svtkViewNode::TraverseAllPasses()
{
  this->Traverse(build);
  this->Traverse(synchronize);
  this->Traverse(render);
}

//----------------------------------------------------------------------------
void svtkViewNode::Traverse(int operation)
{
  this->Apply(operation, true);

  svtkCollectionIterator* it = this->Children->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
  {
    svtkViewNode* child = svtkViewNode::SafeDownCast(it->GetCurrentObject());
    child->Traverse(operation);
    it->GoToNextItem();
  }
  it->Delete();

  this->Apply(operation, false);
}

//----------------------------------------------------------------------------
svtkViewNode* svtkViewNode::CreateViewNode(svtkObject* obj)
{
  svtkViewNode* ret = nullptr;
  if (!this->MyFactory)
  {
    svtkWarningMacro("Can not create view nodes without my own factory");
  }
  else
  {
    ret = this->MyFactory->CreateNode(obj);
    if (ret)
    {
      ret->Renderable = obj;
    }
  }
  return ret;
}

//----------------------------------------------------------------------------
svtkViewNode* svtkViewNode::GetFirstAncestorOfType(const char* type)
{
  if (!this->Parent)
  {
    return nullptr;
  }
  if (this->Parent->IsA(type))
  {
    return this->Parent;
  }
  return this->Parent->GetFirstAncestorOfType(type);
}

//----------------------------------------------------------------------------
void svtkViewNode::SetRenderable(svtkObject* obj)
{
  this->Renderable = obj;
}

//----------------------------------------------------------------------------
void svtkViewNode::Apply(int operation, bool prepass)
{
  // cerr << this->GetClassName() << "(" << this << ") Apply("
  //     << svtkViewNode::operation_type_strings[operation] << ")" << endl;
  switch (operation)
  {
    case noop:
      break;
    case build:
      this->Build(prepass);
      break;
    case synchronize:
      this->Synchronize(prepass);
      break;
    case render:
      this->Render(prepass);
      break;
    case invalidate:
      this->Invalidate(prepass);
      break;
    default:
      cerr << "UNKNOWN OPERATION" << operation << endl;
  }
}

//----------------------------------------------------------------------------
svtkViewNode* svtkViewNode::GetViewNodeFor(svtkObject* obj)
{
  if (this->Renderable == obj)
  {
    return this;
  }

  svtkViewNode* owner = nullptr;
  svtkCollectionIterator* it = this->Children->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
  {
    svtkViewNode* child = svtkViewNode::SafeDownCast(it->GetCurrentObject());
    owner = child->GetViewNodeFor(obj);
    if (owner)
    {
      break;
    }
    it->GoToNextItem();
  }
  it->Delete();
  return owner;
}

//----------------------------------------------------------------------------
svtkViewNode* svtkViewNode::GetFirstChildOfType(const char* type)
{
  if (this->IsA(type))
  {
    return this;
  }

  svtkCollectionIterator* it = this->Children->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
  {
    svtkViewNode* child = svtkViewNode::SafeDownCast(it->GetCurrentObject());
    if (child->IsA(type))
    {
      it->Delete();
      return child;
    }
    it->GoToNextItem();
  }
  it->Delete();
  return nullptr;
}
