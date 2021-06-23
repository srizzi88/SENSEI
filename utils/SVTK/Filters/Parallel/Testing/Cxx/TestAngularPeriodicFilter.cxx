/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "svtkActor.h"
#include "svtkAngularPeriodicFilter.h"
#include "svtkCamera.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkGeometryFilter.h"
#include "svtkLookupTable.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPointSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamTracer.h"
#include "svtkTesting.h"
#include "svtkTriangleFilter.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLUnstructuredGridReader.h"

int TestAngularPeriodicFilter(int argc, char* argv[])
{
  svtkNew<svtkXMLUnstructuredGridReader> reader;

  svtkNew<svtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error : -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();
  std::string inputFileName = dataRoot + "/Data/periodicPiece.vtu";
  reader->SetFileName(inputFileName.c_str());
  reader->Update();

  svtkNew<svtkMultiBlockDataSet> mb;
  mb->SetNumberOfBlocks(1);
  mb->SetBlock(0, reader->GetOutput());

  svtkNew<svtkAngularPeriodicFilter> angularPeriodicFilter;
  angularPeriodicFilter->SetInputData(mb);
  angularPeriodicFilter->AddIndex(1);
  angularPeriodicFilter->SetIterationModeToMax();
  angularPeriodicFilter->SetRotationModeToDirectAngle();
  angularPeriodicFilter->SetRotationAngle(45.);
  angularPeriodicFilter->SetRotationAxisToZ();

  svtkNew<svtkGeometryFilter> geomFilter;
  geomFilter->SetInputData(mb);

  svtkNew<svtkTriangleFilter> triangleFilter;
  triangleFilter->SetInputConnection(geomFilter->GetOutputPort());

  svtkNew<svtkPointSource> seed;
  seed->SetCenter(5.80752824733665, -3.46144284193073, -5.83410675177451);
  seed->SetNumberOfPoints(1);
  seed->SetRadius(2);

  svtkNew<svtkStreamTracer> streamTracer;
  streamTracer->SetInputConnection(angularPeriodicFilter->GetOutputPort());
  streamTracer->SetInputArrayToProcess(0, 0, 0, 0, "Result");
  streamTracer->SetInterpolatorType(0);
  streamTracer->SetIntegrationDirection(2);
  streamTracer->SetIntegratorType(2);
  streamTracer->SetIntegrationStepUnit(2);
  streamTracer->SetInitialIntegrationStep(0.2);
  streamTracer->SetMinimumIntegrationStep(0.01);
  streamTracer->SetMaximumIntegrationStep(0.5);
  streamTracer->SetMaximumNumberOfSteps(2000);
  streamTracer->SetMaximumPropagation(28.);
  streamTracer->SetTerminalSpeed(0.000000000001);
  streamTracer->SetMaximumError(0.000001);
  streamTracer->SetComputeVorticity(1);

  streamTracer->SetSourceConnection(seed->GetOutputPort());
  streamTracer->Update();

  svtkPolyData* pd = streamTracer->GetOutput();
  pd->GetPointData()->SetActiveScalars("RTData");

  svtkNew<svtkLookupTable> hueLut;
  hueLut->SetHueRange(0., 1.);
  hueLut->SetSaturationRange(1., 1.);
  hueLut->Build();

  svtkNew<svtkCompositePolyDataMapper> multiBlockMapper;
  multiBlockMapper->SetInputConnection(triangleFilter->GetOutputPort());
  multiBlockMapper->SetLookupTable(hueLut);
  multiBlockMapper->SetScalarRange(131., 225.);
  multiBlockMapper->SetColorModeToMapScalars();
  multiBlockMapper->SetScalarModeToUsePointData();

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(streamTracer->GetOutputPort());
  mapper->SetLookupTable(hueLut);
  mapper->SetScalarRange(131., 225.);
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointData();

  svtkNew<svtkActor> multiBlockActor;
  multiBlockActor->SetMapper(multiBlockMapper);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(multiBlockActor);
  renderer->AddActor(actor);
  renderer->GetActiveCamera()->SetPosition(
    3.97282457351685, -0.0373859405517578, -59.3025624847687);
  renderer->ResetCamera();
  renderer->SetBackground(1., 1., 1.);

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
