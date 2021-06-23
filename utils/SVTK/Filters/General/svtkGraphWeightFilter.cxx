/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphWeightFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGraphWeightFilter.h"

#include "svtkEdgeListIterator.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"

bool svtkGraphWeightFilter::CheckRequirements(svtkGraph* const svtkNotUsed(graph)) const
{
  return true;
}

int svtkGraphWeightFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkGraph* output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Copy the input graph to the output.
  // We want to keep the vertices and edges, just add a weight array.
  output->ShallowCopy(input);

  if (!this->CheckRequirements(input))
  {
    svtkErrorMacro(<< "Requirements are not met!");
    return 0;
  }

  // Create the edge weight array
  svtkSmartPointer<svtkFloatArray> weights = svtkSmartPointer<svtkFloatArray>::New();
  weights->SetNumberOfComponents(1);
  weights->SetNumberOfTuples(input->GetNumberOfEdges());
  weights->SetName("Weights");

  // Compute the Weight function (in a subclass) for every edge
  svtkSmartPointer<svtkEdgeListIterator> edgeListIterator =
    svtkSmartPointer<svtkEdgeListIterator>::New();
  input->GetEdges(edgeListIterator);

  while (edgeListIterator->HasNext())
  {
    svtkEdgeType edge = edgeListIterator->Next();

    float w = this->ComputeWeight(input, edge);

    weights->SetValue(edge.Id, w);
  }

  output->SetPoints(input->GetPoints());
  output->GetEdgeData()->AddArray(weights);

  return 1;
}

//----------------------------------------------------------------------------
void svtkGraphWeightFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkGraphAlgorithm::PrintSelf(os, indent);
}
