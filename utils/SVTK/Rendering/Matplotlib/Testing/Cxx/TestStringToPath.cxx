/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStringToPath.cxx

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
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPath.h"
#include "svtkPen.h"
#include "svtkPoints.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
class StringToPathContextTest : public svtkContextItem
{
public:
  static StringToPathContextTest* New();
  svtkTypeMacro(StringToPathContextTest, svtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(svtkContext2D* painter) override;

  void SetPath(svtkPath* path) { this->Path = path; }

protected:
  svtkPath* Path;
};

//----------------------------------------------------------------------------
int TestStringToPath(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(325, 150);
  svtkNew<StringToPathContextTest> test;
  view->GetScene()->AddItem(test);

  svtkNew<svtkPath> path;
  svtkNew<svtkTextProperty> tprop;

  svtkMathTextUtilities::GetInstance()->StringToPath(
    "$\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}$", path, tprop, view->GetRenderWindow()->GetDPI());

  test->SetPath(path);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
svtkStandardNewMacro(StringToPathContextTest);

// This function aims to test the primitives provided by the 2D API.
bool StringToPathContextTest::Paint(svtkContext2D* painter)
{
  // RGB color lookup table by path point code:
  double color[4][3];
  color[svtkPath::MOVE_TO][0] = 1.0;
  color[svtkPath::MOVE_TO][1] = 0.0;
  color[svtkPath::MOVE_TO][2] = 0.0;
  color[svtkPath::LINE_TO][0] = 0.0;
  color[svtkPath::LINE_TO][1] = 1.0;
  color[svtkPath::LINE_TO][2] = 0.0;
  color[svtkPath::CONIC_CURVE][0] = 0.0;
  color[svtkPath::CONIC_CURVE][1] = 0.0;
  color[svtkPath::CONIC_CURVE][2] = 1.0;
  color[svtkPath::CUBIC_CURVE][0] = 1.0;
  color[svtkPath::CUBIC_CURVE][1] = 0.0;
  color[svtkPath::CUBIC_CURVE][2] = 1.0;

  svtkPoints* points = this->Path->GetPoints();
  svtkIntArray* codes = this->Path->GetCodes();

  if (points->GetNumberOfPoints() != codes->GetNumberOfTuples())
  {
    return false;
  }

  // scaling factor and offset to ensure that the points will fit the view:
  double scale = 5.16591;
  double offset = 20.0;

  // Draw the control points, colored by codes:
  double point[3];
  painter->GetPen()->SetWidth(2);
  for (svtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
  {
    points->GetPoint(i, point);
    int code = codes->GetValue(i);

    painter->GetPen()->SetColorF(color[code]);
    painter->DrawPoint(point[0] * scale + offset, point[1] * scale + offset);
  }

  return true;
}
