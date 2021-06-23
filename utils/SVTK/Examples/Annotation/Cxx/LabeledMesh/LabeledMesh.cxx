/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LabeledMesh.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This example was translated into C++ from its TCL counterpart
// (SVTK/Examples/Annotation/Tcl/labeledMesh.tcl) by Jake Nickel from
// the University of Iowa.  It demonstrates the use of svtkLabeledDataMapper.
// This class is used for displaying numerical data from an underlying data
// set.  In the case of this example, the underlying data are the point and
// cell ids.

// First we include the necessary header files.
#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkCellArray.h"
#include "svtkCellCenters.h"
#include "svtkIdFilter.h"
#include "svtkLabeledDataMapper.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelectVisiblePoints.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTextProperty.h"

static int xLength;
static int yLength;
static svtkSmartPointer<svtkSelectVisiblePoints> visPts;
static svtkSmartPointer<svtkSelectVisiblePoints> visCells;
static svtkSmartPointer<svtkPoints> pts;
static svtkSmartPointer<svtkRenderWindow> renWin;

// Create a procedure to draw the selection window at each location it
// is moved to.
void PlaceWindow(int xmin, int ymin)
{
  int xmax = xmin + xLength;
  int ymax = ymin + yLength;

  visPts->SetSelection(xmin, xmax, ymin, ymax);
  visCells->SetSelection(xmin, xmax, ymin, ymax);

  pts->InsertPoint(0, xmin, ymin, 0);
  pts->InsertPoint(1, xmin, ymin, 0);
  pts->InsertPoint(2, xmin, ymin, 0);
  pts->InsertPoint(3, xmin, ymin, 0);

  // Call Modified because InsertPoints does not modify svtkPoints
  // (for performance reasons).
  pts->Modified();

  renWin->Render();
}

// Create a procedure to move the selection window across the data set.
void MoveWindow()
{
  for (int y = 100; y < 300; y += 25)
  {
    for (int x = 100; x < 300; x += 25)
    {
      PlaceWindow(x, y);
    }
  }
}

int main(int, char*[])
{
  // Create a selection window.  We will display the point and cell ids that
  // lie within this window.
  int xmin = 200;
  xLength = 100;
  int xmax = xmin + xLength;
  int ymin = 200;
  yLength = 100;
  int ymax = ymin + yLength;

  pts = svtkSmartPointer<svtkPoints>::New();
  pts->InsertPoint(0, xmin, ymin, 0);
  pts->InsertPoint(1, xmax, ymin, 0);
  pts->InsertPoint(2, xmax, ymax, 0);
  pts->InsertPoint(3, xmin, ymax, 0);

  svtkSmartPointer<svtkCellArray> rect = svtkSmartPointer<svtkCellArray>::New();
  rect->InsertNextCell(5);
  rect->InsertCellPoint(0);
  rect->InsertCellPoint(1);
  rect->InsertCellPoint(2);
  rect->InsertCellPoint(3);
  rect->InsertCellPoint(0);

  svtkSmartPointer<svtkPolyData> selectRect = svtkSmartPointer<svtkPolyData>::New();
  selectRect->SetPoints(pts);
  selectRect->SetLines(rect);

  svtkSmartPointer<svtkPolyDataMapper2D> rectMapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  rectMapper->SetInputData(selectRect);

  svtkSmartPointer<svtkActor2D> rectActor = svtkSmartPointer<svtkActor2D>::New();
  rectActor->SetMapper(rectMapper);

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
  visPts = svtkSmartPointer<svtkSelectVisiblePoints>::New();
  visPts->SetInputConnection(ids->GetOutputPort());
  visPts->SetRenderer(ren1);
  visPts->SelectionWindowOn();
  visPts->SetSelection(xmin, xmin + xLength, ymin, ymin + yLength);

  // Create the mapper to display the point ids.  Specify the
  // format to use for the labels.  Also create the associated actor.
  svtkSmartPointer<svtkLabeledDataMapper> ldm = svtkSmartPointer<svtkLabeledDataMapper>::New();
  ldm->SetInputConnection(visPts->GetOutputPort());
  ldm->SetLabelModeToLabelFieldData();

  svtkSmartPointer<svtkActor2D> pointLabels = svtkSmartPointer<svtkActor2D>::New();
  pointLabels->SetMapper(ldm);

  // Create labels for cells
  svtkSmartPointer<svtkCellCenters> cc = svtkSmartPointer<svtkCellCenters>::New();
  cc->SetInputConnection(ids->GetOutputPort());

  visCells = svtkSmartPointer<svtkSelectVisiblePoints>::New();
  visCells->SetInputConnection(cc->GetOutputPort());
  visCells->SetRenderer(ren1);
  visCells->SelectionWindowOn();
  visCells->SetSelection(xmin, xmin + xLength, ymin, ymin + yLength);

  // Create the mapper to display the cell ids.  Specify the
  // format to use for the labels.  Also create the associated actor.
  svtkSmartPointer<svtkLabeledDataMapper> cellMapper = svtkSmartPointer<svtkLabeledDataMapper>::New();
  cellMapper->SetInputConnection(visCells->GetOutputPort());
  cellMapper->SetLabelModeToLabelFieldData();
  cellMapper->GetLabelTextProperty()->SetColor(0, 1, 0);

  svtkSmartPointer<svtkActor2D> cellLabels = svtkSmartPointer<svtkActor2D>::New();
  cellLabels->SetMapper(cellMapper);

  // Create the RenderWindow and RenderWindowInteractor
  renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer; set the background and size; render
  ren1->AddActor(sphereActor);
  ren1->AddActor2D(rectActor);
  ren1->AddActor2D(pointLabels);
  ren1->AddActor2D(cellLabels);

  ren1->SetBackground(1, 1, 1);
  renWin->SetSize(500, 500);
  renWin->Render();

  // Move the selection window across the data set.
  MoveWindow();

  // Put the selection window in the center of the render window.
  // This works because the xmin = ymin = 200, xLength = yLength = 100, and
  // the render window size is 500 x 500.
  PlaceWindow(xmin, ymin);

  iren->Initialize();
  iren->Start();

  return 0;
}
