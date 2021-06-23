/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTranslucentLUTTextureDepthPeeling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers rendering of an actor with a translucent LUT and depth
// peeling. The mapper uses texture coordinate interpolation and
// 1D texture mapping (better quality than color interpolation).
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkCamera.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkImageSinusoidSource.h"
#include "svtkLookupTable.h"
#include "svtkPolyDataMapper.h"

int TestTranslucentLUTTextureDepthPeeling(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetAlphaBitPlanes(1);
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  svtkRenderer* renderer = svtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();
  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(200);
  renderer->SetOcclusionRatio(0.1);

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
  mapper->UseLookupTableScalarRangeOn();
  mapper->InterpolateScalarsBeforeMappingOn();
  lut->Delete();

  svtkActor* actor = svtkActor::New();
  renderer->AddActor(actor);
  actor->Delete();
  actor->SetMapper(mapper);
  mapper->Delete();

  renderer->SetBackground(0.1, 0.3, 0.0);
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
