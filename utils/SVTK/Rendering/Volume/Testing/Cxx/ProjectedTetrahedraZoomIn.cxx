/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ProjectedTetrahedraZoomIn.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// This test makes sure that the mapper behaves well when the user zooms in
// enough to have cells in front of the near plane.

#include "svtkProjectedTetrahedraMapper.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkContourFilter.h"
#include "svtkDataSetTriangleFilter.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSLCReader.h"
#include "svtkStdString.h"
#include "svtkStructuredPoints.h"
#include "svtkStructuredPointsReader.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int ProjectedTetrahedraZoomIn(int argc, char* argv[])
{
  int i;
  // Need to get the data root.
  const char* data_root = nullptr;
  for (i = 0; i < argc - 1; i++)
  {
    if (strcmp("-D", argv[i]) == 0)
    {
      data_root = argv[i + 1];
      break;
    }
  }
  if (!data_root)
  {
    cout << "Need to specify the directory to SVTK_DATA_ROOT with -D <dir>." << endl;
    return 1;
  }

  // Create the standard renderer, render window, and interactor.
  SVTK_CREATE(svtkRenderer, ren1);
  SVTK_CREATE(svtkRenderWindow, renWin);
  renWin->AddRenderer(ren1);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);
  iren->SetDesiredUpdateRate(3);

  // check for driver support
  renWin->Render();
  SVTK_CREATE(svtkProjectedTetrahedraMapper, volumeMapper);
  if (!volumeMapper->IsSupported(renWin))
  {
    svtkGenericWarningMacro("Projected tetrahedra is not supported. Skipping tests.");
    return 0;
  }

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  svtkStdString filename;
  filename = data_root;
  filename += "/Data/ironProt.svtk";
  cout << "Loading " << filename.c_str() << endl;
  SVTK_CREATE(svtkStructuredPointsReader, reader);
  reader->SetFileName(filename.c_str());

  // Create a reader for the other data that will be contoured and
  // displayed as a polygonal mesh.
  filename = data_root;
  filename += "/Data/neghip.slc";
  cout << "Loading " << filename.c_str() << endl;
  SVTK_CREATE(svtkSLCReader, reader2);
  reader2->SetFileName(filename.c_str());

  // Convert from svtkImageData to svtkUnstructuredGrid.
  // Remove any cells where all values are below 80.
  SVTK_CREATE(svtkThreshold, thresh);
  thresh->ThresholdByUpper(80);
  thresh->AllScalarsOff();
  thresh->SetInputConnection(reader->GetOutputPort());

  // Make sure we have only tetrahedra.
  SVTK_CREATE(svtkDataSetTriangleFilter, trifilter);
  trifilter->SetInputConnection(thresh->GetOutputPort());

  // Create transfer mapping scalar value to opacity.
  SVTK_CREATE(svtkPiecewiseFunction, opacityTransferFunction);
  opacityTransferFunction->AddPoint(80.0, 0.0);
  opacityTransferFunction->AddPoint(120.0, 0.2);
  opacityTransferFunction->AddPoint(255.0, 0.2);

  // Create transfer mapping scalar value to color.
  SVTK_CREATE(svtkColorTransferFunction, colorTransferFunction);
  colorTransferFunction->AddRGBPoint(80.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(120.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(160.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(200.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 1.0, 1.0);

  // The property describes how the data will look.
  SVTK_CREATE(svtkVolumeProperty, volumeProperty);
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper that renders the volume data.
  volumeMapper->SetInputConnection(trifilter->GetOutputPort());

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume.
  SVTK_CREATE(svtkVolume, volume);
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Contour the second dataset.
  SVTK_CREATE(svtkContourFilter, contour);
  contour->SetValue(0, 80);
  contour->SetInputConnection(reader2->GetOutputPort());

  // Create a mapper for the polygonal data.
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->ScalarVisibilityOff();

  // Create an actor for the polygonal data.
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);

  ren1->AddViewProp(actor);
  ren1->AddVolume(volume);

  renWin->SetSize(300, 300);
  ren1->ResetCamera();

  svtkCamera* camera = ren1->GetActiveCamera();
  camera->ParallelProjectionOff();
  camera->SetFocalPoint(33, 33, 33);
  camera->SetPosition(43, 38, 61);
  camera->SetViewUp(0, 1, 0);
  camera->SetViewAngle(20);
  camera->SetClippingRange(0.1, 135);
  camera->SetEyeAngle(2);

  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // For now we are just checking to make sure that the mapper does not crash.
  // Maybe in the future we will do an image comparison.
#if 0
  if ((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  else
  {
    return 1;
  }
#else
  svtkGenericWarningMacro("This test will always pass.");
  return 0;
#endif
}
