/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageSliceMapperOrient3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests 3D images that are not in the XY plane.
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReader2.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkMatrix3x3.h"
#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTransform.h"

int TestImageSliceMapperOriented3D(int argc, char* argv[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkInteractorStyleImage> style;
  svtkNew<svtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  svtkNew<svtkImageReader2> reader;
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);

  // compute a direction matrix for testing
  double mat4[16];
  svtkNew<svtkTransform> trans;
  trans->RotateY(20);
  trans->RotateX(20);
  svtkMatrix4x4::DeepCopy(mat4, trans->GetMatrix()->GetData());

  double dir[9] = { mat4[0], mat4[1], mat4[2], mat4[4], mat4[5], mat4[6], mat4[8], mat4[9],
    mat4[10] };
  reader->SetDataDirection(dir);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  reader->SetFilePrefix(fname);
  reader->Update();
  delete[] fname;

  for (int i = 0; i < 4; i++)
  {
    svtkNew<svtkRenderer> renderer;
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);

    svtkNew<svtkImageSliceMapper> imageMapper;
    imageMapper->SetInputConnection(reader->GetOutputPort());
    imageMapper->SliceAtFocalPointOn();

    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);

    if (i < 3)
    {
      imageMapper->SetOrientation(i);
    }

    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 500.0;
    camera->SetPosition(point);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(120.0);
    if (imageMapper->GetOrientation() != 2)
    {
      camera->SetViewUp(0.0, 0.0, -1.0);
    }

    if (i == 3)
    {
      camera->Azimuth(20);
      camera->Elevation(-20);
    }

    svtkNew<svtkImageSlice> image;
    image->SetMapper(imageMapper);
    renderer->AddViewProp(image);

    image->GetProperty()->SetColorWindow(2000);
    image->GetProperty()->SetColorLevel(1000);
  }

  renWin->SetSize(400, 400);

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
