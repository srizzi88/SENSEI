/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostBreadthFirstSearch.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
#include "svtkBoostBreadthFirstSearch.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkConvertSelection.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"

#include "svtkBoostGraphAdapter.h"
#include "svtkDirectedGraph.h"
#include "svtkUndirectedGraph.h"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/pending/queue.hpp>

#include <utility> // for pair

using namespace boost;

svtkStandardNewMacro(svtkBoostBreadthFirstSearch);

// Redefine the bfs visitor, the only visitor we
// are using is the tree_edge visitor.
template <typename DistanceMap>
class my_distance_recorder : public default_bfs_visitor
{
public:
  my_distance_recorder() {}
  my_distance_recorder(DistanceMap dist, svtkIdType* far)
    : d(dist)
    , far_vertex(far)
    , far_dist(-1)
  {
    *far_vertex = -1;
  }

  template <typename Vertex, typename Graph>
  void examine_vertex(Vertex v, const Graph& svtkNotUsed(g))
  {
    if (get(d, v) > far_dist)
    {
      *far_vertex = v;
      far_dist = get(d, v);
    }
  }

  template <typename Edge, typename Graph>
  void tree_edge(Edge e, const Graph& g)
  {
    typename graph_traits<Graph>::vertex_descriptor u = source(e, g), v = target(e, g);
    put(d, v, get(d, u) + 1);
  }

private:
  DistanceMap d;
  svtkIdType* far_vertex;
  svtkIdType far_dist;
};

// Constructor/Destructor
svtkBoostBreadthFirstSearch::svtkBoostBreadthFirstSearch()
{
  // Default values for the origin vertex
  this->OriginVertexIndex = 0;
  this->InputArrayName = 0;
  this->OutputArrayName = 0;
  this->OutputSelectionType = 0;
  this->SetOutputSelectionType("MAX_DIST_FROM_ROOT");
  this->OriginValue = -1;
  this->OutputSelection = false;
  this->OriginFromSelection = false;
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
}

svtkBoostBreadthFirstSearch::~svtkBoostBreadthFirstSearch()
{
  this->SetInputArrayName(0);
  this->SetOutputArrayName(0);
  this->SetOutputSelectionType(0);
}

void svtkBoostBreadthFirstSearch::SetOriginSelection(svtkSelection* s)
{
  this->SetInputData(1, s);
}

// Description:
// Set the index (into the vertex array) of the
// breadth first search 'origin' vertex.
void svtkBoostBreadthFirstSearch::SetOriginVertex(svtkIdType index)
{
  this->OriginVertexIndex = index;
  this->InputArrayName = nullptr; // Reset any origin set by another method
  this->Modified();
}

// Description:
// Set the breadth first search 'origin' vertex.
// This method is basically the same as above
// but allows the application to simply specify
// an array name and value, instead of having to
// know the specific index of the vertex.
void svtkBoostBreadthFirstSearch::SetOriginVertex(svtkStdString arrayName, svtkVariant value)
{
  this->SetInputArrayName(arrayName);
  this->OriginValue = value;
  this->Modified();
}

void svtkBoostBreadthFirstSearch::SetOriginVertexString(char* arrayName, char* value)
{
  this->SetOriginVertex(arrayName, value);
}

svtkIdType svtkBoostBreadthFirstSearch::GetVertexIndex(svtkAbstractArray* abstract, svtkVariant value)
{

  // Okay now what type of array is it
  if (abstract->IsNumeric())
  {
    svtkDataArray* dataArray = svtkArrayDownCast<svtkDataArray>(abstract);
    int intValue = value.ToInt();
    for (int i = 0; i < dataArray->GetNumberOfTuples(); ++i)
    {
      if (intValue == static_cast<int>(dataArray->GetTuple1(i)))
      {
        return i;
      }
    }
  }
  else
  {
    svtkStringArray* stringArray = svtkArrayDownCast<svtkStringArray>(abstract);
    svtkStdString stringValue(value.ToString());
    for (int i = 0; i < stringArray->GetNumberOfTuples(); ++i)
    {
      if (stringValue == stringArray->GetValue(i))
      {
        return i;
      }
    }
  }

  // Failed
  svtkErrorMacro("Did not find a valid vertex index...");
  return 0;
}

int svtkBoostBreadthFirstSearch::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkGraph* output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  // Sanity check
  // The Boost BFS likes to crash on empty datasets
  if (input->GetNumberOfVertices() == 0)
  {
    // svtkWarningMacro("Empty input into " << this->GetClassName());
    return 1;
  }

  if (this->OriginFromSelection)
  {
    svtkSelection* selection = svtkSelection::GetData(inputVector[1], 0);
    if (selection == nullptr)
    {
      svtkErrorMacro("OriginFromSelection set but selection input undefined.");
      return 0;
    }
    svtkSmartPointer<svtkIdTypeArray> idArr = svtkSmartPointer<svtkIdTypeArray>::New();
    svtkConvertSelection::GetSelectedVertices(selection, input, idArr);
    if (idArr->GetNumberOfTuples() == 0)
    {
      svtkErrorMacro("Origin selection is empty.");
      return 0;
    }
    this->OriginVertexIndex = idArr->GetValue(0);
  }
  else
  {
    // Now figure out the origin vertex of the
    // breadth first search
    if (this->InputArrayName)
    {
      svtkAbstractArray* abstract = input->GetVertexData()->GetAbstractArray(this->InputArrayName);

      // Does the array exist at all?
      if (abstract == nullptr)
      {
        svtkErrorMacro("Could not find array named " << this->InputArrayName);
        return 0;
      }

      this->OriginVertexIndex = this->GetVertexIndex(abstract, this->OriginValue);
    }
  }

  // Create the attribute array
  svtkIntArray* BFSArray = svtkIntArray::New();
  if (this->OutputArrayName)
  {
    BFSArray->SetName(this->OutputArrayName);
  }
  else
  {
    BFSArray->SetName("BFS");
  }
  BFSArray->SetNumberOfTuples(output->GetNumberOfVertices());

  // Initialize the BFS array to all 0's
  for (int i = 0; i < BFSArray->GetNumberOfTuples(); ++i)
  {
    BFSArray->SetValue(i, SVTK_INT_MAX);
  }

  svtkIdType maxFromRootVertex = this->OriginVertexIndex;

  // Create a color map (used for marking visited nodes)
  vector_property_map<default_color_type> color(output->GetNumberOfVertices());

  // Set the distance to the source vertex to zero
  BFSArray->SetValue(this->OriginVertexIndex, 0);

  // Create a queue to hand off to the BFS
  boost::queue<int> Q;

  my_distance_recorder<svtkIntArray*> bfsVisitor(BFSArray, &maxFromRootVertex);

  // Is the graph directed or undirected
  if (svtkDirectedGraph::SafeDownCast(output))
  {
    svtkDirectedGraph* g = svtkDirectedGraph::SafeDownCast(output);
    breadth_first_search(g, this->OriginVertexIndex, Q, bfsVisitor, color);
  }
  else
  {
    svtkUndirectedGraph* g = svtkUndirectedGraph::SafeDownCast(output);
    breadth_first_search(g, this->OriginVertexIndex, Q, bfsVisitor, color);
  }

  // Add attribute array to the output
  output->GetVertexData()->AddArray(BFSArray);
  BFSArray->Delete();

  if (this->OutputSelection)
  {
    svtkSelection* sel = svtkSelection::GetData(outputVector, 1);
    svtkIdTypeArray* ids = svtkIdTypeArray::New();

    // Set the output based on the output selection type
    if (!strcmp(OutputSelectionType, "MAX_DIST_FROM_ROOT"))
    {
      ids->InsertNextValue(maxFromRootVertex);
    }

    svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
    sel->AddNode(node);
    node->SetSelectionList(ids);
    node->GetProperties()->Set(svtkSelectionNode::CONTENT_TYPE(), svtkSelectionNode::INDICES);
    node->GetProperties()->Set(svtkSelectionNode::FIELD_TYPE(), svtkSelectionNode::VERTEX);
    ids->Delete();
  }

  return 1;
}

void svtkBoostBreadthFirstSearch::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OriginVertexIndex: " << this->OriginVertexIndex << endl;

  os << indent << "InputArrayName: " << (this->InputArrayName ? this->InputArrayName : "(none)")
     << endl;

  os << indent << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(none)")
     << endl;

  os << indent << "OriginValue: " << this->OriginValue.ToString() << endl;

  os << indent << "OutputSelection: " << (this->OutputSelection ? "on" : "off") << endl;

  os << indent << "OriginFromSelection: " << (this->OriginFromSelection ? "on" : "off") << endl;

  os << indent << "OutputSelectionType: "
     << (this->OutputSelectionType ? this->OutputSelectionType : "(none)") << endl;
}

//----------------------------------------------------------------------------
int svtkBoostBreadthFirstSearch::FillInputPortInformation(int port, svtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkBoostBreadthFirstSearch::FillOutputPortInformation(int port, svtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkGraph");
  }
  else if (port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkSelection");
  }
  return 1;
}
