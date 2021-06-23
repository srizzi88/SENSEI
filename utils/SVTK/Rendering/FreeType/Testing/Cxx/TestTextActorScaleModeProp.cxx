/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActorScaleModeProp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTextActor.h"

#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
int TestTextActorScaleModeProp(int, char*[])
{
  // Test PROP scale mode actor with a text property that's instantiated first
  // See SVTK bug 15412
  svtkNew<svtkTextProperty> textProperty;
  textProperty->SetBold(1);
  textProperty->SetItalic(1);
  textProperty->SetShadow(0);
  textProperty->SetFontFamily(SVTK_ARIAL);
  textProperty->SetJustification(SVTK_TEXT_LEFT);
  textProperty->SetVerticalJustification(SVTK_TEXT_BOTTOM);

  svtkNew<svtkTextActor> textActor;
  textActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  textActor->GetPositionCoordinate()->SetReferenceCoordinate(nullptr);
  textActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  textActor->GetPosition2Coordinate()->SetReferenceCoordinate(nullptr);
  textActor->SetTextScaleModeToProp();
  textActor->SetTextProperty(textProperty);
  textActor->SetInput("15412");

  textActor->GetPositionCoordinate()->SetValue(20.0, 20.0, 0.0);
  textActor->GetPosition2Coordinate()->SetValue(280.0, 80.0, 0.0);

  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->SetBackground(0.1, 0.1, 0.1);
  win->SetSize(300, 300);

  ren->AddActor2D(textActor);

  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
