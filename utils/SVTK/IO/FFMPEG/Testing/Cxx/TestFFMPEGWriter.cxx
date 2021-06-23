/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFFMPEGWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestFFMPEGWriter - Tests svtkFFMPEGWriter.
// .SECTION Description
// Creates a scene and uses FFMPEGWriter to generate a movie file. Test passes
// if the file exists and has non zero length.

#include "svtkFFMPEGWriter.h"
#include "svtkImageCast.h"
#include "svtkImageData.h"
#include "svtkImageMandelbrotSource.h"
#include "svtkImageMapToColors.h"
#include "svtkLookupTable.h"
#include "svtksys/SystemTools.hxx"

int TestFFMPEGWriter(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
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

  svtkFFMPEGWriter* w = svtkFFMPEGWriter::New();
  w->SetInputConnection(colorize->GetOutputPort());
  w->SetFileName("TestFFMPEGWriter.avi");
  cout << "Writing file TestFFMPEGWriter.avi..." << endl;
  w->SetBitRate(1024 * 1024 * 30);
  w->SetBitRateTolerance(1024 * 1024 * 3);
  w->Start();
  for (cc = 2; cc < 99; cc++)
  {
    cout << ".";
    Fractal0->SetMaximumNumberOfIterations(cc);
    table->SetTableRange(0, cc);
    table->SetNumberOfColors(cc);
    table->ForceBuild();
    table->SetTableValue(cc - 1, 0, 0, 0);
    w->Write();
  }
  w->End();
  cout << endl;
  cout << "Done writing file TestFFMPEGWriter.avi..." << endl;
  w->Delete();

  exists = (int)svtksys::SystemTools::FileExists("TestFFMPEGWriter.avi");
  length = svtksys::SystemTools::FileLength("TestFFMPEGWriter.avi");
  cout << "TestFFMPEGWriter.avi file exists: " << exists << endl;
  cout << "TestFFMPEGWriter.avi file length: " << length << endl;
  if (!exists)
  {
    err = 1;
    cerr << "ERROR: 1 - Test failing because TestFFMPEGWriter.avi file doesn't exist..." << endl;
  }
  else
  {
    svtksys::SystemTools::RemoveFile("TestFFMPEGWriter.avi");
  }
  if (0 == length)
  {
    err = 2;
    cerr << "ERROR: 2 - Test failing because TestFFMPEGWriter.avi file has zero length..." << endl;
  }

  colorize->Delete();
  table->Delete();
  cast->Delete();
  Fractal0->Delete();

  // err == 0 means test passes...
  //
  return err;
}
