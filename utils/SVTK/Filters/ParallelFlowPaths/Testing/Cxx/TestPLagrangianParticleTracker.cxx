/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLagrangianParticleTracker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCompositeRenderManager.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkLagrangianMatidaIntegrationModel.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkOutlineFilter.h"
#include "svtkPLagrangianParticleTracker.h"
#include "svtkPointData.h"
#include "svtkPointSource.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRungeKutta2.h"
#include "svtkTrivialProducer.h"

struct PLagrangianParticleTrackerArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

// This will be called by all processes
void MainPLagrangianParticleTracker(svtkMultiProcessController* controller, void* arg)
{
  PLagrangianParticleTrackerArgs_tmp* args =
    reinterpret_cast<PLagrangianParticleTrackerArgs_tmp*>(arg);

  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  // Setup camera
  svtkNew<svtkCamera> camera;
  camera->SetFocalPoint(0, 0, -1);
  camera->SetViewUp(0, 0, 1);
  camera->SetPosition(0, -40, 0);

  // Setup render window, renderer, and interactor
  svtkNew<svtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Create seeds with point source
  svtkNew<svtkPointSource> seeds;
  seeds->SetNumberOfPoints(10);
  seeds->SetRadius(4);
  seeds->Update();
  svtkPolyData* seedPD = seeds->GetOutput();
  svtkPointData* seedData = seedPD->GetPointData();

  svtkNew<svtkDoubleArray> partVel;
  partVel->SetNumberOfComponents(3);
  partVel->SetNumberOfTuples(seedPD->GetNumberOfPoints());
  partVel->SetName("InitialVelocity");

  svtkNew<svtkDoubleArray> partDens;
  partDens->SetNumberOfComponents(1);
  partDens->SetNumberOfTuples(seedPD->GetNumberOfPoints());
  partDens->SetName("ParticleDensity");

  svtkNew<svtkDoubleArray> partDiam;
  partDiam->SetNumberOfComponents(1);
  partDiam->SetNumberOfTuples(seedPD->GetNumberOfPoints());
  partDiam->SetName("ParticleDiameter");

  partVel->FillComponent(0, 2);
  partVel->FillComponent(1, 5);
  partVel->FillComponent(2, 1);
  partDens->FillComponent(0, 1920);
  partDiam->FillComponent(0, 0.1);

  seedData->AddArray(partVel);
  seedData->AddArray(partDens);
  seedData->AddArray(partDiam);

  // Create input (flow) from wavelet
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->UpdateInformation();
  wavelet->UpdatePiece(myId, numProcs, 0);
  svtkImageData* waveletImg = wavelet->GetOutput();

  svtkCellData* cd = waveletImg->GetCellData();

  // Create flow data
  svtkNew<svtkDoubleArray> flowVel;
  flowVel->SetNumberOfComponents(3);
  flowVel->SetNumberOfTuples(waveletImg->GetNumberOfCells());
  flowVel->SetName("FlowVelocity");

  svtkNew<svtkDoubleArray> flowDens;
  flowDens->SetNumberOfComponents(1);
  flowDens->SetNumberOfTuples(waveletImg->GetNumberOfCells());
  flowDens->SetName("FlowDensity");

  svtkNew<svtkDoubleArray> flowDynVisc;
  flowDynVisc->SetNumberOfComponents(1);
  flowDynVisc->SetNumberOfTuples(waveletImg->GetNumberOfCells());
  flowDynVisc->SetName("FlowDynamicViscosity");

  flowVel->FillComponent(0, -0.3);
  flowVel->FillComponent(1, -0.3);
  flowVel->FillComponent(2, -0.3);
  flowDens->FillComponent(0, 1000);
  flowDynVisc->FillComponent(0, 0.894);

  cd->AddArray(flowVel);
  cd->AddArray(flowDens);
  cd->AddArray(flowDynVisc);

  // Create input outline
  svtkNew<svtkOutlineFilter> outline;
  outline->SetInputData(waveletImg);

  svtkNew<svtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());
  outlineMapper->UseLookupTableScalarRangeOn();
  outlineMapper->SetScalarVisibility(0);
  outlineMapper->SetScalarModeToDefault();

  svtkNew<svtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);
  renderer->AddActor(outlineActor);

  // Create Integrator
  svtkNew<svtkRungeKutta2> integrator;

  // Create Integration Model
  svtkNew<svtkLagrangianMatidaIntegrationModel> integrationModel;
  integrationModel->SetInputArrayToProcess(
    0, 1, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "InitialVelocity");
  integrationModel->SetInputArrayToProcess(2, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "");
  integrationModel->SetInputArrayToProcess(
    3, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowVelocity");
  integrationModel->SetInputArrayToProcess(
    4, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDensity");
  integrationModel->SetInputArrayToProcess(
    5, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDynamicViscosity");
  integrationModel->SetInputArrayToProcess(
    6, 1, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDiameter");
  integrationModel->SetInputArrayToProcess(
    7, 1, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDensity");
  integrationModel->SetNumberOfTrackedUserData(17);

  // Put in tracker
  svtkNew<svtkLagrangianParticleTracker> tracker;
  tracker->SetIntegrator(integrator);
  tracker->SetIntegrationModel(integrationModel);
  tracker->SetInputData(waveletImg);
  tracker->SetStepFactor(0.1);
  tracker->SetSourceData(seedPD);
  // Show tracker result
  svtkNew<svtkPolyDataMapper> trackerMapper;
  trackerMapper->SetInputConnection(tracker->GetOutputPort());
  svtkNew<svtkActor> trackerActor;
  trackerActor->SetMapper(trackerMapper);
  renderer->AddActor(trackerActor);

  // Check result
  svtkNew<svtkCompositeRenderManager> compManager;
  compManager->SetRenderWindow(renderWindow);
  compManager->SetController(controller);
  compManager->InitializePieces();

  if (myId)
  {
    compManager->InitializeRMIs();
    controller->ProcessRMIs();
    controller->Receive(args->retVal, 1, 0, 33);
  }
  else
  {
    renderWindow->Render();
    *(args->retVal) = svtkRegressionTester::Test(args->argc, args->argv, renderWindow, 10);
    for (int i = 1; i < numProcs; i++)
    {
      controller->TriggerRMI(i, svtkMultiProcessController::BREAK_RMI_TAG);
      controller->Send(args->retVal, 1, i, 33);
    }
  }

  if (*(args->retVal) == svtkRegressionTester::DO_INTERACTOR)
  {
    compManager->StartInteractor();
  }
}

int TestPLagrangianParticleTracker(int argc, char* argv[])
{
  svtkNew<svtkMPIController> contr;
  contr->Initialize(&argc, &argv);

  // When using MPI, the number of processes is determined
  // by the external program which launches this application.
  // However, when using threads, we need to set it ourselves.
  if (contr->IsA("svtkThreadedController"))
  {
    // Set the number of processes to 2 for this example.
    contr->SetNumberOfProcesses(2);
  }

  // Added for regression test.
  // ----------------------------------------------
  int retVal;
  PLagrangianParticleTrackerArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  contr->SetSingleMethod(MainPLagrangianParticleTracker, &args);
  contr->SingleMethodExecute();

  contr->Finalize();

  return !retVal;
}
