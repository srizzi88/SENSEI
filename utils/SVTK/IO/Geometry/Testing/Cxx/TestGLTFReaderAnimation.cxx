/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGLTFReaderAnimation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkCompositePolyDataMapper.h>
#include <svtkGLTFReader.h>
#include <svtkInformation.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkStreamingDemandDrivenPipeline.h>

int TestGLTFReaderAnimation(int argc, char* argv[])
{
  if (argc <= 2)
  {
    std::cout << "Usage: " << argv[0] << "<step> <gltf file>" << std::endl;
    return EXIT_FAILURE;
  }

  const int step = std::atoi(argv[1]);
  svtkNew<svtkGLTFReader> reader;
  reader->SetFileName(argv[2]);
  reader->SetFrameRate(60);
  reader->ApplyDeformationsToGeometryOn();

  reader->UpdateInformation(); // Read model metadata to get the number of animations
  for (svtkIdType i = 0; i < reader->GetNumberOfAnimations(); i++)
  {
    reader->EnableAnimation(i);
  }

  reader->UpdateInformation(); // Update number of time steps now that animations are enabled
  svtkInformation* readerInfo = reader->GetOutputInformation(0);

  int nbSteps = readerInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (nbSteps < step)
  {
    std::cerr << "Invalid number of steps for input argument: " << step << std::endl;
    return EXIT_FAILURE;
  }

  double time = readerInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), step);
  readerInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
  reader->Update();

  svtkNew<svtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(.0, .0, .2);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->GetActiveCamera()->SetClippingRange(0.1, 1000);
  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
