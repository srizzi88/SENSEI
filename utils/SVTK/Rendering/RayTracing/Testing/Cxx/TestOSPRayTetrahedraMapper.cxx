/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test verifies that we can use ospray to volume render
// svtk unstructured grid

#include "svtkColorTransferFunction.h"
#include "svtkDataSetTriangleFilter.h"
#include "svtkImageCast.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkOSPRayPass.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStructuredPointsReader.h"
#include "svtkTesting.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGridVolumeRayCastMapper.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"
#include <svtkTestUtilities.h>

namespace
{

static const char* TestOSPRayTetrahedraMapperLog = "# StreamVersion 1\n"
                                                   "EnterEvent 299 0 0 0 0 0 0\n"
                                                   "MouseMoveEvent 299 0 0 0 0 0 0\n"
                                                   "MouseMoveEvent 298 2 0 0 0 0 0\n"
                                                   "MouseMoveEvent 297 4 0 0 0 0 0\n"
                                                   "MouseMoveEvent 297 6 0 0 0 0 0\n"
                                                   "MouseMoveEvent 296 8 0 0 0 0 0\n"
                                                   "LeaveEvent 399 -8 0 0 0 0 0\n";

} // end anon namespace

int TestOSPRayTetrahedraMapper(int argc, char* argv[])
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

  // Create the reader for the data
  // This is the data the will be volume rendered
  svtkNew<svtkStructuredPointsReader> reader;
  const char* file1 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.svtk");
  reader->SetFileName(file1);
  reader->Update();

  // currently ospray only supports float, remove when that
  // changes in ospray version that ParaView packages.
  svtkNew<svtkImageCast> toFloat;
  toFloat->SetInputConnection(reader->GetOutputPort());
  toFloat->SetOutputScalarTypeToFloat();

  // convert from svtkImageData to svtkUnstructuredGrid, remove
  // any cells where all values are below 80
  svtkNew<svtkThreshold> thresh;
  thresh->ThresholdByUpper(80);
  thresh->AllScalarsOff();
  thresh->SetInputConnection(toFloat->GetOutputPort());

  // make sure we have only tetrahedra
  svtkNew<svtkDataSetTriangleFilter> trifilter;
  trifilter->SetInputConnection(thresh->GetOutputPort());

  // Create transfer mapping scalar value to opacity
  svtkNew<svtkPiecewiseFunction> opacityTransferFunction;
  opacityTransferFunction->AddPoint(80, 0.0);
  opacityTransferFunction->AddPoint(120, 0.2);
  opacityTransferFunction->AddPoint(255, 0.2);

  // Create transfer mapping scalar value to color
  svtkNew<svtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint(80.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(120.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(160.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(200.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 1.0, 1.0);

  // The property describes how the data will look
  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(colorTransferFunction.GetPointer());
  volumeProperty->SetScalarOpacity(opacityTransferFunction.GetPointer());
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper / ray cast function know how to render the data
  svtkNew<svtkUnstructuredGridVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(trifilter->GetOutputPort());

  // The volume holds the mapper and the property and
  // can be used to position/orient the volume
  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  svtkNew<svtkRenderer> ren1;
  ren1->AddVolume(volume.GetPointer());

  // Create the renderwindow, interactor and renderer
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(401, 399); // NPOT size
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.GetPointer());
  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());
  ren1->SetBackground(0.3, 0.3, 0.4);
  renderWindow->AddRenderer(ren1.GetPointer());

  ren1->ResetCamera();
  renderWindow->Render();

  // Attach OSPRay render pass
  svtkNew<svtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    ren1->SetPass(osprayPass.GetPointer());
  }

  volumeMapper->DebugOn();
  int retVal;
  retVal = !(
    svtkTesting::InteractorEventLoop(argc, argv, iren.GetPointer(), TestOSPRayTetrahedraMapperLog));
  return !retVal;
}
