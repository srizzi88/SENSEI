/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSVTKMClipWithImplicitFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmClip.h"

#include "svtkActor.h"
#include "svtkDataArray.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphere.h"

int TestSVTKMClipWithImplicitFunction(int argc, char* argv[])
{
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-8, 8, -8, 8, -8, 8);
  wavelet->SetCenter(0, 0, 0);

  svtkNew<svtkSphere> sphere;
  sphere->SetCenter(0, 0, 0);
  sphere->SetRadius(10);
  svtkNew<svtkmClip> clip;
  clip->SetInputConnection(wavelet->GetOutputPort());
  clip->SetClipFunction(sphere);

  svtkNew<svtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(clip->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetScalarRange(37, 150);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
