/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMathTextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTextActor.h"

#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestingInteractor.h"
#include "svtkTextProperty.h"
#include "svtkUnsignedCharArray.h"

#include <sstream>

namespace svtkTestMathTextActor
{
void setupTextActor(svtkTextActor* actor, svtkPolyData* anchor)
{
  svtkTextProperty* p = actor->GetTextProperty();
  std::ostringstream label;
  label << p->GetVerticalJustificationAsString()[0] << p->GetJustificationAsString()[0] << " "
        << "$\\theta = " << p->GetOrientation() << "$";
  actor->SetInput(label.str().c_str());

  // Add the anchor point:
  double* pos = actor->GetPosition();
  double* col = p->GetColor();
  svtkIdType ptId = anchor->GetPoints()->InsertNextPoint(pos[0], pos[1], 0.);
  anchor->GetVerts()->InsertNextCell(1, &ptId);
  anchor->GetCellData()->GetScalars()->InsertNextTuple4(
    col[0] * 255, col[1] * 255, col[2] * 255, 255);
}
} // end namespace svtkTestMathTextActor3D

//----------------------------------------------------------------------------
int TestMathTextActor(int, char*[])
{
  using namespace svtkTestMathTextActor;
  svtkNew<svtkRenderer> ren;

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
      svtkNew<svtkTextActor> actor;
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
      actor->GetTextProperty()->SetFontSize(22);
      actor->GetTextProperty()->SetOrientation(45.0 * (3 * row + col));
      actor->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .26);
      actor->GetTextProperty()->SetBackgroundColor(0., 1. - col * .26, 1. - row * .26);
      actor->GetTextProperty()->SetBackgroundOpacity(0.25);
      actor->GetTextProperty()->SetFrame((row + col) % 9 == 0);
      actor->GetTextProperty()->SetFrameColor(
        col > 0 ? 1. : 0., col == 1 ? 1. : 0., col < 2 ? 1. : 0.);
      actor->GetTextProperty()->SetFrameWidth(1);
      actor->SetPosition(x[col], y[row]);
      setupTextActor(actor, anchors);
      ren->AddActor(actor);
    }
  }

  svtkNew<svtkPolyDataMapper2D> anchorMapper;
  anchorMapper->SetInputData(anchors);
  svtkNew<svtkActor2D> anchorActor;
  anchorActor->SetMapper(anchorMapper);
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor(anchorActor);

  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(width, height);

  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
