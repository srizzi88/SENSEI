/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGLTFImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkGLTFImporter.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>

int TestGLTFImporter(int argc, char* argv[])
{
  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <gltf file>" << std::endl;
    return EXIT_FAILURE;
  }

  svtkNew<svtkGLTFImporter> importer;
  importer->SetFileName(argv[1]);

  svtkNew<svtkRenderWindow> renderWindow;
  importer->SetRenderWindow(renderWindow);

  svtkNew<svtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(.0, .0, .2);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  importer->Update();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
