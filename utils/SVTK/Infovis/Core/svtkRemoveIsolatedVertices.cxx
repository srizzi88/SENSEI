/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRemoveIsolatedVertices.cxx

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

#include "svtkRemoveIsolatedVertices.h"

#include "svtkDataSetAttributes.h"
#include "svtkEdgeListIterator.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableGraphHelper.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"

#include <vector>

svtkStandardNewMacro(svtkRemoveIsolatedVertices);
//----------------------------------------------------------------------------
svtkRemoveIsolatedVertices::svtkRemoveIsolatedVertices() = default;

//----------------------------------------------------------------------------
svtkRemoveIsolatedVertices::~svtkRemoveIsolatedVertices() = default;

//----------------------------------------------------------------------------
int svtkRemoveIsolatedVertices::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkGraph* input = svtkGraph::GetData(inputVector[0]);

  // Set up our mutable graph helper.
  svtkSmartPointer<svtkMutableGraphHelper> builder = svtkSmartPointer<svtkMutableGraphHelper>::New();
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    svtkSmartPointer<svtkMutableDirectedGraph> dir = svtkSmartPointer<svtkMutableDirectedGraph>::New();
    builder->SetGraph(dir);
  }
  else
  {
    svtkSmartPointer<svtkMutableUndirectedGraph> undir =
      svtkSmartPointer<svtkMutableUndirectedGraph>::New();
    builder->SetGraph(undir);
  }

  // Initialize edge data, vertex data, and points.
  svtkDataSetAttributes* inputEdgeData = input->GetEdgeData();
  svtkDataSetAttributes* builderEdgeData = builder->GetGraph()->GetEdgeData();
  builderEdgeData->CopyAllocate(inputEdgeData);

  svtkDataSetAttributes* inputVertData = input->GetVertexData();
  svtkDataSetAttributes* builderVertData = builder->GetGraph()->GetVertexData();
  builderVertData->CopyAllocate(inputVertData);

  svtkPoints* inputPoints = input->GetPoints();
  svtkSmartPointer<svtkPoints> builderPoints = svtkSmartPointer<svtkPoints>::New();
  builder->GetGraph()->SetPoints(builderPoints);

  // Vector keeps track of mapping of input vertex ids to
  // output vertex ids.
  svtkIdType numVert = input->GetNumberOfVertices();
  std::vector<int> outputVertex(numVert, -1);

  svtkSmartPointer<svtkEdgeListIterator> edgeIter = svtkSmartPointer<svtkEdgeListIterator>::New();
  input->GetEdges(edgeIter);
  while (edgeIter->HasNext())
  {
    svtkEdgeType e = edgeIter->Next();
    svtkIdType source = outputVertex[e.Source];
    if (source < 0)
    {
      source = builder->AddVertex();
      outputVertex[e.Source] = source;
      builderVertData->CopyData(inputVertData, e.Source, source);
      builderPoints->InsertNextPoint(inputPoints->GetPoint(e.Source));
    }
    svtkIdType target = outputVertex[e.Target];
    if (target < 0)
    {
      target = builder->AddVertex();
      outputVertex[e.Target] = target;
      builderVertData->CopyData(inputVertData, e.Target, target);
      builderPoints->InsertNextPoint(inputPoints->GetPoint(e.Target));
    }
    svtkEdgeType outputEdge = builder->AddEdge(source, target);
    builderEdgeData->CopyData(inputEdgeData, e.Id, outputEdge.Id);
  }

  // Pass constructed graph to output.
  svtkGraph* output = svtkGraph::GetData(outputVector);
  output->ShallowCopy(builder->GetGraph());
  output->GetFieldData()->PassData(input->GetFieldData());

  // Clean up
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void svtkRemoveIsolatedVertices::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
