/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3SILBuilder.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkXdmf3SILBuilder.h"

#include "svtkDataSetAttributes.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkStringArray.h"
#include "svtkUnsignedCharArray.h"

// As soon as num-grids (sub-grids and all) grows beyond this number, we assume
// that the grids are too numerous for the user to select individually and
// hence only the top-level grids are made accessible.
#define MAX_COLLECTABLE_NUMBER_OF_GRIDS 1000

//------------------------------------------------------------------------------
svtkXdmf3SILBuilder::svtkXdmf3SILBuilder()
{
  this->SIL = nullptr;
  this->NamesArray = nullptr;
  this->CrossEdgesArray = nullptr;
  this->RootVertex = -1;
  this->BlocksRoot = -1;
  this->HierarchyRoot = -1;
  this->VertexCount = 0;
}

//------------------------------------------------------------------------------
svtkXdmf3SILBuilder::~svtkXdmf3SILBuilder()
{
  if (this->SIL)
  {
    this->SIL->Delete();
  }
  if (this->NamesArray)
  {
    this->NamesArray->Delete();
  }
  if (this->CrossEdgesArray)
  {
    this->CrossEdgesArray->Delete();
  }
}

//------------------------------------------------------------------------------
void svtkXdmf3SILBuilder::Initialize()
{
  if (this->SIL)
  {
    this->SIL->Delete();
  }
  this->SIL = svtkMutableDirectedGraph::New();
  this->SIL->Initialize();

  if (this->NamesArray)
  {
    this->NamesArray->Delete();
  }
  this->NamesArray = svtkStringArray::New();
  this->NamesArray->SetName("Names");
  this->SIL->GetVertexData()->AddArray(this->NamesArray);

  if (this->CrossEdgesArray)
  {
    this->CrossEdgesArray->Delete();
  }

  this->CrossEdgesArray = svtkUnsignedCharArray::New();
  this->CrossEdgesArray->SetName("CrossEdges");
  this->SIL->GetEdgeData()->AddArray(this->CrossEdgesArray);

  this->RootVertex = this->AddVertex("SIL");
  this->BlocksRoot = this->AddVertex("Blocks");
  this->HierarchyRoot = this->AddVertex("Hierarchy");
  this->AddChildEdge(RootVertex, BlocksRoot);
  this->AddChildEdge(RootVertex, HierarchyRoot);

  this->VertexCount = 0;
}

//------------------------------------------------------------------------------
svtkIdType svtkXdmf3SILBuilder::AddVertex(const char* name)
{
  this->VertexCount++;
  svtkIdType vertex = this->SIL->AddVertex();
  this->NamesArray->InsertValue(vertex, name);
  return vertex;
}

//------------------------------------------------------------------------------
svtkIdType svtkXdmf3SILBuilder::AddChildEdge(svtkIdType parent, svtkIdType child)
{
  svtkIdType id = this->SIL->AddEdge(parent, child).Id;
  this->CrossEdgesArray->InsertValue(id, 0);
  return id;
}

//------------------------------------------------------------------------------
svtkIdType svtkXdmf3SILBuilder::AddCrossEdge(svtkIdType src, svtkIdType dst)
{
  svtkIdType id = this->SIL->AddEdge(src, dst).Id;
  this->CrossEdgesArray->InsertValue(id, 1);
  return id;
}

//------------------------------------------------------------------------------
svtkIdType svtkXdmf3SILBuilder::GetRootVertex()
{
  return this->RootVertex;
}

//------------------------------------------------------------------------------
svtkIdType svtkXdmf3SILBuilder::GetBlocksRoot()
{
  return this->BlocksRoot;
}

//------------------------------------------------------------------------------
svtkIdType svtkXdmf3SILBuilder::GetHierarchyRoot()
{
  return this->HierarchyRoot;
}

//------------------------------------------------------------------------------
bool svtkXdmf3SILBuilder::IsMaxedOut()
{
  return (this->VertexCount >= MAX_COLLECTABLE_NUMBER_OF_GRIDS);
}
