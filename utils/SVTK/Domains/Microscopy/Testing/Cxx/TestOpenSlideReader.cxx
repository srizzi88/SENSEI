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
int TestOpenSlideReader(int argc, char** argv)
{
  // This test is known to fail with openslide library libopenslide-dev shipped
  // with ubuntu 14.04 as of March 31'2016. It does pass on fedora23, or if the
  // openslide library is built from source
  const char* rasterFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Microscopy/small2.ndpi");

  // std::cout << "Got Filename: " << rasterFileName << std::endl;

  // Create reader to read shape file.
  svtkNew<svtkOpenSlideReader> reader;
  reader->SetFileName(rasterFileName);
  reader->UpdateInformation();
  delete[] rasterFileName;

  // For debug
  // reader->SetUpdateExtent(extent);
  // svtkNew<svtkPNGWriter> writer;
  // writer->SetInputConnection(reader->GetOutputPort());
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
  imageViewer->SetInputConnection(reader->GetOutputPort());
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
