/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBooleanOperationPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkEdgeListIterator.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkPassThrough.h"
#include "svtkSmartPointer.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

bool CompareData(svtkGraph* Output, svtkGraph* Input)
{
  bool inputDirected = (svtkDirectedGraph::SafeDownCast(Input) != nullptr);
  bool outputDirected = (svtkDirectedGraph::SafeDownCast(Output) != nullptr);
  if (inputDirected != outputDirected)
  {
    std::cerr << "Directedness not the same" << std::endl;
    return false;
  }

  if (Input->GetNumberOfVertices() != Output->GetNumberOfVertices())
  {
    std::cerr << "GetNumberOfVertices not the same" << std::endl;
    return false;
  }

  if (Input->GetNumberOfEdges() != Output->GetNumberOfEdges())
  {
    std::cerr << "GetNumberOfEdges not the same" << std::endl;
    return false;
  }

  if (Input->GetVertexData()->GetNumberOfArrays() != Output->GetVertexData()->GetNumberOfArrays())
  {
    std::cerr << "GetVertexData()->GetNumberOfArrays() not the same" << std::endl;
    return false;
  }

  if (Input->GetEdgeData()->GetNumberOfArrays() != Output->GetEdgeData()->GetNumberOfArrays())
  {
    std::cerr << "GetEdgeData()->GetNumberOfArrays() not the same" << std::endl;
    return false;
  }

  svtkEdgeListIterator* inputEdges = svtkEdgeListIterator::New();
  svtkEdgeListIterator* outputEdges = svtkEdgeListIterator::New();
  while (inputEdges->HasNext())
  {
    svtkEdgeType inputEdge = inputEdges->Next();
    svtkEdgeType outputEdge = outputEdges->Next();
    if (inputEdge.Source != outputEdge.Source)
    {
      std::cerr << "Input source != output source" << std::endl;
      return false;
    }

    if (inputEdge.Target != outputEdge.Target)
    {
      std::cerr << "Input target != output target" << std::endl;
      return false;
    }
  }
  inputEdges->Delete();
  outputEdges->Delete();

  return true;
}

int TestPassThrough(int, char*[])
{
  std::cerr << "Generating graph ..." << std::endl;
  SVTK_CREATE(svtkMutableDirectedGraph, g);
  SVTK_CREATE(svtkDoubleArray, x);
  x->SetName("x");
  SVTK_CREATE(svtkDoubleArray, y);
  y->SetName("y");
  SVTK_CREATE(svtkDoubleArray, z);
  z->SetName("z");
  for (svtkIdType i = 0; i < 10; ++i)
  {
    for (svtkIdType j = 0; j < 10; ++j)
    {
      g->AddVertex();
      x->InsertNextValue(i);
      y->InsertNextValue(j);
      z->InsertNextValue(1);
    }
  }
  g->GetVertexData()->AddArray(x);
  g->GetVertexData()->AddArray(y);
  g->GetVertexData()->AddArray(z);
  std::cerr << "... done" << std::endl;

  SVTK_CREATE(svtkPassThrough, pass);
  pass->SetInputData(g);
  pass->Update();
  svtkGraph* output = svtkGraph::SafeDownCast(pass->GetOutput());

  if (!CompareData(g, output))
  {
    std::cerr << "ERROR: Graphs not identical!" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
