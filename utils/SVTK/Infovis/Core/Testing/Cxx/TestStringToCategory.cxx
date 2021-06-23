/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStringToCategory.cxx

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
#include "svtkDataSetAttributes.h"
#include "svtkFast2DLayoutStrategy.h"
#include "svtkGraphLayout.h"
#include "svtkGraphMapper.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkStringToCategory.h"
#include "svtkTestUtilities.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestStringToCategory(int argc, char* argv[])
{
  SVTK_CREATE(svtkMutableDirectedGraph, graph);
  SVTK_CREATE(svtkStringArray, vertString);
  vertString->SetName("vertex string");
  for (svtkIdType i = 0; i < 10; ++i)
  {
    graph->AddVertex();
    if (i % 2)
    {
      vertString->InsertNextValue("vertex type 1");
    }
    else
    {
      vertString->InsertNextValue("vertex type 2");
    }
  }
  graph->GetVertexData()->AddArray(vertString);
  SVTK_CREATE(svtkStringArray, edgeString);
  edgeString->SetName("edge string");
  for (svtkIdType i = 0; i < 10; ++i)
  {
    graph->AddEdge(i, (i + 1) % 10);
    graph->AddEdge(i, (i + 3) % 10);
    if (i % 2)
    {
      edgeString->InsertNextValue("edge type 1");
      edgeString->InsertNextValue("edge type 3");
    }
    else
    {
      edgeString->InsertNextValue("edge type 2");
      edgeString->InsertNextValue("edge type 4");
    }
  }
  graph->GetEdgeData()->AddArray(edgeString);

  SVTK_CREATE(svtkStringToCategory, vertexCategory);
  vertexCategory->SetInputData(graph);
  vertexCategory->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, "vertex string");
  vertexCategory->SetCategoryArrayName("vertex category");

  SVTK_CREATE(svtkStringToCategory, edgeCategory);
  edgeCategory->SetInputConnection(vertexCategory->GetOutputPort());
  edgeCategory->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_EDGES, "edge string");
  edgeCategory->SetCategoryArrayName("edge category");

  SVTK_CREATE(svtkCircularLayoutStrategy, strategy);
  SVTK_CREATE(svtkGraphLayout, layout);
  layout->SetInputConnection(edgeCategory->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  SVTK_CREATE(svtkGraphMapper, mapper);
  mapper->SetInputConnection(layout->GetOutputPort());
  mapper->SetEdgeColorArrayName("edge category");
  mapper->ColorEdgesOn();
  mapper->SetVertexColorArrayName("vertex category");
  mapper->ColorVerticesOn();
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);
  SVTK_CREATE(svtkRenderer, ren);
  ren->AddActor(actor);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
