/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTexture32Bits.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTexture.h"

int TestTexture32Bits(int argc, char* argv[])
{
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkPlaneSource> plane;

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->AddRenderer(renderer);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  svtkNew<svtkImageData> image;
  image->SetExtent(0, 256, 0, 256, 0, 0);

  svtkNew<svtkFloatArray> pixels;
  pixels->SetNumberOfComponents(3);
  pixels->SetNumberOfTuples(65536);
  float* p = pixels->GetPointer(0);

  for (int i = 0; i < 65536; i++)
  {
    float v = i / 65536.f;
    *p++ = v;
    *p++ = 1.f - v;
    *p++ = 0.5f + v;
  }

  image->GetPointData()->SetScalars(pixels);

  svtkNew<svtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->SetInputData(image);

  actor->SetTexture(texture);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
  renderer->ResetCameraClippingRange();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
