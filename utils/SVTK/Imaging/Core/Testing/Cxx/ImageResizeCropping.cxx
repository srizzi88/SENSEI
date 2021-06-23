/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageResizeCropping.cxx

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

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataSetMapper.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageResize.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkOutlineSource.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTIFFReader.h"

#include "svtkTestUtilities.h"

int ImageResizeCropping(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkInteractorStyle> style = svtkSmartPointer<svtkInteractorStyle>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  svtkSmartPointer<svtkTIFFReader> reader = svtkSmartPointer<svtkTIFFReader>::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");

  reader->SetFileName(fname);
  reader->SetOrientationType(4);
  delete[] fname;

  double range[2] = { 0, 255 };
  double cropping[4][6] = {
    { 0, 199, 0, 199, 0, 0 },
    { 10, 149, 50, 199, 0, 0 },
    { -0.5, 199.5, -0.5, 199.5, 0, 0 },
    { 9.5, 149.5, 199.5, 49.5, 0, 0 },
  };

  svtkSmartPointer<svtkOutlineSource> outline = svtkSmartPointer<svtkOutlineSource>::New();
  outline->SetBounds(10, 149, 50, 199, -1, 1);

  svtkSmartPointer<svtkDataSetMapper> mapper = svtkSmartPointer<svtkDataSetMapper>::New();
  mapper->SetInputConnection(outline->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  for (int i = 0; i < 4; i++)
  {
    svtkSmartPointer<svtkImageResize> resize = svtkSmartPointer<svtkImageResize>::New();
    resize->SetNumberOfThreads(1);
    resize->SetInputConnection(reader->GetOutputPort());
    resize->SetOutputDimensions(256, 256, 1);
    if ((i & 1) == 1)
    {
      resize->CroppingOn();
      resize->SetCroppingRegion(cropping[i]);
    }

    svtkSmartPointer<svtkImageSliceMapper> imageMapper = svtkSmartPointer<svtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(resize->GetOutputPort());

    if ((i & 2) == 2)
    {
      resize->BorderOn();
      imageMapper->BorderOn();
    }

    svtkSmartPointer<svtkImageSlice> image = svtkSmartPointer<svtkImageSlice>::New();
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));

    svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
    renderer->AddViewProp(image);
    if (i == 0)
    {
      renderer->AddViewProp(actor);
    }
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);

    double point[3] = { 99.5, 99.5, 0.0 };

    svtkCamera* camera = renderer->GetActiveCamera();
    camera->SetFocalPoint(point);
    point[2] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(100);
  }

  renWin->SetSize(512, 512);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
