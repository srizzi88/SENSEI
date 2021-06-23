/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBMPReaderDoNotAllow8BitBMP.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkBMPReader
// .SECTION Description
//

#include "svtkSmartPointer.h"

#include "svtkBMPReader.h"

#include "svtkImageData.h"
#include "svtkImageViewer2.h"
#include "svtkLookupTable.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestBMPReaderDoNotAllow8BitBMP(int argc, char* argv[])
{

  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <bmp file>" << endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  svtkSmartPointer<svtkBMPReader> BMPReader = svtkSmartPointer<svtkBMPReader>::New();

  // Check the image can be read
  if (!BMPReader->CanReadFile(filename.c_str()))
  {
    cerr << "CanReadFile failed for " << filename.c_str() << "\n";
    return EXIT_FAILURE;
  }

  // Read the input image
  BMPReader->SetFileName(filename.c_str());
  BMPReader->Update();

  // Read and display the image properties
  int depth = BMPReader->GetDepth();
  cout << "depth: " << depth << endl;

  const char* fileExtensions = BMPReader->GetFileExtensions();
  cout << "fileExtensions: " << fileExtensions << endl;

  const char* descriptiveName = BMPReader->GetDescriptiveName();
  cout << "descriptiveName: " << *descriptiveName << endl;

  svtkSmartPointer<svtkLookupTable> lookupTable = BMPReader->GetLookupTable();
  lookupTable->Print(cout);

  const unsigned char* colors = BMPReader->GetColors();
  unsigned char const* first = reinterpret_cast<unsigned char*>(&colors);
  unsigned char const* last = reinterpret_cast<unsigned char*>(&colors + 1);
  cout << "colors: ";
  while (first != last)
  {
    cout << (int)*first << ' ';
    ++first;
  }
  cout << std::endl;

  int allow8BitBMP = 0;
  BMPReader->SetAllow8BitBMP(allow8BitBMP);
  cout << "allow8BitBMP: " << BMPReader->GetAllow8BitBMP() << endl;

  // Visualize
  svtkSmartPointer<svtkImageViewer2> imageViewer = svtkSmartPointer<svtkImageViewer2>::New();
  imageViewer->SetInputConnection(BMPReader->GetOutputPort());
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
