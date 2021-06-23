/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCropping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGPUVolumeRayCastMapper.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkContourFilter.h"
#include "svtkCuller.h"
#include "svtkCullerCollection.h"
#include "svtkFrustumCoverageCuller.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSLCReader.h"
#include "svtkStructuredPoints.h"
#include "svtkStructuredPointsReader.h"
#include "svtkTestUtilities.h"
#include "svtkTransform.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastCropping(int argc, char* argv[])
{
  // Create the standard renderer, render window, and interactor.
  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  ren1->Delete();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  iren->SetDesiredUpdateRate(3);

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  svtkSLCReader* reader = svtkSLCReader::New();
  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/sphere.slc");
  reader->SetFileName(cfname);
  delete[] cfname;

  // Create transfer mapping scalar value to opacity.
  svtkPiecewiseFunction* opacityTransferFunction = svtkPiecewiseFunction::New();
  opacityTransferFunction->AddPoint(0.0, 0.0);
  opacityTransferFunction->AddPoint(30.0, 0.0);
  opacityTransferFunction->AddPoint(80.0, 0.5);
  opacityTransferFunction->AddPoint(255.0, 0.5);

  // Create transfer mapping scalar value to color.
  svtkColorTransferFunction* colorTransferFunction = svtkColorTransferFunction::New();
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  // The property describes how the data will look.
  svtkVolumeProperty* volumeProperty = svtkVolumeProperty::New();
  volumeProperty->SetColor(colorTransferFunction);
  colorTransferFunction->Delete();
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  opacityTransferFunction->Delete();
  volumeProperty->ShadeOn(); //
  volumeProperty->SetInterpolationTypeToLinear();

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 300);
  renWin->Render();
  ren1->ResetCamera();
  renWin->Render();

  svtkGPUVolumeRayCastMapper* volumeMapper[2][4];
  int i = 0;
  while (i < 2)
  {
    int j = 0;
    while (j < 4)
    {
      volumeMapper[i][j] = svtkGPUVolumeRayCastMapper::New();
      volumeMapper[i][j]->SetInputConnection(reader->GetOutputPort());
      volumeMapper[i][j]->SetSampleDistance(0.25);
      volumeMapper[i][j]->CroppingOn();
      volumeMapper[i][j]->SetAutoAdjustSampleDistances(0);
      volumeMapper[i][j]->SetCroppingRegionPlanes(17, 33, 17, 33, 17, 33);

      svtkVolume* volume = svtkVolume::New();
      volume->SetMapper(volumeMapper[i][j]);
      volumeMapper[i][j]->Delete();
      volume->SetProperty(volumeProperty);

      svtkTransform* userMatrix = svtkTransform::New();
      userMatrix->PostMultiply();
      userMatrix->Identity();
      userMatrix->Translate(-25, -25, -25);

      if (i == 0)
      {
        userMatrix->RotateX(j * 90 + 20);
        userMatrix->RotateY(20);
      }
      else
      {
        userMatrix->RotateX(20);
        userMatrix->RotateY(j * 90 + 20);
      }

      userMatrix->Translate(j * 55 + 25, i * 55 + 25, 0);
      volume->SetUserTransform(userMatrix);
      userMatrix->Delete();
      //      if(i==1 && j==0)
      //        {
      ren1->AddViewProp(volume);
      //        }
      volume->Delete();
      ++j;
    }
    ++i;
  }
  reader->Delete();
  volumeProperty->Delete();

  int I = 1;
  int J = 0;

  volumeMapper[0][0]->SetCroppingRegionFlagsToSubVolume();
  volumeMapper[0][1]->SetCroppingRegionFlagsToCross();
  volumeMapper[0][2]->SetCroppingRegionFlagsToInvertedCross();
  volumeMapper[0][3]->SetCroppingRegionFlags(24600);

  //
  volumeMapper[1][0]->SetCroppingRegionFlagsToFence();
  volumeMapper[1][1]->SetCroppingRegionFlagsToInvertedFence();
  volumeMapper[1][2]->SetCroppingRegionFlags(1);
  volumeMapper[1][3]->SetCroppingRegionFlags(67117057);
  ren1->GetCullers()->InitTraversal();
  svtkCuller* culler = ren1->GetCullers()->GetNextItem();

  svtkFrustumCoverageCuller* fc = svtkFrustumCoverageCuller::SafeDownCast(culler);
  if (fc != nullptr)
  {
    fc->SetSortingStyleToBackToFront();
  }
  else
  {
    cout << "culler is not a svtkFrustumCoverageCuller" << endl;
  }

  // First test if mapper is supported

  int valid = volumeMapper[I][J]->IsRenderSupported(renWin, volumeProperty);

  int retVal;
  if (valid)
  {
    ren1->ResetCamera();
    ren1->GetActiveCamera()->Zoom(3.0);
    //  ren1->GetActiveCamera()->SetParallelProjection(1);
    renWin->Render();

    retVal = svtkTesting::Test(argc, argv, renWin, 75);
    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
  }
  else
  {
    retVal = svtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
  }

  // Clean up.
  iren->Delete();

  if ((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
