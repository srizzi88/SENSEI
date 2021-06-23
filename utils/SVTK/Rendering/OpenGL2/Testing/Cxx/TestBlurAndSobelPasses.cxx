/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBlurAndSobelPasses.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the combination of two post-processing render passes:
// Gaussian blur first, followed by a Sobel detection. It renders of an
// opaque cone.
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

#include "svtkCamera.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkImageSinusoidSource.h"
#include "svtkLookupTable.h"
#include "svtkPolyDataMapper.h"

#include "svtkCameraPass.h"
#include "svtkLightsPass.h"
#include "svtkOpaquePass.h"
#include "svtkSequencePass.h"
//#include "svtkDepthPeelingPass.h"
#include "svtkConeSource.h"
#include "svtkGaussianBlurPass.h"
#include "svtkOverlayPass.h"
#include "svtkRenderPassCollection.h"
#include "svtkSobelGradientMagnitudePass.h"
#include "svtkTranslucentPass.h"
#include "svtkVolumetricPass.h"

int TestBlurAndSobelPasses(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  svtkRenderer* renderer = svtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);

  svtkCameraPass* cameraP = svtkCameraPass::New();

  svtkSequencePass* seq = svtkSequencePass::New();
  svtkOpaquePass* opaque = svtkOpaquePass::New();
  //  svtkDepthPeelingPass *peeling=svtkDepthPeelingPass::New();
  //  peeling->SetMaximumNumberOfPeels(200);
  //  peeling->SetOcclusionRatio(0.1);

  svtkTranslucentPass* translucent = svtkTranslucentPass::New();
  //  peeling->SetTranslucentPass(translucent);

  svtkVolumetricPass* volume = svtkVolumetricPass::New();
  svtkOverlayPass* overlay = svtkOverlayPass::New();

  svtkLightsPass* lights = svtkLightsPass::New();

  svtkRenderPassCollection* passes = svtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);

  //  passes->AddItem(peeling);
  passes->AddItem(translucent);

  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);

  svtkGaussianBlurPass* blurP = svtkGaussianBlurPass::New();
  blurP->SetDelegatePass(cameraP);

  svtkSobelGradientMagnitudePass* sobelP = svtkSobelGradientMagnitudePass::New();
  sobelP->SetDelegatePass(blurP);

  glrenderer->SetPass(sobelP);

  //  renderer->SetPass(cameraP);

  opaque->Delete();
  //  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  blurP->Delete();
  sobelP->Delete();
  lights->Delete();

  svtkImageSinusoidSource* imageSource = svtkImageSinusoidSource::New();
  imageSource->SetWholeExtent(0, 9, 0, 9, 0, 9);
  imageSource->SetPeriod(5);
  imageSource->Update();

  svtkImageData* image = imageSource->GetOutput();
  double range[2];
  image->GetScalarRange(range);

  svtkDataSetSurfaceFilter* surface = svtkDataSetSurfaceFilter::New();

  surface->SetInputConnection(imageSource->GetOutputPort());
  imageSource->Delete();

  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(surface->GetOutputPort());
  surface->Delete();

  svtkLookupTable* lut = svtkLookupTable::New();
  lut->SetTableRange(range);
  lut->SetAlphaRange(0.5, 0.5);
  lut->SetHueRange(0.2, 0.7);
  lut->SetNumberOfTableValues(256);
  lut->Build();

  mapper->SetScalarVisibility(1);
  mapper->SetLookupTable(lut);
  lut->Delete();

  svtkActor* actor = svtkActor::New();
  renderer->AddActor(actor);
  actor->Delete();
  actor->SetMapper(mapper);
  mapper->Delete();
  actor->SetVisibility(0);

  svtkConeSource* cone = svtkConeSource::New();
  svtkPolyDataMapper* coneMapper = svtkPolyDataMapper::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());
  cone->Delete();
  svtkActor* coneActor = svtkActor::New();
  coneActor->SetMapper(coneMapper);
  coneActor->SetVisibility(1);
  coneMapper->Delete();
  renderer->AddActor(coneActor);
  coneActor->Delete();

  renderer->SetBackground(0.1, 0.3, 0.0);
  renWin->SetSize(400, 400);

  renWin->Render();
  svtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
