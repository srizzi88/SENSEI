/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageThresholdConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the svtkImageThresholdConnectivity class
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReader2.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkImageThresholdConnectivity.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPoints.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include "svtkTestUtilities.h"

int TestImageThresholdConnectivity(int argc, char* argv[])
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
  reader->SetDataExtent(0, 63, 0, 63, 2, 4);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);

  delete[] fname;

  svtkSmartPointer<svtkPoints> seeds = svtkSmartPointer<svtkPoints>::New();
  seeds->InsertNextPoint(1, 1, 5.25);
  seeds->InsertNextPoint(100.8, 100.8, 5.25);

  for (int i = 0; i < 12; i++)
  {
    int j = i % 4;
    int k = i / 4;
    svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->SetViewport(k / 3.0, j / 4.0, (k + 1) / 3.0, (j + 1) / 4.0);
    renWin->AddRenderer(renderer);

    svtkSmartPointer<svtkImageThresholdConnectivity> connectivity =
      svtkSmartPointer<svtkImageThresholdConnectivity>::New();
    connectivity->SetInputConnection(reader->GetOutputPort());
    connectivity->SetSeedPoints(seeds);
    connectivity->SetInValue(2000);
    connectivity->SetOutValue(0);
    connectivity->SetReplaceIn((j & 2) == 0);
    connectivity->SetReplaceOut((j & 1) == 0);
    if (k == 0)
    {
      connectivity->ThresholdByLower(800);
    }
    else if (k == 1)
    {
      connectivity->ThresholdByUpper(1200);
    }
    else
    {
      connectivity->ThresholdBetween(800, 1200);
    }

    // test a previous bug where OutputExtent != InputExtent cause a crash.
    int extent[6] = { 0, 63, 0, 63, 3, 3 };
    connectivity->UpdateExtent(extent);

    svtkSmartPointer<svtkImageSliceMapper> imageMapper = svtkSmartPointer<svtkImageSliceMapper>::New();
    imageMapper->SetInputConnection(connectivity->GetOutputPort());
    imageMapper->BorderOn();
    imageMapper->SliceFacesCameraOn();
    imageMapper->SliceAtFocalPointOn();

    double point[3] = { 100.8, 100.8, 5.25 };
    camera->SetFocalPoint(point);
    point[2] += 500.0;
    camera->SetPosition(point);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(3.2 * 32);

    svtkSmartPointer<svtkImageSlice> image = svtkSmartPointer<svtkImageSlice>::New();
    image->SetMapper(imageMapper);
    image->GetProperty()->SetColorWindow(2000);
    image->GetProperty()->SetColorLevel(1000);
    renderer->AddViewProp(image);
  }

  renWin->SetSize(192, 256);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
