/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextureRGBADepthPeeling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of an RGBA texture on a svtkActor.
// .SECTION Description
// this program tests the rendering of an svtkActor with a translucent texture
// with depth peeling.

#include "svtkActor.h"
#include "svtkImageData.h"
#include "svtkPNGReader.h"
#include "svtkPlaneSource.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"

int TestTextureRGBADepthPeeling(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/textureRGBA.png");

  svtkPNGReader* PNGReader = svtkPNGReader::New();
  PNGReader->SetFileName(fname);
  PNGReader->Update();

  svtkTexture* texture = svtkTexture::New();
  texture->SetInputConnection(PNGReader->GetOutputPort());
  PNGReader->Delete();
  texture->InterpolateOn();

  svtkPlaneSource* planeSource = svtkPlaneSource::New();
  planeSource->Update();

  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(planeSource->GetOutputPort());
  planeSource->Delete();

  svtkActor* actor = svtkActor::New();
  actor->SetTexture(texture);
  texture->Delete();
  actor->SetMapper(mapper);
  mapper->Delete();

  svtkRenderer* renderer = svtkRenderer::New();
  renderer->AddActor(actor);
  actor->Delete();
  renderer->SetBackground(0.5, 0.7, 0.7);

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(200);
  renderer->SetOcclusionRatio(0.1);

  svtkRenderWindowInteractor* interactor = svtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->SetSize(400, 400);
  renWin->Render();
  if (renderer->GetLastRenderingUsedDepthPeeling())
  {
    cout << "depth peeling was used" << endl;
  }
  else
  {
    cout << "depth peeling was not used (alpha blending instead)" << endl;
  }

  interactor->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  renderer->Delete();
  renWin->Delete();
  interactor->Delete();
  delete[] fname;

  return !retVal;
}
