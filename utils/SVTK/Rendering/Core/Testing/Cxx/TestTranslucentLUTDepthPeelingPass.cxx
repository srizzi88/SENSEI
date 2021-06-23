/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTranslucentLUTDepthPeelingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers rendering of an actor with a translucent LUT and depth
// peeling using the multi renderpass classes. The mapper uses color
// interpolation (poor quality).
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkOpenGLExtensionManager.h"
#include "svtkOpenGLRenderWindow.h"
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
#include "svtkDepthPeelingPass.h"
#include "svtkLightsPass.h"
#include "svtkOpaquePass.h"
#include "svtkOverlayPass.h"
#include "svtkRenderPassCollection.h"
#include "svtkSequencePass.h"
#include "svtkTranslucentPass.h"
#include "svtkVolumetricPass.h"

#include "svtkgl.h"

// Make sure to have a valid OpenGL context current on the calling thread
// before calling it.
bool MesaHasSVTKBug8135(svtkRenderWindow* renwin)
{
  svtkOpenGLRenderWindow* context = svtkOpenGLRenderWindow::SafeDownCast(renwin);

  svtkOpenGLExtensionManager* extmgr = context->GetExtensionManager();

  return (extmgr->DriverIsMesa() && !extmgr->DriverVersionAtLeast(7, 3));
}

int TestTranslucentLUTDepthPeelingPass(int argc, char* argv[])
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
  svtkDepthPeelingPass* peeling = svtkDepthPeelingPass::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);

  svtkTranslucentPass* translucent = svtkTranslucentPass::New();
  peeling->SetTranslucentPass(translucent);

  svtkVolumetricPass* volume = svtkVolumetricPass::New();
  svtkOverlayPass* overlay = svtkOverlayPass::New();

  svtkLightsPass* lights = svtkLightsPass::New();

  svtkRenderPassCollection* passes = svtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  passes->AddItem(peeling);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);
  glrenderer->SetPass(cameraP);

  opaque->Delete();
  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
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

  renderer->SetBackground(0.1, 0.3, 0.0);
  renWin->SetSize(400, 400);

  // empty scene during OpenGL detection.
  actor->SetVisibility(0);
  renWin->Render();

  int retVal;
  if (MesaHasSVTKBug8135(renWin))
  {
    // Mesa will crash if version<7.3
    cout << "This version of Mesa would crash. Skip the test." << endl;
    retVal = svtkRegressionTester::PASSED;
  }
  else
  {
    actor->SetVisibility(1);
    renderer->ResetCamera();
    svtkCamera* camera = renderer->GetActiveCamera();
    camera->Azimuth(-40.0);
    camera->Elevation(20.0);
    renWin->Render();

    if (peeling->GetLastRenderingUsedDepthPeeling())
    {
      cout << "depth peeling was used" << endl;
    }
    else
    {
      cout << "depth peeling was not used (alpha blending instead)" << endl;
    }

    retVal = svtkRegressionTestImage(renWin);
    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
  }
  iren->Delete();

  return !retVal;
}
