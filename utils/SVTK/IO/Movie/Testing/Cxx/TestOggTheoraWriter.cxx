/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOggTheoraWriter.cxx

  Copyright (c) Michael Wild, Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestOggTheoraWriter - Tests svtkOggTheoraWriter.
// .SECTION Description
// Creates a scene and uses OggTheoraWriter to generate a movie file. Test passes
// if the file exists and has non zero length.

#include "svtkImageCast.h"
#include "svtkImageData.h"
#include "svtkImageMandelbrotSource.h"
#include "svtkImageMapToColors.h"
#include "svtkLookupTable.h"
#include "svtkOggTheoraWriter.h"
#include "svtkTestUtilities.h"

#include "svtksys/SystemTools.hxx"
#include <string>

int TestOggTheoraWriter(int argc, char* argv[])
{
  char* tempDir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");

  int err = 0;
  int cc = 0;
  int exists = 0;
  unsigned long length = 0;
  svtkImageMandelbrotSource* Fractal0 = svtkImageMandelbrotSource::New();
  Fractal0->SetWholeExtent(0, 247, 0, 247, 0, 0);
  Fractal0->SetProjectionAxes(0, 1, 2);
  Fractal0->SetOriginCX(-1.75, -1.25, 0, 0);
  Fractal0->SetSizeCX(2.5, 2.5, 2, 1.5);
  Fractal0->SetMaximumNumberOfIterations(100);

  svtkImageCast* cast = svtkImageCast::New();
  cast->SetInputConnection(Fractal0->GetOutputPort());
  cast->SetOutputScalarTypeToUnsignedChar();

  svtkLookupTable* table = svtkLookupTable::New();
  table->SetTableRange(0, 100);
  table->SetNumberOfColors(100);
  table->Build();
  table->SetTableValue(99, 0, 0, 0);

  svtkImageMapToColors* colorize = svtkImageMapToColors::New();
  colorize->SetOutputFormatToRGB();
  colorize->SetLookupTable(table);
  colorize->SetInputConnection(cast->GetOutputPort());

  std::string fileName = std::string(tempDir) + std::string("/TestOggTheoraWriter.ogv");
  svtkOggTheoraWriter* w = svtkOggTheoraWriter::New();
  w->SetInputConnection(colorize->GetOutputPort());
  w->SetFileName(fileName.c_str());
  std::cout << "Writing file " << fileName << "..." << std::endl;
  w->Start();
  for (cc = 2; cc < 10; cc++)
  {
    std::cout << ".";
    Fractal0->SetMaximumNumberOfIterations(cc);
    table->SetTableRange(0, cc);
    table->SetNumberOfColors(cc);
    table->ForceBuild();
    table->SetTableValue(cc - 1, 0, 0, 0);
    w->Write();
  }
  w->End();
  std::cout << std::endl;
  std::cout << "Done writing file TestOggTheoraWriter.ogv..." << std::endl;
  w->Delete();

  exists = (int)svtksys::SystemTools::FileExists(fileName.c_str());
  length = svtksys::SystemTools::FileLength(fileName);
  std::cout << "TestOggTheoraWriter.ogv file exists: " << exists << std::endl;
  std::cout << "TestOggTheoraWriter.ogv file length: " << length << std::endl;
  if (!exists)
  {
    err = 1;
    std::cerr << "ERROR: 1 - Test failing because TestOggTheoraWriter.ogv file doesn't exist..."
              << std::endl;
  }
  else
  {
    svtksys::SystemTools::RemoveFile("TestOggTheoraWriter.ogv");
  }
  if (0 == length)
  {
    err = 2;
    std::cerr << "ERROR: 2 - Test failing because TestOggTheoraWriter.ogv file has zero length..."
              << std::endl;
  }

  colorize->Delete();
  table->Delete();
  cast->Delete();
  Fractal0->Delete();

  // err == 0 means test passes...
  //
  delete[] tempDir;

  return err;
}
