/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMinIntensityRendering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkFiniteDifferenceGradientEstimator.h"
#include "svtkFixedPointVolumeRayCastMapper.h"
#include "svtkImageClip.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStructuredPoints.h"
#include "svtkStructuredPointsReader.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include "svtkDebugLeaks.h"
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

int TestMinIntensityRendering(int argc, char* argv[])
{

  // Create the renderers, render window, and interactor
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  svtkRenderer* ren = svtkRenderer::New();
  renWin->AddRenderer(ren);

  // Read the data from a svtk file
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.svtk");
  svtkStructuredPointsReader* reader = svtkStructuredPointsReader::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Create a transfer function mapping scalar value to opacity
  svtkPiecewiseFunction* oTFun = svtkPiecewiseFunction::New();
  oTFun->AddSegment(0, 1.0, 256, 0.1);

  svtkColorTransferFunction* cTFun = svtkColorTransferFunction::New();
  cTFun->AddRGBPoint(0, 1.0, 1.0, 1.0);
  cTFun->AddRGBPoint(255, 1.0, 1.0, 1.0);

  // Need to crop to actually see minimum intensity
  svtkImageClip* clip = svtkImageClip::New();
  clip->SetInputConnection(reader->GetOutputPort());
  clip->SetOutputWholeExtent(0, 66, 0, 66, 30, 37);
  clip->ClipDataOn();

  svtkVolumeProperty* property = svtkVolumeProperty::New();
  property->SetScalarOpacity(oTFun);
  property->SetColor(cTFun);
  property->SetInterpolationTypeToLinear();

  svtkFixedPointVolumeRayCastMapper* mapper = svtkFixedPointVolumeRayCastMapper::New();
  mapper->SetBlendModeToMinimumIntensity();
  mapper->SetInputConnection(clip->GetOutputPort());

  svtkVolume* volume = svtkVolume::New();
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  ren->AddViewProp(volume);

  renWin->Render();
  int retVal = svtkRegressionTestImageThreshold(renWin, 70);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  volume->Delete();
  mapper->Delete();
  property->Delete();
  clip->Delete();
  cTFun->Delete();
  oTFun->Delete();
  reader->Delete();
  renWin->Delete();
  iren->Delete();
  ren->Delete();

  return !retVal;
}
