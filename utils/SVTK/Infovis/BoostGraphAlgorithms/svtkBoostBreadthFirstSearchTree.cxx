/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostBreadthFirstSearchTree.cxx

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
#include "svtkBoostBreadthFirstSearchTree.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"

#include "svtkBoostGraphAdapter.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkTree.h"
#include "svtkUndirectedGraph.h"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/pending/queue.hpp>

using namespace boost;

svtkStandardNewMacro(svtkBoostBreadthFirstSearchTree);

#if BOOST_VERSION >= 104800 // Boost 1.48.x
namespace
{
svtkIdType unwrap_edge_id(svtkEdgeType const& e)
{
  return e.Id;
}
svtkIdType unwrap_edge_id(boost::detail::reverse_graph_edge_descriptor<svtkEdgeType> const& e)
{
#if BOOST_VERSION == 104800
  return e.underlying_desc.Id;
#else
  return e.underlying_descx.Id;
#endif
}
}
#endif

// Redefine the bfs visitor, the only visitor we
// are using is the tree_edge visitor.
template <typename IdMap>
class bfs_tree_builder : public default_bfs_visitor
{
public:
  bfs_tree_builder() {}

  bfs_tree_builder(IdMap& g2t, IdMap& t2g, svtkGraph* g, svtkMutableDirectedGraph* t, svtkIdType root)
    : graph_to_tree(g2t)
    , tree_to_graph(t2g)
    , tree(t)
    , graph(g)
  {
    double x[3];
    graph->GetPoints()->GetPoint(root, x);
    tree->GetPoints()->InsertNextPoint(x);
    svtkIdType tree_root = t->AddVertex();
    put(graph_to_tree, root, tree_root);
    put(tree_to_graph, tree_root, root);
    tree->GetVertexData()->CopyData(graph->GetVertexData(), root, tree_root);
  }

  template <typename Edge, typename Graph>
  void tree_edge(Edge e, const Graph& g) const
  {
    typename graph_traits<Graph>::vertex_descriptor u, v;
    u = source(e, g);
    v = target(e, g);

    // Get the source vertex id (it has already been visited).
    svtkIdType tree_u = get(graph_to_tree, u);

    // Add the point before the vertex so that
    // points match the number of vertices, so that GetPoints()
    // doesn't reallocate and zero-out points.
    double x[3];
    graph->GetPoints()->GetPoint(v, x);
    tree->GetPoints()->InsertNextPoint(x);

    // Create the target vertex in the tree.
    svtkIdType tree_v = tree->AddVertex();
    svtkEdgeType tree_e = tree->AddEdge(tree_u, tree_v);

    // Store the mapping from graph to tree.
    put(graph_to_tree, v, tree_v);
    put(tree_to_graph, tree_v, v);

    // Copy the vertex and edge data from the graph to the tree.
    tree->GetVertexData()->CopyData(graph->GetVertexData(), v, tree_v);
#if BOOST_VERSION < 104800 // Boost 1.48.x
    tree->GetEdgeData()->CopyData(graph->GetEdgeData(), e.Id, tree_e.Id);
#else
    tree->GetEdgeData()->CopyData(graph->GetEdgeData(), unwrap_edge_id(e), tree_e.Id);
#endif
  }

private:
  IdMap graph_to_tree;
  IdMap tree_to_graph;
  svtkMutableDirectedGraph* tree;
  svtkGraph* graph;
};

// Constructor/Destructor
svtkBoostBreadthFirstSearchTree::svtkBoostBreadthFirstSearchTree()
{
  // Default values for the origin vertex
  this->OriginVertexIndex = 0;
  this->ArrayName = 0;
  this->SetArrayName("Not Set");
  this->ArrayNameSet = false;
  this->OriginValue = 0;
  this->CreateGraphVertexIdArray = false;
  this->ReverseEdges = false;
}

svtkBoostBreadthFirstSearchTree::~svtkBoostBreadthFirstSearchTree()
{
  this->SetArrayName(nullptr);
}

// Description:
// Set the index (into the vertex array) of the
// breadth first search 'origin' vertex.
void svtkBoostBreadthFirstSearchTree::SetOriginVertex(svtkIdType index)
{
  this->OriginVertexIndex = index;
  this->ArrayNameSet = false;
  this->Modified();
}

// Description:
// Set the breadth first search 'origin' vertex.
// This method is basically the same as above
// but allows the application to simply specify
// an array name and value, instead of having to
// know the specific index of the vertex.
void svtkBoostBreadthFirstSearchTree::SetOriginVertex(svtkStdString arrayName, svtkVariant value)
{
  this->SetArrayName(arrayName);
  this->ArrayNameSet = true;
  this->OriginValue = value;
  this->Modified();
}

svtkIdType svtkBoostBreadthFirstSearchTree::GetVertexIndex(
  svtkAbstractArray* abstract, svtkVariant value)
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

int svtkBoostBreadthFirstSearchTree::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

int svtkBoostBreadthFirstSearchTree::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Now figure out the origin vertex of the
  // breadth first search
  if (this->ArrayNameSet)
  {
    svtkAbstractArray* abstract = input->GetVertexData()->GetAbstractArray(this->ArrayName);

    // Does the array exist at all?
    if (abstract == nullptr)
    {
      svtkErrorMacro("Could not find array named " << this->ArrayName);
      return 0;
    }

    this->OriginVertexIndex = this->GetVertexIndex(abstract, this->OriginValue);
  }

  // Create tree to graph id map array
  svtkIdTypeArray* treeToGraphIdMap = svtkIdTypeArray::New();

  // Create graph to tree id map array
  svtkIdTypeArray* graphToTreeIdMap = svtkIdTypeArray::New();

  // Create a color map (used for marking visited nodes)
  vector_property_map<default_color_type> color;

  // Create a queue to hand off to the BFS
  queue<int> q;

  // Create the mutable graph to build the tree
  svtkSmartPointer<svtkMutableDirectedGraph> temp = svtkSmartPointer<svtkMutableDirectedGraph>::New();
  // Initialize copying data into tree
  temp->GetFieldData()->PassData(input->GetFieldData());
  temp->GetVertexData()->CopyAllocate(input->GetVertexData());
  temp->GetEdgeData()->CopyAllocate(input->GetEdgeData());

  // Create the visitor which will build the tree
  bfs_tree_builder<svtkIdTypeArray*> builder(
    graphToTreeIdMap, treeToGraphIdMap, input, temp, this->OriginVertexIndex);

  // Run the algorithm
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    svtkDirectedGraph* g = svtkDirectedGraph::SafeDownCast(input);
    if (this->ReverseEdges)
    {
#if BOOST_VERSION < 104100 // Boost 1.41.x
      svtkErrorMacro("ReverseEdges requires Boost 1.41.x or higher");
      return 0;
#else
      boost::reverse_graph<svtkDirectedGraph*> r(g);
      breadth_first_search(r, this->OriginVertexIndex, q, builder, color);
#endif
    }
    else
    {
      breadth_first_search(g, this->OriginVertexIndex, q, builder, color);
    }
  }
  else
  {
    svtkUndirectedGraph* g = svtkUndirectedGraph::SafeDownCast(input);
    breadth_first_search(g, this->OriginVertexIndex, q, builder, color);
  }

  // If the user wants it, store the mapping back to graph vertices
  if (this->CreateGraphVertexIdArray)
  {
    treeToGraphIdMap->SetName("GraphVertexId");
    temp->GetVertexData()->AddArray(treeToGraphIdMap);
  }

  // Copy the builder graph structure into the output tree
  svtkTree* output = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!output->CheckedShallowCopy(temp))
  {
    svtkErrorMacro(<< "Invalid tree.");
    return 0;
  }

  // Clean up
  output->Squeeze();
  graphToTreeIdMap->Delete();
  treeToGraphIdMap->Delete();

  return 1;
}

void svtkBoostBreadthFirstSearchTree::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OriginVertexIndex: " << this->OriginVertexIndex << endl;

  os << indent << "ArrayName: " << (this->ArrayName ? this->ArrayName : "(none)") << endl;

  os << indent << "OriginValue: " << this->OriginValue.ToString() << endl;

  os << indent << "ArrayNameSet: " << (this->ArrayNameSet ? "true" : "false") << endl;

  os << indent << "CreateGraphVertexIdArray: " << (this->CreateGraphVertexIdArray ? "on" : "off")
     << endl;

  os << indent << "ReverseEdges: " << (this->ReverseEdges ? "on" : "off") << endl;
}
