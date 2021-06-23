/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTextMapper.h"

#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkGL2PSExporter.h"
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

namespace svtkTestGL2PSTextMapper
{
void setupTextMapper(svtkTextMapper* mapper, svtkActor2D* actor, svtkPolyData* anchor)
{
  svtkTextProperty* p = mapper->GetTextProperty();
  std::ostringstream label;
  label << "Angle: " << p->GetOrientation() << "\n"
        << "HAlign: " << p->GetJustificationAsString() << "\n"
        << "VAlign: " << p->GetVerticalJustificationAsString();
  mapper->SetInput(label.str().c_str());

  // Add the anchor point:
  double* pos = actor->GetPosition();
  double* col = p->GetColor();
  svtkIdType ptId = anchor->GetPoints()->InsertNextPoint(pos[0], pos[1], 0.);
  anchor->GetVerts()->InsertNextCell(1, &ptId);
  anchor->GetCellData()->GetScalars()->InsertNextTuple4(
    col[0] * 255, col[1] * 255, col[2] * 255, 255);
}
} // end namespace svtkTestGL2PSTextMapper

//----------------------------------------------------------------------------
int TestGL2PSTextMapper(int, char*[])
{
  using namespace svtkTestGL2PSTextMapper;
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
      svtkNew<svtkTextMapper> mapper;
      svtkNew<svtkActor2D> actor;
      actor->SetMapper(mapper);

      switch (row)
      {
        case 0:
          mapper->GetTextProperty()->SetOrientation(45);
          break;
        case 1:
          mapper->GetTextProperty()->SetOrientation(-45);
          break;
        case 2:
          break;
        case 3:
          mapper->GetTextProperty()->SetOrientation(90);
          break;
      }
      switch (col)
      {
        case 0:
          mapper->GetTextProperty()->SetJustificationToRight();
          mapper->GetTextProperty()->SetVerticalJustificationToTop();
          break;
        case 1:
          mapper->GetTextProperty()->SetJustificationToCentered();
          mapper->GetTextProperty()->SetVerticalJustificationToCentered();
          break;
        case 2:
          mapper->GetTextProperty()->SetJustificationToLeft();
          mapper->GetTextProperty()->SetVerticalJustificationToBottom();
          break;
      }
      mapper->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .2);
      mapper->GetTextProperty()->SetBackgroundColor(0.0, 0.8 - col * .26, .8 - row * .2);
      mapper->GetTextProperty()->SetBackgroundOpacity(0.25);
      actor->SetPosition(x[col], y[row]);
      setupTextMapper(mapper, actor, anchors);
      ren->AddActor2D(actor);
    }
  }

  svtkNew<svtkPolyDataMapper2D> anchorMapper;
  anchorMapper->SetInputData(anchors);
  svtkNew<svtkActor2D> anchorActor;
  anchorActor->SetMapper(anchorMapper);
  anchorActor->GetProperty()->SetPointSize(5);
  ren->AddActor2D(anchorActor);

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
  win->Render();

  svtkNew<svtkGL2PSExporter> exp;
  exp->SetRenderWindow(win);
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->TextAsPathOn();
  exp->DrawBackgroundOn();

  std::string fileprefix =
    svtkTestingInteractor::TempDirectory + std::string("/TestGL2PSTextMapper");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
