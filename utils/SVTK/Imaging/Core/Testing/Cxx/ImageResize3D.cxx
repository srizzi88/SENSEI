/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageResize3D.cxx

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
#include "svtkImageReader2.h"
#include "svtkImageResize.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkTestUtilities.h"

int ImageResize3D(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkInteractorStyleImage> style = svtkSmartPointer<svtkInteractorStyleImage>::New();
  style->SetInteractionModeToImageSlicing();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  svtkSmartPointer<svtkImageReader2> reader = svtkSmartPointer<svtkImageReader2>::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);

  delete[] fname;

  svtkSmartPointer<svtkImageResize> resize = svtkSmartPointer<svtkImageResize>::New();
  resize->SetInputConnection(reader->GetOutputPort());
  resize->SetResizeMethodToOutputSpacing();
  resize->SetOutputSpacing(0.80, 0.80, 1.5);
  resize->InterpolateOn();
  resize->Update();

  svtkSmartPointer<svtkImageResize> resize2 = svtkSmartPointer<svtkImageResize>::New();
  resize2->SetInputConnection(reader->GetOutputPort());
  resize2->SetResizeMethodToMagnificationFactors();
  resize2->SetMagnificationFactors(4, 4, 1);
  resize2->InterpolateOff();

  double range[2] = { 0, 4095 };

  for (int i = 0; i < 4; i++)
  {
    svtkSmartPointer<svtkImageSliceMapper> imageMapper = svtkSmartPointer<svtkImageSliceMapper>::New();
    if (i < 3)
    {
      imageMapper->SetInputConnection(resize->GetOutputPort());
    }
    else
    {
      imageMapper->SetInputConnection(resize2->GetOutputPort());
    }
    imageMapper->SetOrientation(i % 3);
    imageMapper->SliceAtFocalPointOn();

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
    if (imageMapper->GetOrientation() == 2)
    {
      camera->SetViewUp(0.0, 1.0, 0.0);
    }
    else
    {
      camera->SetViewUp(0.0, 0.0, -1.0);
    }
    camera->ParallelProjectionOn();
    camera->SetParallelScale(0.8 * 128);
  }

  renWin->SetSize(512, 512);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
