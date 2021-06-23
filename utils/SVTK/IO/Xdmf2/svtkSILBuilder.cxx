
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSILBuilder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSILBuilder.h"

#include "svtkDataSetAttributes.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkSILBuilder);
svtkCxxSetObjectMacro(svtkSILBuilder, SIL, svtkMutableDirectedGraph);
//----------------------------------------------------------------------------
svtkSILBuilder::svtkSILBuilder()
{
  this->NamesArray = 0;
  this->CrossEdgesArray = 0;
  this->SIL = 0;
  this->RootVertex = -1;
}

//----------------------------------------------------------------------------
svtkSILBuilder::~svtkSILBuilder()
{
  this->SetSIL(0);
}

//----------------------------------------------------------------------------
void svtkSILBuilder::Initialize()
{
  this->SIL->Initialize();
  this->NamesArray = svtkStringArray::New();
  this->NamesArray->SetName("Names");
  this->CrossEdgesArray = svtkUnsignedCharArray::New();
  this->CrossEdgesArray->SetName("CrossEdges");
  this->SIL->GetVertexData()->AddArray(this->NamesArray);
  this->SIL->GetEdgeData()->AddArray(this->CrossEdgesArray);
  this->NamesArray->Delete();
  this->CrossEdgesArray->Delete();

  this->RootVertex = this->AddVertex("SIL");
}

//-----------------------------------------------------------------------------
svtkIdType svtkSILBuilder::AddVertex(const char* name)
{
  svtkIdType vertex = this->SIL->AddVertex();
  this->NamesArray->InsertValue(vertex, name);
  return vertex;
}

//-----------------------------------------------------------------------------
svtkIdType svtkSILBuilder::AddChildEdge(svtkIdType src, svtkIdType dst)
{
  svtkIdType id = this->SIL->AddEdge(src, dst).Id;
  this->CrossEdgesArray->InsertValue(id, 0);
  return id;
}

//-----------------------------------------------------------------------------
svtkIdType svtkSILBuilder::AddCrossEdge(svtkIdType src, svtkIdType dst)
{
  svtkIdType id = this->SIL->AddEdge(src, dst).Id;
  this->CrossEdgesArray->InsertValue(id, 1);
  return id;
}

//----------------------------------------------------------------------------
void svtkSILBuilder::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
