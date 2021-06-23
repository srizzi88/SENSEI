/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNIFTIReaderAnalyze.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
Test compatibility of the svtkNIFTIImageReader with Analyze 7.5 files.
*/

#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageMathematics.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkNIFTIImageHeader.h"
#include "svtkNIFTIImageReader.h"
#include "svtkNIFTIImageWriter.h"

#include <string>

static const char* dispfile = "Data/ANALYZE.HDR";

static void TestDisplay(svtkRenderWindow* renwin, const char* infile)
{
  svtkNew<svtkNIFTIImageReader> reader;
  if (!reader->CanReadFile(infile))
  {
    cerr << "CanReadFile failed for " << infile << "\n";
    exit(1);
  }
  reader->SetFileName(infile);
  reader->Update();

  int size[3];
  double center[3], spacing[3];
  reader->GetOutput()->GetDimensions(size);
  reader->GetOutput()->GetCenter(center);
  reader->GetOutput()->GetSpacing(spacing);
  double center1[3] = { center[0], center[1], center[2] };
  double center2[3] = { center[0], center[1], center[2] };
  if (size[2] % 2 == 1)
  {
    center1[2] += 0.5 * spacing[2];
  }
  if (size[0] % 2 == 1)
  {
    center2[0] += 0.5 * spacing[0];
  }
  double vrange[2];
  reader->GetOutput()->GetScalarRange(vrange);

  svtkNew<svtkImageSliceMapper> map1;
  map1->BorderOn();
  map1->SliceAtFocalPointOn();
  map1->SliceFacesCameraOn();
  map1->SetInputConnection(reader->GetOutputPort());
  svtkNew<svtkImageSliceMapper> map2;
  map2->BorderOn();
  map2->SliceAtFocalPointOn();
  map2->SliceFacesCameraOn();
  map2->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkImageSlice> slice1;
  slice1->SetMapper(map1);
  slice1->GetProperty()->SetColorWindow(vrange[1] - vrange[0]);
  slice1->GetProperty()->SetColorLevel(0.5 * (vrange[0] + vrange[1]));

  svtkNew<svtkImageSlice> slice2;
  slice2->SetMapper(map2);
  slice2->GetProperty()->SetColorWindow(vrange[1] - vrange[0]);
  slice2->GetProperty()->SetColorLevel(0.5 * (vrange[0] + vrange[1]));

  double ratio = size[0] * 1.0 / (size[0] + size[2]);

  svtkNew<svtkRenderer> ren1;
  ren1->SetViewport(0, 0, ratio, 1.0);

  svtkNew<svtkRenderer> ren2;
  ren2->SetViewport(ratio, 0.0, 1.0, 1.0);
  ren1->AddViewProp(slice1);
  ren2->AddViewProp(slice2);

  svtkCamera* cam1 = ren1->GetActiveCamera();
  cam1->ParallelProjectionOn();
  cam1->SetParallelScale(0.5 * spacing[1] * size[1]);
  cam1->SetFocalPoint(center1[0], center1[1], center1[2]);
  cam1->SetPosition(center1[0], center1[1], center1[2] - 100.0);

  svtkCamera* cam2 = ren2->GetActiveCamera();
  cam2->ParallelProjectionOn();
  cam2->SetParallelScale(0.5 * spacing[1] * size[1]);
  cam2->SetFocalPoint(center2[0], center2[1], center2[2]);
  cam2->SetPosition(center2[0] + 100.0, center2[1], center2[2]);

  renwin->SetSize(size[0] + size[2], size[1]);
  renwin->AddRenderer(ren1);
  renwin->AddRenderer(ren2);
};

int TestNIFTIReaderAnalyze(int argc, char* argv[])
{
  // perform the display test
  char* infile = svtkTestUtilities::ExpandDataFileName(argc, argv, dispfile);
  if (!infile)
  {
    cerr << "Could not locate input file " << dispfile << "\n";
    return 1;
  }
  std::string inpath = infile;
  delete[] infile;

  svtkNew<svtkRenderWindow> renwin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin);

  TestDisplay(renwin, inpath.c_str());

  int retVal = svtkRegressionTestImage(renwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renwin->Render();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return (retVal != svtkRegressionTester::PASSED);
}
