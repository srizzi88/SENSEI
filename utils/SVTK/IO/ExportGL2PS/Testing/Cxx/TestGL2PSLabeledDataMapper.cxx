/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSLabeledDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGL2PSExporter.h"
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellCenters.h"
#include "svtkIdFilter.h"
#include "svtkLabeledDataMapper.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelectVisiblePoints.h"
#include "svtkSphereSource.h"
#include "svtkTestingInteractor.h"
#include "svtkTextProperty.h"

// This test is adapted from labeledMesh.py to test GL2PS exporting of selection
// labels.
int TestGL2PSLabeledDataMapper(int, char*[])
{
  // Selection rectangle:
  double xmin = 100.;
  double xmax = 400.;
  double ymin = 100.;
  double ymax = 400.;

  svtkNew<svtkPoints> pts;
  pts->InsertPoint(0, xmin, ymin, 0.);
  pts->InsertPoint(1, xmax, ymin, 0.);
  pts->InsertPoint(2, xmax, ymax, 0.);
  pts->InsertPoint(3, xmin, ymax, 0.);

  svtkNew<svtkCellArray> rect;
  rect->InsertNextCell(5);
  rect->InsertCellPoint(0);
  rect->InsertCellPoint(1);
  rect->InsertCellPoint(2);
  rect->InsertCellPoint(3);
  rect->InsertCellPoint(0);

  svtkNew<svtkPolyData> selectRect;
  selectRect->SetPoints(pts);
  selectRect->SetLines(rect);

  svtkNew<svtkPolyDataMapper2D> rectMapper;
  svtkNew<svtkActor2D> rectActor;
  rectMapper->SetInputData(selectRect);
  rectActor->SetMapper(rectMapper);

  // Create sphere
  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkPolyDataMapper> sphereMapper;
  svtkNew<svtkActor> sphereActor;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  // Generate ids for labeling
  svtkNew<svtkIdFilter> ids;
  ids->SetInputConnection(sphere->GetOutputPort());
  ids->PointIdsOn();
  ids->CellIdsOn();
  ids->FieldDataOn();

  // Create labels for points
  svtkNew<svtkSelectVisiblePoints> visPts;
  visPts->SetInputConnection(ids->GetOutputPort());
  visPts->SelectionWindowOn();
  visPts->SetSelection(
    static_cast<int>(xmin), static_cast<int>(xmax), static_cast<int>(ymin), static_cast<int>(ymax));

  svtkNew<svtkLabeledDataMapper> ldm;
  ldm->SetInputConnection(visPts->GetOutputPort());
  ldm->SetLabelModeToLabelFieldData();

  svtkNew<svtkActor2D> pointLabels;
  pointLabels->SetMapper(ldm);

  // Create labels for cells:
  svtkNew<svtkCellCenters> cc;
  cc->SetInputConnection(ids->GetOutputPort());

  svtkNew<svtkSelectVisiblePoints> visCells;
  visCells->SetInputConnection(cc->GetOutputPort());
  visCells->SelectionWindowOn();
  visCells->SetSelection(
    static_cast<int>(xmin), static_cast<int>(xmax), static_cast<int>(ymin), static_cast<int>(ymax));

  svtkNew<svtkLabeledDataMapper> cellMapper;
  cellMapper->SetInputConnection(visCells->GetOutputPort());
  cellMapper->SetLabelModeToLabelFieldData();
  cellMapper->GetLabelTextProperty()->SetColor(0., 1., 0.);

  svtkNew<svtkActor2D> cellLabels;
  cellLabels->SetMapper(cellMapper);

  // Rendering setup
  svtkNew<svtkRenderer> ren;
  visPts->SetRenderer(ren);
  visCells->SetRenderer(ren);
  ren->AddActor(sphereActor);
  ren->AddActor2D(rectActor);
  ren->AddActor2D(pointLabels);
  ren->AddActor2D(cellLabels);
  ren->SetBackground(1., 1., 1.);
  ren->GetActiveCamera()->Zoom(.55);

  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(ren);
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);
  renWin->Render();

  svtkNew<svtkGL2PSExporter> exp;
  exp->SetRenderWindow(renWin);
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetPS3Shading(0);
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();
  exp->Write3DPropsAsRasterImageOff();
  exp->SetTextAsPath(true);

  std::string fileprefix =
    svtkTestingInteractor::TempDirectory + std::string("/TestGL2PSLabeledDataMapper");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
