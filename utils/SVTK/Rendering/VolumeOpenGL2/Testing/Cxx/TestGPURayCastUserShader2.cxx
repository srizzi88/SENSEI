/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastUserShader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test the volume mapper's ability to perform shader substitutions based on
// user specified strings.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkNrrdReader.h"
#include "svtkOpenGLGPUVolumeRayCastMapper.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShaderProperty.h"
#include "svtkTestUtilities.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include <TestGPURayCastUserShader2_FS.h>

int TestGPURayCastUserShader2(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  svtkNew<svtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkImageData* im = reader->GetOutput();
  double* bounds = im->GetBounds();
  double depthRange[2];
  depthRange[0] = svtkMath::Min(bounds[0], bounds[2]);
  depthRange[0] = svtkMath::Min(depthRange[0], bounds[4]);
  depthRange[1] = svtkMath::Max(bounds[1], bounds[3]);
  depthRange[1] = svtkMath::Max(depthRange[1], bounds[5]);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  svtkDataArray* arr = reader->GetOutput()->GetPointData()->GetScalars();
  double range[2];
  arr->GetRange(range);

  // Prepare 1D Transfer Functions
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(depthRange[0], 1.0, 0.0, 0.0);
  ctf->AddRGBPoint(0.5 * (depthRange[0] + depthRange[1]), 0.5, 0.5, 0.5);
  ctf->AddRGBPoint(0.8 * (depthRange[0] + depthRange[1]), 0.5, 0.4, 0.6);
  ctf->AddRGBPoint(depthRange[1], 0.0, 1.0, 1.0);

  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(510, 0.00);
  pf->AddPoint(640, 0.5);
  pf->AddPoint(range[1], 0.5);

  volumeProperty->SetScalarOpacity(pf.GetPointer());
  volumeProperty->SetColor(ctf.GetPointer());

  svtkNew<svtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetUseJittering(0);

  // Tell the mapper to use the min and max of the color function nodes as the
  // lookup table range instead of the volume scalar range.
  mapper->SetColorRangeType(svtkGPUVolumeRayCastMapper::NATIVE);

  svtkNew<svtkShaderProperty> shaderProperty;
  // Clear all custom shader tag replacements
  // The following code is mainly for regression testing as we do not have any
  // custom shader replacements.
  shaderProperty->ClearAllVertexShaderReplacements();
  shaderProperty->ClearAllFragmentShaderReplacements();
  shaderProperty->ClearAllGeometryShaderReplacements();
  shaderProperty->ClearAllShaderReplacements();

  // Modify the shader to color based on the depth of the translucent voxel
  shaderProperty->SetFragmentShaderCode(TestGPURayCastUserShader2_FS);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  volume->SetShaderProperty(shaderProperty);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300); // Intentional NPOT size

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->GetActiveCamera()->Elevation(-50.0);
  ren->GetActiveCamera()->Yaw(-30.0);
  ren->GetActiveCamera()->Roll(-10.0);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.4);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
