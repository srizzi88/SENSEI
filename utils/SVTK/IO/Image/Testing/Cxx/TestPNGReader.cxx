/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPNGReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageData.h"
#include "svtkImageViewer.h"
#include "svtkPNGReader.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestPNGReader(int argc, char* argv[])
{

  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <png file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  svtkNew<svtkPNGReader> pngReader;

  // Check the image can be read
  if (!pngReader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename.c_str() << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  pngReader->SetFileName(filename.c_str());
  pngReader->Update();

  // Read and display the image properties
  const char* fileExtensions = pngReader->GetFileExtensions();
  cout << "File extensions: " << fileExtensions << endl;

  const char* descriptiveName = pngReader->GetDescriptiveName();
  cout << "Descriptive name: " << descriptiveName << endl;

  // Visualize
  svtkNew<svtkImageViewer> imageViewer;
  imageViewer->SetInputConnection(pngReader->GetOutputPort());
  imageViewer->SetColorWindow(256);
  imageViewer->SetColorLevel(127.5);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  imageViewer->SetupInteractor(renderWindowInteractor);
  imageViewer->Render();

  svtkRenderWindow* renWin = imageViewer->GetRenderWindow();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
