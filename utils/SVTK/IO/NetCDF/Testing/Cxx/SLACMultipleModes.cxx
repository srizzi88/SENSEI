// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SLACReaderLinear.cxx

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
#include "svtkInformationVector.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSLACReader.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTestUtilities.h"

int SLACMultipleModes(int argc, char* argv[])
{
  // Set up reader.
  svtkNew<svtkSLACReader> reader;

  char* meshFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SLAC/pillbox/Pillbox3TenDSlice.ncdf");
  char* modeFileName0 = svtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/SLAC/pillbox/omega3p.l0.m0000.1.3138186e+09.mod");
  char* modeFileName1 = svtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/SLAC/pillbox/omega3p.l0.m0001.1.3138187e+09.mod");
  char* modeFileName2 = svtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/SLAC/pillbox/omega3p.l0.m0002.1.3138189e+09.mod");
  reader->SetMeshFileName(meshFileName);
  delete[] meshFileName;
  reader->AddModeFileName(modeFileName0);
  delete[] modeFileName0;
  reader->AddModeFileName(modeFileName1);
  delete[] modeFileName1;
  reader->AddModeFileName(modeFileName2);
  delete[] modeFileName2;

  reader->ReadInternalVolumeOff();
  reader->ReadExternalSurfaceOn();
  reader->ReadMidpointsOff();

  reader->UpdateInformation();
  double period = reader->GetExecutive()
                    ->GetOutputInformation(svtkSLACReader::SURFACE_OUTPUT)
                    ->Get(svtkStreamingDemandDrivenPipeline::TIME_RANGE())[1];

  reader->ResetPhaseShifts();
  reader->SetPhaseShift(1, 0.5 * period);
  reader->SetPhaseShift(2, 0.5 * period);

  reader->ResetFrequencyScales();
  reader->SetFrequencyScale(0, 0.75);
  reader->SetFrequencyScale(1, 1.5);

  // Extract geometry that we can render.
  svtkNew<svtkCompositeDataGeometryFilter> geometry;
  geometry->SetInputConnection(reader->GetOutputPort(svtkSLACReader::SURFACE_OUTPUT));

  // Set up rendering stuff.
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->ColorByArrayComponent("efield", 2);
  mapper->UseLookupTableScalarRangeOff();
  mapper->SetScalarRange(-240, 240);

  svtkNew<svtkLookupTable> lut;
  lut->SetHueRange(0.66667, 0.0);
  mapper->SetLookupTable(lut);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.75, 0.0, 0.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  svtkNew<svtkRenderWindow> renwin;
  renwin->SetSize(600, 150);
  renwin->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin);
  renwin->Render();

  // Change the time to offset the phase.
  geometry->UpdateInformation();
  geometry->GetOutputInformation(0)->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 0.5 * period);

  // Do the test comparison.
  int retVal = svtkRegressionTestImage(renwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return (retVal == svtkRegressionTester::PASSED) ? 0 : 1;
}
