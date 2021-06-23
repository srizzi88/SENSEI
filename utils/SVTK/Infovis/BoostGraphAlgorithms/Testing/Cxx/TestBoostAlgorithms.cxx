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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkActor.h"
#include "svtkBoostBrandesCentrality.h"
#include "svtkBoostBreadthFirstSearch.h"
#include "svtkBoostBreadthFirstSearchTree.h"
#include "svtkBoostConnectedComponents.h"
#include "svtkBoostPrimMinimumSpanningTree.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkGraphToPolyData.h"
#include "svtkGraphWriter.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include "svtkBoostBiconnectedComponents.h"
#include <boost/version.hpp>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

template <typename Algorithm>
void RenderGraph(svtkRenderer* ren, Algorithm* alg, double xoffset, double yoffset,
  const char* vertColorArray, double vertMin, double vertMax, const char* edgeColorArray,
  double edgeMin, double edgeMax)
{
  SVTK_CREATE(svtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(alg->GetOutputPort());

  SVTK_CREATE(svtkGlyphSource2D, glyph);
  glyph->SetGlyphTypeToVertex();
  SVTK_CREATE(svtkGlyph3D, vertexGlyph);
  vertexGlyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  vertexGlyph->SetInputConnection(1, glyph->GetOutputPort());
  SVTK_CREATE(svtkPolyDataMapper, vertexMapper);
  vertexMapper->SetInputConnection(vertexGlyph->GetOutputPort());
  vertexMapper->SetScalarModeToUsePointFieldData();
  if (vertColorArray)
  {
    vertexMapper->SelectColorArray(vertColorArray);
    vertexMapper->SetScalarRange(vertMin, vertMax);
  }
  SVTK_CREATE(svtkActor, vertexActor);
  vertexActor->SetMapper(vertexMapper);
  vertexActor->GetProperty()->SetPointSize(10.0);
  vertexActor->SetPosition(xoffset, yoffset, 0.001);

  SVTK_CREATE(svtkPolyDataMapper, edgeMapper);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeMapper->SetScalarModeToUseCellFieldData();
  if (edgeColorArray)
  {
    edgeMapper->SelectColorArray(edgeColorArray);
    edgeMapper->SetScalarRange(edgeMin, edgeMax);
  }
  SVTK_CREATE(svtkActor, edgeActor);
  edgeActor->SetMapper(edgeMapper);
  edgeActor->SetPosition(xoffset, yoffset, 0);

  ren->AddActor(vertexActor);
  ren->AddActor(edgeActor);
}

int TestBoostAlgorithms(int argc, char* argv[])
{
  // Create the test graph
  SVTK_CREATE(svtkMutableUndirectedGraph, g);

  SVTK_CREATE(svtkPoints, pts);
  g->AddVertex();
  pts->InsertNextPoint(0, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.5, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.25, 0.5, 0);
  g->AddVertex();
  pts->InsertNextPoint(0, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.5, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(1, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.75, 0.5, 0);
  g->SetPoints(pts);

  g->AddEdge(0, 1);
  g->AddEdge(0, 2);
  g->AddEdge(1, 2);
  g->AddEdge(2, 3);
  g->AddEdge(2, 4);
  g->AddEdge(3, 4);

  // Create a connected test graph
  SVTK_CREATE(svtkMutableUndirectedGraph, g2);

  SVTK_CREATE(svtkPoints, pts2);
  g2->AddVertex();
  pts2->InsertNextPoint(0, 1, 0);
  g2->AddVertex();
  pts2->InsertNextPoint(0.5, 1, 0);
  g2->AddVertex();
  pts2->InsertNextPoint(0.25, 0.5, 0);
  g2->AddVertex();
  pts2->InsertNextPoint(0, 0, 0);
  g2->AddVertex();
  pts2->InsertNextPoint(0.5, 0, 0);
  g2->AddVertex();
  pts2->InsertNextPoint(1, 0, 0);
  g2->AddVertex();
  pts2->InsertNextPoint(0.75, 0.5, 0);
  g2->SetPoints(pts);

  SVTK_CREATE(svtkDoubleArray, weights2);
  weights2->SetName("weight");
  g2->AddEdge(0, 1);
  weights2->InsertNextValue(0.5);
  g2->AddEdge(0, 2);
  weights2->InsertNextValue(0.5);
  g2->AddEdge(1, 2);
  weights2->InsertNextValue(0.1);
  g2->AddEdge(2, 3);
  weights2->InsertNextValue(0.5);
  g2->AddEdge(2, 4);
  weights2->InsertNextValue(0.5);
  g2->AddEdge(3, 4);
  weights2->InsertNextValue(0.1);
  g2->AddEdge(4, 5);
  weights2->InsertNextValue(0.5);
  g2->AddEdge(5, 6);
  weights2->InsertNextValue(0.5);
  g2->GetEdgeData()->AddArray(weights2);

  SVTK_CREATE(svtkRenderer, ren);

  // Test biconnected components
  // Only available in Boost 1.33 or later
  SVTK_CREATE(svtkBoostBiconnectedComponents, biconn);
  biconn->SetInputData(g);
  RenderGraph(
    ren, biconn.GetPointer(), 0, 0, "biconnected component", -1, 3, "biconnected component", -1, 3);

  // Test breadth first search
  SVTK_CREATE(svtkBoostBreadthFirstSearch, bfs);
  bfs->SetInputData(g);
  RenderGraph(ren, bfs.GetPointer(), 2, 0, "BFS", 0, 3, nullptr, 0, 0);

  // Test centrality
  SVTK_CREATE(svtkBoostBrandesCentrality, centrality);
  centrality->SetInputData(g);
  RenderGraph(ren, centrality.GetPointer(), 0, 2, "centrality", 0, 1, nullptr, 0, 0);

  // Test connected components
  SVTK_CREATE(svtkBoostConnectedComponents, comp);
  comp->SetInputData(g);
  RenderGraph(ren, comp.GetPointer(), 2, 2, "component", 0, 2, nullptr, 0, 0);

  // Test breadth first search tree
  SVTK_CREATE(svtkBoostBreadthFirstSearchTree, bfsTree);
  bfsTree->SetInputData(g);
  SVTK_CREATE(svtkBoostBreadthFirstSearch, bfs2);
  bfs2->SetInputConnection(bfsTree->GetOutputPort());
  RenderGraph(ren, bfs2.GetPointer(), 0, 4, "BFS", 0, 3, nullptr, 0, 0);

  // Test Prim's MST
  SVTK_CREATE(svtkBoostPrimMinimumSpanningTree, prim);
  prim->SetInputData(g2);
  prim->SetOriginVertex(0);
  prim->SetEdgeWeightArrayName("weight");
  RenderGraph(ren, prim.GetPointer(), 2, 4, nullptr, 0, 0, nullptr, 0, 0);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  win->Render();

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    win->Render();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
