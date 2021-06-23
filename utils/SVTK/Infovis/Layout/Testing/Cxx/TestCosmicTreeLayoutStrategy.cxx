/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCosmicTreeLayoutStrategy.cxx

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
#include "svtkCosmicTreeLayoutStrategy.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGraphLayout.h"
#include "svtkGraphMapper.h"
#include "svtkGraphToPolyData.h"
#include "svtkIdTypeArray.h"
#include "svtkLabeledDataMapper.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTree.h"
#include "svtkTreeLayoutStrategy.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestCosmicTreeLayoutStrategy(int argc, char* argv[])
{
  const double inputRadius[] = { 0.432801, 0.343010, 0.707502, 0.703797, 0.072614, 0.551869,
    0.072092, 0.354239, 0.619700, 0.352652, 0.578812, 0.689687, 0.487843, 0.099574, 0.296240,
    0.757327, 0.103196, 0.657770, 0.623855, 0.485042, 0.379716, 0.887008, 0.400714, 0.553902,
    0.245740, 0.715217, 0.906472, 0.959179, 0.561240, 0.581328 };
  const svtkIdType inputParents[] = { -1, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 5, 5, 5, 6,
    6, 6, 7, 7, 7, 7, 8, 9, 9 };
  int numNodes = sizeof(inputParents) / sizeof(inputParents[0]);

  SVTK_CREATE(svtkMutableDirectedGraph, builder);
  for (int i = 0; i < numNodes; ++i)
  {
    if (inputParents[i] < 0)
    {
      builder->AddVertex();
    }
    else
    {
      builder->AddChild(inputParents[i]);
    }
  }

  SVTK_CREATE(svtkTree, tree);
  tree->ShallowCopy(builder);

  SVTK_CREATE(svtkIdTypeArray, idArr);
  SVTK_CREATE(svtkDoubleArray, radArr);
  idArr->SetName("id");
  radArr->SetName("inputRadius");
  for (svtkIdType i = 0; i < numNodes; ++i)
  {
    idArr->InsertNextValue(i);
    radArr->InsertNextValue(inputRadius[i]);
  }
  tree->GetVertexData()->AddArray(idArr);
  tree->GetVertexData()->AddArray(radArr);

  SVTK_CREATE(svtkCosmicTreeLayoutStrategy, strategy);
  strategy->SizeLeafNodesOnlyOn();
  strategy->SetNodeSizeArrayName("inputRadius");
  SVTK_CREATE(svtkGraphLayout, layout);
  layout->SetInputData(tree);
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
  mapper->SetScalingArrayName("TreeRadius");
  mapper->ScaledGlyphsOn();
  mapper->SetVertexColorArrayName("id");
  mapper->ColorVerticesOn();
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
