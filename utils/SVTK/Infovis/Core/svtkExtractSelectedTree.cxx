/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkExtractSelectedTree.h"

#include "svtkCellData.h"
#include "svtkConvertSelection.h"
#include "svtkEdgeListIterator.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTree.h"

#include <map>

svtkStandardNewMacro(svtkExtractSelectedTree);

svtkExtractSelectedTree::svtkExtractSelectedTree()
{
  this->SetNumberOfInputPorts(2);
}

svtkExtractSelectedTree::~svtkExtractSelectedTree() = default;

//----------------------------------------------------------------------------
void svtkExtractSelectedTree::SetSelectionConnection(svtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}
//----------------------------------------------------------------------------
int svtkExtractSelectedTree::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedTree::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkExtractSelectedTree::BuildTree(
  svtkTree* inputTree, svtkIdTypeArray* selectedVerticesList, svtkMutableDirectedGraph* builder)
{
  // Get the input and builder vertex and edge data.
  svtkDataSetAttributes* inputVertexData = inputTree->GetVertexData();
  svtkDataSetAttributes* inputEdgeData = inputTree->GetEdgeData();

  svtkDataSetAttributes* builderVertexData = builder->GetVertexData();
  svtkDataSetAttributes* builderEdgeData = builder->GetEdgeData();
  builderVertexData->CopyAllocate(inputVertexData);
  builderEdgeData->CopyAllocate(inputEdgeData);

  // Add selected vertices and set up a  map between the input tree vertex id
  // and the output tree vertex id
  std::map<svtkIdType, svtkIdType> vertexMap;
  for (svtkIdType j = 0; j < selectedVerticesList->GetNumberOfTuples(); j++)
  {
    svtkIdType inVert = selectedVerticesList->GetValue(j);
    svtkIdType outVert = builder->AddVertex();

    builderVertexData->CopyData(inputVertexData, inVert, outVert);
    vertexMap[inVert] = outVert;
  }

  // Add edges connecting selected vertices
  svtkSmartPointer<svtkEdgeListIterator> edges = svtkSmartPointer<svtkEdgeListIterator>::New();
  inputTree->GetEdges(edges);
  while (edges->HasNext())
  {
    svtkEdgeType e = edges->Next();
    if (vertexMap.find(e.Source) != vertexMap.end() && vertexMap.find(e.Target) != vertexMap.end())
    {
      svtkIdType source = vertexMap[e.Source];
      svtkIdType target = vertexMap[e.Target];
      svtkEdgeType f = builder->AddEdge(source, target);
      builderEdgeData->CopyData(inputEdgeData, e.Id, f.Id);

      svtkIdType npts;
      double* pts;
      inputTree->GetEdgePoints(e.Id, npts, pts);
      builder->SetEdgePoints(f.Id, npts, pts);
    }
  }

  return 1;
}

int svtkExtractSelectedTree::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTree* inputTree = svtkTree::GetData(inputVector[0]);
  svtkSelection* selection = svtkSelection::GetData(inputVector[1]);
  svtkTree* outputTree = svtkTree::GetData(outputVector);

  if (!selection)
  {
    svtkErrorMacro("No svtkSelection provided as input.");
    return 0;
  }

  // obtain a vertex selection list from the input svtkSelection
  // Convert the selection to an INDICES selection
  svtkSmartPointer<svtkSelection> converted;
  converted.TakeReference(svtkConvertSelection::ToIndexSelection(selection, inputTree));
  if (!converted)
  {
    svtkErrorMacro("Selection conversion to INDICES failed.");
    return 0;
  }
  svtkNew<svtkIdTypeArray> selectedVerticesList;

  for (unsigned int i = 0; i < converted->GetNumberOfNodes(); ++i)
  {
    svtkSelectionNode* node = converted->GetNode(i);

    // Append the selectedVerticesList
    svtkIdTypeArray* curList = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
    if (curList)
    {
      int inverse = node->GetProperties()->Get(svtkSelectionNode::INVERSE());
      if (inverse)
      { // selection is to be removed
        if (node->GetFieldType() == svtkSelectionNode::VERTEX)
        { // keep all the other vertices
          svtkIdType num = inputTree->GetNumberOfVertices();
          for (svtkIdType j = 0; j < num; ++j)
          {
            if (curList->LookupValue(j) < 0 && selectedVerticesList->LookupValue(j) < 0)
            {
              selectedVerticesList->InsertNextValue(j);
            }
          }
        }
        else if (node->GetFieldType() == svtkSelectionNode ::EDGE)
        { // keep all the other edges
          svtkIdType num = inputTree->GetNumberOfEdges();
          for (svtkIdType j = 0; j < num; ++j)
          {
            if (curList->LookupValue(j) < 0)
            {
              svtkIdType s = inputTree->GetSourceVertex(j);
              svtkIdType t = inputTree->GetTargetVertex(j);
              if (selectedVerticesList->LookupValue(s) < 0)
              {
                selectedVerticesList->InsertNextValue(s);
              }
              if (selectedVerticesList->LookupValue(t) < 0)
              {
                selectedVerticesList->InsertNextValue(t);
              }
            }
          }
        }
      } // end of if(!inverse)
      else
      { // selection is to be extracted
        svtkIdType numTuples = curList->GetNumberOfTuples();
        for (svtkIdType j = 0; j < numTuples; ++j)
        {
          if (node->GetFieldType() == svtkSelectionNode::VERTEX)
          {
            svtkIdType curVertexId = curList->GetValue(j);
            if (selectedVerticesList->LookupValue(curVertexId) < 0)
            {
              selectedVerticesList->InsertNextValue(curVertexId);
            }
          }
          else if (node->GetFieldType() == svtkSelectionNode::EDGE)
          { // if an edge is selected to be extracted,
            // keep both source and target vertices
            svtkIdType curEdgeId = curList->GetValue(j);
            svtkIdType t = inputTree->GetTargetVertex(curEdgeId);
            svtkIdType s = inputTree->GetSourceVertex(curEdgeId);
            if (selectedVerticesList->LookupValue(s) < 0)
            {
              selectedVerticesList->InsertNextValue(s);
            }
            if (selectedVerticesList->LookupValue(t) < 0)
            {
              selectedVerticesList->InsertNextValue(t);
            }
          }
        }
      }
    } // end if (curList)
  }   // end for each selection node

  svtkNew<svtkMutableDirectedGraph> builder;
  // build the tree recursively
  this->BuildTree(inputTree, selectedVerticesList, builder);

  // Copy the structure into the output.
  if (!outputTree->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid tree structure." << outputTree->GetNumberOfVertices());
    return 0;
  }

  return 1;
}
