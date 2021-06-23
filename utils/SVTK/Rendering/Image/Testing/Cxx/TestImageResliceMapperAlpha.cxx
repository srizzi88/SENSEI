/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageResliceMapperAlpha.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test alpha blending RGBA, LA, Opacity<1.0, lookup table
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageGridSource.h"
#include "svtkImageMapToColors.h"
#include "svtkImageProperty.h"
#include "svtkImageReader2.h"
#include "svtkImageResliceMapper.h"
#include "svtkImageSlice.h"
#include "svtkInteractorStyleImage.h"
#include "svtkLookupTable.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestImageResliceMapperAlpha(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkInteractorStyle* style = svtkInteractorStyleImage::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  svtkImageReader2* reader = svtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;

  svtkImageGridSource* grid = svtkImageGridSource::New();
  grid->SetDataExtent(0, 60, 0, 60, 1, 93);
  grid->SetDataSpacing(3.2, 3.2, 1.5);
  grid->SetDataOrigin(0, 0, 0);
  grid->SetDataScalarTypeToUnsignedChar();
  grid->SetLineValue(255);

  svtkLookupTable* table = svtkLookupTable::New();
  table->SetRampToLinear();
  table->SetRange(0.0, 255.0);
  table->SetValueRange(1.0, 1.0);
  table->SetSaturationRange(0.0, 0.0);
  table->SetAlphaRange(0.0, 1.0);
  table->Build();

  svtkLookupTable* table2 = svtkLookupTable::New();
  table2->SetRampToLinear();
  table2->SetRange(0.0, 255.0);
  table2->SetValueRange(1.0, 1.0);
  table2->SetHueRange(0.2, 0.4);
  table2->SetSaturationRange(1.0, 1.0);
  table2->SetAlphaRange(0.5, 1.0);
  table2->Build();

  svtkImageMapToColors* colors = svtkImageMapToColors::New();
  colors->SetInputConnection(grid->GetOutputPort());
  colors->SetLookupTable(table);
  colors->PassAlphaToOutputOn();
  colors->SetOutputFormatToLuminanceAlpha();

  svtkImageMapToColors* colors2 = svtkImageMapToColors::New();
  colors2->SetInputConnection(grid->GetOutputPort());
  colors2->SetLookupTable(table2);
  colors2->SetOutputFormatToRGB();

  for (int i = 0; i < 4; i++)
  {
    svtkRenderer* renderer = svtkRenderer::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    svtkImageResliceMapper* imageMapper = svtkImageResliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());
    imageMapper->SliceFacesCameraOn();
    imageMapper->SliceAtFocalPointOn();
    svtkImageSlice* image = svtkImageSlice::New();
    image->SetMapper(imageMapper);
    image->GetProperty()->SetColorWindow(2000.0);
    image->GetProperty()->SetColorLevel(1000.0);
    imageMapper->Delete();

    svtkImageResliceMapper* imageMapper2 = svtkImageResliceMapper::New();
    imageMapper2->SliceFacesCameraOn();
    imageMapper2->SliceAtFocalPointOn();
    svtkImageSlice* image2 = svtkImageSlice::New();
    image2->SetMapper(imageMapper2);
    imageMapper2->Delete();

    if (i == 0)
    {
      imageMapper2->SetInputConnection(grid->GetOutputPort());
      image2->GetProperty()->SetOpacity(0.5);
    }
    else if (i == 1)
    {
      imageMapper2->SetInputConnection(colors->GetOutputPort());
      camera->Elevation(30);
    }
    else if (i == 2)
    {
      imageMapper2->SetInputConnection(colors2->GetOutputPort());
      image2->GetProperty()->SetOpacity(0.5);
    }
    else
    {
      imageMapper2->SetInputConnection(grid->GetOutputPort());
      image2->GetProperty()->SetLookupTable(table2);
      image2->GetProperty()->SetOpacity(0.9);
      image->RotateWXYZ(30, 1, 0.5, 0);
    }

    renderer->AddViewProp(image);
    renderer->AddViewProp(image2);
    camera->ParallelProjectionOn();
    renderer->ResetCamera();
    camera->SetParallelScale(110.0);

    image->Delete();
    image2->Delete();
  }

  colors->Delete();
  colors2->Delete();
  table->Delete();
  table2->Delete();

  renWin->SetSize(400, 400);

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  reader->Delete();
  grid->Delete();

  return !retVal;
}
