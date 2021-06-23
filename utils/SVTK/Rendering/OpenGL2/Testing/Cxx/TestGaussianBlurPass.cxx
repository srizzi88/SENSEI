/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGaussianBlurPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the gaussian blur post-processing render pass.
// It renders an actor with a translucent LUT and depth
// peeling using the multi renderpass classes. The mapper uses color
// interpolation (poor quality).
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkOpenGLRenderer.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"

#include "svtkCamera.h"
#include "svtkConeSource.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkImageSinusoidSource.h"
#include "svtkLookupTable.h"
#include "svtkPolyDataMapper.h"

#include "svtkDepthPeelingPass.h"
#include "svtkGaussianBlurPass.h"
#include "svtkRenderStepsPass.h"

int TestGaussianBlurPass(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);

  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);

  // create the basic SVTK render steps
  svtkSmartPointer<svtkRenderStepsPass> basicPasses = svtkSmartPointer<svtkRenderStepsPass>::New();

  // replace the default translucent pass with
  // a more advanced depth peeling pass
  svtkSmartPointer<svtkDepthPeelingPass> peeling = svtkSmartPointer<svtkDepthPeelingPass>::New();
  peeling->SetMaximumNumberOfPeels(20);
  peeling->SetOcclusionRatio(0.001);
  peeling->SetTranslucentPass(basicPasses->GetTranslucentPass());
  basicPasses->SetTranslucentPass(peeling);

  // finally blur the resulting image
  // The blur delegates rendering the unblured image
  // to the basicPasses
  svtkSmartPointer<svtkGaussianBlurPass> blurP = svtkSmartPointer<svtkGaussianBlurPass>::New();
  blurP->SetDelegatePass(basicPasses);

  // tell the renderer to use our render pass pipeline
  glrenderer->SetPass(blurP);
  // glrenderer->SetPass(basicPasses);

  svtkSmartPointer<svtkImageSinusoidSource> imageSource =
    svtkSmartPointer<svtkImageSinusoidSource>::New();
  imageSource->SetWholeExtent(0, 9, 0, 9, 0, 9);
  imageSource->SetPeriod(5);
  imageSource->Update();

  svtkImageData* image = imageSource->GetOutput();
  double range[2];
  image->GetScalarRange(range);

  svtkSmartPointer<svtkDataSetSurfaceFilter> surface =
    svtkSmartPointer<svtkDataSetSurfaceFilter>::New();

  surface->SetInputConnection(imageSource->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(surface->GetOutputPort());

  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetTableRange(range);
  lut->SetAlphaRange(0.5, 0.5);
  lut->SetHueRange(0.2, 0.7);
  lut->SetNumberOfTableValues(256);
  lut->Build();

  mapper->SetScalarVisibility(1);
  mapper->SetLookupTable(lut);

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  actor->SetVisibility(1);

  svtkSmartPointer<svtkConeSource> cone = svtkSmartPointer<svtkConeSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> coneMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());

  svtkSmartPointer<svtkActor> coneActor = svtkSmartPointer<svtkActor>::New();
  coneActor->SetMapper(coneMapper);
  coneActor->SetVisibility(1);

  renderer->AddActor(coneActor);

  renderer->SetBackground(0.1, 0.3, 0.0);
  renWin->SetSize(400, 400);

  int retVal;
  renderer->ResetCamera();
  svtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renderer->ResetCamera();
  renWin->Render();

  retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
