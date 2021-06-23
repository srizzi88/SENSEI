/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastFourComponentsDependentGradient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This is a test for volume rendering using the GPU ray cast
// mapper of a dataset with four components treating them as
// dependent and applying a gradient opacity function

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLImageDataReader.h"

int TestGPURayCastFourComponentsDependentGradient(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_4comp.vti");

  svtkNew<svtkXMLImageDataReader> reader;
  reader->SetFileName(cfname);
  delete[] cfname;

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(301, 300); // Intentional NPOT size
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  // Volume render the dataset
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->AutoAdjustSampleDistancesOff();
  mapper->SetSampleDistance(0.5);
  mapper->SetInputConnection(reader->GetOutputPort());

  // Opacity transfer function
  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0);
  pf->AddPoint(255, 1);

  // Gradient opacity transfer function
  svtkNew<svtkPiecewiseFunction> pf1;
  pf1->AddPoint(30, 0);
  pf1->AddPoint(255, 1);

  // Volume property with independent components OFF
  svtkNew<svtkVolumeProperty> property;
  property->IndependentComponentsOff();
  property->SetScalarOpacity(pf);
  property->SetGradientOpacity(pf1);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);
  ren->AddVolume(volume);

  ren->ResetCamera();
  renWin->Render();

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
