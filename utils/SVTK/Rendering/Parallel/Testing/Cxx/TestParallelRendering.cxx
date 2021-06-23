/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParallelRendering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtk_mpi.h>

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositedSynchronizedRenderers.h"
#include "svtkLookupTable.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPieceScalars.h"
#include "svtkPolyDataMapper.h"
#include "svtkProcess.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkSynchronizedRenderWindows.h"
#include "svtkTestUtilities.h"

namespace
{

class MyProcess : public svtkProcess
{
public:
  static MyProcess* New();

  virtual void Execute();

  void SetArgs(int anArgc, char* anArgv[])
  {
    this->Argc = anArgc;
    this->Argv = anArgv;
  }

  void CreatePipeline(svtkRenderer* renderer)
  {
    int num_procs = this->Controller->GetNumberOfProcesses();
    int my_id = this->Controller->GetLocalProcessId();

    svtkSphereSource* sphere = svtkSphereSource::New();
    sphere->SetPhiResolution(100);
    sphere->SetThetaResolution(100);

    svtkPieceScalars* piecescalars = svtkPieceScalars::New();
    piecescalars->SetInputConnection(sphere->GetOutputPort());
    piecescalars->SetScalarModeToCellData();

    svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
    mapper->SetInputConnection(piecescalars->GetOutputPort());
    mapper->SetScalarModeToUseCellFieldData();
    mapper->SelectColorArray("Piece");
    mapper->SetScalarRange(0, num_procs - 1);
    mapper->SetPiece(my_id);
    mapper->SetNumberOfPieces(num_procs);
    mapper->Update();

    svtkActor* actor = svtkActor::New();
    actor->SetMapper(mapper);
    renderer->AddActor(actor);

    actor->Delete();
    mapper->Delete();
    piecescalars->Delete();
    sphere->Delete();
  }

protected:
  MyProcess()
  {
    this->Argc = 0;
    this->Argv = nullptr;
  }

  int Argc;
  char** Argv;
};

svtkStandardNewMacro(MyProcess);

void MyProcess::Execute()
{
  this->ReturnValue = 0;
  int my_id = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->DoubleBufferOn();
  svtkRenderer* renderer = svtkRenderer::New();

  renWin->AddRenderer(renderer);

  svtkSynchronizedRenderWindows* syncWindows = svtkSynchronizedRenderWindows::New();
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(this->Controller);
  syncWindows->SetIdentifier(1);

  svtkCompositedSynchronizedRenderers* syncRenderers = svtkCompositedSynchronizedRenderers::New();
  syncRenderers->SetRenderer(renderer);
  syncRenderers->SetParallelController(this->Controller);
  // syncRenderers->SetImageReductionFactor(3);

  this->CreatePipeline(renderer);

  int retVal;

  if (my_id == 0)
  {
    svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
    iren->Initialize();

    retVal = svtkRegressionTester::Test(this->Argc, this->Argv, renWin, 10);

    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    iren->Delete();

    this->Controller->TriggerBreakRMIs();
    // This should really be Broadcast
    for (int i = 1; i < numProcs; i++)
    {
      this->Controller->Send(&retVal, 1, i, 33);
    }
  }
  else
  {
    this->Controller->ProcessRMIs();
    this->Controller->Receive(&retVal, 1, 0, 33);
  }

  renderer->Delete();
  renWin->Delete();
  syncWindows->Delete();
  syncRenderers->Delete();
  this->ReturnValue = retVal;
}

}

int TestParallelRendering(int argc, char* argv[])
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

  int retVal = 1; // 1==failed

  int numProcs = contr->GetNumberOfProcesses();

  if (numProcs < 2 && false)
  {
    cout << "This test requires at least 2 processes" << endl;
    contr->Delete();
    return retVal;
  }

  svtkMultiProcessController::SetGlobalController(contr);

  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv);

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();

  p->Delete();
  contr->Finalize();
  contr->Delete();
  svtkMultiProcessController::SetGlobalController(nullptr);
  return !retVal;
}
