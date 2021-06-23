/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostGraphAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBoostBetweennessClustering.h"

#include "svtkBoostConnectedComponents.h"
#include "svtkBoostGraphAdapter.h"
#include "svtkDataSetAttributes.h"
#include "svtkDirectedGraph.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkUndirectedGraph.h"

#include <boost/graph/bc_clustering.hpp>

// @note: This piece of code is modification of algorithm from boost graph
// library. This modified version allows the user to pass edge weight map
namespace boost
{
// Graph clustering based on edge betweenness centrality.
//
// This algorithm implements graph clustering based on edge
// betweenness centrality. It is an iterative algorithm, where in each
// step it compute the edge betweenness centrality (via @ref
// brandes_betweenness_centrality) and removes the edge with the
// maximum betweenness centrality. The @p done function object
// determines when the algorithm terminates (the edge found when the
// algorithm terminates will not be removed).
//
// @param g The graph on which clustering will be performed. The type
// of this parameter (@c MutableGraph) must be a model of the
// VertexListGraph, IncidenceGraph, EdgeListGraph, and Mutable Graph
// concepts.
//
// @param done The function object that indicates termination of the
// algorithm. It must be a ternary function object that accepts the
// maximum centrality, the descriptor of the edge that will be
// removed, and the graph @p g.
//
// @param edge_centrality (UTIL/out2) The property map that will store
// the betweenness centrality for each edge. When the algorithm
// terminates, it will contain the edge centralities for the
// graph. The type of this property map must model the
// ReadWritePropertyMap concept. Defaults to a @c
// iterator_property_map whose value type is
// @c Done::centrality_type and using @c get(edge_index, g) for the
// index map.
//
// @param vertex_index (IN) The property map that maps vertices to
// indices in the range @c [0, num_vertices(g)). This type of this
// property map must model the ReadablePropertyMap concept and its
// value type must be an integral type. Defaults to
// @c get(vertex_index, g).
template <typename MutableGraph, typename Done, typename EdgeCentralityMap, typename EdgeWeightMap,
  typename VertexIndexMap>
void betweenness_centrality_clustering(MutableGraph& g, Done done,
  EdgeCentralityMap edge_centrality, EdgeWeightMap edge_weight_map, VertexIndexMap vertex_index)
{
  typedef typename property_traits<EdgeCentralityMap>::value_type centrality_type;
  typedef typename graph_traits<MutableGraph>::edge_iterator edge_iterator;
  typedef typename graph_traits<MutableGraph>::edge_descriptor edge_descriptor;

  if (has_no_edges(g))
    return;

  // Function object that compares the centrality of edges
  indirect_cmp<EdgeCentralityMap, std::less<centrality_type> > cmp(edge_centrality);

  bool is_done;
  do
  {
    brandes_betweenness_centrality(g,
      edge_centrality_map(edge_centrality)
        .vertex_index_map(vertex_index)
        .weight_map(edge_weight_map));
    std::pair<edge_iterator, edge_iterator> edges_iters = edges(g);
    edge_descriptor e = *max_element(edges_iters.first, edges_iters.second, cmp);
    is_done = done(get(edge_centrality, e), e, g);
    if (!is_done)
      remove_edge(e, g);
  } while (!is_done && !has_no_edges(g));
}
}

svtkStandardNewMacro(svtkBoostBetweennessClustering);

//-----------------------------------------------------------------------------
svtkBoostBetweennessClustering::svtkBoostBetweennessClustering()
  : svtkGraphAlgorithm()
  , Threshold(0)
  , UseEdgeWeightArray(false)
  , InvertEdgeWeightArray(false)
  , EdgeWeightArrayName(0)
  , EdgeCentralityArrayName(0)
{
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
svtkBoostBetweennessClustering::~svtkBoostBetweennessClustering()
{
  this->SetEdgeWeightArrayName(0);
  this->SetEdgeCentralityArrayName(0);
}

//-----------------------------------------------------------------------------
void svtkBoostBetweennessClustering::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Threshold: " << this->Threshold << endl;
  os << indent << "UseEdgeWeightArray: " << this->UseEdgeWeightArray << endl;
  os << indent << "InvertEdgeWeightArray: " << this->InvertEdgeWeightArray << endl;
  (EdgeWeightArrayName)
    ? os << indent << "EdgeWeightArrayName: " << this->EdgeWeightArrayName << endl
    : os << indent << "EdgeWeightArrayName: nullptr" << endl;

  (EdgeCentralityArrayName)
    ? os << indent << "EdgeCentralityArrayName: " << this->EdgeCentralityArrayName << endl
    : os << indent << "EdgeCentralityArrayName: nullptr" << endl;
}

//-----------------------------------------------------------------------------
int svtkBoostBetweennessClustering::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Helpful vars.
  bool isDirectedGraph(false);

  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    svtkErrorMacro("Failed to get input information.");
    return 1;
  }

  svtkInformation* outInfo1 = outputVector->GetInformationObject(0);
  if (!outInfo1)
  {
    svtkErrorMacro("Failed get output1 on information first port.");
  }

  svtkInformation* outInfo2 = outputVector->GetInformationObject(1);
  if (!outInfo2)
  {
    svtkErrorMacro("Failed to get output2 information on second port.");
    return 1;
  }

  // Get the input, output1 and output2.
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    svtkErrorMacro("Failed to get input graph.");
    return 1;
  }

  if (svtkDirectedGraph::SafeDownCast(input))
  {
    isDirectedGraph = true;
  }

  svtkGraph* output1 = svtkGraph::SafeDownCast(outInfo1->Get(svtkDataObject::DATA_OBJECT()));
  if (!output1)
  {
    svtkErrorMacro("Failed to get output1 graph.");
    return 1;
  }

  svtkGraph* output2 = svtkGraph::SafeDownCast(outInfo2->Get(svtkDataObject::DATA_OBJECT()));
  if (!output2)
  {
    svtkErrorMacro("Failed to get output2 graph.");
    return 1;
  }

  svtkSmartPointer<svtkFloatArray> edgeCM = svtkSmartPointer<svtkFloatArray>::New();
  if (this->EdgeCentralityArrayName)
  {
    edgeCM->SetName(this->EdgeCentralityArrayName);
  }
  else
  {
    edgeCM->SetName("edge_centrality");
  }

  boost::svtkGraphEdgePropertyMapHelper<svtkFloatArray*> helper(edgeCM);

  svtkSmartPointer<svtkDataArray> edgeWeight(0);
  if (this->UseEdgeWeightArray && this->EdgeWeightArrayName)
  {
    if (!this->InvertEdgeWeightArray)
    {
      edgeWeight = input->GetEdgeData()->GetArray(this->EdgeWeightArrayName);
    }
    else
    {
      svtkDataArray* weights = input->GetEdgeData()->GetArray(this->EdgeWeightArrayName);

      if (!weights)
      {
        svtkErrorMacro(<< "Error: Edge weight array " << this->EdgeWeightArrayName
                      << " is set but not found or not a data array.\n");
        return 1;
      }

      edgeWeight.TakeReference(svtkDataArray::CreateDataArray(weights->GetDataType()));

      double range[2];
      weights->GetRange(range);

      if (weights->GetNumberOfComponents() > 1)
      {
        svtkErrorMacro("Expecting single component array.");
        return 1;
      }

      for (int i = 0; i < weights->GetDataSize(); ++i)
      {
        edgeWeight->InsertNextTuple1(range[1] - weights->GetTuple1(i));
      }
    }

    if (!edgeWeight)
    {
      svtkErrorMacro(<< "Error: Edge weight array " << this->EdgeWeightArrayName
                    << " is set but not found or not a data array.\n");
      return 1;
    }
  }

  // First compute the second output and the result will be used
  // as input for the first output.
  if (isDirectedGraph)
  {
    svtkMutableDirectedGraph* out2 = svtkMutableDirectedGraph::New();

    // Copy the data to the second output (as this algorithm most likely
    // going to removed edges (and hence modifies the graph).
    out2->DeepCopy(input);

    if (edgeWeight)
    {
      boost::svtkGraphEdgePropertyMapHelper<svtkDataArray*> helper2(edgeWeight);
      boost::betweenness_centrality_clustering(out2,
        boost::bc_clustering_threshold<double>(this->Threshold, out2, false), helper, helper2,
        boost::get(boost::vertex_index, out2));
    }
    else
    {
      boost::betweenness_centrality_clustering(
        out2, boost::bc_clustering_threshold<double>(this->Threshold, out2, false), helper);
    }
    out2->GetEdgeData()->AddArray(edgeCM);

    // Finally copy the results to the output.
    output2->ShallowCopy(out2);

    out2->Delete();
  }
  else
  {
    svtkMutableUndirectedGraph* out2 = svtkMutableUndirectedGraph::New();

    // Send the data to output2.
    out2->DeepCopy(input);

    if (edgeWeight)
    {
      boost::svtkGraphEdgePropertyMapHelper<svtkDataArray*> helper2(edgeWeight);
      boost::betweenness_centrality_clustering(out2,
        boost::bc_clustering_threshold<double>(this->Threshold, out2, false), helper, helper2,
        boost::get(boost::vertex_index, out2));
    }
    else
    {
      boost::betweenness_centrality_clustering(
        out2, boost::bc_clustering_threshold<double>(this->Threshold, out2, false), helper);
    }
    out2->GetEdgeData()->AddArray(edgeCM);

    // Finally copy the results to the output.
    output2->ShallowCopy(out2);

    out2->Delete();
  }

  // Now take care of the first output.
  svtkSmartPointer<svtkBoostConnectedComponents> bcc(
    svtkSmartPointer<svtkBoostConnectedComponents>::New());

  svtkSmartPointer<svtkGraph> output2Copy(0);

  if (isDirectedGraph)
  {
    output2Copy = svtkSmartPointer<svtkDirectedGraph>::New();
  }
  else
  {
    output2Copy = svtkSmartPointer<svtkUndirectedGraph>::New();
  }

  output2Copy->ShallowCopy(output2);

  bcc->SetInputData(0, output2Copy);
  bcc->Update();

  svtkSmartPointer<svtkGraph> bccOut = bcc->GetOutput(0);

  svtkSmartPointer<svtkAbstractArray> compArray(0);
  if (isDirectedGraph)
  {
    svtkSmartPointer<svtkDirectedGraph> out1(svtkSmartPointer<svtkDirectedGraph>::New());
    out1->ShallowCopy(input);

    compArray = bccOut->GetVertexData()->GetAbstractArray("component");

    if (!compArray)
    {
      svtkErrorMacro("Unable to get component array.");
      return 1;
    }

    out1->GetVertexData()->AddArray(compArray);

    // Finally copy the output to the algorithm output.
    output1->ShallowCopy(out1);
  }
  else
  {
    svtkSmartPointer<svtkUndirectedGraph> out1(svtkSmartPointer<svtkUndirectedGraph>::New());
    out1->ShallowCopy(input);

    compArray = bccOut->GetVertexData()->GetAbstractArray("component");

    if (!compArray)
    {
      svtkErrorMacro("Unable to get component array.");
      return 1;
    }

    out1->GetVertexData()->AddArray(compArray);

    // Finally copy the output to the algorithm output.
    output1->ShallowCopy(out1);
  }

  // Also add the components array to the second output.
  output2->GetVertexData()->AddArray(compArray);

  return 1;
}

//-----------------------------------------------------------------------------
int svtkBoostBetweennessClustering::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0 || port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkGraph");
  }
  return 1;
}
