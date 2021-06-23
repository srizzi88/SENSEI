/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostExtractLargestComponent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBoostExtractLargestComponent.h"

#include "svtkBoostConnectedComponents.h"
#include "svtkDataSetAttributes.h"
#include "svtkDirectedGraph.h"
#include "svtkExecutive.h"
#include "svtkExtractSelectedGraph.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkUndirectedGraph.h"

#include <algorithm>

svtkStandardNewMacro(svtkBoostExtractLargestComponent);

svtkBoostExtractLargestComponent::svtkBoostExtractLargestComponent()
{
  this->InvertSelection = false;
}

int svtkBoostExtractLargestComponent::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkGraph* output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkSmartPointer<svtkGraph> inputCopy;
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    inputCopy = svtkSmartPointer<svtkDirectedGraph>::New();
  }
  else
  {
    inputCopy = svtkSmartPointer<svtkUndirectedGraph>::New();
  }
  inputCopy->ShallowCopy(input);

  // Find all of the connected components
  svtkSmartPointer<svtkBoostConnectedComponents> connectedComponents =
    svtkSmartPointer<svtkBoostConnectedComponents>::New();
  connectedComponents->SetInputData(inputCopy);
  connectedComponents->Update();

  svtkIntArray* components = svtkArrayDownCast<svtkIntArray>(
    connectedComponents->GetOutput()->GetVertexData()->GetArray("component"));

  // Create an array to store the count of the number of vertices
  // in every component
  int componentRange[2];
  components->GetValueRange(componentRange);
  std::vector<int> componentCount(componentRange[1] + 1);

  for (svtkIdType i = 0; i < components->GetNumberOfTuples(); i++)
  {
    componentCount[components->GetValue(i)]++;
  }

  // Save the original counts
  std::vector<int> originalComponentCount(componentCount.size());
  std::copy(componentCount.begin(), componentCount.end(), originalComponentCount.begin());

  // Sort in descending order
  std::sort(componentCount.rbegin(), componentCount.rend());

  // Find the original component ID of the component with the highest count
  std::vector<int>::iterator it =
    find(originalComponentCount.begin(), originalComponentCount.end(), componentCount[0]);

  if (it == originalComponentCount.end())
  {
    svtkErrorMacro("Should never get to the end of the components!");
    return 0;
  }

  int largestComponent = it - originalComponentCount.begin();

  svtkDebugMacro("The largest component is " << largestComponent << " and it has "
                                            << componentCount[0] << " vertices.");

  // Put either the index of the vertices belonging to the largest connected component
  // or the index of the vertices NOT the largest connected component (depending on the
  // InververtSelection flag) into an array to be used to extract this part of the graph.
  svtkSmartPointer<svtkIdTypeArray> ids = svtkSmartPointer<svtkIdTypeArray>::New();
  for (svtkIdType i = 0; i < components->GetNumberOfTuples(); i++)
  {
    if (!this->InvertSelection)
    {
      if (components->GetValue(i) == largestComponent)
      {
        ids->InsertNextValue(i);
      }
    }
    else
    {
      if (components->GetValue(i) != largestComponent)
      {
        ids->InsertNextValue(i);
      }
    }
  }

  svtkDebugMacro(<< ids->GetNumberOfTuples() << " values selected.");

  // Mark all of the things in the graph that should be extracted
  svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();

  svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
  selection->AddNode(node);
  node->SetSelectionList(ids);
  node->SetContentType(svtkSelectionNode::INDICES);
  node->SetFieldType(svtkSelectionNode::VERTEX);

  // Extract them
  svtkSmartPointer<svtkExtractSelectedGraph> extractSelectedGraph =
    svtkSmartPointer<svtkExtractSelectedGraph>::New();
  extractSelectedGraph->SetInputData(0, inputCopy);
  extractSelectedGraph->SetInputData(1, selection);
  extractSelectedGraph->Update();

  output->ShallowCopy(extractSelectedGraph->GetOutput());

  return 1;
}

void svtkBoostExtractLargestComponent::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InvertSelection: " << this->InvertSelection << "\n";
}
