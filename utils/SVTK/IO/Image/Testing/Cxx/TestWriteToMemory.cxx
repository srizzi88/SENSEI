/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMetaIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of WriteToMemory flag for PNG/JPEG/BMP Writers
// .SECTION Description
//

#include <svtkBMPWriter.h>
#include <svtkImageCanvasSource2D.h>
#include <svtkImageCast.h>
#include <svtkJPEGWriter.h>
#include <svtkPNGWriter.h>
#include <svtkSmartPointer.h>

#include <svtksys/SystemTools.hxx>

int TestWriteToMemory(int argc, char* argv[])
{
  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <output file name>" << endl;
    return EXIT_FAILURE;
  }

  int extent[6] = { 0, 99, 0, 99, 0, 0 };
  svtkSmartPointer<svtkImageCanvasSource2D> imageSource =
    svtkSmartPointer<svtkImageCanvasSource2D>::New();
  imageSource->SetExtent(extent);
  imageSource->SetScalarTypeToUnsignedChar();
  imageSource->SetNumberOfScalarComponents(3);
  imageSource->SetDrawColor(127, 45, 255);
  imageSource->FillBox(0, 99, 0, 99);
  imageSource->SetDrawColor(255, 255, 255);
  imageSource->FillBox(40, 70, 20, 50);
  imageSource->Update();

  svtkSmartPointer<svtkImageCast> castFilter = svtkSmartPointer<svtkImageCast>::New();
  castFilter->SetOutputScalarTypeToUnsignedChar();
  castFilter->SetInputConnection(imageSource->GetOutputPort());
  castFilter->Update();

  svtkSmartPointer<svtkImageWriter> writer;

  std::string filename = argv[1];
  std::string fileext = filename.substr(filename.find_last_of('.') + 1);

  // Delete any existing files to prevent false failures
  if (svtksys::SystemTools::FileExists(filename))
  {
    svtksys::SystemTools::RemoveFile(filename);
  }

  if (fileext == "png")
  {
    svtkSmartPointer<svtkPNGWriter> pngWriter = svtkSmartPointer<svtkPNGWriter>::New();
    pngWriter->WriteToMemoryOn();
    writer = pngWriter;
  }
  else if (fileext == "jpeg" || fileext == "jpg")
  {
    svtkSmartPointer<svtkJPEGWriter> jpgWriter = svtkSmartPointer<svtkJPEGWriter>::New();
    jpgWriter->WriteToMemoryOn();
    writer = jpgWriter;
  }
  else if (fileext == "bmp")
  {
    svtkSmartPointer<svtkBMPWriter> bmpWriter = svtkSmartPointer<svtkBMPWriter>::New();
    bmpWriter->WriteToMemoryOn();
    writer = bmpWriter;
  }

  writer->SetFileName(filename.c_str());
  writer->SetInputConnection(castFilter->GetOutputPort());
  writer->Update();
  writer->Write();

  // With WriteToMemory true no file should be written
  if (!svtksys::SystemTools::FileExists(filename))
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
