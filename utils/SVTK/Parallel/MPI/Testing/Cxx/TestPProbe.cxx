/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProcess.cxx

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
#include "svtkCompositeRenderManager.h"
#include "svtkLineSource.h"
#include "svtkMPIController.h"
#include "svtkPDataSetReader.h"
#include "svtkPOutlineFilter.h"
#include "svtkPProbeFilter.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTubeFilter.h"

int TestPProbe(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize();

  int numProcs = contr->GetNumberOfProcesses();
  int me = contr->GetLocalProcessId();

  // create a rendering window and renderer
  svtkRenderer* Ren1 = svtkRenderer::New();
  Ren1->SetBackground(.5, .8, 1);

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(Ren1);
  renWin->SetSize(300, 300);

  if (me > 0)
  {
    renWin->SetPosition(me * 350, 0);
    renWin->OffScreenRenderingOn();
  }

  // camera parameters
  svtkCamera* camera = Ren1->GetActiveCamera();
  camera->SetPosition(199.431, 196.879, 15.7781);
  camera->SetFocalPoint(33.5, 33.5, 33.5);
  camera->SetViewUp(0.703325, -0.702557, 0.108384);
  camera->SetViewAngle(30);
  camera->SetClippingRange(132.14, 361.741);

  svtkPDataSetReader* ironProt0 = svtkPDataSetReader::New();
  char* fname1 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.svtk");
  ironProt0->SetFileName(fname1);
  delete[] fname1;

  svtkPOutlineFilter* Geometry4 = svtkPOutlineFilter::New();
  Geometry4->SetController(contr);
  Geometry4->SetInputConnection(ironProt0->GetOutputPort());

  svtkPolyDataMapper* Mapper4 = svtkPolyDataMapper::New();
  Mapper4->SetInputConnection(Geometry4->GetOutputPort());
  Mapper4->SetScalarRange(0, 1);
  Mapper4->SetScalarVisibility(0);
  Mapper4->SetScalarModeToDefault();

  svtkActor* Actor4 = svtkActor::New();
  Actor4->SetMapper(Mapper4);
  Actor4->GetProperty()->SetRepresentationToSurface();
  Actor4->GetProperty()->SetInterpolationToGouraud();
  Actor4->GetProperty()->SetColor(1, 1, 1);
  Ren1->AddActor(Actor4);

  svtkLineSource* probeLine = svtkLineSource::New();
  probeLine->SetPoint1(0, 67, 10);
  probeLine->SetPoint2(67, 0, 50);
  probeLine->SetResolution(500);

  svtkPProbeFilter* Probe0 = svtkPProbeFilter::New();
  Probe0->SetSourceConnection(ironProt0->GetOutputPort());
  Probe0->SetInputConnection(probeLine->GetOutputPort());
  Probe0->SetController(contr);

  svtkTubeFilter* Tuber0 = svtkTubeFilter::New();
  Tuber0->SetInputConnection(Probe0->GetOutputPort());
  Tuber0->SetNumberOfSides(10);
  Tuber0->SetCapping(0);
  Tuber0->SetRadius(1);
  Tuber0->SetVaryRadius(1);
  Tuber0->SetRadiusFactor(10);
  Tuber0->Update();

  svtkPolyDataMapper* Mapper6 = svtkPolyDataMapper::New();
  Mapper6->SetInputConnection(Tuber0->GetOutputPort());
  Mapper6->SetScalarRange(0, 228);
  Mapper6->SetScalarVisibility(1);
  Mapper6->SetScalarModeToUsePointFieldData();
  Mapper6->ColorByArrayComponent("scalars", -1);
  Mapper6->UseLookupTableScalarRangeOn();

  svtkActor* Actor6 = svtkActor::New();
  Actor6->SetMapper(Mapper6);
  Actor6->GetProperty()->SetRepresentationToSurface();
  Actor6->GetProperty()->SetInterpolationToGouraud();
  Ren1->AddActor(Actor6);

  svtkCompositeRenderManager* compManager = svtkCompositeRenderManager::New();
  compManager->SetRenderWindow(renWin);
  compManager->SetController(contr);
  compManager->InitializePieces();

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  int retVal;

  if (me)
  {
    compManager->InitializeRMIs();
    contr->ProcessRMIs();
    contr->Receive(&retVal, 1, 0, 33);
  }
  else
  {
    renWin->Render();
    retVal = svtkRegressionTester::Test(argc, argv, renWin, 10);
    for (int i = 1; i < numProcs; i++)
    {
      contr->TriggerRMI(i, svtkMultiProcessController::BREAK_RMI_TAG);
      contr->Send(&retVal, 1, i, 33);
    }
  }

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    compManager->StartInteractor();
  }

  contr->Finalize();
  contr->Delete();

  iren->Delete();
  renWin->Delete();
  compManager->Delete();
  Actor6->Delete();
  Mapper6->Delete();
  Tuber0->Delete();
  Probe0->Delete();
  probeLine->Delete();
  Actor4->Delete();
  Mapper4->Delete();
  Geometry4->Delete();
  ironProt0->Delete();
  Ren1->Delete();

  return !retVal;
}
