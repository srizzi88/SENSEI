/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TransmitImageDataRenderPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests svtkTransmitImageData.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCameraPass.h"
#include "svtkCompositeRenderManager.h"
#include "svtkContourFilter.h"
#include "svtkDataObject.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDebugLeaks.h"
#include "svtkDepthPeelingPass.h"
#include "svtkElevationFilter.h"
#include "svtkLightsPass.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkObjectFactory.h"
#include "svtkOpaquePass.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOverlayPass.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataWriter.h"
#include "svtkProcess.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSequencePass.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredPoints.h"
#include "svtkStructuredPointsReader.h"
#include "svtkTestUtilities.h"
#include "svtkTranslucentPass.h"
#include "svtkTransmitImageDataPiece.h"
#include "svtkVolumetricPass.h"

#include <svtk_mpi.h>

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

  svtkSmartPointer<svtkCompositeRenderManager> prm =
    svtkSmartPointer<svtkCompositeRenderManager>::New();

  // READER
  svtkSmartPointer<svtkStructuredPoints> sp;
  if (me == 0)
  {
    svtkSmartPointer<svtkStructuredPointsReader> spr =
      svtkSmartPointer<svtkStructuredPointsReader>::New();
    char* fname = svtkTestUtilities::ExpandDataFileName(this->Argc, this->Argv, "Data/ironProt.svtk");
    spr->SetFileName(fname);
    sp = spr->GetOutput();
    spr->Update();
    delete[] fname;

    go = 1;
    if ((sp == nullptr) || (sp->GetNumberOfCells() == 0))
    {
      if (sp)
      {
        cout << "Failure: input file has no cells" << endl;
      }
      go = 0;
    }
  }
  else
  {
    sp = svtkSmartPointer<svtkStructuredPoints>::New();
  }

  svtkMPICommunicator* comm = svtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
  comm->Broadcast(&go, 1, 0);
  if (!go)
  {
    return;
  }

  // FILTER WE ARE TRYING TO TEST
  svtkSmartPointer<svtkTransmitImageDataPiece> pass =
    svtkSmartPointer<svtkTransmitImageDataPiece>::New();
  pass->SetController(this->Controller);
  pass->SetInputData(sp);

  // FILTERING
  svtkSmartPointer<svtkContourFilter> cf = svtkSmartPointer<svtkContourFilter>::New();
  cf->SetInputConnection(pass->GetOutputPort());
  cf->SetNumberOfContours(1);
  cf->SetValue(0, 10.0);
  // I am not sure that this is needed.
  //(cf->GetInput())->RequestExactExtentOn();
  cf->ComputeNormalsOff();
  svtkSmartPointer<svtkElevationFilter> elev = svtkSmartPointer<svtkElevationFilter>::New();
  elev->SetInputConnection(cf->GetOutputPort());
  elev->SetScalarRange(me, me + .001);

  // COMPOSITE RENDER
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(elev->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  // mapper->SetNumberOfPieces(2);
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  svtkRenderer* renderer = prm->MakeRenderer();
  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);

  // the rendering passes
  svtkSmartPointer<svtkCameraPass> cameraP = svtkSmartPointer<svtkCameraPass>::New();
  svtkSmartPointer<svtkSequencePass> seq = svtkSmartPointer<svtkSequencePass>::New();
  svtkSmartPointer<svtkOpaquePass> opaque = svtkSmartPointer<svtkOpaquePass>::New();
  svtkSmartPointer<svtkDepthPeelingPass> peeling = svtkSmartPointer<svtkDepthPeelingPass>::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);
  svtkSmartPointer<svtkTranslucentPass> translucent = svtkSmartPointer<svtkTranslucentPass>::New();
  peeling->SetTranslucentPass(translucent);
  svtkSmartPointer<svtkVolumetricPass> volume = svtkSmartPointer<svtkVolumetricPass>::New();
  svtkSmartPointer<svtkOverlayPass> overlay = svtkSmartPointer<svtkOverlayPass>::New();
  svtkSmartPointer<svtkLightsPass> lights = svtkSmartPointer<svtkLightsPass>::New();
  svtkSmartPointer<svtkRenderPassCollection> passes = svtkSmartPointer<svtkRenderPassCollection>::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  passes->AddItem(peeling);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);
  glrenderer->SetPass(cameraP);

  renderer->AddActor(actor);
  svtkRenderWindow* renWin = prm->MakeRenderWindow();
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);
  renWin->SetPosition(0, 360 * me);
  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);
  prm->InitializeOffScreen(); // Mesa GL only
  if (me == 0)
  {
  }

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

    prm->StopServices();
    for (i = 1; i < numProcs; i++)
    {
      this->Controller->Send(&this->ReturnValue, 1, i, MY_RETURN_VALUE_MESSAGE);
    }
  }
  else
  {
    prm->StartServices();
    this->Controller->Receive(&this->ReturnValue, 1, 0, MY_RETURN_VALUE_MESSAGE);
  }
  renWin->Delete();
  renderer->Delete();
}

}

int TransmitImageDataRenderPass(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkSmartPointer<svtkMPIController> contr = svtkSmartPointer<svtkMPIController>::New();
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

  return !retVal;
}
