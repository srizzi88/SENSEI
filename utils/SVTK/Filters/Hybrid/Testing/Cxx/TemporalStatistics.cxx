/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TemporalStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.
*/

#include "svtkCamera.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTemporalFractal.h"
#include "svtkTemporalStatistics.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

//-----------------------------------------------------------------------------
static void ShowResult(svtkRenderer* renderer, svtkAlgorithmOutput* input, const char* arrayName);

//-------------------------------------------------------------------------
int TemporalStatistics(int argc, char* argv[])
{
  // We have to use a composite pipeline to handle these composite data
  // structures.
  SVTK_CREATE(svtkCompositeDataPipeline, prototype);
  svtkAlgorithm::SetDefaultExecutivePrototype(prototype);

  // create temporal fractals
  SVTK_CREATE(svtkTemporalFractal, source);
  source->SetMaximumLevel(3);
  source->DiscreteTimeStepsOn();
  // source->GenerateRectilinearGridsOn();
  source->AdaptiveSubdivisionOff();

  SVTK_CREATE(svtkTemporalStatistics, statistics);
  statistics->SetInputConnection(source->GetOutputPort());

  // Convert the hierarchical information into render-able polydata.
  SVTK_CREATE(svtkCompositeDataGeometryFilter, geometry);
  geometry->SetInputConnection(statistics->GetOutputPort());

  SVTK_CREATE(svtkRenderWindow, renWin);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);

  SVTK_CREATE(svtkRenderer, avgRenderer);
  avgRenderer->SetViewport(0.0, 0.5, 0.5, 1.0);
  ShowResult(avgRenderer, geometry->GetOutputPort(), "Fractal Volume Fraction_average");
  renWin->AddRenderer(avgRenderer);

  SVTK_CREATE(svtkRenderer, minRenderer);
  minRenderer->SetViewport(0.5, 0.5, 1.0, 1.0);
  ShowResult(minRenderer, geometry->GetOutputPort(), "Fractal Volume Fraction_minimum");
  renWin->AddRenderer(minRenderer);

  SVTK_CREATE(svtkRenderer, maxRenderer);
  maxRenderer->SetViewport(0.0, 0.0, 0.5, 0.5);
  ShowResult(maxRenderer, geometry->GetOutputPort(), "Fractal Volume Fraction_maximum");
  renWin->AddRenderer(maxRenderer);

  SVTK_CREATE(svtkRenderer, stddevRenderer);
  stddevRenderer->SetViewport(0.5, 0.0, 1.0, 0.5);
  ShowResult(stddevRenderer, geometry->GetOutputPort(), "Fractal Volume Fraction_stddev");
  renWin->AddRenderer(stddevRenderer);

  renWin->SetSize(450, 400);
  iren->SetRenderWindow(renWin);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  svtkAlgorithm::SetDefaultExecutivePrototype(nullptr);
  return !retVal;
}

//-----------------------------------------------------------------------------
static void ShowResult(svtkRenderer* renderer, svtkAlgorithmOutput* input, const char* arrayName)
{
  // Set up rendering classes
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(input);
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray(arrayName);

  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);

  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.5);
}
