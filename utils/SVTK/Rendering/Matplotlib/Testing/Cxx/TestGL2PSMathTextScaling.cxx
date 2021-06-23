/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSMathTextScaling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBrush.h"
#include "svtkContext2D.h"
#include "svtkContextItem.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkGL2PSExporter.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPath.h"
#include "svtkPoints.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestingInteractor.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
class GL2PSMathTextScalingTest : public svtkContextItem
{
public:
  static GL2PSMathTextScalingTest* New();
  svtkTypeMacro(GL2PSMathTextScalingTest, svtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(svtkContext2D* painter) override;
};

//----------------------------------------------------------------------------
int TestGL2PSMathTextScaling(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(500, 500);
  view->GetRenderWindow()->SetDPI(120);
  svtkNew<GL2PSMathTextScalingTest> test;
  view->GetScene()->AddItem(test);

  view->GetRenderWindow()->SetMultiSamples(0);

  svtkNew<svtkGL2PSExporter> exp;

  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();
  exp->Write3DPropsAsRasterImageOff();

  std::string fileprefix =
    svtkTestingInteractor::TempDirectory + std::string("/TestGL2PSMathTextScaling");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}

svtkStandardNewMacro(GL2PSMathTextScalingTest);

bool GL2PSMathTextScalingTest::Paint(svtkContext2D* painter)
{
  painter->GetBrush()->SetColor(50, 50, 128);
  painter->DrawRect(0, 0, 500, 500);

  painter->GetTextProp()->SetColor(.7, .4, .5);
  painter->GetTextProp()->SetJustificationToLeft();
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->UseTightBoundingBoxOn();

  for (int i = 0; i < 10; ++i)
  {
    int fontSize = 5 + i * 3;
    float y = 500 - ((pow(i, 1.2) + 0.5) * 30);
    painter->GetTextProp()->SetFontSize(fontSize);
    painter->DrawString(5, y, "Text");
    painter->DrawMathTextString(120, y, "MathText$\\ast$");
  }

  return true;
}
