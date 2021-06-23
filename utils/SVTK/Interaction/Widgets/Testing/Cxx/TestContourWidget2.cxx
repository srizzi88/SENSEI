/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContourWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test functionality to initialize a contour widget from user supplied
// polydata. Here we will create closed circle and initialize it from that.
#include "svtkSmartPointer.h"

#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCommand.h"
#include "svtkContourWidget.h"
#include "svtkMath.h"
#include "svtkOrientedGlyphContourRepresentation.h"
#include "svtkPlane.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

int TestContourWidget2(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 600);

  svtkSmartPointer<svtkOrientedGlyphContourRepresentation> contourRep =
    svtkSmartPointer<svtkOrientedGlyphContourRepresentation>::New();
  svtkSmartPointer<svtkContourWidget> contourWidget = svtkSmartPointer<svtkContourWidget>::New();
  contourWidget->SetInteractor(iren);
  contourWidget->SetRepresentation(contourRep);
  contourWidget->On();

  for (int i = 0; i < argc; i++)
  {
    if (strcmp("-Shift", argv[i]) == 0)
    {
      contourWidget->GetEventTranslator()->RemoveTranslation(svtkCommand::LeftButtonPressEvent);
      contourWidget->GetEventTranslator()->SetTranslation(
        svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Translate);
    }
    else if (strcmp("-Scale", argv[i]) == 0)
    {
      contourWidget->GetEventTranslator()->RemoveTranslation(svtkCommand::LeftButtonPressEvent);
      contourWidget->GetEventTranslator()->SetTranslation(
        svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Scale);
    }
  }

  svtkSmartPointer<svtkPolyData> pd = svtkSmartPointer<svtkPolyData>::New();

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
  svtkIdType* lineIndices = new svtkIdType[21];
  for (int i = 0; i < 20; i++)
  {
    const double angle = 2.0 * svtkMath::Pi() * i / 20.0;
    points->InsertPoint(static_cast<svtkIdType>(i), 0.1 * cos(angle), 0.1 * sin(angle), 0.0);
    lineIndices[i] = static_cast<svtkIdType>(i);
  }

  lineIndices[20] = 0;
  lines->InsertNextCell(21, lineIndices);
  delete[] lineIndices;
  pd->SetPoints(points);
  pd->SetLines(lines);

  contourWidget->Initialize(pd);
  contourWidget->Render();
  ren1->ResetCamera();
  renWin->Render();

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
