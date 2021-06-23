// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SLACParticleReader.cxx

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
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSLACParticleReader.h"
#include "svtkSLACReader.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTestUtilities.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include <sstream>

int SLACParticleReader(int argc, char* argv[])
{
  char* meshFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SLAC/pic-example/mesh.ncdf");
  char* modeFileNamePattern =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SLAC/pic-example/fields_%d.mod");
  char* particleFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SLAC/pic-example/particles_5.ncdf");

  // Set up mesh reader.
  SVTK_CREATE(svtkSLACReader, meshReader);
  meshReader->SetMeshFileName(meshFileName);
  delete[] meshFileName;

  size_t modeFileNameLength = strlen(modeFileNamePattern) + 10;
  char* modeFileName = new char[modeFileNameLength];
  for (int i = 0; i < 9; i++)
  {
    snprintf(modeFileName, modeFileNameLength, modeFileNamePattern, i);
    meshReader->AddModeFileName(modeFileName);
  }
  delete[] modeFileName;
  delete[] modeFileNamePattern;

  meshReader->ReadInternalVolumeOn();
  meshReader->ReadExternalSurfaceOff();
  meshReader->ReadMidpointsOff();

  // Extract geometry that we can render.
  SVTK_CREATE(svtkCompositeDataGeometryFilter, geometry);
  geometry->SetInputConnection(meshReader->GetOutputPort(svtkSLACReader::VOLUME_OUTPUT));

  // Set up particle reader.
  SVTK_CREATE(svtkSLACParticleReader, particleReader);
  particleReader->SetFileName(particleFileName);
  delete[] particleFileName;

  // Set up rendering stuff.
  SVTK_CREATE(svtkPolyDataMapper, meshMapper);
  meshMapper->SetInputConnection(geometry->GetOutputPort());
  meshMapper->SetScalarModeToUsePointFieldData();
  meshMapper->ColorByArrayComponent("efield", 2);
  meshMapper->UseLookupTableScalarRangeOff();
  meshMapper->SetScalarRange(1.0, 1e+05);

  SVTK_CREATE(svtkLookupTable, lut);
  lut->SetHueRange(0.66667, 0.0);
  lut->SetScaleToLog10();
  meshMapper->SetLookupTable(lut);

  SVTK_CREATE(svtkActor, meshActor);
  meshActor->SetMapper(meshMapper);
  meshActor->GetProperty()->FrontfaceCullingOn();

  SVTK_CREATE(svtkPolyDataMapper, particleMapper);
  particleMapper->SetInputConnection(particleReader->GetOutputPort());
  particleMapper->ScalarVisibilityOff();

  SVTK_CREATE(svtkActor, particleActor);
  particleActor->SetMapper(particleMapper);

  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(meshActor);
  renderer->AddActor(particleActor);
  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.2, 0.05, 0.0);
  camera->SetFocalPoint(0.0, 0.05, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetSize(300, 200);
  renwin->AddRenderer(renderer);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();

  double time = particleReader->GetOutput()->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
  cout << "Time in particle reader: " << time << endl;

  // Change the time to test the time step field load and to have the field
  // match the particles in time.
  geometry->UpdateInformation();
  geometry->GetOutputInformation(0)->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
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
