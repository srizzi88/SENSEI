/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageResize.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the svtkImageResize class
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkSmartPointer.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageResize.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPNGReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkTestUtilities.h"

int ImageResize(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkInteractorStyle> style = svtkSmartPointer<svtkInteractorStyle>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  svtkSmartPointer<svtkPNGReader> reader = svtkSmartPointer<svtkPNGReader>::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  double range[2] = { 0, 4095 };

  for (int i = 0; i < 4; i++)
  {
    svtkSmartPointer<svtkImageResize> resize = svtkSmartPointer<svtkImageResize>::New();
    resize->SetInputConnection(reader->GetOutputPort());
    resize->SetOutputDimensions(64, 64, 1);

    svtkSmartPointer<svtkImageSliceMapper> imageMapper = svtkSmartPointer<svtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(resize->GetOutputPort());
    imageMapper->BorderOn();

    if ((i & 1) == 0)
    {
      resize->BorderOff();
    }
    else
    {
      resize->BorderOn();
    }

    if ((i & 2) == 0)
    {
      resize->InterpolateOff();
    }
    else
    {
      resize->InterpolateOn();
    }

    svtkSmartPointer<svtkImageSlice> image = svtkSmartPointer<svtkImageSlice>::New();
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
    image->GetProperty()->SetInterpolationTypeToNearest();

    svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
    renderer->AddViewProp(image);
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);

    // use center point to set camera
    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);

    svtkCamera* camera = renderer->GetActiveCamera();
    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(128);
  }

  renWin->SetSize(512, 512);

  iren->Initialize();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
