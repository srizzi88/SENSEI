/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSimpleFontRendering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContext2D.h"
#include "svtkContextItem.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFreeTypeTools.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLContextDevice2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
class SystemFontRenderTest : public svtkContextItem
{
public:
  static SystemFontRenderTest* New();
  svtkTypeMacro(SystemFontRenderTest, svtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  bool Paint(svtkContext2D* painter) override;
};

//----------------------------------------------------------------------------
int TestSystemFontRendering(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(580, 360);
  svtkNew<SystemFontRenderTest> test;
  view->GetScene()->AddItem(test);

  // Force the use of the freetype based rendering strategy
  svtkOpenGLContextDevice2D::SafeDownCast(view->GetContext()->GetDevice())
    ->SetStringRendererToFreeType();

  // Use the FontConfig font lookup
  svtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
svtkStandardNewMacro(SystemFontRenderTest);
bool SystemFontRenderTest::Paint(svtkContext2D* painter)
{
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);

  int y = 360;

  painter->GetTextProp()->SetFontFamilyToArial();

  const char* testString = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\xce\xb1\xce\xb2\xce\xb3\xce\xb4";

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, testString);

  painter->GetTextProp()->SetFontFamilyToTimes();

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, testString);

  painter->GetTextProp()->SetFontFamilyToCourier();

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(false);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(false);
  painter->DrawString(5, y, testString);

  y -= 30;
  painter->GetTextProp()->SetBold(true);
  painter->GetTextProp()->SetItalic(true);
  painter->DrawString(5, y, testString);

  return true;
}
