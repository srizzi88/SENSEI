/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageResliceMapperSlab.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the slab modes of svtkImageResliceMapper
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReader2.h"
#include "svtkImageResliceMapper.h"
#include "svtkImageSlice.h"
#include "svtkInteractorStyleImage.h"
#include "svtkLookupTable.h"
#include "svtkPlane.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestImageResliceMapperSlab(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkInteractorStyle* style = svtkInteractorStyleImage::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetSize(400, 400);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  svtkImageReader2* reader = svtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetDataOrigin(-100.8, -100.9, -69.0);
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;

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
    imageMapper->SetSlabThickness(20);
    imageMapper->SliceFacesCameraOn();

    svtkImageSlice* image = svtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    image->GetProperty()->SetInterpolationTypeToLinear();
    image->GetProperty()->SetColorWindow(2000);
    image->GetProperty()->SetColorLevel(1000);
    renderer->AddViewProp(image);

    if (i < 3)
    {
      if (i == 0)
      {
        imageMapper->SetSlabTypeToMin();
      }
      if (i == 1)
      {
        imageMapper->SetSlabTypeToMax();
      }
      if (i == 2)
      {
        imageMapper->SetSlabTypeToMean();
      }
      if (i != 2)
      {
        camera->Azimuth(90);
        camera->Roll(85);
        camera->Azimuth(40);
        camera->Elevation(30);
      }
    }
    else
    {
      imageMapper->ResampleToScreenPixelsOff();
      imageMapper->SetSlabTypeToSum();
      imageMapper->SetSlabThickness(100);
      image->GetProperty()->SetColorWindow(2000 * 100);
      image->GetProperty()->SetColorLevel(1000 * 100);
      camera->Azimuth(91);
      camera->Roll(90);
    }

    image->Delete();
    camera->ParallelProjectionOn();
    renderer->ResetCamera();
    camera->SetParallelScale(120.0);
  }

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  reader->Delete();

  return !retVal;
}
