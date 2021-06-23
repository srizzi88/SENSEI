/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorByStringArrayDefaultLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include <svtkActor.h>
#include <svtkCellData.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSphereSource.h>
#include <svtkStdString.h>
#include <svtkStringArray.h>

int TestColorByStringArrayDefaultLookupTable(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sphere;
  sphere->Update();

  svtkNew<svtkPolyData> polydata;
  polydata->ShallowCopy(sphere->GetOutput());

  // Set up string array associated with cells
  svtkNew<svtkStringArray> sArray;
  char arrayName[] = "string type";
  sArray->SetName(arrayName);
  sArray->SetNumberOfComponents(1);
  sArray->SetNumberOfTuples(polydata->GetNumberOfCells());

  svtkVariant strings[5];
  strings[0] = "violin";
  strings[1] = "viola";
  strings[2] = "cello";
  strings[3] = "bass";
  strings[4] = "double bass";

  // Round-robin assignment of color strings
  for (int i = 0; i < polydata->GetNumberOfCells(); ++i)
  {
    sArray->SetValue(i, strings[i % 5].ToString());
  }

  svtkCellData* cd = polydata->GetCellData();
  cd->AddArray(sArray);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputDataObject(polydata);
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray(arrayName);

  // Direct coloring shouldn't be possible with string arrays, so we
  // enable direct scalars to test that the string arrays get mapped
  // despite the color mode setting being direct scalars.
  mapper->SetColorModeToDirectScalars();

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
