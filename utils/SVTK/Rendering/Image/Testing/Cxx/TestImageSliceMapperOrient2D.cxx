/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageSliceMapperOrient2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests 2D images that are not in the XY plane.
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImagePermute.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPNGReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkActor2D.h"
#include "svtkImageMapper.h"

int TestImageSliceMapperOrient2D(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkInteractorStyle* style = svtkInteractorStyleImage::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  svtkPNGReader* reader = svtkPNGReader::New();
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);
  reader->SetDataSpacing(0.9, 0.9, 1.0);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/svtk.png");

  reader->SetFileName(fname);
  delete[] fname;

  for (int i = 0; i < 4; i++)
  {
    svtkRenderer* renderer = svtkRenderer::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    svtkImageSliceMapper* imageMapper = svtkImageSliceMapper::New();

    if (i == 0 || i == 1)
    {
      svtkImagePermute* permute = svtkImagePermute::New();
      permute->SetInputConnection(reader->GetOutputPort());
      permute->SetFilteredAxes((2 - i) % 3, (3 - i) % 3, (4 - i) % 3);
      imageMapper->SetInputConnection(permute->GetOutputPort());
      permute->Delete();
      imageMapper->SetOrientation(i);
    }
    else
    {
      imageMapper->SetInputConnection(reader->GetOutputPort());
    }

    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);

    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 1.0;
    camera->SetPosition(point);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(120.0);
    if (imageMapper->GetOrientation() == 1)
    {
      camera->SetViewUp(1.0, 0.0, 0.0);
    }
    else if (imageMapper->GetOrientation() == 0)
    {
      camera->SetViewUp(0.0, 0.0, 1.0);
    }

    svtkImageSlice* image = svtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    renderer->AddViewProp(image);

    if (i == 3)
    {
      image->GetProperty()->SetColorWindow(127.5);
    }

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

  reader->Delete();

  return !retVal;
}
