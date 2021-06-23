/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollapseGraph.cxx

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
#include "svtkCollapseGraph.h"

#include "svtkConvertSelection.h"
#include "svtkDataSetAttributes.h"
#include "svtkEdgeListIterator.h"
#include "svtkIdTypeArray.h"
#include "svtkInEdgeIterator.h"
#include "svtkInformation.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSmartPointer.h"

#include <vector>

/// Defines storage for a collection of edges
typedef std::vector<svtkEdgeType> EdgeListT;

///////////////////////////////////////////////////////////////////////////////////
// BuildGraph

template <typename GraphT>
static void BuildGraph(svtkGraph* input_graph, const std::vector<svtkIdType>& vertex_map,
  const EdgeListT& edge_list, svtkGraph* destination_graph)
{
  svtkSmartPointer<GraphT> output_graph = svtkSmartPointer<GraphT>::New();

  output_graph->GetFieldData()->ShallowCopy(input_graph->GetFieldData());

  svtkDataSetAttributes* const input_vertex_data = input_graph->GetVertexData();
  svtkDataSetAttributes* const output_vertex_data = output_graph->GetVertexData();
  output_vertex_data->CopyAllocate(input_vertex_data);
  for (std::vector<svtkIdType>::size_type i = 0; i != vertex_map.size(); ++i)
  {
    if (vertex_map[i] == -1)
      continue;

    output_graph->AddVertex();
    output_vertex_data->CopyData(input_vertex_data, static_cast<svtkIdType>(i), vertex_map[i]);
  }

  svtkDataSetAttributes* const input_edge_data = input_graph->GetEdgeData();
  svtkDataSetAttributes* const output_edge_data = output_graph->GetEdgeData();
  output_edge_data->CopyAllocate(input_edge_data);
  for (EdgeListT::const_iterator input_edge = edge_list.begin(); input_edge != edge_list.end();
       ++input_edge)
  {
    svtkEdgeType output_edge =
      output_graph->AddEdge(vertex_map[input_edge->Source], vertex_map[input_edge->Target]);
    output_edge_data->CopyData(input_edge_data, input_edge->Id, output_edge.Id);
  }

  destination_graph->ShallowCopy(output_graph);
}

///////////////////////////////////////////////////////////////////////////////////
// svtkCollapseGraph

svtkStandardNewMacro(svtkCollapseGraph);

svtkCollapseGraph::svtkCollapseGraph()
{
  this->SetNumberOfInputPorts(2);
}

svtkCollapseGraph::~svtkCollapseGraph() = default;

void svtkCollapseGraph::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkCollapseGraph::SetGraphConnection(svtkAlgorithmOutput* input)
{
  this->SetInputConnection(0, input);
}

void svtkCollapseGraph::SetSelectionConnection(svtkAlgorithmOutput* input)
{
  this->SetInputConnection(1, input);
}

int svtkCollapseGraph::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    return 1;
  }

  return 0;
}

int svtkCollapseGraph::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Ensure we have valid inputs ...
  svtkGraph* const input_graph = svtkGraph::GetData(inputVector[0]);
  svtkGraph* const output_graph = svtkGraph::GetData(outputVector);

  svtkSmartPointer<svtkIdTypeArray> input_indices = svtkSmartPointer<svtkIdTypeArray>::New();
  svtkConvertSelection::GetSelectedVertices(
    svtkSelection::GetData(inputVector[1]), input_graph, input_indices);

  // Convert the input selection into an "expanding" array that contains "true" for each
  // vertex that is expanding (i.e. its neighbors are collapsing into it)
  std::vector<bool> expanding(input_graph->GetNumberOfVertices(), false);

  for (svtkIdType i = 0; i != input_indices->GetNumberOfTuples(); ++i)
  {
    expanding[input_indices->GetValue(i)] = true;
  }

  // Create a mapping from each child vertex to its expanding neighbor (if any)
  std::vector<svtkIdType> parent(input_graph->GetNumberOfVertices());
  svtkSmartPointer<svtkInEdgeIterator> in_edge_iterator = svtkSmartPointer<svtkInEdgeIterator>::New();
  for (svtkIdType vertex = 0; vertex != input_graph->GetNumberOfVertices(); ++vertex)
  {
    // By default, vertices map to themselves, i.e: they aren't collapsed
    parent[vertex] = vertex;

    if (expanding[vertex])
      continue;

    input_graph->GetInEdges(vertex, in_edge_iterator);
    while (in_edge_iterator->HasNext())
    {
      const svtkIdType adjacent_vertex = in_edge_iterator->Next().Source;
      if (expanding[adjacent_vertex])
      {
        parent[vertex] = adjacent_vertex;
        break;
      }
    }
  }

  // Create a mapping from vertex IDs in the original graph to vertex IDs in the output graph
  std::vector<svtkIdType> vertex_map(input_graph->GetNumberOfVertices(), -1);
  for (svtkIdType old_vertex = 0, new_vertex = 0; old_vertex != input_graph->GetNumberOfVertices();
       ++old_vertex)
  {
    if (parent[old_vertex] != old_vertex)
      continue;

    vertex_map[old_vertex] = new_vertex++;
  }

  // Create a new edge list, mapping each edge from children to parents, eliminating duplicates as
  // we go
  EdgeListT edge_list;

  svtkSmartPointer<svtkEdgeListIterator> edge_iterator = svtkSmartPointer<svtkEdgeListIterator>::New();
  input_graph->GetEdges(edge_iterator);
  while (edge_iterator->HasNext())
  {
    svtkEdgeType edge = edge_iterator->Next();

    edge.Source = parent[edge.Source];
    edge.Target = parent[edge.Target];
    if (edge.Source == edge.Target)
      continue;

    edge_list.push_back(edge);
  }

  // Build the new output graph, based on the graph type ...
  if (svtkDirectedGraph::SafeDownCast(input_graph))
  {
    BuildGraph<svtkMutableDirectedGraph>(input_graph, vertex_map, edge_list, output_graph);
  }
  else if (svtkUndirectedGraph::SafeDownCast(input_graph))
  {
    BuildGraph<svtkMutableUndirectedGraph>(input_graph, vertex_map, edge_list, output_graph);
  }
  else
  {
    svtkErrorMacro(<< "Unknown input graph type");
    return 0;
  }

  return 1;
}
