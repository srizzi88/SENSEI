/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastUserShader.cxx

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

int TestGPURayCastUserShader(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  svtkNew<svtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  svtkDataArray* arr = reader->GetOutput()->GetPointData()->GetScalars();
  double range[2];
  arr->GetRange(range);

  // Prepare 1D Transfer Functions
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(510, 0.4, 0.4, 1.0);
  ctf->AddRGBPoint(640, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(range[1], 0.9, 0.1, 0.1);

  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(510, 0.00);
  pf->AddPoint(640, 0.5);
  pf->AddPoint(range[1], 0.4);

  volumeProperty->SetScalarOpacity(pf.GetPointer());
  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetShade(1);

  svtkNew<svtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetUseJittering(1);

  svtkNew<svtkShaderProperty> shaderProperty;

  // Modify the shader to color based on the depth of the translucent voxel
  shaderProperty->AddFragmentShaderReplacement("//SVTK::Base::Dec", // Source string to replace
    true,              // before the standard replacements
    "//SVTK::Base::Dec" // We still want the default
    "\n bool l_updateDepth;"
    "\n vec3 l_opaqueFragPos;",
    false // only do it once i.e. only replace the first match
  );
  shaderProperty->AddFragmentShaderReplacement("//SVTK::Base::Init", true,
    "//SVTK::Base::Init\n"
    "\n l_updateDepth = true;"
    "\n l_opaqueFragPos = vec3(0.0);",
    false);
  shaderProperty->AddFragmentShaderReplacement("//SVTK::Base::Impl", true,
    "//SVTK::Base::Impl"
    "\n    if(!g_skip && g_srcColor.a > 0.0 && l_updateDepth)"
    "\n      {"
    "\n      l_opaqueFragPos = g_dataPos;"
    "\n      l_updateDepth = false;"
    "\n      }",
    false);
  shaderProperty->AddFragmentShaderReplacement("//SVTK::RenderToImage::Exit", true,
    "//SVTK::RenderToImage::Exit"
    "\n  if (l_opaqueFragPos == vec3(0.0))"
    "\n    {"
    "\n    fragOutput0 = vec4(0.0);"
    "\n    }"
    "\n  else"
    "\n    {"
    "\n    vec4 depthValue = in_projectionMatrix * in_modelViewMatrix *"
    "\n                      in_volumeMatrix[0] * in_textureDatasetMatrix[0] *"
    "\n                      vec4(l_opaqueFragPos, 1.0);"
    "\n    depthValue /= depthValue.w;"
    "\n    fragOutput0 = vec4(vec3(0.5 * (gl_DepthRange.far -"
    "\n                       gl_DepthRange.near) * depthValue.z + 0.5 *"
    "\n                      (gl_DepthRange.far + gl_DepthRange.near)), 1.0);"
    "\n    }",
    false);
  shaderProperty->AddFragmentShaderReplacement( // add dummy replacement
    "//SVTK::ComputeGradient::Dec", true, "SVTK::ComputeGradient::Dec", false);
  shaderProperty->ClearFragmentShaderReplacement( // clear dummy replacement
    "//SVTK::ComputeGradient::Dec", true);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  volume->SetShaderProperty(shaderProperty.GetPointer());

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300); // Intentional NPOT size

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->GetActiveCamera()->Elevation(-60.0);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.3);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
