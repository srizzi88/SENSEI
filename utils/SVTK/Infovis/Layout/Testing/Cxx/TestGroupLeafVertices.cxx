/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGroupLeafVertices.cxx

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
#include "svtkGlyph3D.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToPolyData.h"
#include "svtkGroupLeafVertices.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTableToTreeFilter.h"
#include "svtkTree.h"
#include "svtkTreeLayoutStrategy.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestGroupLeafVertices(int argc, char* argv[])
{
  int imode = 0; // Interactive mode
  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-I"))
    {
      imode = 1;
      continue;
    }

    cerr << argv[0] << " Options:\n  "
         << " -h (prints this message)\n  "
         << " -I (run interactively)\n  ";
    return 0;
  }

  SVTK_CREATE(svtkTable, table);
  SVTK_CREATE(svtkStringArray, nameArray);
  nameArray->SetName("name");
  SVTK_CREATE(svtkStringArray, typeArray);
  typeArray->SetName("type");
  SVTK_CREATE(svtkStringArray, colorArray);
  colorArray->SetName("color");

  nameArray->InsertNextValue("bobo");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("black");

  nameArray->InsertNextValue("rex");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("brown");

  nameArray->InsertNextValue("bill");
  typeArray->InsertNextValue("cat");
  colorArray->InsertNextValue("black");

  nameArray->InsertNextValue("eli");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("black");

  nameArray->InsertNextValue("spot");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("white");

  nameArray->InsertNextValue("roger");
  typeArray->InsertNextValue("cat");
  colorArray->InsertNextValue("black");

  nameArray->InsertNextValue("tweety");
  typeArray->InsertNextValue("bird");
  colorArray->InsertNextValue("white");

  nameArray->InsertNextValue("mike");
  typeArray->InsertNextValue("bird");
  colorArray->InsertNextValue("brown");

  nameArray->InsertNextValue("spike");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("brown");

  table->AddColumn(nameArray);
  table->AddColumn(typeArray);
  table->AddColumn(colorArray);

  //
  // Create a tree from the table
  //

  SVTK_CREATE(svtkTableToTreeFilter, tableToTree);
  tableToTree->SetInputData(table);
  tableToTree->Update();
  svtkTree* tree = tableToTree->GetOutput();
  for (svtkIdType i = 0; i < tree->GetNumberOfVertices(); i++)
  {
    cerr << i << " has parent " << tree->GetParent(i) << endl;
  }

  SVTK_CREATE(svtkGroupLeafVertices, group);
  group->SetInputConnection(tableToTree->GetOutputPort());
  group->SetInputArrayToProcess(0, 0, 0, svtkDataSet::FIELD_ASSOCIATION_VERTICES, "type");
  group->SetInputArrayToProcess(1, 0, 0, svtkDataSet::FIELD_ASSOCIATION_VERTICES, "name");
  group->Update();
  tree = group->GetOutput();
  for (svtkIdType i = 0; i < tree->GetNumberOfVertices(); i++)
  {
    cerr << i << " has parent " << tree->GetParent(i) << endl;
  }

  SVTK_CREATE(svtkGroupLeafVertices, group2);
  group2->SetInputConnection(group->GetOutputPort());
  group2->SetInputArrayToProcess(0, 0, 0, svtkDataSet::FIELD_ASSOCIATION_VERTICES, "color");
  group2->SetInputArrayToProcess(1, 0, 0, svtkDataSet::FIELD_ASSOCIATION_VERTICES, "name");
  group2->Update();
  tree = group2->GetOutput();
  for (svtkIdType i = 0; i < tree->GetNumberOfVertices(); i++)
  {
    cerr << i << " has parent " << tree->GetParent(i) << endl;
  }

  //
  // Render the tree
  //

  SVTK_CREATE(svtkTreeLayoutStrategy, strategy);
  strategy->SetRadial(true);
  strategy->SetAngle(360);

  SVTK_CREATE(svtkGraphLayout, layout);
  layout->SetInputConnection(group2->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  SVTK_CREATE(svtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(layout->GetOutputPort());

  SVTK_CREATE(svtkPolyDataMapper, polyMapper);
  polyMapper->SetInputConnection(graphToPoly->GetOutputPort());

  SVTK_CREATE(svtkActor, polyActor);
  polyActor->SetMapper(polyMapper);
  polyActor->GetProperty()->SetColor(0.3, 0.3, 1.0);

  //
  // Make some glyphs
  //

  SVTK_CREATE(svtkSphereSource, sphere);
  sphere->SetRadius(0.05);
  sphere->SetPhiResolution(6);
  sphere->SetThetaResolution(6);

  SVTK_CREATE(svtkGlyph3D, glyph);
  glyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  glyph->SetInputConnection(1, sphere->GetOutputPort());

  SVTK_CREATE(svtkPolyDataMapper, glyphMap);
  glyphMap->SetInputConnection(glyph->GetOutputPort());

  SVTK_CREATE(svtkActor, glyphActor);
  glyphActor->SetMapper(glyphMap);
  glyphActor->GetProperty()->SetColor(0.3, 0.3, 1.0);

  //
  // Set up the main window
  //

  SVTK_CREATE(svtkRenderer, ren);
  ren->AddActor(polyActor);
  // ren->AddActor(labelActor);
  ren->AddActor(glyphActor);

  SVTK_CREATE(svtkRenderWindow, win);
  win->AddRenderer(ren);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);

  if (imode)
  {
    iren->Initialize();
    iren->Start();
  }

  return 0;
}
