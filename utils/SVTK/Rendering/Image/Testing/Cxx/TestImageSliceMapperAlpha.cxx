/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageSliceMapperAlpha.cxx

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
#include "svtkImageMapToColors.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkLookupTable.h"
#include "svtkPNGReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestImageSliceMapperAlpha(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkInteractorStyle* style = svtkInteractorStyleImage::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  svtkPNGReader* reader = svtkPNGReader::New();
  svtkPNGReader* reader2 = svtkPNGReader::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fullhead15.png");
  char* fname2 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/alphachannel.png");
  reader->SetFileName(fname);
  reader2->SetFileName(fname2);
  delete[] fname;
  delete[] fname2;

  svtkLookupTable* table = svtkLookupTable::New();
  table->SetRampToLinear();
  table->SetRange(0.0, 255.0);
  table->SetValueRange(0.0, 1.0);
  table->SetSaturationRange(0.0, 0.0);
  table->SetVectorModeToRGBColors();
  table->Build();

  svtkLookupTable* table2 = svtkLookupTable::New();
  table2->SetRampToLinear();
  table2->SetRange(0, 255);
  table2->SetHueRange(0.3, 0.3);
  table2->SetValueRange(0.0, 1.0);
  table2->SetSaturationRange(1.0, 1.0);
  table2->SetAlphaRange(0.0, 1.0);
  table2->SetVectorModeToComponent();
  table2->SetVectorComponent(3);
  table2->Build();

  svtkImageMapToColors* colors = svtkImageMapToColors::New();
  colors->SetInputConnection(reader2->GetOutputPort());
  colors->SetLookupTable(table);
  colors->PassAlphaToOutputOn();
  colors->SetOutputFormatToLuminanceAlpha();

  svtkImageMapToColors* colors2 = svtkImageMapToColors::New();
  colors2->SetInputConnection(reader2->GetOutputPort());
  colors2->SetLookupTable(table);
  colors2->SetOutputFormatToRGB();

  for (int i = 0; i < 4; i++)
  {
    svtkRenderer* renderer = svtkRenderer::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    svtkImageSliceMapper* imageMapper = svtkImageSliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());
    svtkImageSlice* image = svtkImageSlice::New();
    image->SetMapper(imageMapper);
    image->GetProperty()->SetColorWindow(2000.0);
    image->GetProperty()->SetColorLevel(1000.0);
    imageMapper->Delete();

    svtkImageSliceMapper* imageMapper2 = svtkImageSliceMapper::New();
    svtkImageSlice* image2 = svtkImageSlice::New();
    image2->SetMapper(imageMapper2);
    imageMapper2->Delete();

    if (i == 0)
    {
      imageMapper2->SetInputConnection(reader2->GetOutputPort());
    }
    else if (i == 1)
    {
      imageMapper2->SetInputConnection(colors->GetOutputPort());
    }
    else if (i == 2)
    {
      imageMapper2->SetInputConnection(colors2->GetOutputPort());
      image2->GetProperty()->SetOpacity(0.5);
    }
    else
    {
      imageMapper2->SetInputConnection(reader2->GetOutputPort());
      image2->GetProperty()->SetLookupTable(table2);
      image2->GetProperty()->SetOpacity(0.9);
    }

    renderer->AddViewProp(image);
    renderer->AddViewProp(image2);
    camera->ParallelProjectionOn();
    renderer->ResetCamera();
    camera->SetParallelScale(200.0);

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
  reader2->Delete();

  return !retVal;
}
