/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTIFFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkSmartPointer.h>

#include <svtkImageData.h>
#include <svtkImageViewer2.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkTIFFReader.h>

int TestTIFFReader(int argc, char* argv[])
{
  // Verify input arguments
  if (argc < 3)
  {
    std::cout << "Usage: " << argv[0] << " Filename(.tif)" << std::endl;
    return EXIT_FAILURE;
  }

  // Read the image
  svtkSmartPointer<svtkTIFFReader> reader = svtkSmartPointer<svtkTIFFReader>::New();
  reader->SetFileName(argv[1]);
  reader->SetOrientationType(4);
  reader->Update();

  // Display the center slice
  int sliceNumber = (reader->GetOutput()->GetExtent()[5] + reader->GetOutput()->GetExtent()[4]) / 2;

  // Visualize
  svtkSmartPointer<svtkImageViewer2> imageViewer = svtkSmartPointer<svtkImageViewer2>::New();
  imageViewer->SetInputConnection(reader->GetOutputPort());
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  imageViewer->SetupInteractor(renderWindowInteractor);
  imageViewer->SetSlice(sliceNumber);
  imageViewer->Render();
  imageViewer->GetRenderer()->ResetCamera();
  renderWindowInteractor->Initialize();
  imageViewer->Render();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
