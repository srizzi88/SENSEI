/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPruneTreeFilter.cxx

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
#include "svtkActor2D.h"
#include "svtkDataSetAttributes.h"
#include "svtkGraphLayout.h"
#include "svtkGraphMapper.h"
#include "svtkGraphToPolyData.h"
#include "svtkIdTypeArray.h"
#include "svtkLabeledDataMapper.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkPruneTreeFilter.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTreeLayoutStrategy.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestPruneTreeFilter(int argc, char* argv[])
{
  SVTK_CREATE(svtkMutableDirectedGraph, builder);
  builder->AddVertex(); // 0
  builder->AddChild(0); // 1
  builder->AddChild(0); // 2
  builder->AddChild(1); // 3
  builder->AddChild(1); // 4
  builder->AddChild(2); // 5
  builder->AddChild(2); // 6
  builder->AddChild(3); // 7
  builder->AddChild(3); // 8
  builder->AddChild(4); // 9
  builder->AddChild(4); // 10
  SVTK_CREATE(svtkTree, tree);
  tree->ShallowCopy(builder);

  SVTK_CREATE(svtkIdTypeArray, idArr);
  idArr->SetName("id");
  for (svtkIdType i = 0; i < 11; ++i)
  {
    idArr->InsertNextValue(i);
  }
  tree->GetVertexData()->AddArray(idArr);

  SVTK_CREATE(svtkPruneTreeFilter, prune);
  prune->SetInputData(tree);
  prune->SetParentVertex(2);

  SVTK_CREATE(svtkTreeLayoutStrategy, strategy);
  SVTK_CREATE(svtkGraphLayout, layout);
  // layout->SetInput(tree);
  layout->SetInputConnection(prune->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  SVTK_CREATE(svtkGraphToPolyData, poly);
  poly->SetInputConnection(layout->GetOutputPort());
  SVTK_CREATE(svtkLabeledDataMapper, labelMapper);
  labelMapper->SetInputConnection(poly->GetOutputPort());
  labelMapper->SetLabelModeToLabelFieldData();
  labelMapper->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "id");
  SVTK_CREATE(svtkActor2D, labelActor);
  labelActor->SetMapper(labelMapper);

  SVTK_CREATE(svtkGraphMapper, mapper);
  mapper->SetInputConnection(layout->GetOutputPort());
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);
  SVTK_CREATE(svtkRenderer, ren);
  ren->AddActor(actor);
  ren->AddActor(labelActor);
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
