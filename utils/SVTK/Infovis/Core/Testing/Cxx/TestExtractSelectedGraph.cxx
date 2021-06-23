/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelectedGraph.cxx

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
#include "svtkCircularLayoutStrategy.h"
#include "svtkDoubleArray.h"
#include "svtkExtractSelectedGraph.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToPolyData.h"
#include "svtkIdTypeArray.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkTestUtilities.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

void RenderGraph(
  svtkAlgorithm* alg, svtkRenderer* ren, double r, double g, double b, double z, float size)
{
  SVTK_CREATE(svtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(alg->GetOutputPort());
  SVTK_CREATE(svtkPolyDataMapper, edgeMapper);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  SVTK_CREATE(svtkActor, edgeActor);
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(r, g, b);
  edgeActor->GetProperty()->SetLineWidth(size / 2);
  edgeActor->SetPosition(0, 0, z);
  SVTK_CREATE(svtkGlyphSource2D, vertex);
  vertex->SetGlyphTypeToVertex();
  SVTK_CREATE(svtkGlyph3D, glyph);
  glyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  glyph->SetInputConnection(1, vertex->GetOutputPort());
  SVTK_CREATE(svtkPolyDataMapper, vertMapper);
  vertMapper->SetInputConnection(glyph->GetOutputPort());
  SVTK_CREATE(svtkActor, vertActor);
  vertActor->SetMapper(vertMapper);
  vertActor->GetProperty()->SetColor(r, g, b);
  vertActor->GetProperty()->SetPointSize(size);
  vertActor->SetPosition(0, 0, z);
  ren->AddActor(edgeActor);
  ren->AddActor(vertActor);
}

int TestExtractSelectedGraph(int argc, char* argv[])
{
  SVTK_CREATE(svtkRenderer, ren);

  cerr << "Creating test graph..." << endl;
  SVTK_CREATE(svtkMutableUndirectedGraph, graph);
  graph->AddVertex();
  graph->AddVertex();
  graph->AddVertex();
  graph->AddVertex();
  graph->AddVertex();
  graph->AddEdge(0, 1);
  graph->AddEdge(1, 2);
  graph->AddEdge(2, 3);
  graph->AddEdge(3, 4);
  graph->AddEdge(4, 0);
  SVTK_CREATE(svtkDoubleArray, valueArr);
  valueArr->InsertNextValue(-0.5);
  valueArr->InsertNextValue(0.0);
  valueArr->InsertNextValue(0.5);
  valueArr->InsertNextValue(1.0);
  valueArr->InsertNextValue(1.5);
  valueArr->SetName("value");
  graph->GetVertexData()->AddArray(valueArr);
  SVTK_CREATE(svtkGraphLayout, layout);
  layout->SetInputData(graph);
  SVTK_CREATE(svtkCircularLayoutStrategy, circular);
  layout->SetLayoutStrategy(circular);
  RenderGraph(layout, ren, 1, 1, 1, 0.01, 2.0f);
  cerr << "...done." << endl;

  cerr << "Testing threshold selection..." << endl;
  SVTK_CREATE(svtkSelection, threshold);
  SVTK_CREATE(svtkSelectionNode, thresholdNode);
  threshold->AddNode(thresholdNode);
  thresholdNode->SetContentType(svtkSelectionNode::THRESHOLDS);
  thresholdNode->SetFieldType(svtkSelectionNode::VERTEX);
  SVTK_CREATE(svtkDoubleArray, thresholdArr);
  thresholdArr->SetName("value");
  thresholdArr->InsertNextValue(0.0);
  thresholdArr->InsertNextValue(1.0);
  thresholdNode->SetSelectionList(thresholdArr);

  SVTK_CREATE(svtkExtractSelectedGraph, extractThreshold);
  extractThreshold->SetInputConnection(0, layout->GetOutputPort());
  extractThreshold->SetInputData(1, threshold);
  RenderGraph(extractThreshold, ren, 1, 0, 0, -0.01, 5.0f);
  cerr << "...done." << endl;

  cerr << "Testing indices selection..." << endl;
  SVTK_CREATE(svtkSelection, indices);
  SVTK_CREATE(svtkSelectionNode, indicesNode);
  indices->AddNode(indicesNode);
  indicesNode->SetContentType(svtkSelectionNode::INDICES);
  indicesNode->SetFieldType(svtkSelectionNode::VERTEX);
  SVTK_CREATE(svtkIdTypeArray, indicesArr);
  indicesArr->InsertNextValue(0);
  indicesArr->InsertNextValue(2);
  indicesArr->InsertNextValue(4);
  indicesNode->SetSelectionList(indicesArr);

  SVTK_CREATE(svtkExtractSelectedGraph, extractIndices);
  extractIndices->SetInputConnection(0, layout->GetOutputPort());
  extractIndices->SetInputData(1, indices);
  RenderGraph(extractIndices, ren, 0, 1, 0, -0.02, 9.0f);
  cerr << "...done." << endl;

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkRenderWindow, win);
  win->SetMultiSamples(0);
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
