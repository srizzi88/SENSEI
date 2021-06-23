/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGLTFReaderGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkActor.h>
#include <svtkCompositePolyDataMapper.h>
#include <svtkGLTFReader.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>

int TestGLTFReaderGeometry(int argc, char* argv[])
{

  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <gltf file>" << std::endl;
    return EXIT_FAILURE;
  }

  svtkNew<svtkGLTFReader> reader;
  reader->SetFileName(argv[1]);
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

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
