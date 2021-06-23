/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContextGL2PS.cxx

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
#include "svtkFloatArray.h"
#include "svtkGL2PSExporter.h"
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

#include <string>

//----------------------------------------------------------------------------
class ContextGL2PSTest : public svtkContextItem
{
public:
  static ContextGL2PSTest* New();
  svtkTypeMacro(ContextGL2PSTest, svtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  bool Paint(svtkContext2D* painter) override;
};

//----------------------------------------------------------------------------
int TestGL2PSContext(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);
  svtkNew<ContextGL2PSTest> test;
  view->GetScene()->AddItem(test);

  // Force the use of the freetype based rendering strategy
  svtkOpenGLContextDevice2D::SafeDownCast(view->GetContext()->GetDevice())
    ->SetStringRendererToFreeType();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  svtkNew<svtkGL2PSExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileFormatToPS();
  exp->UsePainterSettings();
  exp->CompressOff();
  exp->DrawBackgroundOn();
  exp->SetLineWidthFactor(1.0);
  exp->SetPointSizeFactor(1.0);
  exp->SetTextAsPath(true);

  std::string fileprefix = svtkTestingInteractor::TempDirectory + std::string("/TestGL2PSContext");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(view->GetRenderWindow());
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->GetInteractor()->Start();

  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
svtkStandardNewMacro(ContextGL2PSTest);
// This function aims to test the primitives provided by the 2D API.
bool ContextGL2PSTest::Paint(svtkContext2D* painter)
{
  // Test the string drawing functionality of the context
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetTextProp()->SetFontFamilyToArial();
  painter->GetPen()->SetColor(0, 0, 0, 255);
  painter->GetBrush()->SetColor(0, 0, 0, 255);

  // Ensure transform works
  svtkNew<svtkTransform2D> tform;
  tform->Translate(400, 25);
  painter->PushMatrix();
  painter->AppendTransform(tform);
  painter->DrawString(0, 0, "GL2PS is used as a backend to the context.");
  painter->PopMatrix();

  // Draw some individual lines of different thicknesses.
  for (int i = 0; i < 10; ++i)
  {
    painter->GetPen()->SetColor(255, static_cast<unsigned char>(float(i) * 25.0), 0);
    painter->GetPen()->SetWidth(1.0 + float(i));
    painter->DrawLine(10, 50 + float(i) * 10, 60, 50 + float(i) * 10);
  }

  // Draw some individual lines of different thicknesses.
  painter->GetPen()->SetWidth(10);
  for (int i = 0; i < 10; ++i)
  {
    painter->GetPen()->SetLineType(i % (svtkPen::DASH_DOT_DOT_LINE + 1));
    painter->GetPen()->SetColor(255, static_cast<unsigned char>(float(i) * 25.0), 0);
    painter->DrawLine(10, 250 + float(i) * 10, 60, 250 + float(i) * 10);
  }
  painter->GetPen()->SetLineType(svtkPen::SOLID_LINE);

  // Use the draw lines function now to draw a shape.
  svtkSmartPointer<svtkPoints2D> points = svtkSmartPointer<svtkPoints2D>::New();
  points->SetNumberOfPoints(30);
  for (int i = 0; i < 30; ++i)
  {
    double point[2] = { float(i) * 25.0 + 10.0, sin(float(i) / 5.0) * 100.0 + 200.0 };
    points->SetPoint(i, point);
  }
  painter->GetPen()->SetColor(0, 255, 0);
  painter->GetPen()->SetWidth(5.0);
  painter->DrawPoly(points);

  // Now to draw some points
  painter->GetPen()->SetColor(0, 0, 255);
  painter->GetPen()->SetWidth(5.0);
  painter->DrawPoint(10, 10);
  painter->DrawPoint(790, 10);
  painter->DrawPoint(10, 590);
  painter->DrawPoint(790, 590);

  // Test the markers
  float markerPoints[10 * 2];
  unsigned char markerColors[10 * 4];
  for (int i = 0; i < 10; ++i)
  {
    markerPoints[2 * i] = 500.0 + i * 30.0;
    markerPoints[2 * i + 1] = 20 * sin(markerPoints[2 * i]) + 375.0;

    markerColors[4 * i] = static_cast<unsigned char>(255 * i / 10.0);
    markerColors[4 * i + 1] = static_cast<unsigned char>(255 * (1.0 - i / 10.0));
    markerColors[4 * i + 2] = static_cast<unsigned char>(255 * (0.3));
    markerColors[4 * i + 3] = static_cast<unsigned char>(255 * (1.0 - ((i / 10.0) * 0.25)));
  }

  for (int style = SVTK_MARKER_NONE + 1; style < SVTK_MARKER_UNKNOWN; ++style)
  {
    // Increment the y values:
    for (int i = 1; i < 20; i += 2)
    {
      markerPoints[i] += 35.0;
    }
    painter->GetPen()->SetWidth(style * 5 + 5);
    // Not highlighted:
    painter->DrawMarkers(style, false, markerPoints, 10, markerColors, 4);
    // Highlight the middle 4 points.
    // Note that the colors will not be correct for these points in the
    // postscript output -- They are drawn yellow with alpha=0.5 over the
    // existing colored points, but PS doesn't support transparency, so they
    // just come out yellow.
    painter->GetPen()->SetColorF(0.9, 0.8, 0.1, 0.5);
    painter->DrawMarkers(style, true, markerPoints + 3 * 2, 4);
  }

  // Draw some individual lines of different thicknesses.
  for (int i = 0; i < 10; ++i)
  {
    painter->GetPen()->SetColor(0, static_cast<unsigned char>(float(i) * 25.0), 255, 255);
    painter->GetPen()->SetWidth(1.0 + float(i));
    painter->DrawPoint(75, 50 + float(i) * 10);
  }

  painter->GetPen()->SetColor(0, 0, 255);
  painter->GetPen()->SetWidth(3.0);
  painter->DrawPoints(points);

  // Now draw a rectangle
  painter->GetPen()->SetColor(100, 200, 255);
  painter->GetPen()->SetWidth(3.0);
  painter->GetBrush()->SetColor(100, 255, 100);
  painter->DrawRect(100, 50, 200, 100);

  // Add in an arbitrary quad
  painter->GetPen()->SetColor(159, 0, 255);
  painter->GetPen()->SetWidth(1.0);
  painter->GetBrush()->SetColor(100, 55, 0, 200);
  painter->DrawQuad(350, 50, 375, 150, 525, 199, 666, 45);

  // Now to test out the transform...
  svtkNew<svtkTransform2D> transform;
  transform->Translate(20, 200);
  painter->SetTransform(transform);
  painter->GetPen()->SetColor(255, 0, 0);
  painter->GetPen()->SetWidth(6.0);
  painter->DrawPoly(points);

  transform->Translate(0, 10);
  painter->SetTransform(transform);
  painter->GetPen()->SetColor(0, 0, 200);
  painter->GetPen()->SetWidth(2.0);
  painter->DrawPoints(points);

  transform->Translate(0, -20);
  painter->SetTransform(transform);
  painter->GetPen()->SetColor(100, 0, 200);
  painter->GetPen()->SetWidth(5.0);
  painter->DrawPoints(points);

  // Now for an ellipse...
  painter->GetPen()->SetColor(0, 0, 0);
  painter->GetPen()->SetWidth(1.0);
  painter->GetBrush()->SetColor(0, 0, 100, 69);
  // Draws smooth path (Full circle, testing oddball angles):
  painter->DrawEllipseWedge(100.0, 89.0, 20, 100, 15, 75, -26.23, 333.77);
  // Polygon approximation:
  painter->DrawEllipseWedge(150.0, 89.0, 20, 100, 15, 75, 43, 181);
  // Smooth path:
  painter->DrawEllipticArc(200.0, 89.0, 20, 100, 0, 360);
  // Polygon approximation:
  painter->DrawEllipticArc(250.0, 89.0, 20, 100, 43, 181);

  // Remove the transform:
  transform->Identity();
  painter->SetTransform(transform);

  // Toss some images in:
  svtkNew<svtkRTAnalyticSource> imageSrc;
  imageSrc->SetWholeExtent(0, 49, 0, 49, 0, 0);
  imageSrc->SetMaximum(1.0);
  imageSrc->Update();
  svtkImageData* image = imageSrc->GetOutput();

  // convert to RGB bytes:
  svtkFloatArray* vals = static_cast<svtkFloatArray*>(image->GetPointData()->GetScalars());
  float imgRange[2];
  vals->GetValueRange(imgRange);
  float invRange = 1.f / (imgRange[1] - imgRange[0]);
  svtkUnsignedCharArray* scalars = svtkUnsignedCharArray::New();
  scalars->SetNumberOfComponents(3);
  scalars->SetNumberOfTuples(vals->GetNumberOfTuples());
  for (svtkIdType i = 0; i < vals->GetNumberOfTuples(); ++i)
  {
    float val = vals->GetValue(i);
    val = (val - imgRange[0]) * invRange; // normalize to (0, 1)
    scalars->SetComponent(i, 0, val * 255);
    scalars->SetComponent(i, 1, (1.f - val) * 255);
    scalars->SetComponent(i, 2, (val * val) * 255);
  }
  image->GetPointData()->SetScalars(scalars);
  scalars->Delete();
  painter->PushMatrix();

  // Ensure transform works
  tform->Identity();
  tform->Translate(10, 525);
  painter->PushMatrix();
  painter->AppendTransform(tform);
  painter->DrawImage(0, 0, image);
  painter->PopMatrix();

  painter->DrawImage(65, 500, 2.f, image);
  painter->DrawImage(svtkRectf(170, 537.5f, 25, 25), image);

  return true;
}
