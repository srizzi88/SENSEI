/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenSlideReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkImageData.h>
#include <svtkImageViewer2.h>
#include <svtkNew.h>
#include <svtkOpenSlideReader.h>
#include <svtkPNGWriter.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

// SVTK includes
#include <svtkTestUtilities.h>

// C++ includes
#include <sstream>

// Main program
int TestOpenSlideReaderPartial(int argc, char** argv)
{
  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <image file>" << endl;
    return EXIT_FAILURE;
  }

  std::cout << "Got Filename: " << argv[1] << std::endl;

  // Create reader to read shape file.
  svtkNew<svtkOpenSlideReader> reader;
  reader->SetFileName(argv[1]);
  reader->UpdateInformation();

  int extent[6] = { 100, 299, 100, 299, 0, 0 };

  reader->UpdateExtent(extent);

  svtkNew<svtkImageData> data;
  data->ShallowCopy(reader->GetOutput());

  // // For debug
  // svtkNew<svtkPNGWriter> writer;
  // writer->SetInputData(data);
  // writer->SetFileName("this.png");
  // writer->SetUpdateExtent(extent);
  // writer->Update();
  // writer->Write();

  // Visualize
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> window;
  window->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(window);

  svtkNew<svtkImageViewer2> imageViewer;
  imageViewer->SetInputData(data);
  // imageViewer->SetExtent(1000,1500,1000,1500,0,0);
  imageViewer->SetupInteractor(renderWindowInteractor);
  // imageViewer->SetSlice(0);
  imageViewer->Render();
  imageViewer->GetRenderer()->ResetCamera();
  renderWindowInteractor->Initialize();
  imageViewer->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
