/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCoincidentGraphLayoutView.cxx

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
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkDataRepresentation.h"
#include "svtkDoubleArray.h"
#include "svtkGraphLayoutView.h"
#include "svtkIdTypeArray.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkPoints.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringArray.h"
#include "svtkStringToNumeric.h"
#include "svtkTestUtilities.h"
#include "svtkXMLTreeReader.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include <sstream>

template <typename T>
std::string ToString(const T& x)
{
  std::ostringstream oss;
  oss << x;
  return oss.str();
}

int TestCoincidentGraphLayoutView(int argc, char* argv[])
{
  SVTK_CREATE(svtkMutableUndirectedGraph, graph);
  SVTK_CREATE(svtkPoints, points);
  SVTK_CREATE(svtkDoubleArray, pointData);
  pointData->SetNumberOfComponents(3);
  points->SetData(static_cast<svtkDataArray*>(pointData));
  graph->SetPoints(points);
  svtkIdType i = 0;

  for (i = 0; i < 10; i++)
  {
    graph->AddVertex();
    points->InsertNextPoint(0.0, 0.0, 0.0);
  }

  graph->AddVertex();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(3.0, 0.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, 2.5, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(0.0, -2.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(2.0, -1.5, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(-1.0, 2.0, 0.0);
  graph->AddVertex();
  points->InsertNextPoint(3.0, 0.0, 0.0);

  for (i = 1; i < 10; i++)
  {
    graph->AddEdge(0, i);
  }

  for (i = 10; i < 17; i++)
  {
    graph->AddEdge(i, i + 1);
  }
  graph->AddEdge(0, 10);

  SVTK_CREATE(svtkStringArray, name);
  name->SetName("name");

  for (i = 0; i < graph->GetNumberOfVertices(); i++)
  {
    name->InsertNextValue("Vert" + ToString(i));
  }
  graph->GetVertexData()->AddArray(name);

  SVTK_CREATE(svtkStringArray, label);
  label->SetName("edge label");
  SVTK_CREATE(svtkIdTypeArray, dist);
  dist->SetName("distance");
  for (i = 0; i < graph->GetNumberOfEdges(); i++)
  {
    dist->InsertNextValue(i);
    switch (i % 4)
    {
      case 0:
        label->InsertNextValue("a");
        break;
      case 1:
        label->InsertNextValue("b");
        break;
      case 2:
        label->InsertNextValue("c");
        break;
      case 3:
        label->InsertNextValue("d");
        break;
    }
  }
  graph->GetEdgeData()->AddArray(dist);
  graph->GetEdgeData()->AddArray(label);

  // Graph layout view
  SVTK_CREATE(svtkGraphLayoutView, view);
  view->DisplayHoverTextOff();
  view->SetLayoutStrategyToPassThrough();
  view->SetVertexLabelArrayName("name");
  view->VertexLabelVisibilityOn();
  view->SetVertexColorArrayName("size");
  view->ColorVerticesOn();
  view->SetEdgeColorArrayName("distance");
  view->ColorEdgesOn();
  view->SetEdgeLabelArrayName("edge label");
  view->EdgeLabelVisibilityOn();
  view->SetRepresentationFromInput(graph);

  view->ResetCamera();
  view->Render();

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
