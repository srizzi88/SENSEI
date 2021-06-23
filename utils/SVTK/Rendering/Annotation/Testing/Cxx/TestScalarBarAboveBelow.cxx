/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarBarAboveBelow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarBarActor.h"

int TestScalarBarAboveBelow(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int resolution = 3;
  svtkNew<svtkPlaneSource> plane;
  plane->SetXResolution(resolution);
  plane->SetYResolution(resolution);

  svtkNew<svtkDoubleArray> cellData;
  for (int i = 0; i < resolution * resolution; i++)
  {
    cellData->InsertNextValue(i);
  }

  plane->Update(); // Force an update so we can set cell data
  plane->GetOutput()->GetCellData()->SetScalars(cellData);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());
  mapper->SetScalarRange(1, 7);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkScalarsToColors* stc = mapper->GetLookupTable();
  svtkLookupTable* lut = svtkLookupTable::SafeDownCast(stc);
  lut->SetUseBelowRangeColor(true);
  lut->SetUseAboveRangeColor(true);
  lut->SetNumberOfColors(7);

  svtkNew<svtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable(stc);
  scalarBar->SetDrawBelowRangeSwatch(true);
  scalarBar->SetDrawAboveRangeSwatch(true);

  svtkNew<svtkScalarBarActor> scalarBar2;
  scalarBar2->SetLookupTable(stc);
  scalarBar2->SetDrawBelowRangeSwatch(true);
  scalarBar2->SetOrientationToHorizontal();
  scalarBar2->SetWidth(0.5);
  scalarBar2->SetHeight(0.15);
  scalarBar2->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar2->GetPositionCoordinate()->SetValue(.05, .8);

  svtkNew<svtkScalarBarActor> scalarBar3;
  scalarBar3->SetLookupTable(stc);
  scalarBar3->SetDrawAboveRangeSwatch(true);
  scalarBar3->SetOrientationToHorizontal();
  scalarBar3->SetWidth(0.5);
  scalarBar3->SetHeight(0.15);
  scalarBar3->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar3->GetPositionCoordinate()->SetValue(.05, .2);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderer->AddActor(actor);
  renderer->AddActor(scalarBar);
  renderer->AddActor(scalarBar2);
  renderer->AddActor(scalarBar3);
  renderer->SetBackground(.5, .5, .5);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
