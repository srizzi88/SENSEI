/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBrush.h"
#include "svtkContext2D.h"
#include "svtkContextActor.h"
#include "svtkContextItem.h"
#include "svtkContextScene.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
class APIDiagram : public svtkContextItem
{
public:
  static APIDiagram* New();
  svtkTypeMacro(APIDiagram, svtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  bool Paint(svtkContext2D* painter) override;
};

//----------------------------------------------------------------------------
int TestDiagram(int, char*[])
{
  // Set up a 2D chart actor, APIDiagram object andn add them to the renderer
  svtkNew<svtkContextActor> actor;
  svtkNew<APIDiagram> diagram;
  actor->GetScene()->AddItem(diagram);
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);
  renderWindow->SetMultiSamples(0);
  interactor->Initialize();
  interactor->Start();
  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
svtkStandardNewMacro(APIDiagram);
// This function draws our API diagram
bool APIDiagram::Paint(svtkContext2D* painter)
{
  // Drawing a hard wired diagram 800x600 as a demonstration of the 2D API
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetPen()->SetColor(0, 0, 0);

  painter->GetBrush()->SetColor(100, 255, 100);
  painter->DrawRect(100, 50, 200, 100);
  painter->DrawString(200, 100, "OpenGL");

  painter->GetBrush()->SetColor(255, 100, 0);
  painter->DrawRect(300, 50, 200, 100);
  painter->DrawString(400, 100, "Others?");

  painter->GetBrush()->SetColor(100, 0, 255);
  painter->DrawRect(500, 50, 200, 100);
  painter->DrawString(600, 100, "Others?");

  painter->GetBrush()->SetColor(180, 180, 255);
  painter->DrawRect(100, 150, 600, 100);
  painter->DrawString(400, 200, "2D API");

  painter->GetBrush()->SetColor(255, 255, 180);
  painter->DrawRect(100, 250, 600, 200);
  painter->DrawString(400, 400, "Canvas API");

  painter->GetBrush()->SetColor(180, 255, 180);
  painter->DrawRect(100, 250, 300, 100);
  painter->DrawString(250, 300, "Point Mark");

  painter->GetBrush()->SetColor(255, 255, 255);
  painter->DrawRect(100, 450, 600, 100);
  painter->DrawString(400, 500, "Canvas View");

  return true;
}
