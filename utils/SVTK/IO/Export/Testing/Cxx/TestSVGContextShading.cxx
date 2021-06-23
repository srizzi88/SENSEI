/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSVGContextShading.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSVGExporter.h"

#include "svtkBrush.h"
#include "svtkContext2D.h"
#include "svtkContextItem.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLContextDevice2D.h"
#include "svtkPen.h"
#include "svtkPointData.h"
#include "svtkPoints2D.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestingInteractor.h"
#include "svtkTextProperty.h"
#include "svtkTransform2D.h"
#include "svtkUnsignedCharArray.h"

#include "svtkRenderingOpenGLConfigure.h"

#include <string>

namespace
{
class ContextSVGTest : public svtkContextItem
{
public:
  static ContextSVGTest* New();
  svtkTypeMacro(ContextSVGTest, svtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  bool Paint(svtkContext2D* painter) override;
};
svtkStandardNewMacro(ContextSVGTest);
} // end anon namespace

int TestSVGContextShading(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(300, 300);
  svtkNew<ContextSVGTest> test;
  view->GetScene()->AddItem(test);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  std::string filename =
    svtkTestingInteractor::TempDirectory + std::string("/TestSVGContextShading.svg");

  svtkNew<svtkSVGExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileName(filename.c_str());
  exp->Write();

#if 0
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(view->GetRenderWindow());
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->GetInteractor()->Start();
#endif

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
// This function aims to test the primitives provided by the 2D API.
namespace
{
bool ContextSVGTest::Paint(svtkContext2D* painter)
{
  float poly[] = {
    50.f, 50.f,   //
    25.f, 150.f,  //
    50.f, 250.f,  //
    150.f, 275.f, //
    250.f, 250.f, //
    275.f, 150.f, //
    250.f, 50.f,  //
    150.f, 25.f   //
  };
  unsigned char polyColor[] = {
    32, 192, 64,  //
    128, 32, 64,  //
    192, 16, 128, //
    255, 16, 92,  //
    128, 128, 16, //
    64, 255, 32,  //
    32, 192, 128, //
    32, 128, 255  //
  };
  painter->DrawPolygon(poly, 8, polyColor, 3);

  float triangle[] = {
    100.f, 100.f, //
    150.f, 200.f, //
    200.f, 100.f  //
  };
  unsigned char triangleColor[] = {
    255, 0, 0, //
    0, 255, 0, //
    0, 0, 255  //
  };
  painter->DrawPolygon(triangle, 3, triangleColor, 3);

  float line[] = {
    290, 290, //
    290, 150, //
    290, 10,  //
    150, 10,  //
    10, 10,   //
    10, 150,  //
    10, 290,  //
    150, 290, //
    290, 290  //
  };
  unsigned char lineColor[] = {
    255, 32, 16,   //
    128, 128, 32,  //
    255, 255, 64,  //
    128, 192, 128, //
    64, 128, 192,  //
    255, 0, 0,     //
    0, 255, 0,     //
    0, 0, 255,     //
    255, 32, 16    //
  };
  painter->DrawPoly(line, 9, lineColor, 3);

  return true;
}
} // end anon namespace
