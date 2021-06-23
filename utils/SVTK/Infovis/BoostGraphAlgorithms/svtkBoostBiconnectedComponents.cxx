/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostBiconnectedComponents.cxx

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
#include "svtkBoostBiconnectedComponents.h"

#include "svtkCellData.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkVertexListIterator.h"

#include "svtkBoostGraphAdapter.h"
#include "svtkGraph.h"
#include <boost/graph/biconnected_components.hpp>
#include <boost/version.hpp>
#include <utility>
#include <vector>

using namespace boost;

svtkStandardNewMacro(svtkBoostBiconnectedComponents);

svtkBoostBiconnectedComponents::svtkBoostBiconnectedComponents()
{
  this->OutputArrayName = 0;
}

svtkBoostBiconnectedComponents::~svtkBoostBiconnectedComponents()
{
  // release mem
  this->SetOutputArrayName(0);
}

int svtkBoostBiconnectedComponents::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkUndirectedGraph* input =
    svtkUndirectedGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUndirectedGraph* output =
    svtkUndirectedGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  // Create edge biconnected component array.
  // This will be populated directly by the boost algorithm.
  svtkSmartPointer<svtkIntArray> edgeCompArr = svtkSmartPointer<svtkIntArray>::New();
  edgeCompArr->SetNumberOfTuples(input->GetNumberOfEdges());
  for (svtkIdType i = 0; i < input->GetNumberOfEdges(); ++i)
  {
    edgeCompArr->SetValue(i, -1);
  }
  if (this->OutputArrayName)
  {
    edgeCompArr->SetName(this->OutputArrayName);
  }
  else
  {
    edgeCompArr->SetName("biconnected component");
  }
  svtkGraphEdgePropertyMapHelper<svtkIntArray*> helper(edgeCompArr);

  // Create vector of articulation points and set it up for insertion
  // by the algorithm.
  std::vector<svtkIdType> artPoints;
  std::pair<size_t, std::back_insert_iterator<std::vector<svtkIdType> > > res(
    0, std::back_inserter(artPoints));

  // Call BGL biconnected_components.
  // It appears that the signature for this
  // algorithm has changed in 1.32, 1.33, and 1.34 ;p
#if BOOST_VERSION < 103300 // Boost 1.32.x
  // TODO I have no idea what the 1.32 signature is suppose to be
  // res = biconnected_components(
  //  output, helper, std::back_inserter(artPoints), svtkGraphIndexMap());
#elif BOOST_VERSION < 103400 // Boost 1.33.x
  res = biconnected_components(output, helper, std::back_inserter(artPoints), svtkGraphIndexMap());
#else                        // Anything after Boost 1.34.x
  res = biconnected_components(
    output, helper, std::back_inserter(artPoints), vertex_index_map(svtkGraphIndexMap()));
#endif

  size_t numComp = res.first;

  // Assign component values to vertices based on the first edge.
  // If isolated, assign a new value.
  svtkSmartPointer<svtkIntArray> vertCompArr = svtkSmartPointer<svtkIntArray>::New();
  if (this->OutputArrayName)
  {
    vertCompArr->SetName(this->OutputArrayName);
  }
  else
  {
    vertCompArr->SetName("biconnected component");
  }
  vertCompArr->SetNumberOfTuples(output->GetNumberOfVertices());
  svtkSmartPointer<svtkVertexListIterator> vertIt = svtkSmartPointer<svtkVertexListIterator>::New();
  svtkSmartPointer<svtkOutEdgeIterator> edgeIt = svtkSmartPointer<svtkOutEdgeIterator>::New();
  output->GetVertices(vertIt);
  while (vertIt->HasNext())
  {
    svtkIdType u = vertIt->Next();
    output->GetOutEdges(u, edgeIt);
    int comp = -1;
    while (edgeIt->HasNext() && comp == -1)
    {
      svtkOutEdgeType e = edgeIt->Next();
      int value = edgeCompArr->GetValue(e.Id);
      comp = value;
    }
    if (comp == -1)
    {
      comp = static_cast<int>(numComp);
      numComp++;
    }
    vertCompArr->SetValue(u, comp);
  }

  // Articulation points belong to multiple biconnected components.
  // Indicate these by assigning a component value of -1.
  // It belongs to whatever components its incident edges belong to.
  std::vector<svtkIdType>::size_type i;
  for (i = 0; i < artPoints.size(); i++)
  {
    vertCompArr->SetValue(artPoints[i], -1);
  }

  // Add edge and vertex component arrays to the output
  output->GetEdgeData()->AddArray(edgeCompArr);
  output->GetVertexData()->AddArray(vertCompArr);

  return 1;
}

void svtkBoostBiconnectedComponents::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(none)")
     << endl;
}
