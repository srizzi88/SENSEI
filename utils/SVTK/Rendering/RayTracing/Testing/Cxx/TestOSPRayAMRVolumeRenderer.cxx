/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayAMRVolumeRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test checks if OSPRay based AMR Volume rendering works

#include "svtkAMRGaussianPulseSource.h"
#include "svtkAMRVolumeMapper.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkNew.h"
#include "svtkOSPRayPass.h"
#include "svtkOverlappingAMR.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolumeProperty.h"

int TestOSPRayAMRVolumeRenderer(int argc, char* argv[])
{
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      cerr << "GL" << endl;
      useOSP = false;
    }
  }

  double scalarRange[2] = { 4.849e-23, 0.4145 };
  svtkNew<svtkAMRVolumeMapper> volumeMapper;

  svtkNew<svtkAMRGaussianPulseSource> amrSource;
  amrSource->SetXPulseOrigin(0);
  amrSource->SetYPulseOrigin(0);
  amrSource->SetZPulseOrigin(0);
  amrSource->SetXPulseWidth(.5);
  amrSource->SetYPulseWidth(.5);
  amrSource->SetZPulseWidth(.5);
  amrSource->SetPulseAmplitude(0.5);
  amrSource->SetDimension(3);
  amrSource->SetRootSpacing(0.5);
  amrSource->SetRefinementRatio(2);
  amrSource->Update();
  volumeMapper->SetInputConnection(amrSource->GetOutputPort());

  volumeMapper->SelectScalarArray("Gaussian-Pulse");
  volumeMapper->SetScalarMode(SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 0.2);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  svtkColorTransferFunction* colorTransferFunction = volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.8, 0.6, 0.1);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 0.1, 0.2, 0.8);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  // Attach OSPRay render pass
  svtkNew<svtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    ren->SetPass(osprayPass.GetPointer());
  }

  ren->AddViewProp(volume.GetPointer());
  renWin->Render();
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(140);
  ren->GetActiveCamera()->Elevation(30);

  iren->Initialize();
  iren->SetDesiredUpdateRate(30.0);

  int retVal = svtkRegressionTestImage(renWin.GetPointer());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  volumeMapper->SetInputConnection(nullptr);
  return !retVal;
}
