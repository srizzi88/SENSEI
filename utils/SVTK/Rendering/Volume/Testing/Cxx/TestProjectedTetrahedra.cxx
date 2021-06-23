/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProjectedTetrahedra.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

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

int TestProjectedTetrahedra(int argc, char* argv[])
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
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderer* ren1 = svtkRenderer::New();
  renWin->AddRenderer(ren1);
  ren1->Delete();

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->SetDesiredUpdateRate(3);
  renWin->Delete();

  // check for driver support
  renWin->Render();
  svtkProjectedTetrahedraMapper* volumeMapper = svtkProjectedTetrahedraMapper::New();
  if (!volumeMapper->IsSupported(renWin))
  {
    volumeMapper->Delete();
    iren->Delete();
    svtkGenericWarningMacro("Projected tetrahedra is not supported. Skipping tests.");
    return 0;
  }

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  svtkStdString filename;
  filename = data_root;
  filename += "/Data/ironProt.svtk";
  cout << "Loading " << filename.c_str() << endl;
  svtkStructuredPointsReader* reader = svtkStructuredPointsReader::New();
  reader->SetFileName(filename.c_str());

  // Create a reader for the other data that will be contoured and
  // displayed as a polygonal mesh.
  filename = data_root;
  filename += "/Data/neghip.slc";
  cout << "Loading " << filename.c_str() << endl;
  svtkSLCReader* reader2 = svtkSLCReader::New();
  reader2->SetFileName(filename.c_str());

  // Convert from svtkImageData to svtkUnstructuredGrid.
  // Remove any cells where all values are below 80.
  svtkThreshold* thresh = svtkThreshold::New();
  thresh->ThresholdByUpper(80);
  thresh->AllScalarsOff();
  thresh->SetInputConnection(reader->GetOutputPort());

  // Make sure we have only tetrahedra.
  svtkDataSetTriangleFilter* trifilter = svtkDataSetTriangleFilter::New();
  trifilter->SetInputConnection(thresh->GetOutputPort());

  // Create transfer mapping scalar value to opacity.
  svtkPiecewiseFunction* opacityTransferFunction = svtkPiecewiseFunction::New();
  opacityTransferFunction->AddPoint(80.0, 0.0);
  opacityTransferFunction->AddPoint(120.0, 0.2);
  opacityTransferFunction->AddPoint(255.0, 0.2);

  // Create transfer mapping scalar value to color.
  svtkColorTransferFunction* colorTransferFunction = svtkColorTransferFunction::New();
  colorTransferFunction->AddRGBPoint(80.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(120.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(160.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(200.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 1.0, 1.0);

  // The property describes how the data will look.
  svtkVolumeProperty* volumeProperty = svtkVolumeProperty::New();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper that renders the volume data.
  volumeMapper->SetInputConnection(trifilter->GetOutputPort());

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume.
  svtkVolume* volume = svtkVolume::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Contour the second dataset.
  svtkContourFilter* contour = svtkContourFilter::New();
  contour->SetValue(0, 80);
  contour->SetInputConnection(reader2->GetOutputPort());

  // Create a mapper for the polygonal data.
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->ScalarVisibilityOff();

  // Create an actor for the polygonal data.
  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  ren1->AddViewProp(actor);
  ren1->AddVolume(volume);

  renWin->SetSize(300, 300);

  ren1->ResetCamera();
  ren1->GetActiveCamera()->Azimuth(20.0);
  ren1->GetActiveCamera()->Elevation(10.0);
  ren1->GetActiveCamera()->Zoom(1.5);

  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Clean up.
  iren->Delete();
  reader->Delete();
  reader2->Delete();
  thresh->Delete();
  trifilter->Delete();
  opacityTransferFunction->Delete();
  colorTransferFunction->Delete();
  volumeProperty->Delete();
  volumeMapper->Delete();
  volume->Delete();
  contour->Delete();
  mapper->Delete();
  actor->Delete();

  if ((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
