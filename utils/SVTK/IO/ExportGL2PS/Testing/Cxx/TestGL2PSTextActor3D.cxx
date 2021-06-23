/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTextActor3D.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkGL2PSExporter.h"
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

#include <sstream>

namespace svtkTestGL2PSTextActor3D
{
void setupTextActor3D(svtkTextActor3D* actor, svtkPolyData* anchor)
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
} // end namespace svtkTestGL2PSTextActor3D

//----------------------------------------------------------------------------
int TestGL2PSTextActor3D(int, char*[])
{
  using namespace svtkTestGL2PSTextActor3D;
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
      svtkNew<svtkTextActor3D> actor;
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
      actor->GetTextProperty()->SetBackgroundOpacity(0.25);
      actor->SetPosition(x[col], y[row], 0.);
      setupTextActor3D(actor, anchors);
      ren->AddActor(actor);
    }
  }

  svtkNew<svtkPolyDataMapper> anchorMapper;
  anchorMapper->SetInputData(anchors);
  svtkNew<svtkActor> anchorActor;
  anchorActor->SetMapper(anchorMapper);
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor(anchorActor);

  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->SetBackground(0.0, 0.0, 0.0);
  ren->GetActiveCamera()->SetPosition(width / 2, height / 2, 1400);
  ren->GetActiveCamera()->SetFocalPoint(width / 2, height / 2, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->ResetCameraClippingRange();
  win->SetSize(width, height);
  win->Render();

  svtkNew<svtkGL2PSExporter> exp;
  exp->SetRenderWindow(win);
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();

  std::string fileprefix =
    svtkTestingInteractor::TempDirectory + std::string("/TestGL2PSTextActor3D");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
