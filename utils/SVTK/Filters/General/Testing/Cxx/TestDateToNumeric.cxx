/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTestUtilities.h"
#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkDateToNumeric.h>
#include <svtkNew.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkXMLPolyDataReader.h>

int TestDateToNumeric(int argc, char* argv[])
{
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/mine_with_dates.vtp");

  // Read the data
  svtkNew<svtkXMLPolyDataReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  svtkNew<svtkDateToNumeric> d2n;
  d2n->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(d2n->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetColorModeToMapScalars();
  mapper->SelectColorArray("START_numeric");
  mapper->SetScalarRange(1.5444e9, 1.5921e9);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(.2, .3, .4);
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetMultiSamples(0);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->ResetCamera();
  // renderer->GetActiveCamera()->Azimuth(120);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->GetActiveCamera()->Dolly(6.0);
  renderer->ResetCameraClippingRange();

  // Render and interact
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
