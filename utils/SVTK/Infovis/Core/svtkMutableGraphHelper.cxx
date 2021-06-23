/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMutableGraphHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkMutableGraphHelper.h"

#include "svtkGraphEdge.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"

svtkCxxSetObjectMacro(svtkMutableGraphHelper, InternalGraph, svtkGraph);
svtkStandardNewMacro(svtkMutableGraphHelper);
//----------------------------------------------------------------------------
svtkMutableGraphHelper::svtkMutableGraphHelper()
{
  this->InternalGraph = nullptr;
  this->DirectedGraph = nullptr;
  this->UndirectedGraph = nullptr;
  this->GraphEdge = svtkGraphEdge::New();
  this->GraphEdge->SetId(-1);
  this->GraphEdge->SetSource(-1);
  this->GraphEdge->SetTarget(-1);
}

//----------------------------------------------------------------------------
svtkMutableGraphHelper::~svtkMutableGraphHelper()
{
  if (this->InternalGraph)
  {
    this->InternalGraph->Delete();
  }
  this->GraphEdge->Delete();
}

//----------------------------------------------------------------------------
void svtkMutableGraphHelper::SetGraph(svtkGraph* g)
{
  this->SetInternalGraph(g);
  this->DirectedGraph = svtkMutableDirectedGraph::SafeDownCast(this->InternalGraph);
  this->UndirectedGraph = svtkMutableUndirectedGraph::SafeDownCast(this->InternalGraph);
  if (!this->DirectedGraph && !this->UndirectedGraph)
  {
    svtkErrorMacro("The graph must be mutable.");
  }
}

//----------------------------------------------------------------------------
svtkGraph* svtkMutableGraphHelper::GetGraph()
{
  return this->GetInternalGraph();
}

//----------------------------------------------------------------------------
svtkIdType svtkMutableGraphHelper::AddVertex()
{
  if (!this->InternalGraph)
  {
    return -1;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->AddVertex();
  }
  else
  {
    return this->UndirectedGraph->AddVertex();
  }
}

//----------------------------------------------------------------------------
svtkEdgeType svtkMutableGraphHelper::AddEdge(svtkIdType u, svtkIdType v)
{
  if (!this->InternalGraph)
  {
    return svtkEdgeType();
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->AddEdge(u, v);
  }
  else
  {
    return this->UndirectedGraph->AddEdge(u, v);
  }
}

//----------------------------------------------------------------------------
svtkGraphEdge* svtkMutableGraphHelper::AddGraphEdge(svtkIdType u, svtkIdType v)
{
  if (!this->InternalGraph)
  {
    return this->GraphEdge;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->AddGraphEdge(u, v);
  }
  else
  {
    return this->UndirectedGraph->AddGraphEdge(u, v);
  }
}

//----------------------------------------------------------------------------
void svtkMutableGraphHelper::RemoveVertex(svtkIdType v)
{
  if (!this->InternalGraph)
  {
    return;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->RemoveVertex(v);
  }
  else
  {
    return this->UndirectedGraph->RemoveVertex(v);
  }
}

//----------------------------------------------------------------------------
void svtkMutableGraphHelper::RemoveVertices(svtkIdTypeArray* verts)
{
  if (!this->InternalGraph)
  {
    return;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->RemoveVertices(verts);
  }
  else
  {
    return this->UndirectedGraph->RemoveVertices(verts);
  }
}

//----------------------------------------------------------------------------
void svtkMutableGraphHelper::RemoveEdge(svtkIdType e)
{
  if (!this->InternalGraph)
  {
    return;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->RemoveEdge(e);
  }
  else
  {
    return this->UndirectedGraph->RemoveEdge(e);
  }
}

//----------------------------------------------------------------------------
void svtkMutableGraphHelper::RemoveEdges(svtkIdTypeArray* edges)
{
  if (!this->InternalGraph)
  {
    return;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->RemoveEdges(edges);
  }
  else
  {
    return this->UndirectedGraph->RemoveEdges(edges);
  }
}

//----------------------------------------------------------------------------
void svtkMutableGraphHelper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InternalGraph: " << (this->InternalGraph ? "" : "(null)") << endl;
  if (this->InternalGraph)
  {
    this->InternalGraph->PrintSelf(os, indent.GetNextIndent());
  }
}
