/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestKdTreeBoxSelection.cxx

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
#include "svtkAreaPicker.h"
#include "svtkBSPCuts.h"
#include "svtkCubeSource.h"
#include "svtkFloatArray.h"
#include "svtkForceDirectedLayoutStrategy.h"
#include "svtkGlyph3D.h"
#include "svtkGraph.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToPolyData.h"
#include "svtkIdTypeArray.h"
#include "svtkInteractorStyleRubberBandPick.h"
#include "svtkKdNode.h"
#include "svtkKdTree.h"
#include "svtkLookupTable.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRandomGraphSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSimple2DLayoutStrategy.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"
#include "svtkTree.h"
#include "svtkTreeLevelsFilter.h"
#include "svtkTreeMapToPolyData.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

//
// Make a svtkTree from a kd-tree
//
void BuildTree(svtkIdType parent, svtkKdNode* parentVertex, svtkMutableDirectedGraph* tree,
  svtkFloatArray* rectArray)
{
  double bounds[6];
  parentVertex->GetBounds(bounds);
  rectArray->InsertTuple(parent, bounds);
  if (parentVertex->GetLeft() != nullptr)
  {
    svtkIdType curIndex = tree->AddChild(parent);
    BuildTree(curIndex, parentVertex->GetLeft(), tree, rectArray);
    curIndex = tree->AddChild(parent);
    BuildTree(curIndex, parentVertex->GetRight(), tree, rectArray);
  }
}

int TestKdTreeBoxSelection(int argc, char* argv[])
{
  bool interactive = false;
  bool threedim = false;
  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-I"))
    {
      interactive = true;
      continue;
    }
    if (!strcmp(argv[i], "-d"))
    {
      threedim = true;
      continue;
    }

    cerr << argv[0] << " options:\n"
         << "  -I run interactively\n"
         << "  -d three-dimensional\n";
    return 0;
  }

  //
  // Create a random graph and perform layout
  //

  SVTK_CREATE(svtkRandomGraphSource, source);
  source->SetStartWithTree(true);
  source->SetNumberOfVertices(100);
  source->SetNumberOfEdges(15);

  SVTK_CREATE(svtkGraphLayout, layout);
  layout->SetInputConnection(source->GetOutputPort());
  if (threedim)
  {
    SVTK_CREATE(svtkForceDirectedLayoutStrategy, forceLayout);
    forceLayout->SetGraphBounds(-3, 3, -3, 3, -3, 3);
    layout->SetLayoutStrategy(forceLayout);
  }
  else
  {
    SVTK_CREATE(svtkSimple2DLayoutStrategy, simpleLayout);
    simpleLayout->SetJitter(true);
    layout->SetLayoutStrategy(simpleLayout);
  }

  layout->Update();
  svtkGraph* g = svtkGraph::SafeDownCast(layout->GetOutput());

  //
  // Create the kd-tree
  //

  SVTK_CREATE(svtkKdTree, kdTree);
  kdTree->OmitZPartitioning();
  kdTree->SetMinCells(1);
  kdTree->BuildLocatorFromPoints(g->GetPoints());

  //
  // Perform an area selection
  //

  SVTK_CREATE(svtkIdTypeArray, selection);
  double bounds[6] = { -2, 2, -0.5, 3, -1, 1 };
  //{-1, 1, -1, 1, -1, 1};
  kdTree->FindPointsInArea(bounds, selection);

  //
  // Create selected vertex glyphs
  //

  double glyphSize = 0.05;

  SVTK_CREATE(svtkPolyData, selectPoly);
  SVTK_CREATE(svtkPoints, selectPoints);
  double pt[3];
  for (svtkIdType i = 0; i < selection->GetNumberOfTuples(); i++)
  {
    g->GetPoint(selection->GetValue(i), pt);
    selectPoints->InsertNextPoint(pt);
  }
  selectPoly->SetPoints(selectPoints);

  SVTK_CREATE(svtkSphereSource, selectSphere);
  selectSphere->SetRadius(1.1 * glyphSize);

  SVTK_CREATE(svtkGlyph3D, selectGlyph);
  selectGlyph->SetInputData(0, selectPoly);
  selectGlyph->SetInputConnection(1, selectSphere->GetOutputPort());

  SVTK_CREATE(svtkPolyDataMapper, selectMapper);
  selectMapper->SetInputConnection(selectGlyph->GetOutputPort());

  SVTK_CREATE(svtkActor, selectActor);
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  //
  // Create selection box actor
  //

  SVTK_CREATE(svtkCubeSource, cubeSource);
  cubeSource->SetBounds(bounds);

  SVTK_CREATE(svtkPolyDataMapper, cubeMapper);
  cubeMapper->SetInputConnection(cubeSource->GetOutputPort());

  SVTK_CREATE(svtkActor, cubeActor);
  cubeActor->SetMapper(cubeMapper);
  cubeActor->GetProperty()->SetColor(0.0, 0.0, 1.0);
  cubeActor->GetProperty()->SetOpacity(0.5);

  //
  // Create kd-tree actor
  //

  SVTK_CREATE(svtkMutableDirectedGraph, tree);
  SVTK_CREATE(svtkFloatArray, rectArray);
  rectArray->SetName("rectangles");
  rectArray->SetNumberOfComponents(4);
  tree->GetVertexData()->AddArray(rectArray);
  svtkKdNode* top = kdTree->GetCuts()->GetKdNodeTree();
  BuildTree(tree->AddVertex(), top, tree, rectArray);

  SVTK_CREATE(svtkTree, realTree);
  if (!realTree->CheckedShallowCopy(tree))
  {
    cerr << "Invalid tree structure." << endl;
  }

  SVTK_CREATE(svtkTreeLevelsFilter, treeLevels);
  treeLevels->SetInputData(realTree);

  SVTK_CREATE(svtkTreeMapToPolyData, treePoly);
  treePoly->SetInputConnection(treeLevels->GetOutputPort());

  SVTK_CREATE(svtkLookupTable, lut);

  SVTK_CREATE(svtkPolyDataMapper, treeMapper);
  treeMapper->SetInputConnection(treePoly->GetOutputPort());
  treeMapper->SetScalarRange(0, 10);
  treeMapper->SetLookupTable(lut);

  SVTK_CREATE(svtkActor, treeActor);
  treeActor->SetMapper(treeMapper);
  // treeActor->GetProperty()->SetRepresentationToWireframe();
  // treeActor->GetProperty()->SetOpacity(0.2);

  //
  // Create graph actor
  //

  SVTK_CREATE(svtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputData(g);

  SVTK_CREATE(svtkTransform, transform);
  if (threedim)
  {
    transform->Translate(0, 0, 0);
  }
  else
  {
    transform->Translate(0, 0, glyphSize);
  }

  SVTK_CREATE(svtkTransformFilter, transFilter);
  transFilter->SetInputConnection(graphToPoly->GetOutputPort());
  transFilter->SetTransform(transform);

  SVTK_CREATE(svtkPolyDataMapper, graphMapper);
  graphMapper->SetInputConnection(transFilter->GetOutputPort());

  SVTK_CREATE(svtkActor, graphActor);
  graphActor->SetMapper(graphMapper);

  //
  // Create vertex glyphs
  //

  SVTK_CREATE(svtkSphereSource, sphere);
  sphere->SetRadius(glyphSize);

  SVTK_CREATE(svtkGlyph3D, glyph);
  glyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  glyph->SetInputConnection(1, sphere->GetOutputPort());

  SVTK_CREATE(svtkPolyDataMapper, glyphMapper);
  glyphMapper->SetInputConnection(glyph->GetOutputPort());

  SVTK_CREATE(svtkActor, glyphActor);
  glyphActor->SetMapper(glyphMapper);

  //
  // Set up render window
  //

  SVTK_CREATE(svtkRenderer, ren);
  if (!threedim)
  {
    ren->AddActor(treeActor);
  }
  ren->AddActor(graphActor);
  ren->AddActor(glyphActor);
  ren->AddActor(cubeActor);
  ren->AddActor(selectActor);

  SVTK_CREATE(svtkRenderWindow, win);
  win->AddRenderer(ren);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);

  SVTK_CREATE(svtkAreaPicker, picker);
  iren->SetPicker(picker);

  SVTK_CREATE(svtkInteractorStyleRubberBandPick, interact);
  iren->SetInteractorStyle(interact);

  if (interactive)
  {
    iren->Initialize();
    iren->Start();
  }

  return 0;
}
