// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PSLACReaderLinear.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeRenderManager.h"
#include "svtkLookupTable.h"
#include "svtkMPIController.h"
#include "svtkPLSDynaReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

#include "svtkNew.h"
#include "svtkSmartPointer.h"

struct TestArgs
{
  int* retval;
  int argc;
  char** argv;
};

//=============================================================================
void PLSDynaReader(svtkMultiProcessController* controller, void* _args)
{
  TestArgs* args = reinterpret_cast<TestArgs*>(_args);
  int argc = args->argc;
  char** argv = args->argv;
  *(args->retval) = 1;

  // Set up reader.
  svtkNew<svtkPLSDynaReader> reader;

  char* meshFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/LSDyna/hemi.draw/hemi_draw.d3plot");
  reader->SetFileName(meshFileName);

  // Extract geometry that we can render.
  svtkNew<svtkCompositeDataGeometryFilter> geometry;
  geometry->SetInputConnection(reader->GetOutputPort());

  // Set up rendering stuff.
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkCompositeRenderManager> prm;

  svtkSmartPointer<svtkRenderer> renderer;
  renderer.TakeReference(prm->MakeRenderer());
  renderer->AddActor(actor);

  svtkSmartPointer<svtkRenderWindow> renwin;
  renwin.TakeReference(prm->MakeRenderWindow());
  renwin->SetSize(300, 300);
  renwin->SetPosition(0, 200 * controller->GetLocalProcessId());
  renwin->AddRenderer(renderer);

  prm->SetRenderWindow(renwin);
  prm->SetController(controller);
  prm->InitializePieces();
  prm->InitializeOffScreen(); // Mesa GL only

  if (controller->GetLocalProcessId() == 0)
  {
    renwin->Render();

    // Do the test comparison.
    int retval = svtkRegressionTestImage(renwin);
    if (retval == svtkRegressionTester::DO_INTERACTOR)
    {
      svtkNew<svtkRenderWindowInteractor> iren;
      iren->SetRenderWindow(renwin);
      iren->Initialize();
      iren->Start();
      retval = svtkRegressionTester::PASSED;
    }

    *(args->retval) = (retval == svtkRegressionTester::PASSED) ? 0 : 1;

    prm->StopServices();
  }
  else // not root node
  {
    prm->StartServices();
  }

  controller->Broadcast(args->retval, 1, 0);
}

//=============================================================================
int PLSDynaReader(int argc, char* argv[])
{
  int retval = 1;

  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  svtkMultiProcessController::SetGlobalController(controller);

  TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(PLSDynaReader, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
