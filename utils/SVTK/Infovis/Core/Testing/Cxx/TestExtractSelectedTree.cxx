/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelectedTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkExtractSelectedTree.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkStringArray.h"
#include "svtkTree.h"

//----------------------------------------------------------------------------
int TestExtractSelectedTree(int, char*[])
{
  svtkNew<svtkMutableDirectedGraph> graph;
  svtkIdType root = graph->AddVertex();
  svtkIdType internalOne = graph->AddChild(root);
  svtkIdType internalTwo = graph->AddChild(internalOne);
  svtkIdType a = graph->AddChild(internalTwo);
  graph->AddChild(internalTwo);
  graph->AddChild(internalOne);
  svtkIdType b = graph->AddChild(a);
  svtkIdType c = graph->AddChild(a);

  int numNodes = 8;
  svtkNew<svtkDoubleArray> weights;
  weights->SetNumberOfComponents(1);
  weights->SetName("weight");
  weights->SetNumberOfValues(numNodes - 1); // the number of edges = number of nodes -1 for a tree
  weights->FillComponent(0, 0.0);

  // Create the names array
  svtkNew<svtkStringArray> names;
  names->SetNumberOfComponents(1);
  names->SetName("node name");
  names->SetNumberOfValues(numNodes);
  names->SetValue(0, "root");
  names->SetValue(5, "d");
  names->SetValue(3, "a");
  names->SetValue(6, "b");
  names->SetValue(7, "c");

  graph->GetEdgeData()->AddArray(weights);
  graph->GetVertexData()->AddArray(names);

  svtkNew<svtkTree> tree;
  tree->ShallowCopy(graph);

  int SUCCESS = 0;

  // subtest 1
  svtkNew<svtkSelection> sel;
  svtkNew<svtkSelectionNode> selNode;
  svtkNew<svtkIdTypeArray> selArr;
  selArr->InsertNextValue(a);
  selArr->InsertNextValue(b);
  selArr->InsertNextValue(c);
  selNode->SetContentType(svtkSelectionNode::INDICES);
  selNode->SetFieldType(svtkSelectionNode::VERTEX);
  selNode->SetSelectionList(selArr);
  selNode->GetProperties()->Set(svtkSelectionNode::INVERSE(), 1);
  sel->AddNode(selNode);

  svtkNew<svtkExtractSelectedTree> filter1;
  filter1->SetInputData(0, tree);
  filter1->SetInputData(1, sel);
  svtkTree* resultTree1 = filter1->GetOutput();
  filter1->Update();

  if (resultTree1->GetNumberOfVertices() == 5)
  {

    svtkDataSetAttributes* vertexData = resultTree1->GetVertexData();
    svtkDataSetAttributes* edgeData = resultTree1->GetEdgeData();
    if (vertexData->GetNumberOfTuples() != 5)
    {
      std::cerr << "vertex # =" << vertexData->GetNumberOfTuples() << std::endl;
      return EXIT_FAILURE;
    }
    else
    {
      svtkStringArray* nodename =
        svtkArrayDownCast<svtkStringArray>(vertexData->GetAbstractArray("node name"));
      svtkStdString n = nodename->GetValue(4);
      if (n.compare("d") != 0)
      {
        std::cerr << "The node name should be \'d\', but appear to be: " << n.c_str() << std::endl;
        return EXIT_FAILURE;
      }
    }

    if (edgeData->GetNumberOfTuples() != 4)
    {
      std::cerr << "edge # =" << edgeData->GetNumberOfTuples() << std::endl;
      return EXIT_FAILURE;
    }
    SUCCESS++;
  }

  // subtest 2
  svtkNew<svtkExtractSelectedTree> filter2;
  selNode->GetProperties()->Set(svtkSelectionNode::INVERSE(), 0);
  filter2->SetInputData(0, tree);
  filter2->SetInputData(1, sel);
  svtkTree* resultTree2 = filter2->GetOutput();
  filter2->Update();

  if (resultTree2->GetNumberOfVertices() == 3)
  {
    SUCCESS++;
  }
  else
  {
    std::cerr << "sub test 2: edge # =" << resultTree2->GetNumberOfEdges() << std::endl;
    std::cerr << "vertex # =" << resultTree2->GetNumberOfVertices() << std::endl;
    return EXIT_FAILURE;
  }

  // sub test 3
  svtkNew<svtkExtractSelectedTree> filter3;
  svtkNew<svtkSelection> sel3;
  svtkNew<svtkSelectionNode> selEdge;
  svtkNew<svtkIdTypeArray> selArrEdge;
  selArrEdge->InsertNextValue(5);
  selArrEdge->InsertNextValue(6);
  selEdge->SetContentType(svtkSelectionNode::INDICES);
  selEdge->SetFieldType(svtkSelectionNode::EDGE);
  selEdge->SetSelectionList(selArrEdge);
  selEdge->GetProperties()->Set(svtkSelectionNode::INVERSE(), 0);
  sel3->AddNode(selEdge);

  filter3->SetInputData(0, tree);
  filter3->SetInputData(1, sel3);
  svtkTree* resultTree3 = filter3->GetOutput();
  filter3->Update();

  if (resultTree3->GetNumberOfVertices() == 3)
  {
    SUCCESS++;
  }
  else
  {
    std::cerr << "sub test 3: edge # =" << resultTree3->GetNumberOfEdges() << std::endl;
    std::cerr << "vertex # =" << resultTree3->GetNumberOfVertices() << std::endl;
    return EXIT_FAILURE;
  }

  if (SUCCESS == 3)
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
