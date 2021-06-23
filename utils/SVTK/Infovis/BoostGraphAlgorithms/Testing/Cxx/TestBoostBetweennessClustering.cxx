/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoostAlgorithms.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkBoostBetweennessClustering.h"
#include "svtkDataSetAttributes.h"
#include "svtkIntArray.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"
#include "svtkVertexListIterator.h"

#include <boost/version.hpp>

#include <map>

int TestBoostBetweennessClustering(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create the test graph
  svtkSmartPointer<svtkMutableUndirectedGraph> g(svtkSmartPointer<svtkMutableUndirectedGraph>::New());

  svtkSmartPointer<svtkIntArray> weights(svtkSmartPointer<svtkIntArray>::New());
  weights->SetName("weights");

  g->GetEdgeData()->AddArray(weights);

  svtkSmartPointer<svtkPoints> pts(svtkSmartPointer<svtkPoints>::New());
  g->AddVertex();
  pts->InsertNextPoint(1, 1, 0);

  g->AddVertex();
  pts->InsertNextPoint(1, 0, 0);

  g->AddVertex();
  pts->InsertNextPoint(1, -1, 0);

  g->AddVertex();
  pts->InsertNextPoint(2, 0, 0);

  g->AddVertex();
  pts->InsertNextPoint(3, 0, 0);

  g->AddVertex();
  pts->InsertNextPoint(2.5, 1, 0);

  g->AddVertex();
  pts->InsertNextPoint(4, 1, 0);

  g->AddVertex();
  pts->InsertNextPoint(4, 0, 0);

  g->AddVertex();
  pts->InsertNextPoint(4, -1, 0);

  g->SetPoints(pts);

  svtkEdgeType e = g->AddEdge(0, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(1, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(2, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(3, 4);
  weights->InsertTuple1(e.Id, 1);

  e = g->AddEdge(3, 5);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(5, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(6, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(7, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(8, 4);
  weights->InsertTuple1(e.Id, 10);

  // Test centrality
  svtkSmartPointer<svtkBoostBetweennessClustering> bbc(
    svtkSmartPointer<svtkBoostBetweennessClustering>::New());
  bbc->SetInputData(g);
  bbc->SetThreshold(4);
  bbc->SetEdgeWeightArrayName("weights");
  bbc->SetEdgeCentralityArrayName("bbc_centrality");
  bbc->UseEdgeWeightArrayOn();
  bbc->Update();

  svtkGraph* og = bbc->GetOutput();

  if (!og)
  {
    return 1;
  }

  svtkIntArray* compArray =
    svtkArrayDownCast<svtkIntArray>(og->GetVertexData()->GetArray("component"));
  if (!compArray)
  {
    return 1;
  }

  // Now lets create the correct mapping so that we can compare the results
  // against it.
  std::map<int, int> expResults;
  expResults[0] = 0;
  expResults[1] = 0;
  expResults[2] = 0;
  expResults[3] = 0;
  expResults[4] = 1;
  expResults[5] = 1;
  expResults[6] = 1;
  expResults[7] = 1;
  expResults[8] = 2;

  svtkSmartPointer<svtkVertexListIterator> vlItr(svtkSmartPointer<svtkVertexListIterator>::New());
  vlItr->SetGraph(og);

  while (vlItr->HasNext())
  {
    svtkIdType id = vlItr->Next();

    if (expResults[id] != compArray->GetVariantValue(id).ToInt())
    {
      return 1;
    }
  }

  return 0;
}
