/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageResliceMapperBackground.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the Border variable on ImageResliceMapper
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkImageClip.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageResliceMapper.h"
#include "svtkImageSlice.h"
#include "svtkInteractorStyleImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTIFFReader.h"

int TestImageResliceMapperBackground(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkInteractorStyle* style = svtkInteractorStyleImage::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  svtkTIFFReader* reader = svtkTIFFReader::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");

  reader->SetFileName(fname);
  delete[] fname;

  svtkImageClip* clip = svtkImageClip::New();
  clip->SetInputConnection(reader->GetOutputPort());
  clip->SetOutputWholeExtent(100, 107, 100, 107, 0, 0);

  for (int i = 0; i < 4; i++)
  {
    svtkRenderer* renderer = svtkRenderer::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    svtkImageResliceMapper* imageMapper = svtkImageResliceMapper::New();
    imageMapper->SetInputConnection(clip->GetOutputPort());

    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);

    camera->SetFocalPoint(point);
    point[2] += 500.0;
    camera->SetPosition(point);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(5.0);

    svtkImageSlice* image = svtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    renderer->AddViewProp(image);
    image->GetMapper()->BackgroundOn();
    image->GetMapper()->SliceFacesCameraOn();

    if ((i & 1))
    {
      imageMapper->ResampleToScreenPixelsOff();
    }
    if ((i & 2))
    {
      imageMapper->SeparateWindowLevelOperationOn();
    }

    image->GetProperty()->SetColorWindow(255.0);
    image->GetProperty()->SetColorLevel(127.5);

    image->Delete();
  }

  renWin->SetSize(400, 400);

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  clip->Delete();
  reader->Delete();

  return !retVal;
}
