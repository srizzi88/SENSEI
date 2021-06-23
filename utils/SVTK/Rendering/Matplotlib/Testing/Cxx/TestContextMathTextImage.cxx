/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContextMathTextImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMathTextUtilities.h"

#include "svtkContext2D.h"
#include "svtkContextItem.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
class ContextMathTextImageTest : public svtkContextItem
{
public:
  static ContextMathTextImageTest* New();
  svtkTypeMacro(ContextMathTextImageTest, svtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(svtkContext2D* painter) override;
};

//----------------------------------------------------------------------------
int TestContextMathTextImage(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(325, 150);
  svtkNew<ContextMathTextImageTest> test;
  view->GetScene()->AddItem(test);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
svtkStandardNewMacro(ContextMathTextImageTest);

// This function aims to test the primitives provided by the 2D API.
bool ContextMathTextImageTest::Paint(svtkContext2D* painter)
{
  painter->GetTextProp()->SetColor(0.4, 0.6, 0.7);
  painter->GetTextProp()->SetFontSize(60);
  painter->DrawMathTextString(20, 20,
    "$\\frac{-b\\pm\\sqrt{b^2-4ac}}"
    "{2a}$");

  return true;
}
