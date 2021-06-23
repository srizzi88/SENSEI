/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext.cxx

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
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"

#include "svtkUnicodeString.h"

#include "svtkRegressionTestImage.h"

#include <string>

//----------------------------------------------------------------------------
class ContextUnicode : public svtkContextItem
{
public:
  static ContextUnicode* New();
  svtkTypeMacro(ContextUnicode, svtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  bool Paint(svtkContext2D* painter) override;
  std::string FontFile;
};

//----------------------------------------------------------------------------
int TestContextUnicode(int argc, char* argv[])
{
  if (argc < 2)
  {
    cout << "Missing font filename." << endl;
    return EXIT_FAILURE;
  }

  std::string fontFile(argv[1]);

  // Set up a 2D context view, context test object and add it to the scene
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderWindow()->SetSize(200, 100);
  svtkSmartPointer<ContextUnicode> test = svtkSmartPointer<ContextUnicode>::New();
  test->FontFile = fontFile;
  view->GetScene()->AddItem(test);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }
  return !retVal;
}

// Make our new derived class to draw a diagram
svtkStandardNewMacro(ContextUnicode);
// This function aims to test the primitives provided by the 2D API.
bool ContextUnicode::Paint(svtkContext2D* painter)
{
  // Test the string drawing functionality of the context
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetTextProp()->SetFontFamily(SVTK_FONT_FILE);
  painter->GetTextProp()->SetFontFile(this->FontFile.c_str());
  painter->DrawString(70, 20, "Angstrom");
  painter->DrawString(150, 20, svtkUnicodeString::from_utf8("\xe2\x84\xab"));
  painter->DrawString(100, 80, svtkUnicodeString::from_utf8("a\xce\xb1"));
  painter->DrawString(100, 50, svtkUnicodeString::from_utf8("\xce\xb1\xce\xb2\xce\xb3"));
  return true;
}
