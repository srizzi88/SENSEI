/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSimple3DCirclesStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkEdgeLayout.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToPolyData.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkPassThroughEdgeStrategy.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRandomGraphSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSimple3DCirclesStrategy.h"
#include "svtkSmartPointer.h"
#include "svtkVertexGlyphFilter.h"

int TestSimple3DCirclesStrategy(int argc, char* argv[])
{
  // graph
  svtkSmartPointer<svtkMutableDirectedGraph> graph = svtkSmartPointer<svtkMutableDirectedGraph>::New();
  // mapper
  svtkSmartPointer<svtkPolyDataMapper> edgeMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  svtkSmartPointer<svtkPolyDataMapper> vertMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  svtkSmartPointer<svtkPassThroughEdgeStrategy> edgeStrategy =
    svtkSmartPointer<svtkPassThroughEdgeStrategy>::New();
  svtkSmartPointer<svtkSimple3DCirclesStrategy> strategy =
    svtkSmartPointer<svtkSimple3DCirclesStrategy>::New();
  svtkSmartPointer<svtkGraphLayout> layout = svtkSmartPointer<svtkGraphLayout>::New();
  svtkSmartPointer<svtkEdgeLayout> edgeLayout = svtkSmartPointer<svtkEdgeLayout>::New();
  svtkSmartPointer<svtkGraphToPolyData> graphToPoly = svtkSmartPointer<svtkGraphToPolyData>::New();
  svtkSmartPointer<svtkVertexGlyphFilter> vertGlyph = svtkSmartPointer<svtkVertexGlyphFilter>::New();
  svtkSmartPointer<svtkActor> edgeActor = svtkSmartPointer<svtkActor>::New();
  svtkSmartPointer<svtkActor> vertActor = svtkSmartPointer<svtkActor>::New();
  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renwin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renwin);
  renwin->SetMultiSamples(0);

  // Vertices:
  // layer 0: 0,1,2
  // layer 1: 3,4,5,6
  // layer 2: 7,8,9
  // standalone: 10,11
  for (int i = 0; i < 12; ++i)
    graph->AddVertex();

  // Edges:
  // layer 0 -> 1
  graph->AddEdge(0, 4);
  graph->AddEdge(0, 6);
  graph->AddEdge(1, 5);
  graph->AddEdge(1, 6);
  graph->AddEdge(2, 3);
  graph->AddEdge(2, 4);
  graph->AddEdge(2, 5);
  // layer 1 -> 2
  graph->AddEdge(3, 8);
  graph->AddEdge(3, 7);
  graph->AddEdge(4, 9);
  graph->AddEdge(4, 8);
  graph->AddEdge(5, 7);
  // layer 0 -> 2
  graph->AddEdge(0, 9);

  strategy->SetMethod(svtkSimple3DCirclesStrategy::FixedDistanceMethod);
  strategy->AutoHeightOn();
  strategy->SetDirection(0.0, -1.0, 0.0);
  strategy->SetMinimumDegree(45.0);
  layout->SetInputData(graph);
  layout->SetLayoutStrategy(strategy);

  // Uncomment the following for a more interesting result!
#if 0
  svtkSmartPointer<svtkRandomGraphSource> src = svtkSmartPointer<svtkRandomGraphSource>::New();
  src->SetNumberOfVertices(1000);
  src->SetNumberOfEdges(0);
  src->SetDirected(true);
  src->SetStartWithTree(true);
  layout->SetInputConnection( src->GetOutputPort() );
#endif

  edgeLayout->SetInputConnection(layout->GetOutputPort());
  edgeLayout->SetLayoutStrategy(edgeStrategy);
  edgeLayout->Update();

  graphToPoly->EdgeGlyphOutputOn();
  graphToPoly->SetInputConnection(edgeLayout->GetOutputPort());
  vertGlyph->SetInputConnection(edgeLayout->GetOutputPort());

  edgeMapper->ScalarVisibilityOff();
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeActor->GetProperty()->SetColor(0.75, 0.75, 0.75);
  edgeActor->GetProperty()->SetOpacity(1.0);
  edgeActor->GetProperty()->SetLineWidth(2);
  edgeActor->PickableOff();
  edgeActor->SetMapper(edgeMapper);
  ren->AddActor(edgeActor);

  vertMapper->ScalarVisibilityOff();
  vertMapper->SetInputConnection(vertGlyph->GetOutputPort());
  vertActor->GetProperty()->SetColor(0.5, 0.5, 0.5);
  vertActor->GetProperty()->SetOpacity(1.0);
  vertActor->GetProperty()->SetPointSize(7);
  vertActor->PickableOff();
  vertActor->SetMapper(vertMapper);
  ren->AddActor(vertActor);

  renwin->SetSize(800, 600);

  renwin->AddRenderer(ren);
  renwin->Render();

  int retVal = svtkRegressionTestImage(renwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
