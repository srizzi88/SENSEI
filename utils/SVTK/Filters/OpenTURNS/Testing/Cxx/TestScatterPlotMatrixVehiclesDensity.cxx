/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScatterPlotMatrixVehiclesDensity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkDelimitedTextReader.h"
#include "svtkNew.h"
#include "svtkOTScatterPlotMatrix.h"
#include "svtkPlotPoints.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTable.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestScatterPlotMatrixVehiclesDensity(int argc, char* argv[])
{
  // Get the file name, and read the CSV file.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vehicle_data.csv");
  svtkNew<svtkDelimitedTextReader> reader;
  reader->SetFileName(fname);
  reader->SetHaveHeaders(true);
  reader->SetDetectNumericColumns(true);
  delete[] fname;
  reader->Update();

  // Set up a 2D scene, add a chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  svtkNew<svtkOTScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix);

  // Set the scatter plot matrix up to analyze all columns in the table.
  matrix->SetInput(reader->GetOutput());
  matrix->SetPlotMarkerStyle(svtkOTScatterPlotMatrix::ACTIVEPLOT, svtkPlotPoints::NONE);
  matrix->SetDensityMapVisibility(svtkOTScatterPlotMatrix::ACTIVEPLOT, true);
  matrix->SetDensityMapVisibility(svtkOTScatterPlotMatrix::SCATTERPLOT, true);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();

  view->GetRenderWindow()->Render();
  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Start();
  }

  return !retVal;
}
