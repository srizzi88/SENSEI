/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPDFTransformedText.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPDFExporter.h"

#include "svtkBrush.h"
#include "svtkContext2D.h"
#include "svtkContextItem.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLContextDevice2D.h"
#include "svtkPen.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestingInteractor.h"
#include "svtkTextProperty.h"
#include "svtkTransform2D.h"

#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

namespace
{

const int Width = 900;
const int Height = 900;

using Rotation = std::pair<double, double>; // tprop, painter
std::array<Rotation, 4> RotArray{
  Rotation{ -45., -45. },
  Rotation{ -45., 0. },
  Rotation{ 0., 0. },
  Rotation{ 0., 45. },
};

using Scale = std::pair<double, double>; // x, y
std::array<Scale, 3> ScaleArray{
  Scale{ 0.1, 0.1 },
  Scale{ 1, 1 },
  Scale{ 10, 10 },
};

using Justification = std::pair<int, int>; // horiz, vert
std::array<Justification, 3> JustArray{
  Justification{ SVTK_TEXT_LEFT, SVTK_TEXT_BOTTOM },
  Justification{ SVTK_TEXT_CENTERED, SVTK_TEXT_CENTERED },
  Justification{ SVTK_TEXT_RIGHT, SVTK_TEXT_TOP },
};

//----------------------------------------------------------------------------
class TransformedTextPDFTest : public svtkContextItem
{
public:
  static TransformedTextPDFTest* New();
  svtkTypeMacro(TransformedTextPDFTest, svtkContextItem);
  bool Paint(svtkContext2D* painter) override;

private:
  void PaintCell(svtkContext2D* painter, size_t rotIdx, size_t scaleIdx, size_t justIdx);
};

svtkStandardNewMacro(TransformedTextPDFTest);

bool TransformedTextPDFTest::Paint(svtkContext2D* painter)
{
  // Reset painter state that we care about:
  painter->GetBrush()->SetTexture(nullptr);
  painter->GetBrush()->SetColor(0, 0, 0, 255);
  painter->GetPen()->SetColor(0, 0, 0, 255);
  painter->GetPen()->SetWidth(1.f);
  painter->GetTextProp()->SetUseTightBoundingBox(1);
  painter->GetTextProp()->SetOrientation(0.);
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetOpacity(1.);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetTextProp()->SetBold(0);
  painter->GetTextProp()->SetItalic(0);
  painter->GetTextProp()->SetFontFamilyToArial();

  for (size_t rot = 0; rot < RotArray.size(); ++rot)
  {
    for (size_t scale = 0; scale < ScaleArray.size(); ++scale)
    {
      for (size_t just = 0; just < JustArray.size(); ++just)
      {
        this->PaintCell(painter, rot, scale, just);
      }
    }
  }

  return true;
}

void TransformedTextPDFTest::PaintCell(
  svtkContext2D* painter, size_t rotIdx, size_t scaleIdx, size_t justIdx)
{
  // Cells are arranged:
  //
  // +---+---+---+     +---+     +-+-+
  // |   |   |   |     |   |     | | |
  // |   |   |   |     |   | --> +-+-+
  // |   |   |   |     |   |     | | |
  // |   |   |   |     +---+     +-+-+
  // |   |   |   |     |   |
  // |   |   |   | --> |   |
  // |   |   |   |     |   |
  // |   |   |   |     +---+
  // |   |   |   |     |   |
  // |   |   |   |     |   |
  // |   |   |   |     |   |
  // +---+---+---+     +---+
  //
  //                   Split     Split
  //  Split Just       Scale      Rot
  //
  const int numCellsX = (RotArray.size() / 2) * JustArray.size();
  const int numCellsY = (RotArray.size() / 2) * ScaleArray.size();
  const int cellIdX = justIdx * (RotArray.size() / 2) + rotIdx % 2;
  const int cellIdY = scaleIdx * (RotArray.size() / 2) + rotIdx / 2;
  const int cellWidth = Width / numCellsX;
  const int cellHeight = Height / numCellsY;
  const int cellX = cellWidth * cellIdX;
  const int cellY = cellHeight * cellIdY;
  const int cellId = cellIdY * numCellsX + cellIdX;

  painter->GetPen()->SetColor(0, 0, 0, 255);
  painter->GetPen()->SetWidth(1);
  painter->GetBrush()->SetOpacity(0);
  painter->DrawRect(cellX, cellY, cellWidth, cellHeight);

  std::array<double, 2> textAnchor{ 0., 0. };

  double tpropRot;
  double painterRot;
  std::tie(tpropRot, painterRot) = RotArray[rotIdx];

  double scaleX;
  double scaleY;
  std::tie(scaleX, scaleY) = ScaleArray[scaleIdx];

  int hJust;
  int vJust;
  std::tie(hJust, vJust) = JustArray[justIdx];

  auto scaleToStr = [](double scale) -> std::string {
    if (scale < 0.5)
    {
      return "S";
    }
    else if (scale > 1.5)
    {
      return "L";
    }
    return "1";
  };

  std::ostringstream str;
  str << "ID<" << rotIdx << "," << scaleIdx << "," << justIdx << ">(" << cellId << ")\n"
      << "TPropRot = " << static_cast<int>(tpropRot) << "\n"
      << "PainterRot = " << static_cast<int>(painterRot) << "\n"
      << "Scale = " << scaleToStr(scaleX) << scaleToStr(scaleY) << "\n"
      << "Justification = ";

  switch (vJust)
  {
    case SVTK_TEXT_TOP:
      textAnchor[1] = cellY + cellHeight * 0.5;
      str << "T";
      break;
    case SVTK_TEXT_CENTERED:
      textAnchor[1] = cellY + cellHeight * 0.5;
      str << "C";
      break;
    case SVTK_TEXT_BOTTOM:
      textAnchor[1] = cellY + cellHeight * 0.55;
      str << "B";
      break;
    default:
      textAnchor[1] = cellY + cellHeight * 0.5;
      str << "X";
      break;
  }
  switch (hJust)
  {
    case SVTK_TEXT_LEFT:
      textAnchor[0] = cellX + cellWidth * 0.15;
      str << "L";
      break;
    case SVTK_TEXT_CENTERED:
      textAnchor[0] = cellX + cellWidth * 0.5;
      str << "C";
      break;
    case SVTK_TEXT_RIGHT:
      textAnchor[0] = cellX + cellWidth * 0.85;
      str << "R";
      break;
    default:
      textAnchor[0] = cellX + cellWidth * 0.5;
      str << "X";
      break;
  }

  svtkNew<svtkTransform2D> xform;
  xform->Identity();
  xform->Scale(scaleX, scaleY);
  xform->Rotate(painterRot);

  xform->InverseTransformPoints(textAnchor.data(), textAnchor.data(), 1);

  painter->PushMatrix();
  painter->AppendTransform(xform);

  painter->GetTextProp()->SetFontSize(10);
  painter->GetTextProp()->SetOrientation(tpropRot);
  painter->GetTextProp()->SetJustification(hJust);
  painter->GetTextProp()->SetVerticalJustification(vJust);
  painter->DrawString(textAnchor[0], textAnchor[1], str.str().c_str());

  painter->GetPen()->SetColor(255, 0, 0, 255);
  painter->GetPen()->SetWidth(5);
  painter->DrawPoint(textAnchor[0], textAnchor[1]);

  painter->PopMatrix();
}

} // end anon namespace

int TestPDFTransformedText(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(Width, Height);
  svtkNew<TransformedTextPDFTest> test;
  view->GetScene()->AddItem(test);

  // Force the use of the freetype based rendering strategy
  svtkOpenGLContextDevice2D::SafeDownCast(view->GetContext()->GetDevice())
    ->SetStringRendererToFreeType();

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(view->GetRenderWindow());
  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  std::string filename =
    svtkTestingInteractor::TempDirectory + std::string("/TestPDFTransformedText.pdf");

  svtkNew<svtkPDFExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileName(filename.c_str());
  exp->Write();

  view->GetRenderWindow()->Render();
  view->GetRenderWindow()->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
