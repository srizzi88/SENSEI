/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageReslice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the svtkImageReslice class
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkNew.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReslice.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPNGReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"

#include "svtkTestUtilities.h"

int ImageReslice(int argc, char* argv[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkInteractorStyle> style;
  svtkNew<svtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  svtkNew<svtkPNGReader> reader;

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  double range[2] = { 0.0, 4095.0 };

  svtkNew<svtkTransform> transform;
  transform->RotateZ(25.0);
  transform->Scale(0.9, 0.9, 1.0);

  for (int i = 0; i < 4; i++)
  {
    svtkNew<svtkImageReslice> reslice;
    reslice->SetInputConnection(reader->GetOutputPort());
    reslice->SetOutputSpacing(1.0, 1.0, 1.0);

    if ((i & 1) == 0)
    {
      // Images on the left
      reslice->TransformInputSamplingOff();
    }
    else
    {
      // Images on the right
      reslice->TransformInputSamplingOn();
    }

    if ((i & 2) == 0)
    {
      // Images on the bottom
      reslice->SetResliceAxes(transform->GetMatrix());
    }
    else
    {
      // Images on the top, note that (by design) the ResliceTransform
      // is ignored by TransformInputSampling, unlike the ResliceAxes
      reslice->SetResliceTransform(transform);
    }

    svtkNew<svtkImageSliceMapper> imageMapper;
    imageMapper->SetInputConnection(reslice->GetOutputPort());
    imageMapper->BorderOn();

    svtkNew<svtkImageSlice> image;
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
    image->GetProperty()->SetInterpolationTypeToNearest();

    svtkNew<svtkRenderer> renderer;
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
