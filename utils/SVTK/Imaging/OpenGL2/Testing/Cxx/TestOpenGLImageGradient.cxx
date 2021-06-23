/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkNew.h"

#include "svtkCamera.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReader2.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkOpenGLImageGradient.h"
#include "svtkPointData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

int TestOpenGLImageGradient(int argc, char* argv[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkInteractorStyleImage> style;
  style->SetInteractionModeToImageSlicing();
  svtkNew<svtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  svtkNew<svtkImageReader2> reader;
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);

  delete[] fname;

  svtkNew<svtkOpenGLImageGradient> filter;
  //  svtkNew<svtkImageGradient> filter;
  filter->SetInputConnection(reader->GetOutputPort());
  filter->Update();
  // double *rnger = filter->GetOutput()->GetPointData()->GetScalars()->GetRange();

  svtkNew<svtkImageSliceMapper> imageMapper;
  imageMapper->SetInputConnection(filter->GetOutputPort());
  imageMapper->SetOrientation(2);
  imageMapper->SliceAtFocalPointOn();

  svtkNew<svtkImageSlice> image;
  image->SetMapper(imageMapper);

  double range[2] = { -100, 100 };

  image->GetProperty()->SetColorWindow(range[1] - range[0]);
  image->GetProperty()->SetColorLevel(0.5 * (range[0] + range[1]));
  image->GetProperty()->SetInterpolationTypeToNearest();

  svtkNew<svtkRenderer> renderer;
  renderer->AddViewProp(image);
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);

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

  renWin->SetSize(512, 512);
  iren->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
