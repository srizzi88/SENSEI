/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextMapper.cxx

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

namespace svtkTestTextMapper
{
void setupTextMapper(svtkTextMapper* mapper, svtkActor2D* actor, svtkPolyData* anchor)
{
  svtkTextProperty* p = mapper->GetTextProperty();
  std::ostringstream label;
  label << "TProp Angle: " << p->GetOrientation() << "\n"
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
} // end namespace svtkTestTextMapper

//----------------------------------------------------------------------------
int TestTextMapper(int, char*[])
{
  using namespace svtkTestTextMapper;
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
      svtkNew<svtkTextMapper> mapper;
      switch (row)
      {
        case 0:
          mapper->GetTextProperty()->SetJustificationToRight();
          break;
        case 1:
          mapper->GetTextProperty()->SetJustificationToCentered();
          break;
        case 2:
          mapper->GetTextProperty()->SetJustificationToLeft();
          break;
      }
      switch (col)
      {
        case 0:
          mapper->GetTextProperty()->SetVerticalJustificationToBottom();
          break;
        case 1:
          mapper->GetTextProperty()->SetVerticalJustificationToCentered();
          break;
        case 2:
          mapper->GetTextProperty()->SetVerticalJustificationToTop();
          break;
      }
      mapper->GetTextProperty()->SetOrientation(45.0 * (3 * row + col));
      mapper->GetTextProperty()->SetColor(0.75, .2 + col * .26, .2 + row * .2);
      svtkNew<svtkActor2D> actor;
      actor->SetPosition(x[col], y[row]);
      actor->SetMapper(mapper);
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

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
