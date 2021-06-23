/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBarChartActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the spider plot capabilities in SVTK.
#include "svtkBarChartActor.h"
#include "svtkDataObject.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkLegendBoxActor.h"
#include "svtkMath.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkProperty2D.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
int TestBarChartActor(int argc, char* argv[])
{
  int numTuples = 6;

  svtkFloatArray* bitter = svtkFloatArray::New();
  bitter->SetNumberOfTuples(numTuples);

  for (int i = 0; i < numTuples; i++)
  {
    bitter->SetTuple1(i, svtkMath::Random(7, 100));
  }

  svtkDataObject* dobj = svtkDataObject::New();
  dobj->GetFieldData()->AddArray(bitter);

  svtkBarChartActor* actor = svtkBarChartActor::New();
  actor->SetInput(dobj);
  actor->SetTitle("Bar Chart");
  actor->GetPositionCoordinate()->SetValue(0.05, 0.05, 0.0);
  actor->GetPosition2Coordinate()->SetValue(0.95, 0.85, 0.0);
  actor->GetProperty()->SetColor(1, 1, 1);
  actor->GetLegendActor()->SetNumberOfEntries(numTuples);
  for (int i = 0; i < numTuples; i++)
  {
    double red = svtkMath::Random(0, 1);
    double green = svtkMath::Random(0, 1);
    double blue = svtkMath::Random(0, 1);
    actor->SetBarColor(i, red, green, blue);
  }
  actor->SetBarLabel(0, "oil");
  actor->SetBarLabel(1, "gas");
  actor->SetBarLabel(2, "water");
  actor->SetBarLabel(3, "snake oil");
  actor->SetBarLabel(4, "tequila");
  actor->SetBarLabel(5, "beer");
  actor->LegendVisibilityOn();

  // Set text colors (same as actor for backward compat with test)
  actor->GetTitleTextProperty()->SetColor(1, 1, 0);
  actor->GetLabelTextProperty()->SetColor(1, 0, 0);

  // Create the RenderWindow, Renderer and both Actors
  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren1->AddActor(actor);
  ren1->SetBackground(0, 0, 0);
  renWin->SetSize(500, 200);

  // render the image
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  bitter->Delete();
  dobj->Delete();
  actor->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
