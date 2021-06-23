/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompressedTIFFReader.cxx

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

int TestCompressedTIFFReader(int argc, char* argv[])
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
  reader->UpdateInformation();

  // Read the image in 4 chunks. This exercises the logic to read random scan
  // lines from files that do not support it.
  const int maxNumPieces = 4;
  for (int cc = 0; cc < maxNumPieces; cc++)
  {
    reader->UpdatePiece(cc, maxNumPieces, 0);
  }
  reader->UpdateWholeExtent();

  // Visualize
  svtkSmartPointer<svtkImageViewer2> imageViewer = svtkSmartPointer<svtkImageViewer2>::New();
  imageViewer->SetInputConnection(reader->GetOutputPort());
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  imageViewer->SetupInteractor(renderWindowInteractor);
  imageViewer->Render();
  imageViewer->GetRenderer()->ResetCamera();
  renderWindowInteractor->Initialize();
  imageViewer->Render();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
