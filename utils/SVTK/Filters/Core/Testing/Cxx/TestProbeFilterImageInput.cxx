/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProbeFilterImageInput.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkProbeFilter.h"

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkDelaunay3D.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPointSource.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartVolumeMapper.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include "svtkRegressionTestImage.h"

#include <iostream>

int TestProbeFilterImageInput(int argc, char* argv[])
{
  static const int dim = 48;
  double center[3];
  center[0] = center[1] = center[2] = static_cast<double>(dim) / 2.0;
  int extent[6] = { 0, dim - 1, 0, dim - 1, 0, dim - 1 };

  svtkNew<svtkRTAnalyticSource> imageSource;
  imageSource->SetWholeExtent(extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);
  imageSource->SetCenter(center);
  imageSource->Update();

  svtkImageData* img = imageSource->GetOutput();
  double range[2], origin[3], spacing[3];
  img->GetScalarRange(range);
  img->GetOrigin(origin);
  img->GetSpacing(spacing);

  // create an unstructured grid by generating a point cloud and
  // applying Delaunay triangulation on it.
  svtkMath::RandomSeed(0); // svtkPointSource internally uses svtkMath::Random()
  svtkNew<svtkPointSource> pointSource;
  pointSource->SetCenter(center);
  pointSource->SetRadius(center[0]);
  pointSource->SetNumberOfPoints(24 * 24 * 24);

  svtkNew<svtkDelaunay3D> delaunay3D;
  delaunay3D->SetInputConnection(pointSource->GetOutputPort());

  // probe into img using unstructured grif geometry
  svtkNew<svtkProbeFilter> probe1;
  probe1->SetSourceData(img);
  probe1->SetInputConnection(delaunay3D->GetOutputPort());

  // probe into the unstructured grid using ImageData geometry
  svtkNew<svtkImageData> outputData;
  outputData->SetExtent(extent);
  outputData->SetOrigin(origin);
  outputData->SetSpacing(spacing);
  svtkNew<svtkFloatArray> fa;
  fa->SetName("scalars");
  fa->Allocate(dim * dim * dim);
  outputData->GetPointData()->SetScalars(fa);

  svtkNew<svtkProbeFilter> probe2;
  probe2->SetSourceConnection(probe1->GetOutputPort());
  probe2->SetInputData(outputData);

  // render using ray-cast volume rendering
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;
  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  svtkNew<svtkSmartVolumeMapper> volumeMapper;
  volumeMapper->SetInputConnection(probe2->GetOutputPort());
  volumeMapper->SetRequestedRenderModeToRayCast();

  svtkNew<svtkColorTransferFunction> volumeColor;
  volumeColor->AddRGBPoint(range[0], 0.0, 0.0, 1.0);
  volumeColor->AddRGBPoint((range[0] + range[1]) * 0.5, 0.0, 1.0, 0.0);
  volumeColor->AddRGBPoint(range[1], 1.0, 0.0, 0.0);

  svtkNew<svtkPiecewiseFunction> volumeScalarOpacity;
  volumeScalarOpacity->AddPoint(range[0], 0.0);
  volumeScalarOpacity->AddPoint((range[0] + range[1]) * 0.5, 0.0);
  volumeScalarOpacity->AddPoint(range[1], 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(volumeColor);
  volumeProperty->SetScalarOpacity(volumeScalarOpacity);
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOn();
  volumeProperty->SetAmbient(0.5);
  volumeProperty->SetDiffuse(0.8);
  volumeProperty->SetSpecular(0.2);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->ResetCamera();
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
