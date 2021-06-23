/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostBrandesCentrality.cxx

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
#include "svtkBoostBrandesCentrality.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"

#include "svtkBoostGraphAdapter.h"
#include "svtkDirectedGraph.h"
#include "svtkUndirectedGraph.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/betweenness_centrality.hpp>
#include <boost/graph/properties.hpp>

using namespace boost;

svtkStandardNewMacro(svtkBoostBrandesCentrality);

//-----------------------------------------------------------------------------
svtkBoostBrandesCentrality::svtkBoostBrandesCentrality()
  : UseEdgeWeightArray(false)
  , InvertEdgeWeightArray(false)
  , EdgeWeightArrayName(nullptr)
{
}

//-----------------------------------------------------------------------------
svtkBoostBrandesCentrality::~svtkBoostBrandesCentrality()
{
  this->SetEdgeWeightArrayName(nullptr);
}

//-----------------------------------------------------------------------------
void svtkBoostBrandesCentrality::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "UseEdgeWeightArray: " << this->UseEdgeWeightArray << endl;

  os << indent << "InvertEdgeWeightArray: " << this->InvertEdgeWeightArray << endl;

  os << indent << "this->EdgeWeightArrayName: "
     << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "nullptr") << endl;
}

//-----------------------------------------------------------------------------
int svtkBoostBrandesCentrality::RequestData(svtkInformation* svtkNotUsed(request),
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

  // Compute betweenness centrality

  // Property map for vertices
  svtkFloatArray* vertexCMap = svtkFloatArray::New();
  vertexCMap->SetName("centrality");
  identity_property_map imap;

  // Property map for edges
  svtkFloatArray* edgeCMap = svtkFloatArray::New();
  edgeCMap->SetName("centrality");
  svtkGraphEdgePropertyMapHelper<svtkFloatArray*> helper(edgeCMap);

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
        return 0;
      }

      edgeWeight.TakeReference(svtkDataArray::CreateDataArray(weights->GetDataType()));

      double range[2];
      weights->GetRange(range);

      if (weights->GetNumberOfComponents() > 1)
      {
        return 0;
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
      return 0;
    }
  }

  // Is the graph directed or undirected
  if (svtkDirectedGraph::SafeDownCast(output))
  {
    svtkDirectedGraph* g = svtkDirectedGraph::SafeDownCast(output);
    if (edgeWeight)
    {
      svtkGraphEdgePropertyMapHelper<svtkDataArray*> helper2(edgeWeight);
      brandes_betweenness_centrality(g,
        centrality_map(vertexCMap)
          .edge_centrality_map(helper)
          .vertex_index_map(imap)
          .weight_map(helper2));
    }
    else
    {
      brandes_betweenness_centrality(
        g, centrality_map(vertexCMap).edge_centrality_map(helper).vertex_index_map(imap));
    }
  }
  else
  {
    svtkUndirectedGraph* g = svtkUndirectedGraph::SafeDownCast(output);
    if (edgeWeight)
    {
      svtkGraphEdgePropertyMapHelper<svtkDataArray*> helper2(edgeWeight);
      brandes_betweenness_centrality(g,
        centrality_map(vertexCMap)
          .edge_centrality_map(helper)
          .vertex_index_map(imap)
          .weight_map(helper2));
    }
    else
    {
      brandes_betweenness_centrality(
        g, centrality_map(vertexCMap).edge_centrality_map(helper).vertex_index_map(imap));
    }
  }

  // Add the arrays to the output and dereference
  output->GetVertexData()->AddArray(vertexCMap);
  vertexCMap->Delete();
  output->GetEdgeData()->AddArray(edgeCMap);
  edgeCMap->Delete();

  return 1;
}
