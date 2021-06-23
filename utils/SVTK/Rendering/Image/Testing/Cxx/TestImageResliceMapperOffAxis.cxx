/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageResliceMapperOffAxis.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests off-axis views of 3D images.
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataSetMapper.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReader2.h"
#include "svtkImageResliceMapper.h"
#include "svtkImageSlice.h"
#include "svtkInteractorStyleImage.h"
#include "svtkOutlineFilter.h"
#include "svtkPlane.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestImageResliceMapperOffAxis(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  renWin->SetMultiSamples(0);
  renWin->Delete();

  svtkImageReader2* reader = svtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  reader->SetFilePrefix(fname);
  reader->Update();
  delete[] fname;

  for (int i = 0; i < 4; i++)
  {
    svtkRenderer* renderer = svtkRenderer::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    svtkImageProperty* property = svtkImageProperty::New();
    property->SetColorWindow(2000);
    property->SetColorLevel(1000);
    property->SetAmbient(0.0);
    property->SetDiffuse(1.0);
    property->SetInterpolationTypeToLinear();

    for (int j = 0; j < 3; j++)
    {
      double normal[3];
      normal[0] = 0;
      normal[1] = 0;
      normal[2] = 0;
      normal[j] = 1;

      svtkImageResliceMapper* imageMapper = svtkImageResliceMapper::New();
      imageMapper->SetInputConnection(reader->GetOutputPort());
      imageMapper->GetSlicePlane()->SetNormal(normal);
      imageMapper->SliceAtFocalPointOn();
      imageMapper->BorderOn();
      imageMapper->SetResampleToScreenPixels((i >= 2));

      svtkImageSlice* image = svtkImageSlice::New();
      image->SetProperty(property);
      image->SetMapper(imageMapper);
      imageMapper->Delete();

      svtkOutlineFilter* outline = svtkOutlineFilter::New();
      outline->SetInputConnection(reader->GetOutputPort());

      svtkDataSetMapper* mapper = svtkDataSetMapper::New();
      mapper->SetInputConnection(outline->GetOutputPort());
      outline->Delete();

      svtkActor* actor = svtkActor::New();
      actor->SetMapper(mapper);
      mapper->Delete();

      if (i % 2)
      {
        image->RotateX(10);
        image->RotateY(5);
        actor->RotateX(10);
        actor->RotateY(5);
      }

      renderer->AddViewProp(image);
      renderer->AddViewProp(actor);
      image->Delete();
      actor->Delete();
    }

    property->Delete();

    if (i < 2)
    {
      camera->ParallelProjectionOn();
    }

    camera->Azimuth(10);
    camera->Elevation(-120);
    renderer->ResetCamera();
    camera->Dolly(1.2);
    camera->SetParallelScale(125);
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
