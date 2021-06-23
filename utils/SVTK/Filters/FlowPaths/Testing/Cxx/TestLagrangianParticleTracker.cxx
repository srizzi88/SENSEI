/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLagrangianParticleTracker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLagrangianMatidaIntegrationModel.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataSetMapper.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDoubleArray.h"
#include "svtkGlyph3D.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkLagrangianParticleTracker.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataGroupFilter.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPointSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRungeKutta2.h"
#include "svtkSphereSource.h"

int TestLagrangianParticleTracker(int, char*[])
{
  // Create a point source
  svtkNew<svtkPointSource> seeds;
  seeds->SetNumberOfPoints(10);
  seeds->SetRadius(4);
  seeds->Update();
  svtkPolyData* seedPD = seeds->GetOutput();
  svtkPointData* seedData = seedPD->GetPointData();

  // Create seed data
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

  // Create a wavelet
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->Update();
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

  // Create surface
  svtkNew<svtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(wavelet->GetOutputPort());
  surface->Update();
  svtkPolyData* surfacePd = surface->GetOutput();

  // Create Surface data
  svtkNew<svtkDoubleArray> surfaceTypeTerm;
  surfaceTypeTerm->SetNumberOfComponents(1);
  surfaceTypeTerm->SetName("SurfaceType");
  surfaceTypeTerm->SetNumberOfTuples(surfacePd->GetNumberOfCells());
  surfaceTypeTerm->FillComponent(0, svtkLagrangianBasicIntegrationModel::SURFACE_TYPE_TERM);
  surfacePd->GetCellData()->AddArray(surfaceTypeTerm);

  // Create plane passThrough
  svtkNew<svtkPlaneSource> surfacePass;
  surfacePass->SetOrigin(-10, -10, 0);
  surfacePass->SetPoint1(10, -10, 0);
  surfacePass->SetPoint2(-10, 10, 0);
  surfacePass->Update();
  svtkPolyData* passPd = surfacePass->GetOutput();

  // Create Surface data
  svtkNew<svtkDoubleArray> surfaceTypePass;
  surfaceTypePass->SetNumberOfComponents(1);
  surfaceTypePass->SetName("SurfaceType");
  surfaceTypePass->SetNumberOfTuples(passPd->GetNumberOfCells());
  surfaceTypePass->FillComponent(0, svtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS);
  passPd->GetCellData()->AddArray(surfaceTypePass);

  // Create plane passThrough
  svtkNew<svtkPlaneSource> surfaceBounce;
  surfaceBounce->SetOrigin(-2, -2, -2);
  surfaceBounce->SetPoint1(5, -2, -2);
  surfaceBounce->SetPoint2(-2, 5, -2);
  surfaceBounce->Update();
  svtkPolyData* bouncePd = surfaceBounce->GetOutput();

  // Create Surface data
  svtkNew<svtkDoubleArray> surfaceTypeBounce;
  surfaceTypeBounce->SetNumberOfComponents(1);
  surfaceTypeBounce->SetName("SurfaceType");
  surfaceTypeBounce->SetNumberOfTuples(bouncePd->GetNumberOfCells());
  surfaceTypeBounce->FillComponent(0, svtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BOUNCE);
  bouncePd->GetCellData()->AddArray(surfaceTypeBounce);

  svtkNew<svtkMultiBlockDataGroupFilter> groupSurface;
  groupSurface->AddInputDataObject(surfacePd);
  groupSurface->AddInputDataObject(passPd);
  groupSurface->AddInputDataObject(bouncePd);

  svtkNew<svtkMultiBlockDataGroupFilter> groupFlow;
  groupFlow->AddInputDataObject(waveletImg);

  svtkNew<svtkImageDataToPointSet> ugFlow;
  ugFlow->AddInputData(waveletImg);

  svtkNew<svtkMultiBlockDataGroupFilter> groupSeed;
  groupSeed->AddInputDataObject(seedPD);
  groupSeed->AddInputDataObject(seedPD);

  // Create Integrator
  svtkNew<svtkRungeKutta2> integrator;

  // Create Integration Model
  svtkNew<svtkLagrangianMatidaIntegrationModel> integrationModel;
  integrationModel->SetInputArrayToProcess(
    0, 1, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "InitialVelocity");
  integrationModel->SetInputArrayToProcess(
    2, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "SurfaceType");
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
  integrationModel->SetNumberOfTrackedUserData(13);

  // Put in tracker
  svtkNew<svtkLagrangianParticleTracker> tracker;
  tracker->SetIntegrator(nullptr);
  tracker->SetIntegrationModel(nullptr);
  tracker->Print(cout);
  if (tracker->GetSource() != nullptr || tracker->GetSurface() != nullptr)
  {
    std::cerr << "Incorrect Input Initialization" << std::endl;
    return EXIT_FAILURE;
  }
  tracker->SetIntegrator(integrator);
  if (tracker->GetIntegrator() != integrator)
  {
    std::cerr << "Incorrect Integrator" << std::endl;
    return EXIT_FAILURE;
  }

  tracker->SetIntegrationModel(integrationModel);
  if (tracker->GetIntegrationModel() != integrationModel)
  {
    std::cerr << "Incorrect Integration Model" << std::endl;
    return EXIT_FAILURE;
  }

  tracker->SetInputConnection(groupFlow->GetOutputPort());
  tracker->SetStepFactor(0.1);
  tracker->SetStepFactorMin(0.1);
  tracker->SetStepFactorMax(0.1);
  tracker->SetMaximumNumberOfSteps(150);
  tracker->SetSourceConnection(groupSeed->GetOutputPort());
  tracker->SetSurfaceData(surfacePd);
  tracker->SetCellLengthComputationMode(svtkLagrangianParticleTracker::STEP_CUR_CELL_VEL_DIR);
  tracker->AdaptiveStepReintegrationOn();
  tracker->GenerateParticlePathsOutputOff();
  tracker->Update();
  tracker->GenerateParticlePathsOutputOn();
  tracker->SetInputConnection(ugFlow->GetOutputPort());
  tracker->SetMaximumNumberOfSteps(30);
  tracker->SetCellLengthComputationMode(svtkLagrangianParticleTracker::STEP_CUR_CELL_DIV_THEO);
  tracker->Update();
  tracker->SetMaximumNumberOfSteps(-1);
  tracker->SetMaximumIntegrationTime(10.0);
  tracker->Update();
  tracker->SetInputData(waveletImg);
  tracker->SetSourceData(seedPD);
  tracker->SetMaximumNumberOfSteps(300);
  tracker->SetMaximumIntegrationTime(-1.0);
  tracker->SetSurfaceConnection(groupSurface->GetOutputPort());
  tracker->SetCellLengthComputationMode(svtkLagrangianParticleTracker::STEP_LAST_CELL_VEL_DIR);
  tracker->AdaptiveStepReintegrationOff();
  tracker->Update();
  if (tracker->GetStepFactor() != 0.1)
  {
    std::cerr << "Incorrect StepFactor" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetStepFactorMin() != 0.1)
  {
    std::cerr << "Incorrect StepFactorMin" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetStepFactorMax() != 0.1)
  {
    std::cerr << "Incorrect StepFactorMax" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetMaximumNumberOfSteps() != 300)
  {
    std::cerr << "Incorrect MaximumNumberOfSteps" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetMaximumIntegrationTime() != -1.0)
  {
    std::cerr << "Incorrect MaximumIntegrationTime" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetCellLengthComputationMode() !=
    svtkLagrangianParticleTracker::STEP_LAST_CELL_VEL_DIR)
  {
    std::cerr << "Incorrect CellLengthComputationMode" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetAdaptiveStepReintegration())
  {
    std::cerr << "Incorrect AdaptiveStepReintegration" << std::endl;
    return EXIT_FAILURE;
  }
  if (!tracker->GetGenerateParticlePathsOutput())
  {
    std::cerr << "Incorrect GenerateParticlePathsOutput" << std::endl;
    return EXIT_FAILURE;
  }
  tracker->Print(cout);
  if (tracker->GetSource() != seedPD)
  {
    std::cerr << "Incorrect Source" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetSurface() != groupSurface->GetOutput())
  {
    std::cerr << "Incorrect Surface" << std::endl;
    return EXIT_FAILURE;
  }

  // Glyph for interaction points
  svtkNew<svtkSphereSource> sphereGlyph;
  sphereGlyph->SetRadius(0.1);

  svtkNew<svtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(2, 2, 2);
  svtkNew<svtkPolyData> polydata;
  polydata->SetPoints(points);

  svtkNew<svtkGlyph3D> glyph;
  glyph->SetSourceConnection(sphereGlyph->GetOutputPort());
  svtkMultiBlockDataSet* mbInter = svtkMultiBlockDataSet::SafeDownCast(tracker->GetOutput(1));
  glyph->SetInputData(mbInter->GetBlock(1));

  // Setup actor and mapper
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputData(svtkPolyData::SafeDownCast(tracker->GetOutput()));

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkPolyDataMapper> surfaceMapper;
  surfaceMapper->SetInputConnection(surfaceBounce->GetOutputPort());
  svtkNew<svtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  svtkNew<svtkPolyDataMapper> surfaceMapper2;
  surfaceMapper2->SetInputConnection(surfacePass->GetOutputPort());
  svtkNew<svtkActor> surfaceActor2;
  surfaceActor2->SetMapper(surfaceMapper2);

  svtkNew<svtkPolyDataMapper> glyphMapper;
  glyphMapper->SetInputConnection(glyph->GetOutputPort());
  svtkNew<svtkActor> glyphActor;
  glyphActor->SetMapper(glyphMapper);

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
  renderer->AddActor(actor);
  renderer->AddActor(surfaceActor);
  renderer->AddActor(surfaceActor2);
  renderer->AddActor(glyphActor);
  renderer->SetBackground(0.1, .5, 1);

  renderWindow->Render();
  renderWindowInteractor->Start();
  return EXIT_SUCCESS;
}
