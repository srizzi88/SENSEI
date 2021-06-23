// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PSLACReaderQuadratic.cxx

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
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkMPIController.h"
#include "svtkPSLACReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTestUtilities.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

struct TestArgs
{
  int* retval;
  int argc;
  char** argv;
};

//=============================================================================
void PSLACReaderQuadraticMethod(svtkMultiProcessController* controller, void* _args)
{
  TestArgs* args = reinterpret_cast<TestArgs*>(_args);
  int argc = args->argc;
  char** argv = args->argv;
  *(args->retval) = 1;

  // Set up reader.
  SVTK_CREATE(svtkPSLACReader, reader);

  char* meshFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SLAC/ll-9cell-f523/ll-9cell-f523.ncdf");
  char* modeFileName = svtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/SLAC/ll-9cell-f523/mode0.l0.R2.457036E+09I2.778314E+04.m3");
  reader->SetMeshFileName(meshFileName);
  reader->AddModeFileName(modeFileName);

  reader->ReadInternalVolumeOff();
  reader->ReadExternalSurfaceOn();
  reader->ReadMidpointsOn();

  // Extract geometry that we can render.
  SVTK_CREATE(svtkCompositeDataGeometryFilter, geometry);
  geometry->SetInputConnection(reader->GetOutputPort(svtkSLACReader::SURFACE_OUTPUT));

  // Set up rendering stuff.
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->ColorByArrayComponent("bfield", 1);
  mapper->UseLookupTableScalarRangeOff();
  mapper->SetScalarRange(-1e-08, 1e-08);
  // This is handled later by the ParallelRenderManager.
  // mapper->SetPiece(controller->GetLocalProcessId());
  // mapper->SetNumberOfPieces(controller->GetNumberOfProcesses());
  // mapper->Update();

  SVTK_CREATE(svtkLookupTable, lut);
  lut->SetHueRange(0.66667, 0.0);
  mapper->SetLookupTable(lut);

  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);

  SVTK_CREATE(svtkCompositeRenderManager, prm);

  svtkSmartPointer<svtkRenderer> renderer;
  renderer.TakeReference(prm->MakeRenderer());
  renderer->AddActor(actor);
  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.75, 0.0, 0.7);
  camera->SetFocalPoint(0.0, 0.0, 0.7);
  camera->SetViewUp(0.0, 1.0, 0.0);

  svtkSmartPointer<svtkRenderWindow> renwin;
  renwin.TakeReference(prm->MakeRenderWindow());
  renwin->SetSize(600, 150);
  renwin->SetPosition(0, 200 * controller->GetLocalProcessId());
  renwin->AddRenderer(renderer);

  prm->SetRenderWindow(renwin);
  prm->SetController(controller);
  prm->InitializePieces();
  prm->InitializeOffScreen(); // Mesa GL only

  if (controller->GetLocalProcessId() == 0)
  {
    renwin->Render();

    prm->StopServices();
    // Change the time to test the periodic mode interpolation.
    geometry->UpdateInformation();
    geometry->GetOutputInformation(0)->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 3e-10);
    renwin->Render();

    // Do the test comparison.
    int retval = svtkRegressionTestImage(renwin);
    if (retval == svtkRegressionTester::DO_INTERACTOR)
    {
      SVTK_CREATE(svtkRenderWindowInteractor, iren);
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
    // Change the time to test the periodic mode interpolation.
    geometry->UpdateInformation();
    geometry->GetOutputInformation(0)->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 3e-10);
    prm->StartServices();
  }

  controller->Broadcast(args->retval, 1, 0);
}

//=============================================================================
int PSLACReaderQuadratic(int argc, char* argv[])
{
  int retval = 1;

  SVTK_CREATE(svtkMPIController, controller);
  controller->Initialize(&argc, &argv);

  svtkMultiProcessController::SetGlobalController(controller);

  TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(PSLACReaderQuadraticMethod, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
