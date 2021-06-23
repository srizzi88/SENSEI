/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedGraph.cxx

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

#include "svtkExtractSelectedGraph.h"

#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkConvertSelection.h"
#include "svtkDataArray.h"
#include "svtkEdgeListIterator.h"
#include "svtkEventForwarderCommand.h"
#include "svtkExtractSelection.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkVertexListIterator.h"

#include <map>

svtkStandardNewMacro(svtkExtractSelectedGraph);
//----------------------------------------------------------------------------
svtkExtractSelectedGraph::svtkExtractSelectedGraph()
{
  this->SetNumberOfInputPorts(3);
  this->RemoveIsolatedVertices = false;
}

//----------------------------------------------------------------------------
svtkExtractSelectedGraph::~svtkExtractSelectedGraph() = default;

//----------------------------------------------------------------------------
int svtkExtractSelectedGraph::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    return 1;
  }
  else if (port == 2)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkAnnotationLayers");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedGraph::SetSelectionConnection(svtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

//----------------------------------------------------------------------------
void svtkExtractSelectedGraph::SetAnnotationLayersConnection(svtkAlgorithmOutput* in)
{
  this->SetInputConnection(2, in);
}

//----------------------------------------------------------------------------
int svtkExtractSelectedGraph::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input)
  {
    svtkInformation* info = outputVector->GetInformationObject(0);
    svtkGraph* output = svtkGraph::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));

    // Output a svtkDirectedGraph if the input is a tree.
    if (!output || (svtkTree::SafeDownCast(input) && !svtkDirectedGraph::SafeDownCast(output)) ||
      (!svtkTree::SafeDownCast(input) && !output->IsA(input->GetClassName())))
    {
      if (svtkTree::SafeDownCast(input))
      {
        output = svtkDirectedGraph::New();
      }
      else
      {
        output = input->NewInstance();
      }
      info->Set(svtkDataObject::DATA_OBJECT(), output);
      output->Delete();
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedGraph::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkGraph* input = svtkGraph::GetData(inputVector[0]);
  svtkSelection* inputSelection = svtkSelection::GetData(inputVector[1]);
  svtkAnnotationLayers* inputAnnotations = svtkAnnotationLayers::GetData(inputVector[2]);
  svtkGraph* output = svtkGraph::GetData(outputVector);

  if (!inputSelection && !inputAnnotations)
  {
    svtkErrorMacro("No svtkSelection or svtkAnnotationLayers provided as input.");
    return 0;
  }

  svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();
  int numSelections = 0;
  if (inputSelection)
  {
    selection->DeepCopy(inputSelection);
    numSelections++;
  }

  // If input annotations are provided, extract their selections only if
  // they are enabled and not hidden.
  if (inputAnnotations)
  {
    for (unsigned int i = 0; i < inputAnnotations->GetNumberOfAnnotations(); ++i)
    {
      svtkAnnotation* a = inputAnnotations->GetAnnotation(i);
      if ((a->GetInformation()->Has(svtkAnnotation::ENABLE()) &&
            a->GetInformation()->Get(svtkAnnotation::ENABLE()) == 0) ||
        (a->GetInformation()->Has(svtkAnnotation::ENABLE()) &&
          a->GetInformation()->Get(svtkAnnotation::ENABLE()) == 1 &&
          a->GetInformation()->Has(svtkAnnotation::HIDE()) &&
          a->GetInformation()->Get(svtkAnnotation::HIDE()) == 1))
      {
        continue;
      }
      selection->Union(a->GetSelection());
      numSelections++;
    }
  }

  // Handle case where there was no input selection and no enabled, non-hidden
  // annotations
  if (numSelections == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Convert the selection to an INDICES selection
  svtkSmartPointer<svtkSelection> converted;
  converted.TakeReference(svtkConvertSelection::ToIndexSelection(selection, input));
  if (!converted)
  {
    svtkErrorMacro("Selection conversion to INDICES failed.");
    return 0;
  }

  // Collect vertex and edge selections.
  svtkSmartPointer<svtkIdTypeArray> edgeList = svtkSmartPointer<svtkIdTypeArray>::New();
  bool hasEdges = false;
  svtkSmartPointer<svtkIdTypeArray> vertexList = svtkSmartPointer<svtkIdTypeArray>::New();
  bool hasVertices = false;
  for (unsigned int i = 0; i < converted->GetNumberOfNodes(); ++i)
  {
    svtkSelectionNode* node = converted->GetNode(i);
    svtkIdTypeArray* list = nullptr;
    if (node->GetFieldType() == svtkSelectionNode::VERTEX)
    {
      list = vertexList;
      hasVertices = true;
    }
    else if (node->GetFieldType() == svtkSelectionNode::EDGE)
    {
      list = edgeList;
      hasEdges = true;
    }

    if (list)
    {
      // Append the selection list to the selection
      svtkIdTypeArray* curList = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
      if (curList)
      {
        int inverse = node->GetProperties()->Get(svtkSelectionNode::INVERSE());
        if (inverse)
        {
          svtkIdType num = (node->GetFieldType() == svtkSelectionNode::VERTEX)
            ? input->GetNumberOfVertices()
            : input->GetNumberOfEdges();
          for (svtkIdType j = 0; j < num; ++j)
          {
            if (curList->LookupValue(j) < 0 && list->LookupValue(j) < 0)
            {
              list->InsertNextValue(j);
            }
          }
        }
        else
        {
          svtkIdType numTuples = curList->GetNumberOfTuples();
          for (svtkIdType j = 0; j < numTuples; ++j)
          {
            svtkIdType curValue = curList->GetValue(j);
            if (list->LookupValue(curValue) < 0)
            {
              list->InsertNextValue(curValue);
            }
          }
        }
      } // end if (curList)
    }   // end if (list)
  }     // end for each child

  // If there is no selection list, return an empty graph
  if (vertexList->GetNumberOfTuples() == 0 && edgeList->GetNumberOfTuples() == 0)
  {
    return 1;
  }

  svtkSmartPointer<svtkMutableDirectedGraph> dirBuilder =
    svtkSmartPointer<svtkMutableDirectedGraph>::New();
  svtkSmartPointer<svtkMutableUndirectedGraph> undirBuilder =
    svtkSmartPointer<svtkMutableUndirectedGraph>::New();
  bool directed;
  svtkGraph* builder = nullptr;
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    directed = true;
    builder = dirBuilder;
  }
  else
  {
    directed = false;
    builder = undirBuilder;
  }

  // There are three cases to handle:
  // 1. Selecting vertices only: Select the vertices along with any edges
  //    connecting two selected vertices.
  // 2. Selecting edges only: Select the edges along with all vertices
  //    adjacent to a selected edge.
  // 3. Selecting vertices and edges: Select the edges along with all vertices
  //    adjacent to a selected edge, plus any additional vertex specified
  //    in the vertex selection.

  svtkDataSetAttributes* vdIn = input->GetVertexData();
  svtkDataSetAttributes* edIn = input->GetEdgeData();
  svtkDataSetAttributes* vdOut = builder->GetVertexData();
  svtkDataSetAttributes* edOut = builder->GetEdgeData();
  svtkPoints* ptsIn = input->GetPoints();
  svtkPoints* ptsOut = builder->GetPoints();
  vdOut->CopyAllocate(vdIn);
  edOut->CopyAllocate(edIn);
  std::map<svtkIdType, svtkIdType> vertexMap;

  // Step 1: Add the vertices.
  // If the user has specified a vertex selection, add them.
  // Else if only an edge selection and RemoveIsolatedVertices is off,
  //   add all vertices to the output.
  // Otherwise, let the edge selection determine the vertices to add.
  if (hasVertices)
  {
    // Add selected vertices
    svtkIdType numSelectedVerts = vertexList->GetNumberOfTuples();
    for (svtkIdType i = 0; i < numSelectedVerts; ++i)
    {
      svtkIdType inVert = vertexList->GetValue(i);
      svtkIdType outVert = 0;
      if (directed)
      {
        outVert = dirBuilder->AddVertex();
      }
      else
      {
        outVert = undirBuilder->AddVertex();
      }
      vdOut->CopyData(vdIn, inVert, outVert);
      ptsOut->InsertNextPoint(ptsIn->GetPoint(inVert));
      vertexMap[inVert] = outVert;
    }
  }
  else if (!this->RemoveIsolatedVertices)
  {
    // In the special case where there is only an edge selection, the user may
    // specify that they want all vertices in the output.
    svtkIdType numVert = input->GetNumberOfVertices();
    for (svtkIdType inVert = 0; inVert < numVert; ++inVert)
    {
      svtkIdType outVert = 0;
      if (directed)
      {
        outVert = dirBuilder->AddVertex();
      }
      else
      {
        outVert = undirBuilder->AddVertex();
      }
      vdOut->CopyData(vdIn, inVert, outVert);
      ptsOut->InsertNextPoint(ptsIn->GetPoint(inVert));
      vertexMap[inVert] = outVert;
    }
  }

  // Step 2: Add the edges
  // If there is an edge selection, add those edges.
  // Otherwise, add all edges connecting selected vertices.
  if (hasEdges)
  {
    // Add selected edges
    svtkIdType numSelectedEdges = edgeList->GetNumberOfTuples();
    for (svtkIdType i = 0; i < numSelectedEdges; ++i)
    {
      svtkEdgeType e;
      e.Id = edgeList->GetValue(i);
      e.Source = input->GetSourceVertex(e.Id);
      e.Target = input->GetTargetVertex(e.Id);

      // Add source and target vertices if they are not yet in output
      svtkIdType addVert[2];
      int numAddVert = 0;
      if (vertexMap.find(e.Source) == vertexMap.end())
      {
        addVert[numAddVert] = e.Source;
        ++numAddVert;
      }
      if (vertexMap.find(e.Target) == vertexMap.end())
      {
        addVert[numAddVert] = e.Target;
        ++numAddVert;
      }
      for (int j = 0; j < numAddVert; ++j)
      {
        svtkIdType outVert = 0;
        if (directed)
        {
          outVert = dirBuilder->AddVertex();
        }
        else
        {
          outVert = undirBuilder->AddVertex();
        }
        vdOut->CopyData(vdIn, addVert[j], outVert);
        ptsOut->InsertNextPoint(ptsIn->GetPoint(addVert[j]));
        vertexMap[addVert[j]] = outVert;
      }

      // Add the selected edge
      svtkIdType source = vertexMap[e.Source];
      svtkIdType target = vertexMap[e.Target];
      svtkEdgeType f;
      if (directed)
      {
        f = dirBuilder->AddEdge(source, target);
      }
      else
      {
        f = undirBuilder->AddEdge(source, target);
      }
      edOut->CopyData(edIn, e.Id, f.Id);
      // Copy edge layout to the output.
      svtkIdType npts;
      double* pts;
      input->GetEdgePoints(e.Id, npts, pts);
      builder->SetEdgePoints(f.Id, npts, pts);
    }
  }
  else
  {
    // Add edges between selected vertices
    svtkSmartPointer<svtkEdgeListIterator> edges = svtkSmartPointer<svtkEdgeListIterator>::New();
    input->GetEdges(edges);
    while (edges->HasNext())
    {
      svtkEdgeType e = edges->Next();
      if (vertexMap.find(e.Source) != vertexMap.end() &&
        vertexMap.find(e.Target) != vertexMap.end())
      {
        svtkIdType source = vertexMap[e.Source];
        svtkIdType target = vertexMap[e.Target];
        svtkEdgeType f;
        if (directed)
        {
          f = dirBuilder->AddEdge(source, target);
        }
        else
        {
          f = undirBuilder->AddEdge(source, target);
        }
        edOut->CopyData(edIn, e.Id, f.Id);
        // Copy edge layout to the output.
        svtkIdType npts;
        double* pts;
        input->GetEdgePoints(e.Id, npts, pts);
        builder->SetEdgePoints(f.Id, npts, pts);
      }
    }
  }

  // Pass constructed graph to output.
  if (directed)
  {
    if (!output->CheckedShallowCopy(dirBuilder))
    {
      svtkErrorMacro(<< "Invalid graph structure.");
      return 0;
    }
  }
  else
  {
    if (!output->CheckedShallowCopy(undirBuilder))
    {
      svtkErrorMacro(<< "Invalid graph structure.");
      return 0;
    }
  }
  output->GetFieldData()->PassData(input->GetFieldData());

  // Clean up
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedGraph::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoveIsolatedVertices: " << (this->RemoveIsolatedVertices ? "on" : "off")
     << endl;
}
