/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestClipLabels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of clipping with svtkLabeledDataMapper
// .SECTION Description
// this program tests that clipping planes affect labels

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkCellCenters.h"
#include "svtkIdFilter.h"
#include "svtkLabeledDataMapper.h"
#include "svtkPlane.h"
#include "svtkPlaneCollection.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelectVisiblePoints.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"

int TestClipLabels(int argc, char* argv[])
{
  // Selecting point/cells within the entire window
  int xmin = 0;
  int ymin = 0;
  int xmax = 400;
  int ymax = 400;

  // Create a sphere and its associated mapper and actor.
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> sphereMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  svtkSmartPointer<svtkActor> sphereActor = svtkSmartPointer<svtkActor>::New();
  sphereActor->SetMapper(sphereMapper);

  // Generate data arrays containing point and cell ids
  svtkSmartPointer<svtkIdFilter> ids = svtkSmartPointer<svtkIdFilter>::New();
  ids->SetInputConnection(sphere->GetOutputPort());
  ids->PointIdsOn();
  ids->CellIdsOn();
  ids->FieldDataOn();

  // Create the renderer here because svtkSelectVisiblePoints needs it.
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();

  // Create labels for points
  svtkSmartPointer<svtkSelectVisiblePoints> visPts = svtkSmartPointer<svtkSelectVisiblePoints>::New();
  visPts->SetInputConnection(ids->GetOutputPort());
  visPts->SetRenderer(ren1);
  visPts->SelectionWindowOn();
  visPts->SetSelection(xmin, xmax, ymin, ymax);

  // Create the mapper to display the point ids.  Specify the
  // format to use for the labels.  Also create the associated actor.
  svtkSmartPointer<svtkLabeledDataMapper> pointMapper = svtkSmartPointer<svtkLabeledDataMapper>::New();
  pointMapper->SetInputConnection(visPts->GetOutputPort());
  pointMapper->SetLabelModeToLabelFieldData();

  svtkSmartPointer<svtkActor2D> pointLabels = svtkSmartPointer<svtkActor2D>::New();
  pointLabels->SetMapper(pointMapper);

  // Create labels for cells
  svtkSmartPointer<svtkCellCenters> cc = svtkSmartPointer<svtkCellCenters>::New();
  cc->SetInputConnection(ids->GetOutputPort());

  svtkSmartPointer<svtkSelectVisiblePoints> visCells = svtkSmartPointer<svtkSelectVisiblePoints>::New();
  visCells->SetInputConnection(cc->GetOutputPort());
  visCells->SetRenderer(ren1);
  visCells->SelectionWindowOn();
  visCells->SetSelection(xmin, xmax, ymin, ymax);

  // Create the mapper to display the cell ids.  Specify the
  // format to use for the labels.  Also create the associated actor.
  svtkSmartPointer<svtkLabeledDataMapper> cellMapper = svtkSmartPointer<svtkLabeledDataMapper>::New();
  cellMapper->SetInputConnection(visCells->GetOutputPort());
  cellMapper->SetLabelModeToLabelFieldData();
  cellMapper->GetLabelTextProperty()->SetColor(0, 1, 0);

  svtkSmartPointer<svtkActor2D> cellLabels = svtkSmartPointer<svtkActor2D>::New();
  cellLabels->SetMapper(cellMapper);

  // Create the RenderWindow and RenderWindowInteractor
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  renWin->SetSize(xmax, ymax);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer; set the background and size; render
  ren1->AddActor(sphereActor);

  ren1->SetBackground(1, 1, 1);
  renWin->SetSize(xmax, ymax);
  renWin->Render();

  svtkNew<svtkPlane> clipPlane1;
  clipPlane1->SetOrigin(-.1, 0.0, 0.0);
  clipPlane1->SetNormal(1, 0, 0.0);
  svtkNew<svtkPlane> clipPlane2;
  clipPlane2->SetOrigin(0.1, 0.0, 0.0);
  clipPlane2->SetNormal(-1, 0, 0.0);

  svtkNew<svtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1);
  clipPlaneCollection->AddItem(clipPlane2);
  sphereMapper->SetClippingPlanes(clipPlaneCollection);
  pointMapper->SetClippingPlanes(clipPlaneCollection);
  cellMapper->SetClippingPlanes(clipPlaneCollection);
  ren1->AddActor2D(pointLabels);
  ren1->AddActor2D(cellLabels);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
