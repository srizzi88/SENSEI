/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestJPEGReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkJPEGReader
// .SECTION Description
//

#include "svtkImageData.h"
#include "svtkImageViewer.h"
#include "svtkJPEGReader.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

int TestJPEGReader(int argc, char* argv[])
{

  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <jpeg file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  svtkSmartPointer<svtkJPEGReader> JPEGReader = svtkSmartPointer<svtkJPEGReader>::New();

  // Check the image can be read
  if (!JPEGReader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename.c_str() << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  JPEGReader->SetFileName(filename.c_str());
  JPEGReader->Update();

  // Read and display the image properties
  const char* fileExtensions = JPEGReader->GetFileExtensions();
  cout << "File xtensions: " << fileExtensions << endl;

  const char* descriptiveName = JPEGReader->GetDescriptiveName();
  cout << "Descriptive name: " << descriptiveName << endl;

  // Visualize
  svtkSmartPointer<svtkImageViewer> imageViewer = svtkSmartPointer<svtkImageViewer>::New();
  imageViewer->SetInputConnection(JPEGReader->GetOutputPort());
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
