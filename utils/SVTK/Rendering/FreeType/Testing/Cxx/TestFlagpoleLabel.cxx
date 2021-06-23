/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkFlagpoleLabel.h"

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

namespace
{
void setupFlagpoleText(svtkFlagpoleLabel* actor, svtkPolyData* anchor)
{
  svtkTextProperty* p = actor->GetTextProperty();
  std::ostringstream label;
  label << "HAlign: " << p->GetJustificationAsString() << "\n"
        << "VAlign: " << p->GetVerticalJustificationAsString();
  actor->SetInput(label.str().c_str());

  // Add the anchor point:
  double* pos = actor->GetTopPosition();
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

}

//----------------------------------------------------------------------------
int TestFlagpoleLabel(int, char*[])
{
  svtkNew<svtkRenderer> ren;
  ren->UseDepthPeelingOn();

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
      svtkNew<svtkFlagpoleLabel> actor;
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
      actor->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .26);
      actor->GetTextProperty()->SetBackgroundColor(0., 1. - col * .26, 1. - row * .26);
      actor->GetTextProperty()->SetFrameColor(actor->GetTextProperty()->GetBackgroundColor());
      actor->GetTextProperty()->SetBackgroundOpacity(0.85);
      actor->SetBasePosition(x[col], y[row] - 50.0, 0.);
      actor->SetTopPosition(x[col], y[row] + 50.0, 0.);
      setupFlagpoleText(actor, anchors);
      ren->AddActor(actor);
    }
  }

  svtkNew<svtkPolyDataMapper> anchorMapper;
  anchorMapper->SetInputData(anchors);
  svtkNew<svtkActor> anchorActor;
  anchorActor->SetMapper(anchorMapper);
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor(anchorActor);

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
  ren->GetActiveCamera()->Azimuth(15.);
  ren->GetActiveCamera()->Roll(5.);
  ren->ResetCameraClippingRange();
  win->SetSize(width, height);

  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
