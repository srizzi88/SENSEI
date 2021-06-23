/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorByStringArrayDefaultLookupTable2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include <svtkActor2D.h>
#include <svtkCellData.h>
#include <svtkDiskSource.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper2D.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkStdString.h>
#include <svtkStringArray.h>

int TestColorByStringArrayDefaultLookupTable2D(int argc, char* argv[])
{
  svtkNew<svtkDiskSource> disk;
  disk->SetInnerRadius(0.0);
  disk->SetCircumferentialResolution(32);
  disk->Update();

  svtkNew<svtkPolyData> polydata;
  polydata->ShallowCopy(disk->GetOutput());

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

  // Round-robin assignment of string strings
  for (int i = 0; i < polydata->GetNumberOfCells(); ++i)
  {
    sArray->SetValue(i, strings[i % 5].ToString());
  }

  svtkCellData* cd = polydata->GetCellData();
  cd->AddArray(sArray);

  svtkNew<svtkCoordinate> pCoord;
  pCoord->SetCoordinateSystemToWorld();

  svtkNew<svtkCoordinate> coord;
  coord->SetCoordinateSystemToNormalizedViewport();
  coord->SetReferenceCoordinate(pCoord);

  svtkNew<svtkPolyDataMapper2D> mapper;
  mapper->SetInputDataObject(polydata);
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUseCellFieldData();

  mapper->ColorByArrayComponent(arrayName, -1);
  mapper->SetTransformCoordinate(coord);

  svtkNew<svtkActor2D> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
