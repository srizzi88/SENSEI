// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SLACReaderQuadratic.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSLACReader.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTestUtilities.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int SLACReaderQuadratic(int argc, char* argv[])
{
  // Set up reader.
  SVTK_CREATE(svtkSLACReader, reader);

  char* meshFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SLAC/ll-9cell-f523/ll-9cell-f523.ncdf");
  char* modeFileName = svtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/SLAC/ll-9cell-f523/mode0.l0.R2.457036E+09I2.778314E+04.m3");
  reader->SetMeshFileName(meshFileName);
  delete[] meshFileName;
  reader->AddModeFileName(modeFileName);
  delete[] modeFileName;

  reader->ReadInternalVolumeOff();
  reader->ReadExternalSurfaceOn();
  reader->ReadMidpointsOn();

  // Extract geometry that we can render.
  SVTK_CREATE(svtkCompositeDataGeometryFilter, geometry);
  geometry->SetInputConnection(reader->GetOutputPort(svtkSLACReader::SURFACE_OUTPUT));

  // Set up rendering stuff.
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->ColorByArrayComponent("bfield", 1);
  mapper->UseLookupTableScalarRangeOff();
  mapper->SetScalarRange(-1e-08, 1e-08);

  SVTK_CREATE(svtkLookupTable, lut);
  lut->SetHueRange(0.66667, 0.0);
  mapper->SetLookupTable(lut);

  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);

  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(actor);
  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.75, 0.0, 0.7);
  camera->SetFocalPoint(0.0, 0.0, 0.7);
  camera->SetViewUp(0.0, 1.0, 0.0);

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetSize(600, 150);
  renwin->AddRenderer(renderer);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();

  // Change the time to test the periodic mode interpolation.
  geometry->UpdateInformation();
  geometry->GetOutputInformation(0)->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 3e-10);
  renwin->Render();

  // Do the test comparison.
  int retVal = svtkRegressionTestImage(renwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return (retVal == svtkRegressionTester::PASSED) ? 0 : 1;
}
