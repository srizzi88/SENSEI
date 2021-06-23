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
#include "svtkDataSetAttributes.h"
#include "svtkFloatArray.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkGraphLayoutView.h"
#include "svtkGraphToPolyData.h"
#include "svtkGraphWriter.h"
#include "svtkMatrix4x4.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTransform.h"

#include "svtkBoostBiconnectedComponents.h"
#include <boost/version.hpp>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestBoostBrandesCentrality(int argc, char* argv[])
{
  // Create the test graph
  SVTK_CREATE(svtkMutableUndirectedGraph, g);

  SVTK_CREATE(svtkMatrix4x4, mat1);
  mat1->SetElement(1, 3, 5);
  SVTK_CREATE(svtkTransform, transform1);
  transform1->SetMatrix(mat1);

  SVTK_CREATE(svtkMatrix4x4, mat2);
  mat2->SetElement(1, 3, 0);
  SVTK_CREATE(svtkTransform, transform2);
  transform2->SetMatrix(mat2);

  SVTK_CREATE(svtkFloatArray, weights);
  weights->SetName("weights");
  g->GetEdgeData()->AddArray(weights);

  SVTK_CREATE(svtkPoints, pts);
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
  SVTK_CREATE(svtkBoostBrandesCentrality, centrality);
  centrality->SetInputData(g);
  centrality->SetEdgeWeightArrayName("weights");
  centrality->SetInvertEdgeWeightArray(1);
  centrality->UseEdgeWeightArrayOn();

  SVTK_CREATE(svtkGraphLayoutView, view);
  view->SetLayoutStrategyToPassThrough();
  view->SetRepresentationFromInputConnection(centrality->GetOutputPort());
  view->ResetCamera();
  view->SetColorVertices(1);
  view->SetVertexColorArrayName("centrality");
  view->SetColorEdges(1);
  view->SetEdgeColorArrayName("centrality");

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
