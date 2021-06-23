/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DistributedDataRenderPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test of svtkDistributedDataFilter and supporting classes, covering as much
// code as possible.  This test requires 4 MPI processes.
//
// To cover ghost cell creation, use svtkDataSetSurfaceFilter.
//
// To cover clipping code:  SetBoundaryModeToSplitBoundaryCells()
//
// To run fast redistribution: SetUseMinimalMemoryOff() (Default)
// To run memory conserving code instead: SetUseMinimalMemoryOn()

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCameraPass.h"
#include "svtkClearZPass.h"
#include "svtkCompositeRGBAPass.h"
#include "svtkCompositeRenderManager.h"
#include "svtkDataSetReader.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDistributedDataFilter.h"
#include "svtkImageRenderManager.h"
#include "svtkLightsPass.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpaquePass.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOverlayPass.h"
#include "svtkPieceScalars.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSequencePass.h"
#include "svtkTestErrorObserver.h"
#include "svtkTestUtilities.h"
#include "svtkTranslucentPass.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVolumetricPass.h"

/*
** This test only builds if MPI is in use
*/
#include "svtkMPICommunicator.h"

#include "svtkProcess.h"

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

  //  svtkCompositeRenderManager *prm = svtkCompositeRenderManager::New();
  //  svtkParallelRenderManager *prm = svtkParallelRenderManager::New();
  svtkImageRenderManager* prm = svtkImageRenderManager::New();

  svtkRenderWindowInteractor* iren = nullptr;

  if (me == 0)
  {
    iren = svtkRenderWindowInteractor::New();
  }

  // READER

  svtkDataSetReader* dsr = svtkDataSetReader::New();
  svtkUnstructuredGrid* ug = svtkUnstructuredGrid::New();

  svtkDataSet* ds = nullptr;

  if (me == 0)
  {
    char* fname =
      svtkTestUtilities::ExpandDataFileName(this->Argc, this->Argv, "Data/tetraMesh.svtk");

    dsr->SetFileName(fname);

    ds = dsr->GetOutput();

    dsr->Update();

    delete[] fname;

    go = 1;

    if ((ds == nullptr) || (ds->GetNumberOfCells() == 0))
    {
      if (ds)
      {
        cout << "Failure: input file has no cells" << endl;
      }
      go = 0;
    }
  }
  else
  {
    ds = static_cast<svtkDataSet*>(ug);
  }

  svtkMPICommunicator* comm = svtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());

  comm->Broadcast(&go, 1, 0);

  if (!go)
  {
    dsr->Delete();
    ug->Delete();
    prm->Delete();
    return;
  }

  // DATA DISTRIBUTION FILTER

  svtkDistributedDataFilter* dd = svtkDistributedDataFilter::New();

  dd->SetInputData(ds);
  dd->SetController(this->Controller);

  dd->SetBoundaryModeToSplitBoundaryCells(); // clipping
  dd->UseMinimalMemoryOff();

  // COLOR BY PROCESS NUMBER

  svtkPieceScalars* ps = svtkPieceScalars::New();
  ps->SetInputConnection(dd->GetOutputPort());
  ps->SetScalarModeToCellData();

  // MORE FILTERING - this will request ghost cells

  svtkDataSetSurfaceFilter* dss = svtkDataSetSurfaceFilter::New();
  dss->SetInputConnection(ps->GetOutputPort());

  // COMPOSITE RENDER

  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(dss->GetOutputPort());

  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, numProcs - 1);

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  svtkProperty* p = actor->GetProperty();
  p->SetOpacity(0.3);

  svtkRenderer* renderer = prm->MakeRenderer();
  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);

#if 1
  // the rendering passes
  svtkCameraPass* cameraP = svtkCameraPass::New();

  svtkSequencePass* seq = svtkSequencePass::New();
  svtkOpaquePass* opaque = svtkOpaquePass::New();
  //  svtkDepthPeelingPass *peeling=svtkDepthPeelingPass::New();
  //  peeling->SetMaximumNumberOfPeels(200);
  //  peeling->SetOcclusionRatio(0.1);

  svtkTranslucentPass* translucent = svtkTranslucentPass::New();
  //  peeling->SetTranslucentPass(translucent);

  svtkVolumetricPass* volume = svtkVolumetricPass::New();
  svtkOverlayPass* overlay = svtkOverlayPass::New();

  svtkLightsPass* lights = svtkLightsPass::New();

  svtkClearZPass* clearZ = svtkClearZPass::New();
  clearZ->SetDepth(0.9);

  svtkNew<svtkTest::ErrorObserver> errorObserver1;
  svtkCompositeRGBAPass* compositeRGBAPass = svtkCompositeRGBAPass::New();
  compositeRGBAPass->AddObserver(svtkCommand::ErrorEvent, errorObserver1);
  compositeRGBAPass->SetController(this->Controller);
  compositeRGBAPass->SetKdtree(dd->GetKdtree());
  svtkRenderPassCollection* passes = svtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  //  passes->AddItem(clearZ);
  //  passes->AddItem(peeling);
  passes->AddItem(translucent);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  passes->AddItem(compositeRGBAPass);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);
  glrenderer->SetPass(cameraP);

  opaque->Delete();
  //  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  compositeRGBAPass->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  clearZ->Delete();
#endif

  renderer->AddActor(actor);

  svtkRenderWindow* renWin = prm->MakeRenderWindow();
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);

  if (me == 0)
  {
    iren->SetRenderWindow(renWin);
  }

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
    renderer->ResetCamera();
    svtkCamera* camera = renderer->GetActiveCamera();
    // camera->UpdateViewport(renderer);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(16);

    if (compositeRGBAPass->IsSupported(static_cast<svtkOpenGLRenderWindow*>(renWin)))
    {
      renWin->Render();
      this->ReturnValue = svtkRegressionTester::Test(this->Argc, this->Argv, renWin, 10);
    }
    else
    {
      std::string gotMsg(errorObserver1->GetErrorMessage());
      std::string expectedMsg("Missing required OpenGL extensions");
      if (gotMsg.find(expectedMsg) == std::string::npos)
      {
        std::cout << "ERROR: Error message does not contain \"" << expectedMsg << "\" got \n\""
                  << gotMsg << std::endl;
        this->ReturnValue = svtkTesting::FAILED;
      }
      else
      {
        std::cout << expectedMsg << std::endl;
      }
      this->ReturnValue = svtkTesting::PASSED; // not supported.
    }

    if (this->ReturnValue == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
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

  if (this->ReturnValue == svtkTesting::PASSED)
  {
    // Now try using the memory conserving *Lean methods.  The
    // image produced should be identical

    dd->UseMinimalMemoryOn();
    mapper->SetPiece(me);
    mapper->SetNumberOfPieces(numProcs);
    mapper->Update();

    if (me == 0)
    {
      renderer->ResetCamera();
      svtkCamera* camera = renderer->GetActiveCamera();
      camera->UpdateViewport(renderer);
      camera->ParallelProjectionOn();
      camera->SetParallelScale(16);

      renWin->Render();
      if (compositeRGBAPass->IsSupported(static_cast<svtkOpenGLRenderWindow*>(renWin)))
      {
        this->ReturnValue = svtkRegressionTester::Test(this->Argc, this->Argv, renWin, 10);
      }
      else
      {
        this->ReturnValue = svtkTesting::PASSED; // not supported.
      }

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
  }

  if (me == 0)
  {
    iren->Delete();
  }

  // CLEAN UP

  mapper->Delete();
  actor->Delete();
  renderer->Delete();
  renWin->Delete();

  dd->Delete();
  dsr->Delete();
  ug->Delete();

  ps->Delete();
  dss->Delete();

  prm->Delete();
}

}

int DistributedDataRenderPass(int argc, char* argv[])
{
  int retVal = 1;

  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize(&argc, &argv);

  svtkMultiProcessController::SetGlobalController(contr);

  int numProcs = contr->GetNumberOfProcesses();
  int me = contr->GetLocalProcessId();

  if (numProcs < 2)
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
