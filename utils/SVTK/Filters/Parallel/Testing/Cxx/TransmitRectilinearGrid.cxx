/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TransmitRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests svtkTransmitRectilinearGrid.

/*
** This test only builds if MPI is in use
*/
#include "svtkMPICommunicator.h"
#include "svtkObjectFactory.h"
#include <svtk_mpi.h>

#include "svtkMPIController.h"
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeRenderManager.h"
#include "svtkContourFilter.h"
#include "svtkDataObject.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkElevationFilter.h"
#include "svtkPolyDataMapper.h"
#include "svtkProcess.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTransmitRectilinearGridPiece.h"

namespace
{

class MyProcess : public svtkProcess
{
public:
  static MyProcess* New();

  virtual void Execute();

  void SetArgs(int anArgc, char* anArgv[]);

protected:
  MyProcess();

  int Argc;
  char** Argv;
};

svtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc = 0;
  this->Argv = nullptr;
}

void MyProcess::SetArgs(int anArgc, char* anArgv[])
{
  this->Argc = anArgc;
  this->Argv = anArgv;
}

void MyProcess::Execute()
{
  this->ReturnValue = 1;
  int numProcs = this->Controller->GetNumberOfProcesses();
  int me = this->Controller->GetLocalProcessId();

  int i, go;

  svtkCompositeRenderManager* prm = svtkCompositeRenderManager::New();

  // READER

  svtkRectilinearGridReader* rgr = nullptr;
  svtkRectilinearGrid* rg = nullptr;

  if (me == 0)
  {
    rgr = svtkRectilinearGridReader::New();

    char* fname =
      svtkTestUtilities::ExpandDataFileName(this->Argc, this->Argv, "Data/RectGrid2.svtk");

    rgr->SetFileName(fname);

    rg = rgr->GetOutput();
    rg->Register(nullptr);

    rgr->Update();

    delete[] fname;

    go = 1;

    if ((rg == nullptr) || (rg->GetNumberOfCells() == 0))
    {
      if (rg)
        cout << "Failure: input file has no cells" << endl;
      go = 0;
    }
  }
  else
  {
    rg = svtkRectilinearGrid::New();
  }

  svtkMPICommunicator* comm = svtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());

  comm->Broadcast(&go, 1, 0);

  if (!go)
  {
    if (rgr)
    {
      rgr->Delete();
    }
    rg->Delete();
    prm->Delete();
    return;
  }

  // FILTER WE ARE TRYING TO TEST
  svtkTransmitRectilinearGridPiece* pass = svtkTransmitRectilinearGridPiece::New();
  pass->SetController(this->Controller);
  pass->SetInputData(rg);

  // FILTERING
  svtkContourFilter* cf = svtkContourFilter::New();
  cf->SetInputConnection(pass->GetOutputPort());
  cf->SetNumberOfContours(1);
  cf->SetValue(0, 0.1);
  // I don't think this is needed
  //(cf->GetInput())->RequestExactExtentOn();
  cf->ComputeNormalsOff();
  svtkElevationFilter* elev = svtkElevationFilter::New();
  elev->SetInputConnection(cf->GetOutputPort());
  elev->SetScalarRange(me, me + .001);

  // COMPOSITE RENDER
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(elev->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  svtkRenderer* renderer = prm->MakeRenderer();
  renderer->AddActor(actor);
  svtkRenderWindow* renWin = prm->MakeRenderWindow();
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);
  renWin->SetPosition(0, 360 * me);
  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);
  prm->InitializeOffScreen(); // Mesa GL only

  // We must update the whole pipeline here, otherwise node 0
  // goes into GetActiveCamera which updates the pipeline, putting
  // it into svtkDistributedDataFilter::Execute() which then hangs.
  // If it executes here, dd will be up-to-date won't have to
  // execute in GetActiveCamera.

  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();

  const int MY_RETURN_VALUE_MESSAGE = 0x11;

  if (me == 0)
  {
    svtkCamera* camera = renderer->GetActiveCamera();
    // camera->UpdateViewport(renderer);
    camera->SetParallelScale(16);

    prm->ResetAllCameras();

    renWin->Render();
    renWin->Render();

    this->ReturnValue = svtkRegressionTester::Test(this->Argc, this->Argv, renWin, 10);

    for (i = 1; i < numProcs; i++)
    {
      this->Controller->Send(&this->ReturnValue, 1, i, MY_RETURN_VALUE_MESSAGE);
    }

    prm->StopServices();
  }
  else
  {
    prm->StartServices();
    this->Controller->Receive(&this->ReturnValue, 1, 0, MY_RETURN_VALUE_MESSAGE);
  }

  // CLEAN UP
  renWin->Delete();
  renderer->Delete();
  actor->Delete();
  mapper->Delete();
  elev->Delete();
  cf->Delete();
  pass->Delete();
  if (me == 0)
  {
    rgr->Delete();
  }
  rg->Delete();
  prm->Delete();
}

}

int TransmitRectilinearGrid(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = 1;

  svtkMultiProcessController::SetGlobalController(contr);

  int numProcs = contr->GetNumberOfProcesses();
  int me = contr->GetLocalProcessId();

  if (numProcs != 2)
  {
    if (me == 0)
    {
      cout << "DistributedData test requires 2 processes" << endl;
    }
    contr->Delete();
    return retVal;
  }

  if (!contr->IsA("svtkMPIController"))
  {
    if (me == 0)
    {
      cout << "DistributedData test requires MPI" << endl;
    }
    contr->Delete();
    return retVal; // is this the right error val?   TODO
  }

  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv);
  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();
  p->Delete();
  contr->Finalize();
  contr->Delete();

  return !retVal;
}
