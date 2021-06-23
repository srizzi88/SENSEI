/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkBoostPrimMinimumSpanningTree.cxx

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
#include "svtkBoostPrimMinimumSpanningTree.h"

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
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkUndirectedGraph.h"

#include <boost/graph/prim_minimum_spanning_tree.hpp>
#include <boost/pending/queue.hpp>

using namespace boost;

svtkStandardNewMacro(svtkBoostPrimMinimumSpanningTree);

// Constructor/Destructor
svtkBoostPrimMinimumSpanningTree::svtkBoostPrimMinimumSpanningTree()
{
  this->EdgeWeightArrayName = 0;
  this->OriginVertexIndex = 0;
  this->ArrayName = 0;
  this->SetArrayName("Not Set");
  this->ArrayNameSet = false;
  this->NegateEdgeWeights = false;
  this->EdgeWeightMultiplier = 1;
  this->OriginValue = 0;
  this->CreateGraphVertexIdArray = false;
}

//----------------------------------------------------------------------------
svtkBoostPrimMinimumSpanningTree::~svtkBoostPrimMinimumSpanningTree()
{
  this->SetEdgeWeightArrayName(0);
  this->SetArrayName(0);
}

// Description:
// Set the index (into the vertex array) of the
// breadth first search 'origin' vertex.
void svtkBoostPrimMinimumSpanningTree::SetOriginVertex(svtkIdType index)
{
  this->OriginVertexIndex = index;
  this->Modified();
}

// Description:
// Set the breadth first search 'origin' vertex.
// This method is basically the same as above
// but allows the application to simply specify
// an array name and value, instead of having to
// know the specific index of the vertex.
void svtkBoostPrimMinimumSpanningTree::SetOriginVertex(svtkStdString arrayName, svtkVariant value)
{
  this->SetArrayName(arrayName);
  this->ArrayNameSet = true;
  this->OriginValue = value;
  this->Modified();
}

void svtkBoostPrimMinimumSpanningTree::SetNegateEdgeWeights(bool value)
{
  this->NegateEdgeWeights = value;
  if (this->NegateEdgeWeights)
  {
    svtkWarningMacro("The Boost implementation of Prim's minimum spanning tree algorithm does not "
                    "allow negation of edge weights.");
    return;

    // this->EdgeWeightMultiplier = -1.0;
  }
  else
    this->EdgeWeightMultiplier = 1.0;

  this->Modified();
}

//----------------------------------------------------------------------------
svtkIdType svtkBoostPrimMinimumSpanningTree::GetVertexIndex(
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

//----------------------------------------------------------------------------
int svtkBoostPrimMinimumSpanningTree::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Now figure out the origin vertex of the MST
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

  // Create the mutable graph to build the tree
  svtkSmartPointer<svtkMutableDirectedGraph> temp = svtkSmartPointer<svtkMutableDirectedGraph>::New();

  // Initialize copying data into tree
  temp->GetFieldData()->PassData(input->GetFieldData());
  temp->GetVertexData()->PassData(input->GetVertexData());
  temp->GetPoints()->ShallowCopy(input->GetPoints());
  // FIXME - We're not copying the edge data, see FIXME note below.
  //  temp->GetEdgeData()->CopyAllocate(input->GetEdgeData());

  // Send the property map through both the multiplier and the
  // helper (for edge_descriptor indexing)
  typedef svtkGraphPropertyMapMultiplier<svtkDataArray*> mapMulti;
  mapMulti multi(edgeWeightArray, this->EdgeWeightMultiplier);
  svtkGraphEdgePropertyMapHelper<mapMulti> weight_helper(multi);

  // Create a predecessorMap
  svtkIdTypeArray* predecessorMap = svtkIdTypeArray::New();

  // Run the algorithm
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    svtkDirectedGraph* g = svtkDirectedGraph::SafeDownCast(input);
    prim_minimum_spanning_tree(
      g, predecessorMap, weight_map(weight_helper).root_vertex(this->OriginVertexIndex));
  }
  else
  {
    svtkUndirectedGraph* g = svtkUndirectedGraph::SafeDownCast(input);
    prim_minimum_spanning_tree(
      g, predecessorMap, weight_map(weight_helper).root_vertex(this->OriginVertexIndex));
  }

  svtkIdType i;
  if (temp->SetNumberOfVertices(input->GetNumberOfVertices()) < 0)
  { // The graph must be distributed.
    svtkErrorMacro("Prim MST algorithm will not work on distributed graphs.");
    return 0;
  }
  for (i = 0; i < temp->GetNumberOfVertices(); i++)
  {
    if (predecessorMap->GetValue(i) == i)
    {
      if (i == this->OriginVertexIndex)
      {
        continue;
      }
      else
      {
        svtkErrorMacro("Unexpected result: MST is a forest (collection of trees).");
        return 0;
      }
    }

    temp->AddEdge(predecessorMap->GetValue(i), i);

    // FIXME - We're not copying the edge data from the graph to the MST because
    //  of the ambiguity associated with copying data when parallel edges between
    //  vertices in the original graph exist.
    //    temp->GetEdgeData()->CopyData(input->GetEdgeData(), e.Id, tree_e.Id);
  }

  if (this->CreateGraphVertexIdArray)
  {
    predecessorMap->SetName("predecessorMap");
    temp->GetVertexData()->AddArray(predecessorMap);
  }

  // Copy the builder graph structure into the output tree
  svtkTree* output = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!output->CheckedShallowCopy(temp))
  {
    svtkErrorMacro(<< "Invalid tree.");
    return 0;
  }

  predecessorMap->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkBoostPrimMinimumSpanningTree::FillInputPortInformation(int port, svtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkBoostPrimMinimumSpanningTree::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OriginVertexIndex: " << this->OriginVertexIndex << endl;

  os << indent << "ArrayName: " << (this->ArrayName ? this->ArrayName : "(none)") << endl;

  os << indent << "OriginValue: " << this->OriginValue.ToString() << endl;

  os << indent << "ArrayNameSet: " << (this->ArrayNameSet ? "true" : "false") << endl;

  os << indent << "NegateEdgeWeights: " << (this->NegateEdgeWeights ? "true" : "false") << endl;

  os << indent << "EdgeWeightMultiplier: " << this->EdgeWeightMultiplier << endl;

  os << indent << "CreateGraphVertexIdArray: " << (this->CreateGraphVertexIdArray ? "on" : "off")
     << endl;

  os << indent << "EdgeWeightArrayName: "
     << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(none)") << endl;
}
