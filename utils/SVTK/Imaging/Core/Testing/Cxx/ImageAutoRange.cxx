/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageAutoRange.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Use svtkImageHistogramStatistics to auto compute the window/level
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkSmartPointer.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageHistogramStatistics.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPNGReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkTestUtilities.h"

int ImageAutoRange(int argc, char* argv[])
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

  svtkSmartPointer<svtkImageHistogramStatistics> statistics =
    svtkSmartPointer<svtkImageHistogramStatistics>::New();
  statistics->SetInputConnection(reader->GetOutputPort());
  statistics->GenerateHistogramImageOff();
  statistics->Update();

  // Get a viewing range based on the full data range
  double range[2];
  range[0] = statistics->GetMinimum();
  range[1] = statistics->GetMaximum();

  // Use the autorange feature to get a better image range
  double autorange[2];
  statistics->GetAutoRange(autorange);

  for (int i = 0; i < 2; i++)
  {
    svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->SetViewport(0.5 * (i & 1), 0.0, 0.5 + 0.5 * (i & 1), 1.0);
    renWin->AddRenderer(renderer);

    svtkSmartPointer<svtkImageSliceMapper> imageMapper = svtkSmartPointer<svtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());

    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);

    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(128);

    svtkSmartPointer<svtkImageSlice> image = svtkSmartPointer<svtkImageSlice>::New();
    image->SetMapper(imageMapper);
    renderer->AddViewProp(image);

    if ((i & 1) == 0)
    {
      image->GetProperty()->SetColorWindow(range[1] - range[0]);
      image->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
    }
    else
    {
      image->GetProperty()->SetColorWindow(autorange[1] - autorange[0]);
      image->GetProperty()->SetColorLevel(0.5 * (autorange[0] + autorange[1]));
    }
  }

  renWin->SetSize(512, 256);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
