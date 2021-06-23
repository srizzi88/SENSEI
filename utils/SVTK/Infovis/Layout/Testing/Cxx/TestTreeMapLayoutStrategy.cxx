/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeMapLayoutStrategy.cxx

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
#include "svtkBoxLayoutStrategy.h"
#include "svtkIntArray.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSliceAndDiceLayoutStrategy.h"
#include "svtkSmartPointer.h"
#include "svtkSquarifyLayoutStrategy.h"
#include "svtkTestUtilities.h"
#include "svtkTree.h"
#include "svtkTreeFieldAggregator.h"
#include "svtkTreeMapLayout.h"
#include "svtkTreeMapToPolyData.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

void TestStrategy(svtkTreeMapLayoutStrategy* strategy, svtkTreeAlgorithm* input, double posX,
  double posY, svtkRenderer* ren)
{
  strategy->SetShrinkPercentage(0.1);
  SVTK_CREATE(svtkTreeMapLayout, layout);
  layout->SetLayoutStrategy(strategy);
  layout->SetInputConnection(input->GetOutputPort());
  SVTK_CREATE(svtkTreeMapToPolyData, poly);
  poly->SetInputConnection(layout->GetOutputPort());
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(poly->GetOutputPort());
  mapper->SetScalarRange(0, 100);
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("size");
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);
  actor->SetPosition(posX, posY, 0);
  ren->AddActor(actor);
}

int TestTreeMapLayoutStrategy(int argc, char* argv[])
{
  SVTK_CREATE(svtkRenderer, ren);

  // Create input
  SVTK_CREATE(svtkMutableDirectedGraph, builder);
  SVTK_CREATE(svtkIntArray, sizeArr);
  sizeArr->SetName("size");
  builder->GetVertexData()->AddArray(sizeArr);
  builder->AddVertex();
  sizeArr->InsertNextValue(0);
  builder->AddChild(0);
  sizeArr->InsertNextValue(15);
  builder->AddChild(0);
  sizeArr->InsertNextValue(50);
  builder->AddChild(0);
  sizeArr->InsertNextValue(0);
  builder->AddChild(3);
  sizeArr->InsertNextValue(2);
  builder->AddChild(3);
  sizeArr->InsertNextValue(12);
  builder->AddChild(3);
  sizeArr->InsertNextValue(10);
  builder->AddChild(3);
  sizeArr->InsertNextValue(8);
  builder->AddChild(3);
  sizeArr->InsertNextValue(6);
  builder->AddChild(3);
  sizeArr->InsertNextValue(4);

  SVTK_CREATE(svtkTree, tree);
  if (!tree->CheckedShallowCopy(builder))
  {
    cerr << "Invalid tree structure." << endl;
  }

  SVTK_CREATE(svtkTreeFieldAggregator, agg);
  agg->SetInputData(tree);
  agg->SetField("size");
  agg->SetLeafVertexUnitSize(false);

  // Test box layout
  SVTK_CREATE(svtkBoxLayoutStrategy, box);
  TestStrategy(box, agg, 0, 0, ren);

  // Test slice and dice layout
  SVTK_CREATE(svtkSliceAndDiceLayoutStrategy, sd);
  TestStrategy(sd, agg, 0, 1.1, ren);

  // Test squarify layout
  SVTK_CREATE(svtkSquarifyLayoutStrategy, sq);
  TestStrategy(sq, agg, 1.1, 0, ren);

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
