/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParallelIso.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates the use of data parallelism in SVTK. The
// pipeline ( svtkMPIImageReader -> svtkContourFilter -> svtkElevationFilter )
// is created in parallel and each process is assigned 1 piece to process.
// All satellite processes send the result to the first process which
// collects and renders them.

#include <svtkMPI.h>

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkCamera.h"
#include "svtkConeSource.h"
#include "svtkContourFilter.h"
#include "svtkDataSet.h"
#include "svtkElevationFilter.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMPIController.h"
#include "svtkMath.h"
#include "svtkPNrrdReader.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTestUtilities.h"
#include "svtkWindowToImageFilter.h"

#include "svtkDebugLeaks.h"

namespace
{

static const float ISO_START = 4250.0;
static const float ISO_STEP = -1250.0;
static const int ISO_NUM = 3;
// Just pick a tag which is available
static const int ISO_VALUE_RMI_TAG = 300;
static const int ISO_OUTPUT_TAG = 301;

struct ParallelIsoArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

struct ParallelIsoRMIArgs_tmp
{
  svtkContourFilter* ContourFilter;
  svtkMultiProcessController* Controller;
  svtkElevationFilter* Elevation;
};

// call back to set the iso surface value.
void SetIsoValueRMI(
  void* localArg, void* svtkNotUsed(remoteArg), int svtkNotUsed(remoteArgLen), int svtkNotUsed(id))
{
  ParallelIsoRMIArgs_tmp* args = (ParallelIsoRMIArgs_tmp*)localArg;

  float val;

  svtkMultiProcessController* contrl = args->Controller;
  int myid = contrl->GetLocalProcessId();
  int numProcs = contrl->GetNumberOfProcesses();

  svtkContourFilter* iso = args->ContourFilter;
  val = iso->GetValue(0);
  iso->SetValue(0, val + ISO_STEP);
  args->Elevation->UpdatePiece(myid, numProcs, 0);

  contrl->Send(args->Elevation->GetOutput(), 0, ISO_OUTPUT_TAG);
}

// This will be called by all processes
void MyMain(svtkMultiProcessController* controller, void* arg)
{
  svtkPNrrdReader* reader;
  svtkContourFilter* iso;
  svtkElevationFilter* elev;
  int myid, numProcs;
  float val;
  ParallelIsoArgs_tmp* args = reinterpret_cast<ParallelIsoArgs_tmp*>(arg);

  // Obtain the id of the running process and the total
  // number of processes
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  // Create the reader, the data file name might have
  // to be changed depending on where the data files are.
  char* fname =
    svtkTestUtilities::ExpandDataFileName(args->argc, args->argv, "Data/headsq/quarter.nhdr");
  reader = svtkPNrrdReader::New();
  reader->SetFileName(fname);
  // reader->SetDataByteOrderToLittleEndian();
  // reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  // reader->SetFilePrefix(fname);
  // reader->SetDataSpacing(3.2, 3.2, 1.5);
  delete[] fname;

  // Iso-surface.
  iso = svtkContourFilter::New();
  iso->SetInputConnection(reader->GetOutputPort());
  iso->SetValue(0, ISO_START);
  iso->ComputeScalarsOff();
  iso->ComputeGradientsOff();

  // Compute a different color for each process.
  elev = svtkElevationFilter::New();
  elev->SetInputConnection(iso->GetOutputPort());
  val = (myid + 1) / static_cast<float>(numProcs);
  elev->SetScalarRange(val, val + 0.001);

  // Make sure all processes update at the same time.
  elev->UpdatePiece(myid, numProcs, 0);

  if (myid != 0)
  {
    // If I am not the root process
    ParallelIsoRMIArgs_tmp args2;
    args2.ContourFilter = iso;
    args2.Controller = controller;
    args2.Elevation = elev;

    // Last, set up a RMI call back to change the iso surface value.
    // This is done so that the root process can let this process
    // know that it wants the contour value to change.
    controller->AddRMI(SetIsoValueRMI, (void*)&args2, ISO_VALUE_RMI_TAG);
    controller->ProcessRMIs();
  }
  else
  {
    // Create the rendering part of the pipeline
    svtkAppendPolyData* app = svtkAppendPolyData::New();
    svtkRenderer* ren = svtkRenderer::New();
    svtkRenderWindow* renWindow = svtkRenderWindow::New();
    svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
    svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
    svtkActor* actor = svtkActor::New();
    svtkCamera* cam = svtkCamera::New();
    renWindow->AddRenderer(ren);
    iren->SetRenderWindow(renWindow);
    ren->SetBackground(0.9, 0.9, 0.9);
    renWindow->SetSize(400, 400);
    mapper->SetInputConnection(app->GetOutputPort());
    actor->SetMapper(mapper);
    ren->AddActor(actor);
    cam->SetFocalPoint(100, 100, 65);
    cam->SetPosition(100, 450, 65);
    cam->SetViewUp(0, 0, -1);
    cam->SetViewAngle(30);
    cam->SetClippingRange(177.0, 536.0);
    ren->SetActiveCamera(cam);

    // loop through some iso surface values.
    for (int j = 0; j < ISO_NUM; ++j)
    {
      for (int i = 1; i < numProcs; ++i)
      {
        // trigger the RMI to change the iso surface value.
        controller->TriggerRMI(i, ISO_VALUE_RMI_TAG);
      }

      // set the local value
      iso->SetValue(0, iso->GetValue(0) + ISO_STEP);
      elev->UpdatePiece(myid, numProcs, 0);

      for (int i = 1; i < numProcs; ++i)
      {
        svtkPolyData* pd = svtkPolyData::New();
        controller->Receive(pd, i, ISO_OUTPUT_TAG);
        if (j == ISO_NUM - 1)
        {
          app->AddInputData(pd);
        }
        pd->Delete();
      }
    }

    // Tell the other processors to stop processing RMIs.
    for (int i = 1; i < numProcs; ++i)
    {
      controller->TriggerRMI(i, svtkMultiProcessController::BREAK_RMI_TAG);
    }

    svtkPolyData* outputCopy = svtkPolyData::New();
    outputCopy->ShallowCopy(elev->GetOutput());
    app->AddInputData(outputCopy);
    outputCopy->Delete();
    app->Update();
    outputCopy->RemoveGhostCells();
    renWindow->Render();

    *(args->retVal) = svtkRegressionTester::Test(args->argc, args->argv, renWindow, 10);

    if (*(args->retVal) == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }

    // Clean up
    app->Delete();
    ren->Delete();
    renWindow->Delete();
    iren->Delete();
    mapper->Delete();
    actor->Delete();
    cam->Delete();
  }

  // clean up objects in all processes.
  reader->Delete();
  iso->Delete();
  elev->Delete();
}

} // end anon namespace

int ParallelIso(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkMPIController* controller = svtkMPIController::New();

  controller->Initialize(&argc, &argv, 1);

  // Added for regression test.
  // ----------------------------------------------
  int retVal = 1;
  ParallelIsoArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  controller->SetSingleMethod(MyMain, &args);
  controller->SingleMethodExecute();

  controller->Finalize();
  controller->Delete();

  return !retVal;
}
