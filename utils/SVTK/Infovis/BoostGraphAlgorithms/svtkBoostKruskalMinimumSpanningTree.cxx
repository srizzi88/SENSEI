/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkBoostKruskalMinimumSpanningTree.cxx

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
#include "svtkBoostKruskalMinimumSpanningTree.h"

#include "svtkBoostGraphAdapter.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkUndirectedGraph.h"

#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/pending/queue.hpp>

using namespace boost;

svtkStandardNewMacro(svtkBoostKruskalMinimumSpanningTree);

// Constructor/Destructor
svtkBoostKruskalMinimumSpanningTree::svtkBoostKruskalMinimumSpanningTree()
{
  this->EdgeWeightArrayName = 0;
  this->OutputSelectionType = 0;
  this->SetOutputSelectionType("MINIMUM_SPANNING_TREE_EDGES");
  this->NegateEdgeWeights = false;
  this->EdgeWeightMultiplier = 1;
}

svtkBoostKruskalMinimumSpanningTree::~svtkBoostKruskalMinimumSpanningTree()
{
  this->SetEdgeWeightArrayName(0);
}

void svtkBoostKruskalMinimumSpanningTree::SetNegateEdgeWeights(bool value)
{
  this->NegateEdgeWeights = value;
  if (this->NegateEdgeWeights)
    this->EdgeWeightMultiplier = -1.0;
  else
    this->EdgeWeightMultiplier = 1.0;

  this->Modified();
}

int svtkBoostKruskalMinimumSpanningTree::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkSelection* output = svtkSelection::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Retrieve the edge-weight array.
  if (!this->EdgeWeightArrayName)
  {
    svtkErrorMacro("Edge-weight array name is required");
    return 0;
  }
  svtkDataArray* edgeWeightArray = input->GetEdgeData()->GetArray(this->EdgeWeightArrayName);

  // Does the edge-weight array exist at all?
  if (edgeWeightArray == nullptr)
  {
    svtkErrorMacro("Could not find edge-weight array named " << this->EdgeWeightArrayName);
    return 0;
  }

  // Send the property map through both the multiplier and the
  // helper (for edge_descriptor indexing)
  typedef svtkGraphPropertyMapMultiplier<svtkDataArray*> mapMulti;
  mapMulti multi(edgeWeightArray, this->EdgeWeightMultiplier);
  svtkGraphEdgePropertyMapHelper<mapMulti> weight_helper(multi);

  // Run the algorithm
  std::vector<svtkEdgeType> mstEdges;
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    svtkDirectedGraph* g = svtkDirectedGraph::SafeDownCast(input);
    kruskal_minimum_spanning_tree(g, std::back_inserter(mstEdges), weight_map(weight_helper));
  }
  else
  {
    svtkUndirectedGraph* g = svtkUndirectedGraph::SafeDownCast(input);
    kruskal_minimum_spanning_tree(g, std::back_inserter(mstEdges), weight_map(weight_helper));
  }

  // Select the minimum spanning tree edges.
  if (!strcmp(OutputSelectionType, "MINIMUM_SPANNING_TREE_EDGES"))
  {
    svtkIdTypeArray* ids = svtkIdTypeArray::New();

    // Add the ids of each MST edge.
    for (std::vector<svtkEdgeType>::iterator i = mstEdges.begin(); i != mstEdges.end(); ++i)
    {
      ids->InsertNextValue(i->Id);
    }

    svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
    output->AddNode(node);
    node->SetSelectionList(ids);
    node->SetContentType(svtkSelectionNode::INDICES);
    node->SetFieldType(svtkSelectionNode::EDGE);
    ids->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkBoostKruskalMinimumSpanningTree::FillInputPortInformation(int port, svtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkBoostKruskalMinimumSpanningTree::FillOutputPortInformation(int port, svtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkSelection");
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkBoostKruskalMinimumSpanningTree::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EdgeWeightArrayName: "
     << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(none)") << endl;

  os << indent << "OutputSelectionType: "
     << (this->OutputSelectionType ? this->OutputSelectionType : "(none)") << endl;

  os << indent << "NegateEdgeWeights: " << (this->NegateEdgeWeights ? "true" : "false") << endl;

  os << indent << "EdgeWeightMultiplier: " << this->EdgeWeightMultiplier << endl;
}
