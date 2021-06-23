/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSpiderPlotActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the spider plot capabilities in SVTK.
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
#include "svtkSpiderPlotActor.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
int TestSpiderPlotActor(int argc, char* argv[])
{
  int numTuples = 12;

  svtkFloatArray* bitter = svtkFloatArray::New();
  bitter->SetNumberOfTuples(numTuples);

  svtkFloatArray* crispy = svtkFloatArray::New();
  crispy->SetNumberOfTuples(numTuples);

  svtkFloatArray* crunchy = svtkFloatArray::New();
  crunchy->SetNumberOfTuples(numTuples);

  svtkFloatArray* salty = svtkFloatArray::New();
  salty->SetNumberOfTuples(numTuples);

  svtkFloatArray* oily = svtkFloatArray::New();
  oily->SetNumberOfTuples(numTuples);

  for (int i = 0; i < numTuples; i++)
  {
    bitter->SetTuple1(i, svtkMath::Random(1, 10));
    crispy->SetTuple1(i, svtkMath::Random(-1, 1));
    crunchy->SetTuple1(i, svtkMath::Random(1, 100));
    salty->SetTuple1(i, svtkMath::Random(0, 10));
    oily->SetTuple1(i, svtkMath::Random(5, 25));
  }

  svtkDataObject* dobj = svtkDataObject::New();
  dobj->GetFieldData()->AddArray(bitter);
  dobj->GetFieldData()->AddArray(crispy);
  dobj->GetFieldData()->AddArray(crunchy);
  dobj->GetFieldData()->AddArray(salty);
  dobj->GetFieldData()->AddArray(oily);

  svtkSpiderPlotActor* actor = svtkSpiderPlotActor::New();
  actor->SetInputData(dobj);
  actor->SetTitle("Spider Plot");
  actor->SetIndependentVariablesToColumns();
  actor->GetPositionCoordinate()->SetValue(0.05, 0.1, 0.0);
  actor->GetPosition2Coordinate()->SetValue(0.95, 0.85, 0.0);
  actor->GetProperty()->SetColor(1, 0, 0);
  actor->SetAxisLabel(0, "Bitter");
  actor->SetAxisRange(0, 1, 10);
  actor->SetAxisLabel(1, "Crispy");
  actor->SetAxisRange(1, -1, 1);
  actor->SetAxisLabel(2, "Crunchy");
  actor->SetAxisRange(2, 1, 100);
  actor->SetAxisLabel(3, "Salty");
  actor->SetAxisRange(3, 0, 10);
  actor->SetAxisLabel(4, "Oily");
  actor->SetAxisRange(4, 5, 25);
  actor->GetLegendActor()->SetNumberOfEntries(numTuples);
  for (int i = 0; i < numTuples; i++)
  {
    double red = svtkMath::Random(0, 1);
    double green = svtkMath::Random(0, 1);
    double blue = svtkMath::Random(0, 1);
    actor->SetPlotColor(i, red, green, blue);
  }
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
  iren->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  bitter->Delete();
  crispy->Delete();
  crunchy->Delete();
  salty->Delete();
  oily->Delete();
  dobj->Delete();
  actor->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
