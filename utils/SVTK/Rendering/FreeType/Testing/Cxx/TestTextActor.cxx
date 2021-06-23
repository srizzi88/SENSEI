/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActor.cxx

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
#include "svtkPoints.h"
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

namespace svtkTestTextActor
{
void setupTextActor(svtkTextActor* actor, svtkPolyData* anchor)
{
  svtkTextProperty* p = actor->GetTextProperty();
  std::ostringstream label;
  label << "TProp Angle: " << p->GetOrientation() << "\n"
        << "Actor Angle: " << actor->GetOrientation() << "\n"
        << "HAlign: " << p->GetJustificationAsString() << "\n"
        << "VAlign: " << p->GetVerticalJustificationAsString();
  actor->SetInput(label.str().c_str());

  // Add the anchor point:
  double* pos = actor->GetPosition();
  double* col = p->GetColor();
  svtkIdType ptId = anchor->GetPoints()->InsertNextPoint(pos[0], pos[1], 0.);
  anchor->GetVerts()->InsertNextCell(1, &ptId);
  anchor->GetCellData()->GetScalars()->InsertNextTuple4(
    col[0] * 255, col[1] * 255, col[2] * 255, 255);
}
} // end namespace svtkTestTextActor

//----------------------------------------------------------------------------
int TestTextActor(int, char*[])
{
  using namespace svtkTestTextActor;
  svtkNew<svtkRenderer> ren;

  int width = 600;
  int height = 600;
  int x[3] = { 100, 300, 500 };
  int y[4] = { 100, 233, 366, 500 };

  // Render the anchor points to check alignment:
  svtkNew<svtkPolyData> anchors;
  svtkNew<svtkPoints> points;
  anchors->SetPoints(points);
  svtkNew<svtkCellArray> verts;
  anchors->SetVerts(verts);
  svtkNew<svtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  anchors->GetCellData()->SetScalars(colors);

  for (size_t row = 0; row < 4; ++row)
  {
    for (size_t col = 0; col < 3; ++col)
    {
      svtkNew<svtkTextActor> actor;
      switch (row)
      {
        case 0:
          actor->GetTextProperty()->SetOrientation(45);
          break;
        case 1:
          actor->SetOrientation(-45);
          break;
        case 2:
          break;
        case 3:
          actor->GetTextProperty()->SetOrientation(45);
          actor->SetOrientation(45);
          break;
      }
      switch (col)
      {
        case 0:
          actor->GetTextProperty()->SetJustificationToRight();
          actor->GetTextProperty()->SetVerticalJustificationToTop();
          break;
        case 1:
          actor->GetTextProperty()->SetJustificationToCentered();
          actor->GetTextProperty()->SetVerticalJustificationToCentered();
          break;
        case 2:
          actor->GetTextProperty()->SetJustificationToLeft();
          actor->GetTextProperty()->SetVerticalJustificationToBottom();
          break;
      }
      actor->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .2);
      actor->GetTextProperty()->SetBackgroundColor(0.25, 0.4 - col * .13, .5 - row * .1);
      actor->GetTextProperty()->SetBackgroundOpacity(1.0);

      actor->SetPosition(x[col], y[row]);

      actor->GetTextProperty()->SetFrame((row + col) % 2 == 0);
      actor->GetTextProperty()->SetFrameColor(
        col > 0 ? 1. : 0., col == 1 ? 1. : 0., col < 2 ? 1. : 0.);
      actor->GetTextProperty()->SetFrameWidth((row) % 3 + 1);

      setupTextActor(actor, anchors);
      ren->AddActor2D(actor);
    }
  }

  svtkNew<svtkPolyDataMapper2D> anchorMapper;
  anchorMapper->SetInputData(anchors);
  svtkNew<svtkActor2D> anchorActor;
  anchorActor->SetMapper(anchorMapper);
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor2D(anchorActor);

  // Add some various 'empty' actors to make sure there are no surprises:
  svtkNew<svtkTextActor> nullInputActor;
  nullInputActor->SetInput(nullptr);
  ren->AddActor2D(nullInputActor);

  svtkNew<svtkTextActor> emptyInputActor;
  emptyInputActor->SetInput("");
  ren->AddActor2D(emptyInputActor);

  svtkNew<svtkTextActor> spaceActor;
  spaceActor->SetInput(" ");
  ren->AddActor2D(spaceActor);

  svtkNew<svtkTextActor> tabActor;
  tabActor->SetInput("\t");
  ren->AddActor2D(tabActor);

  svtkNew<svtkTextActor> newlineActor;
  newlineActor->SetInput("\n");
  ren->AddActor2D(newlineActor);

  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->SetBackground(0.0, 0.0, 0.0);
  ren->GetActiveCamera()->SetPosition(0, 0, 400);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->ResetCameraClippingRange();
  win->SetSize(width, height);

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
