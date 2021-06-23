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
#include "svtkTestUtilities.h"

#include <svtkBMPReader.h>
#include <svtkBMPWriter.h>
#include <svtkImageCanvasSource2D.h>
#include <svtkImageCast.h>
#include <svtkImageViewer.h>
#include <svtkJPEGReader.h>
#include <svtkJPEGWriter.h>
#include <svtkPNGReader.h>
#include <svtkPNGWriter.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>

#include <svtksys/SystemTools.hxx>

int TestWriteToUnicodeFile(int argc, char* argv[])
{
  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <output file name>" << endl;
    return EXIT_FAILURE;
  }

  const char* tdir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir = tdir;
  delete[] tdir;
  if (tempDir.empty())
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }

  tempDir = tempDir + "/" +
    "\xc3\xba\xce\xae\xd1\x97\xc3\xa7\xe1\xbb\x99\xe2\x82\xab\xe2\x84\xae"; // u8"úήїçộ₫℮"
  if (!svtksys::SystemTools::FileExists(tempDir))
  {
    svtksys::SystemTools::MakeDirectory(tempDir);
  }

  std::string filename = tempDir + "/" + "\xef\xbd\xb7\xef\xbe\x80"; // u8"ｷﾀ"
  filename.append(argv[1]);
  size_t dotpos = filename.find_last_of('.');
  if (dotpos == std::string::npos)
  {
    std::cout << "Could not determine file extension.\n";
    return EXIT_FAILURE;
  }
  std::string fileext = filename.substr(dotpos + 1);
  filename.insert(dotpos, "\xea\x92\x84"); // u8"ꒄ"

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

  svtkSmartPointer<svtkImageCast> filter = svtkSmartPointer<svtkImageCast>::New();
  filter->SetOutputScalarTypeToUnsignedChar();
  filter->SetInputConnection(imageSource->GetOutputPort());
  filter->Update();

  svtkSmartPointer<svtkImageWriter> writer;
  svtkSmartPointer<svtkImageReader2> reader;

  // Delete any existing files to prevent false failures
  if (!svtksys::SystemTools::FileExists(filename))
  {
    svtksys::SystemTools::RemoveFile(filename);
  }

  if (fileext == "png")
  {
    writer = svtkSmartPointer<svtkPNGWriter>::New();
    reader = svtkSmartPointer<svtkPNGReader>::New();
  }
  else if (fileext == "jpeg" || fileext == "jpg")
  {
    writer = svtkSmartPointer<svtkJPEGWriter>::New();
    reader = svtkSmartPointer<svtkJPEGReader>::New();
  }
  else if (fileext == "bmp")
  {
    writer = svtkSmartPointer<svtkBMPWriter>::New();
    reader = svtkSmartPointer<svtkBMPReader>::New();
  }

  writer->SetInputConnection(filter->GetOutputPort());
  writer->SetFileName(filename.c_str());
  writer->Update();
  writer->Write();

  if (svtksys::SystemTools::FileExists(filename))
  {
    if (!reader->CanReadFile(filename.c_str()))
    {
      cerr << "CanReadFile failed for " << filename.c_str() << "\n";
      return EXIT_FAILURE;
    }

    // Read the input image
    reader->SetFileName(filename.c_str());
    reader->Update();

    const char* fileExtensions = reader->GetFileExtensions();
    cout << "File extensions: " << fileExtensions << endl;

    const char* descriptiveName = reader->GetDescriptiveName();
    cout << "Descriptive name: " << descriptiveName << endl;

    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
