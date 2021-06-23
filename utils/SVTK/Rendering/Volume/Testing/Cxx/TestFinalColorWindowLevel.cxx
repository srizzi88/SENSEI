/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFinalColorWindowLevel.cxx

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

// Create an 8x7 grid of render windows in a renderer and render a volume
// using various techniques for testing purposes
int TestFinalColorWindowLevel(int argc, char* argv[])
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
  oTFun->AddSegment(10, 0.0, 255, 0.3);

  // Create a transfer function mapping scalar value to color (color)
  svtkColorTransferFunction* cTFun = svtkColorTransferFunction::New();
  cTFun->AddRGBPoint(0, 1.0, 0.0, 0.0);
  cTFun->AddRGBPoint(64, 1.0, 1.0, 0.0);
  cTFun->AddRGBPoint(128, 0.0, 1.0, 0.0);
  cTFun->AddRGBPoint(192, 0.0, 1.0, 1.0);
  cTFun->AddRGBPoint(255, 0.0, 0.0, 1.0);

  svtkVolumeProperty* property = svtkVolumeProperty::New();
  property->SetShade(0);
  property->SetAmbient(0.3);
  property->SetDiffuse(1.0);
  property->SetSpecular(0.2);
  property->SetSpecularPower(50.0);
  property->SetScalarOpacity(oTFun);
  property->SetColor(cTFun);
  property->SetInterpolationTypeToLinear();

  svtkFixedPointVolumeRayCastMapper* mapper = svtkFixedPointVolumeRayCastMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  svtkVolume* volume = svtkVolume::New();
  volume->SetProperty(property);
  volume->SetMapper(mapper);
  ren->AddViewProp(volume);

  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);

  mapper->SetFinalColorWindow(.5);
  mapper->SetFinalColorLevel(.75);

  renWin->Render();

  int retVal = svtkRegressionTestImageThreshold(renWin, 70);

  // Interact with the data at 3 frames per second
  iren->SetDesiredUpdateRate(3.0);
  iren->SetStillUpdateRate(0.001);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Clean up
  reader->Delete();
  oTFun->Delete();
  cTFun->Delete();
  property->Delete();
  mapper->Delete();
  volume->Delete();

  ren->Delete();
  iren->Delete();
  renWin->Delete();

  return !retVal;
}
