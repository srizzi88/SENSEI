/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraphAlgorithms.cxx

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
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkGraph.h"
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
#include "svtkVertexDegree.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

void PerformAlgorithm(svtkRenderer* ren, svtkAlgorithm* alg, double xoffset, double yoffset,
  const char* vertColorArray, double vertMin, double vertMax, const char* edgeColorArray = nullptr,
  double edgeMin = 0, double edgeMax = 0)
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

int TestGraphAlgorithms(int argc, char* argv[])
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

  SVTK_CREATE(svtkRenderer, ren);

  // Test vertex degree
  SVTK_CREATE(svtkVertexDegree, degree);
  degree->SetInputData(g);
  PerformAlgorithm(ren, degree, 0, 0, "VertexDegree", 0, 4);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    win->Render();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
