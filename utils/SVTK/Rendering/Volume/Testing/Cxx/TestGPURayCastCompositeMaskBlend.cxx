/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCompositeMaskBlend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageCheckerboard.h"
#include "svtkImageData.h"
#include "svtkImageGridSource.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLImageDataReader.h"

int TestGPURayCastCompositeMaskBlend(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;
  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");

  svtkXMLImageDataReader* reader = svtkXMLImageDataReader::New();
  reader->SetFileName(cfname);
  delete[] cfname;

  reader->Update();
  svtkImageData* input = reader->GetOutput();

  int dim[3];
  double spacing[3];
  input->GetSpacing(spacing);
  input->GetDimensions(dim);

  svtkGPUVolumeRayCastMapper* mapper = svtkGPUVolumeRayCastMapper::New();
  svtkVolume* volume = svtkVolume::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetAutoAdjustSampleDistances(0);

  // assume the scalar field is a set of samples taken from a
  // contiguous band-limited volumetric field.
  // assume max frequency is present:
  // min spacing divided by 2. Nyquist-Shannon theorem says so.
  // sample distance could be bigger if we compute the actual max frequency
  // in the data.
  double distance = spacing[0];
  if (distance > spacing[1])
  {
    distance = spacing[1];
  }
  if (distance > spacing[2])
  {
    distance = spacing[2];
  }
  distance = distance / 2.0;

  mapper->SetSampleDistance(static_cast<float>(distance));

  svtkColorTransferFunction* colorFun = svtkColorTransferFunction::New();
  svtkPiecewiseFunction* opacityFun = svtkPiecewiseFunction::New();

  // Create the property and attach the transfer functions
  svtkVolumeProperty* property = svtkVolumeProperty::New();
  property->SetIndependentComponents(true);
  property->SetColor(colorFun);
  property->SetScalarOpacity(opacityFun);
  property->SetInterpolationTypeToLinear();

  // connect up the volume to the property and the mapper
  volume->SetProperty(property);
  volume->SetMapper(mapper);

  double opacityLevel = 120;
  double opacityWindow = 240;

  colorFun->AddRGBSegment(opacityLevel - 0.5 * opacityWindow, 0.0, 0.0, 0.0,
    opacityLevel + 0.5 * opacityWindow, 1.0, 1.0, 1.0);
  opacityFun->AddSegment(opacityLevel - 0.5 * opacityWindow, 0.0, // 0.0, 0.01
    opacityLevel + 0.5 * opacityWindow, 1.0);                     // 1.0, 0.01
  mapper->SetBlendModeToComposite();
  property->ShadeOff();

  // Make the mask
  svtkImageGridSource* grid = svtkImageGridSource::New();
  grid->SetDataScalarTypeToUnsignedChar();
  grid->SetDataExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  grid->SetLineValue(1); // mask value
  grid->SetFillValue(0);
  grid->SetGridSpacing(5, 5, 5);
  grid->Update();
  mapper->SetMaskInput(grid->GetOutput());

  svtkImageGridSource* grid2 = svtkImageGridSource::New();
  grid2->SetDataScalarTypeToUnsignedChar();
  grid2->SetDataExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  grid2->SetLineValue(2); // mask value
  grid2->SetFillValue(0);
  grid2->SetGridSpacing(6, 6, 6);
  grid2->Update();
  //    mapper->SetMaskInput(grid2->GetOutput());
  //    grid2->Delete();

  svtkImageCheckerboard* checkerboard = svtkImageCheckerboard::New();
  checkerboard->SetInputConnection(0, grid->GetOutputPort());

  checkerboard->SetInputConnection(1, grid2->GetOutputPort());
  grid->Delete();
  grid2->Delete();
  checkerboard->Update();
  mapper->SetMaskInput(checkerboard->GetOutput());
  mapper->SetMaskBlendFactor(0.1f);
  checkerboard->Delete();

  // Add color transfer functions for the masks
  svtkColorTransferFunction* mask1colorFun = svtkColorTransferFunction::New();
  property->SetLabelColor(1, mask1colorFun);
  property->SetLabelScalarOpacity(1, opacityFun);
  mask1colorFun->Delete();

  // yellow.
  mask1colorFun->AddRGBSegment(opacityLevel - 0.5 * opacityWindow, 0.0, 1.0, 0.0,
    opacityLevel + 0.5 * opacityWindow, 1.0, 1.0, 0.0);

  svtkColorTransferFunction* mask2colorFun = svtkColorTransferFunction::New();
  property->SetLabelColor(2, mask2colorFun);
  property->SetLabelScalarOpacity(2, opacityFun);
  mask2colorFun->Delete();

  // red
  mask2colorFun->AddRGBSegment(opacityLevel - 0.5 * opacityWindow, 0.5, 0.0, 0.0,
    opacityLevel + 0.5 * opacityWindow, 1.0, 0.0, 0.0);

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetSize(300, 300);
  iren->SetRenderWindow(renWin);

  svtkRenderer* ren1 = svtkRenderer::New();
  renWin->AddRenderer(ren1);

  renWin->Render();

  int valid = mapper->IsRenderSupported(renWin, property);

  int retVal;
  if (valid)
  {
    ren1->AddViewProp(volume);
    iren->Initialize();
    ren1->SetBackground(0.1, 0.4, 0.2);
    ren1->ResetCamera();
    ren1->GetActiveCamera()->Zoom(1.5);
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

  volume->Delete();
  mapper->Delete();
  colorFun->Delete();
  opacityFun->Delete();
  property->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();

  if ((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
