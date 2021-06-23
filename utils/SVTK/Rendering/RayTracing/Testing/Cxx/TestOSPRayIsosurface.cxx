/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayIsosurface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTestUtilities.h"

#include "svtkColorTransferFunction.h"
#include "svtkContourValues.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayVolumeMapper.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkVolumeProperty.h"

int TestOSPRayIsosurface(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRTAnalyticSource> wavelet;

  svtkNew<svtkOSPRayVolumeMapper> volumeMapper;
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());
  volumeMapper->SetBlendModeToIsoSurface();

  svtkNew<svtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint(220.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(150.0, 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(190.0, 0.0, 1.0, 1.0);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(220.0, 1.0);
  scalarOpacity->AddPoint(150.0, 0.2);
  scalarOpacity->AddPoint(190.0, 0.6);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);
  volumeProperty->GetIsoSurfaceValues()->SetValue(0, 220.0);
  volumeProperty->GetIsoSurfaceValues()->SetValue(1, 150.0);
  volumeProperty->GetIsoSurfaceValues()->SetValue(2, 190.0);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  renderer->AddVolume(volume);
  renWin->SetSize(400, 400);

  svtkNew<svtkOSPRayPass> ospray;
  renderer->SetPass(ospray);

  renWin->Render();

  iren->Start();
  return 0;
}
