/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBillboardTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBillboardTextActor3D.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestingInteractor.h"
#include "svtkTextProperty.h"
#include "svtkUnsignedCharArray.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace svtkTestBillboardTextActor3D
{
void setupBillboardTextActor3D(svtkBillboardTextActor3D* actor, svtkPolyData* anchor)
{
  svtkTextProperty* p = actor->GetTextProperty();
  std::ostringstream label;
  label << "TProp Angle: " << p->GetOrientation() << "\n"
        << "HAlign: " << p->GetJustificationAsString() << "\n"
        << "VAlign: " << p->GetVerticalJustificationAsString();
  actor->SetInput(label.str().c_str());

  // Add the anchor point:
  double* pos = actor->GetPosition();
  double* col = p->GetColor();
  svtkIdType ptId = anchor->GetPoints()->InsertNextPoint(pos[0], pos[1], pos[2]);
  anchor->GetVerts()->InsertNextCell(1, &ptId);
  anchor->GetCellData()->GetScalars()->InsertNextTuple4(
    col[0] * 255, col[1] * 255, col[2] * 255, 255);
}

void setupGrid(svtkPolyData* grid)
{
  double marks[4] = { 0., 200., 400., 600. };
  double thickness = 200.;

  svtkNew<svtkPoints> points;
  grid->SetPoints(points);
  for (int x_i = 0; x_i < 4; ++x_i)
  {
    for (int y_i = 0; y_i < 4; ++y_i)
    {
      points->InsertNextPoint(marks[x_i], marks[y_i], -thickness / 2.);
      points->InsertNextPoint(marks[x_i], marks[y_i], +thickness / 2.);
    }
  }

  std::vector<svtkIdType> quads;

  for (svtkIdType col = 0; col < 4; ++col)
  {
    for (int row = 0; row < 3; ++row)
    {
      // Along y:
      svtkIdType base = 8 * col + 2 * row;
      quads.push_back(base + 0);
      quads.push_back(base + 1);
      quads.push_back(base + 3);
      quads.push_back(base + 2);
    }
  }

  svtkNew<svtkCellArray> cellArray;
  grid->SetPolys(cellArray);
  for (size_t i = 0; i < quads.size(); i += 4)
  {
    grid->InsertNextCell(SVTK_QUAD, 4, &quads[i]);
  }
}

// Test for bug #17233: https://gitlab.kitware.com/svtk/svtk/issues/17233
// The Bounds were not updated when the position changed. Ensure that we aren't
// returning stale bounds after modifying the actor.
bool RegressionTest_17233(svtkBillboardTextActor3D* actor)
{
  double* bounds = actor->GetBounds();
  double origBounds[6] = { bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5] };

  double pos[3];
  actor->GetPosition(pos);
  pos[0] += 50.;
  pos[1] += 50.;
  pos[2] += 50.;
  actor->SetPosition(pos);

  bounds = actor->GetBounds();
  if (std::equal(bounds, bounds + 6, origBounds))
  {
    std::cerr << "Regression for bug #17233: Stale bounds used.\n";
    return false;
  }
  return true;
}

} // end namespace svtkTestBillboardTextActor3D

//----------------------------------------------------------------------------
int TestBillboardTextActor3D(int, char*[])
{
  using namespace svtkTestBillboardTextActor3D;
  svtkNew<svtkRenderer> ren;
  ren->UseDepthPeelingOn();

  // use this to capture one of the text actors for later regression testing:
  svtkBillboardTextActor3D* bbActor = nullptr;

  int width = 600;
  int height = 600;
  int x[3] = { 100, 300, 500 };
  int y[3] = { 100, 300, 500 };

  // Render the anchor points to check alignment:
  svtkNew<svtkPolyData> anchors;
  svtkNew<svtkPoints> points;
  anchors->SetPoints(points);
  svtkNew<svtkCellArray> verts;
  anchors->SetVerts(verts);
  svtkNew<svtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  anchors->GetCellData()->SetScalars(colors);

  for (size_t row = 0; row < 3; ++row)
  {
    for (size_t col = 0; col < 3; ++col)
    {
      svtkNew<svtkBillboardTextActor3D> actor;
      switch (row)
      {
        case 0:
          actor->GetTextProperty()->SetJustificationToRight();
          break;
        case 1:
          actor->GetTextProperty()->SetJustificationToCentered();
          break;
        case 2:
          actor->GetTextProperty()->SetJustificationToLeft();
          break;
      }
      switch (col)
      {
        case 0:
          actor->GetTextProperty()->SetVerticalJustificationToBottom();
          break;
        case 1:
          actor->GetTextProperty()->SetVerticalJustificationToCentered();
          break;
        case 2:
          actor->GetTextProperty()->SetVerticalJustificationToTop();
          break;
      }
      actor->GetTextProperty()->SetFontSize(20);
      actor->GetTextProperty()->SetOrientation(45.0 * (3 * row + col));
      actor->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .26);
      actor->GetTextProperty()->SetBackgroundColor(0., 1. - col * .26, 1. - row * .26);
      actor->GetTextProperty()->SetBackgroundOpacity(0.85);
      actor->SetPosition(x[col], y[row], 0.);
      setupBillboardTextActor3D(actor, anchors);
      ren->AddActor(actor);
      bbActor = actor;
    }
  }

  svtkNew<svtkPolyDataMapper> anchorMapper;
  anchorMapper->SetInputData(anchors);
  svtkNew<svtkActor> anchorActor;
  anchorActor->SetMapper(anchorMapper);
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor(anchorActor);

  // Add some various 'empty' actors to make sure there are no surprises:
  svtkNew<svtkBillboardTextActor3D> nullInputActor;
  nullInputActor->SetInput(nullptr);
  ren->AddActor(nullInputActor);

  svtkNew<svtkBillboardTextActor3D> emptyInputActor;
  emptyInputActor->SetInput("");
  ren->AddActor(emptyInputActor);

  svtkNew<svtkBillboardTextActor3D> spaceActor;
  spaceActor->SetInput(" ");
  ren->AddActor(spaceActor);

  svtkNew<svtkBillboardTextActor3D> tabActor;
  tabActor->SetInput("\t");
  ren->AddActor(tabActor);

  svtkNew<svtkBillboardTextActor3D> newlineActor;
  newlineActor->SetInput("\n");
  ren->AddActor(newlineActor);

  svtkNew<svtkPolyData> grid;
  setupGrid(grid);
  svtkNew<svtkPolyDataMapper> gridMapper;
  gridMapper->SetInputData(grid);
  svtkNew<svtkActor> gridActor;
  gridActor->GetProperty()->SetRepresentationToSurface();
  gridActor->GetProperty()->SetColor(0.6, 0.6, 0.6);
  gridActor->SetMapper(gridMapper);
  ren->AddActor(gridActor);

  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->SetBackground(0.0, 0.0, 0.0);
  ren->GetActiveCamera()->SetPosition(width / 2, height / 2, 1400);
  ren->GetActiveCamera()->SetFocalPoint(width / 2, height / 2, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->GetActiveCamera()->Roll(45.);
  ren->GetActiveCamera()->Elevation(45.);
  ren->ResetCameraClippingRange();
  win->SetSize(width, height);

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  // Now that the image has been rendered, use one of the actors to do
  // regression testing:
  if (!RegressionTest_17233(bbActor))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
