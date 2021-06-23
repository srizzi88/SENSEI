/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPNGReaderReadFromMemory.cxx

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
#include "svtksys/FStream.hxx"
#include "svtksys/SystemTools.hxx"

#include <fstream>
#include <vector>

int TestPNGReaderReadFromMemory(int argc, char* argv[])
{

  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <png file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  // Open the file
  svtksys::ifstream stream(filename.c_str(), std::ios::in | std::ios::binary);
  if (!stream.is_open())
  {
    std::cerr << "Could not open file " << filename.c_str() << std::endl;
  }
  // Get file size
  unsigned long len = svtksys::SystemTools::FileLength(filename);

  // Load the file into a buffer
  std::vector<char> buffer = std::vector<char>(std::istreambuf_iterator<char>(stream), {});

  // Initialize reader
  svtkNew<svtkPNGReader> pngReader;
  pngReader->SetMemoryBuffer(buffer.data());
  pngReader->SetMemoryBufferLength(len);

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
