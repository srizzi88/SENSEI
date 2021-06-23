/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRasterReprojectionFiltercxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test for the svtkRasterReprojectionFilter using GDAL

#include "svtkCellDataToPointData.h"
#include "svtkGDALRasterReader.h"
#include "svtkImageActor.h"
#include "svtkImageMapToColors.h"
#include "svtkImageMapper3D.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkRasterReprojectionFilter.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"

int TestRasterReprojectionFilter(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GIS/sa052483.tif");

  // Load input file
  svtkNew<svtkGDALRasterReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  // test that we read the NoData value correctly
  reader->Update();
  double nodata = reader->GetInvalidValue(0);
  double expectedNodata = -32768;
  if (nodata != expectedNodata)
  {
    std::cerr << "Error NoData value. Found: " << nodata << ". Expected: " << expectedNodata
              << std::endl;
    return 1;
  }

  // Apply reprojection filter
  svtkNew<svtkRasterReprojectionFilter> filter;
  filter->SetInputConnection(reader->GetOutputPort());
  filter->SetOutputProjection("EPSG:3857");

  svtkNew<svtkLookupTable> lut;
  lut->SetNumberOfTableValues(256);
  lut->SetRange(296, 334);
  lut->SetRampToLinear();
  lut->Build();

  svtkNew<svtkCellDataToPointData> c2p1;
  c2p1->SetInputConnection(reader->GetOutputPort());
  svtkNew<svtkImageMapToColors> c;
  c->SetLookupTable(lut);
  c->SetInputConnection(c2p1->GetOutputPort());
  svtkNew<svtkImageActor> inputSlice;
  inputSlice->GetMapper()->SetInputConnection(c->GetOutputPort());
  svtkNew<svtkRenderer> leftRen;
  leftRen->SetViewport(0, 0, 0.5, 1);
  leftRen->SetBackground(0.2, 0.2, 0.2);
  leftRen->AddActor(inputSlice);

  svtkNew<svtkCellDataToPointData> c2p2;
  c2p2->SetInputConnection(filter->GetOutputPort());
  svtkNew<svtkImageMapToColors> co;
  co->SetLookupTable(lut);
  co->SetInputConnection(c2p2->GetOutputPort());
  svtkNew<svtkImageActor> outputSlice;
  outputSlice->GetMapper()->SetInputConnection(co->GetOutputPort());
  svtkNew<svtkRenderer> rightRen;
  rightRen->SetViewport(0.5, 0, 1, 1);
  rightRen->AddActor(outputSlice);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->AddRenderer(leftRen);
  renWin->AddRenderer(rightRen);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  leftRen->ResetCamera();
  rightRen->ResetCamera();
  renWin->Render();
  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
