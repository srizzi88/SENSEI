/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageInterpolateSlidingWindow2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the "SlidingWindow" option of the image interpolators
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkSmartPointer.h"

#include "svtkCamera.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReslice.h"
#include "svtkImageSincInterpolator.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPNGReader.h"
#include "svtkPointData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkTestUtilities.h"

int ImageInterpolateSlidingWindow2D(int argc, char* argv[])
{
  auto iren = svtkSmartPointer<svtkRenderWindowInteractor>::New();
  auto style = svtkSmartPointer<svtkInteractorStyle>::New();
  auto renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  auto reader = svtkSmartPointer<svtkPNGReader>::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  double range[2] = { 0, 4095 };

  for (int i = 0; i < 4; i++)
  {
    // compare results for SlidingWindowOn and SlidingWindowOff
    auto interpolator = svtkSmartPointer<svtkImageSincInterpolator>::New();
    interpolator->SlidingWindowOn();

    auto interpolatorOff = svtkSmartPointer<svtkImageSincInterpolator>::New();
    interpolatorOff->SlidingWindowOff();

    auto reslice = svtkSmartPointer<svtkImageReslice>::New();
    reslice->SetInputConnection(reader->GetOutputPort());
    reslice->SetInterpolator(interpolator);
    reslice->SetOutputScalarType(SVTK_DOUBLE);

    auto resliceOff = svtkSmartPointer<svtkImageReslice>::New();
    resliceOff->SetInputConnection(reader->GetOutputPort());
    resliceOff->SetInterpolator(interpolatorOff);
    resliceOff->SetOutputScalarType(SVTK_DOUBLE);

    auto imageMapper = svtkSmartPointer<svtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(reslice->GetOutputPort());
    imageMapper->BorderOn();

    // perform stretching and shrinking in x and y directions
    switch (i)
    {
      case 0:
        reslice->SetOutputSpacing(0.7, 0.8, 1.0);
        resliceOff->SetOutputSpacing(0.7, 0.8, 1.0);
        break;
      case 1:
        reslice->SetOutputSpacing(1.0, 0.8, 1.0);
        resliceOff->SetOutputSpacing(1.0, 0.8, 1.0);
        break;
      case 2:
        reslice->SetOutputSpacing(1.7, 1.8, 1.0);
        resliceOff->SetOutputSpacing(1.7, 1.8, 1.0);
        break;
      case 3:
        reslice->SetOutputSpacing(0.7, 1.0, 1.0);
        resliceOff->SetOutputSpacing(0.7, 1.0, 1.0);
        break;
    }

    reslice->Update();
    resliceOff->Update();

    // does "On" give the same results as "Off"?
    svtkDoubleArray* scalars =
      static_cast<svtkDoubleArray*>(reslice->GetOutput()->GetPointData()->GetScalars());
    svtkDoubleArray* scalarsOff =
      static_cast<svtkDoubleArray*>(resliceOff->GetOutput()->GetPointData()->GetScalars());
    double maxdiff = 0.0;
    for (svtkIdType j = 0; j < scalars->GetNumberOfValues(); j++)
    {
      double diff = scalars->GetValue(j) - scalarsOff->GetValue(j);
      maxdiff = (fabs(diff) > fabs(maxdiff) ? diff : maxdiff);
    }
    std::cerr << "Maximum Pixel Error: " << maxdiff << "\n";
    const double tol = 1e-10;
    if (fabs(maxdiff) > tol)
    {
      std::cerr << "Difference is larger than tolerance " << tol << "\n";
      return EXIT_FAILURE;
    }

    auto image = svtkSmartPointer<svtkImageSlice>::New();
    image->SetMapper(imageMapper);

    image->GetProperty()->SetColorWindow(range[1] - range[0]);
    image->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
    image->GetProperty()->SetInterpolationTypeToNearest();

    auto renderer = svtkSmartPointer<svtkRenderer>::New();
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
