/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssemblyPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAssemblyPath.h"

#include "svtkAssemblyNode.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkTransform.h"

svtkStandardNewMacro(svtkAssemblyPath);

svtkAssemblyPath::svtkAssemblyPath()
{
  this->Transform = svtkTransform::New();
  this->Transform->PreMultiply();
  this->TransformedProp = nullptr;
}

svtkAssemblyPath::~svtkAssemblyPath()
{
  this->Transform->Delete();
  if (this->TransformedProp != nullptr)
  {
    this->TransformedProp->Delete();
  }
}

void svtkAssemblyPath::AddNode(svtkProp* p, svtkMatrix4x4* m)
{
  svtkAssemblyNode* n = svtkAssemblyNode::New();
  n->SetViewProp(p);
  n->SetMatrix(m); // really a copy because we're gonna compute with it
  this->AddNode(n);
  n->Delete(); // ok reference counted
}

void svtkAssemblyPath::AddNode(svtkAssemblyNode* n)
{
  // First add the node to the list
  this->svtkCollection::AddItem(n);

  // Grab the matrix, if any, and concatenate it
  this->Transform->Push(); // keep in synch with list of nodes
  svtkMatrix4x4* matrix;
  if ((matrix = n->GetMatrix()) != nullptr)
  {
    this->Transform->Concatenate(matrix);
    this->Transform->GetMatrix(matrix); // replace previous matrix
  }
}

svtkAssemblyNode* svtkAssemblyPath::GetNextNode()
{
  return static_cast<svtkAssemblyNode*>(this->GetNextItemAsObject());
}

svtkAssemblyNode* svtkAssemblyPath::GetFirstNode()
{
  return this->Top ? static_cast<svtkAssemblyNode*>(this->Top->Item) : nullptr;
}

svtkAssemblyNode* svtkAssemblyPath::GetLastNode()
{
  return this->Bottom ? static_cast<svtkAssemblyNode*>(this->Bottom->Item) : nullptr;
}

void svtkAssemblyPath::DeleteLastNode()
{
  svtkAssemblyNode* node = this->GetLastNode();
  this->svtkCollection::RemoveItem(node);
  this->Transform->Pop();
}

void svtkAssemblyPath::ShallowCopy(svtkAssemblyPath* path)
{
  this->RemoveAllItems();

  svtkAssemblyNode* node;
  for (path->InitTraversal(); (node = path->GetNextNode());)
  {
    this->svtkCollection::AddItem(node);
  }
}

svtkMTimeType svtkAssemblyPath::GetMTime()
{
  svtkMTimeType mtime = this->svtkCollection::GetMTime();

  svtkAssemblyNode* node;
  for (this->InitTraversal(); (node = this->GetNextNode());)
  {
    svtkMTimeType nodeMTime = node->GetMTime();
    if (nodeMTime > mtime)
    {
      mtime = nodeMTime;
    }
  }
  return mtime;
}

void svtkAssemblyPath::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
