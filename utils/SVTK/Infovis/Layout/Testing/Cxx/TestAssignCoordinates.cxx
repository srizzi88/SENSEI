/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAssignCoordinates.cxx

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

#include "svtkAssignCoordinates.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGraphMapper.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestAssignCoordinates(int argc, char* argv[])
{
  cerr << "Generating graph ..." << endl;
  SVTK_CREATE(svtkMutableDirectedGraph, g);
  SVTK_CREATE(svtkDoubleArray, x);
  x->SetName("x");
  SVTK_CREATE(svtkDoubleArray, y);
  y->SetName("y");
  SVTK_CREATE(svtkDoubleArray, z);
  z->SetName("z");
  for (svtkIdType i = 0; i < 10; ++i)
  {
    for (svtkIdType j = 0; j < 10; ++j)
    {
      g->AddVertex();
      x->InsertNextValue(i);
      y->InsertNextValue(j);
      z->InsertNextValue(1);
    }
  }
  g->GetVertexData()->AddArray(x);
  g->GetVertexData()->AddArray(y);
  g->GetVertexData()->AddArray(z);
  cerr << "... done" << endl;

  cerr << "Sending graph through svtkAssignCoordinates ..." << endl;
  SVTK_CREATE(svtkAssignCoordinates, assign);
  assign->SetInputData(g);
  assign->SetXCoordArrayName("x");
  assign->SetYCoordArrayName("y");
  assign->SetZCoordArrayName("z");
  assign->Update();
  cerr << "... done" << endl;

  SVTK_CREATE(svtkGraphMapper, mapper);
  mapper->SetInputConnection(assign->GetOutputPort());
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
