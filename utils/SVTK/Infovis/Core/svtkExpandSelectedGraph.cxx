/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExpandSelectedGraph.cxx

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

#include "svtkExpandSelectedGraph.h"

#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkConvertSelection.h"
#include "svtkDataArray.h"
#include "svtkEventForwarderCommand.h"
#include "svtkExtractSelection.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInEdgeIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"

#include <set>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkExpandSelectedGraph);

svtkExpandSelectedGraph::svtkExpandSelectedGraph()
{
  this->SetNumberOfInputPorts(2);
  this->BFSDistance = 1;
  this->IncludeShortestPaths = false;
  this->UseDomain = false;
  this->Domain = nullptr;
}

svtkExpandSelectedGraph::~svtkExpandSelectedGraph()
{
  this->SetDomain(nullptr);
}

int svtkExpandSelectedGraph::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    return 1;
  }
  return 0;
}

void svtkExpandSelectedGraph::SetGraphConnection(svtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

int svtkExpandSelectedGraph::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkSelection* input = svtkSelection::GetData(inputVector[0]);
  svtkGraph* graph = svtkGraph::GetData(inputVector[1]);
  svtkSelection* output = svtkSelection::GetData(outputVector);

  SVTK_CREATE(svtkIdTypeArray, indexArray);
  svtkConvertSelection::GetSelectedVertices(input, graph, indexArray);
  this->Expand(indexArray, graph);

  // TODO: Get rid of this HACK.
  // This guarantees we have unique indices.
  // svtkConvertSelection should "flatten out" the input selection
  // to a single index selection which we then expand, instead of
  // expanding each child selection and merging them which creates
  // duplicates.
  std::set<svtkIdType> indexSet;
  for (int i = 0; i < indexArray->GetNumberOfTuples(); ++i)
  {
    indexSet.insert(indexArray->GetValue(i));
  }
  // Delete any entries in the current selection list
  indexArray->Reset();
  // Convert the stl set into the selection list
  std::set<svtkIdType>::iterator I;
  for (I = indexSet.begin(); I != indexSet.end(); ++I)
  {
    indexArray->InsertNextValue(*I);
  }

  // Convert back to a pedigree id selection
  SVTK_CREATE(svtkSelection, indexSelection);
  SVTK_CREATE(svtkSelectionNode, node);
  indexSelection->AddNode(node);
  node->SetSelectionList(indexArray);
  node->SetFieldType(svtkSelectionNode::VERTEX);
  node->SetContentType(svtkSelectionNode::INDICES);
  SVTK_CREATE(svtkSelection, pedigreeIdSelection);
  pedigreeIdSelection.TakeReference(
    svtkConvertSelection::ToPedigreeIdSelection(indexSelection, graph));
  output->DeepCopy(pedigreeIdSelection);

  return 1;
}

void svtkExpandSelectedGraph::Expand(svtkIdTypeArray* indexArray, svtkGraph* graph)
{
  // Now expand the selection to include neighborhoods around
  // the selected vertices
  int distance = this->BFSDistance;
  while (distance > 0)
  {
    this->BFSExpandSelection(indexArray, graph);
    --distance;
  }
}

void svtkExpandSelectedGraph::BFSExpandSelection(svtkIdTypeArray* indexArray, svtkGraph* graph)
{
  // For each vertex in the selection get its adjacent vertices
  SVTK_CREATE(svtkInEdgeIterator, inIt);
  SVTK_CREATE(svtkOutEdgeIterator, outIt);

  svtkAbstractArray* domainArr = graph->GetVertexData()->GetAbstractArray("domain");
  std::set<svtkIdType> indexSet;
  for (int i = 0; i < indexArray->GetNumberOfTuples(); ++i)
  {
    // First insert myself
    indexSet.insert(indexArray->GetValue(i));

    // Now insert all adjacent vertices
    graph->GetInEdges(indexArray->GetValue(i), inIt);
    while (inIt->HasNext())
    {
      svtkInEdgeType e = inIt->Next();
      if (this->UseDomain && this->Domain &&
        domainArr->GetVariantValue(e.Source).ToString() != this->Domain)
      {
        continue;
      }
      indexSet.insert(e.Source);
    }
    graph->GetOutEdges(indexArray->GetValue(i), outIt);
    while (outIt->HasNext())
    {
      svtkOutEdgeType e = outIt->Next();
      if (this->UseDomain && this->Domain && domainArr &&
        domainArr->GetVariantValue(e.Target).ToString() != this->Domain)
      {
        continue;
      }
      indexSet.insert(e.Target);
    }
  }

  // Delete any entries in the current selection list
  indexArray->Reset();

  // Convert the stl set into the selection list
  std::set<svtkIdType>::iterator I;
  for (I = indexSet.begin(); I != indexSet.end(); ++I)
  {
    indexArray->InsertNextValue(*I);
  }
}

void svtkExpandSelectedGraph::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BFSDistance: " << this->BFSDistance << endl;
  os << indent << "IncludeShortestPaths: " << (this->IncludeShortestPaths ? "on" : "off") << endl;
  os << indent << "Domain: " << (this->Domain ? this->Domain : "(null)") << endl;
  os << indent << "UseDomain: " << (this->UseDomain ? "on" : "off") << endl;
}
