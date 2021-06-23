/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScatterPlotMatrixVehicles.cxx

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
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkScatterPlotMatrix.h"
#include "svtkTable.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
int TestScatterPlotMatrixVehicles(int argc, char* argv[])
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
  svtkNew<svtkScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix);

  // Set the scatter plot matrix up to analyze all columns in the table.
  matrix->SetInput(reader->GetOutput());

  // Add a title
  matrix->SetTitle("Vehicles");
  svtkTextProperty* prop = matrix->GetTitleProperties();
  prop->SetJustification(1);
  prop->SetColor(0, 0, 0);
  prop->SetFontSize(15);
  prop->BoldOn();

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
