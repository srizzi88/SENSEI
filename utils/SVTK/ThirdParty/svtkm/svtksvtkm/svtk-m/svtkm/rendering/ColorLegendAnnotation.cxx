//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/ColorLegendAnnotation.h>

namespace svtkm
{
namespace rendering
{

ColorLegendAnnotation::ColorLegendAnnotation()
{
  this->FontScale = 0.05f;
  this->LabelColor = svtkm::rendering::Color::white;
}

ColorLegendAnnotation::~ColorLegendAnnotation()
{
}

void ColorLegendAnnotation::Clear()
{
  this->Labels.clear();
  this->ColorSwatchList.clear();
}

void ColorLegendAnnotation::AddItem(const std::string& label, svtkm::rendering::Color color)
{
  this->Labels.push_back(label);
  this->ColorSwatchList.push_back(color);
}

void ColorLegendAnnotation::Render(const svtkm::rendering::Camera& camera,
                                   const svtkm::rendering::WorldAnnotator& annotator,
                                   svtkm::rendering::Canvas& canvas)
{
  svtkm::Float32 l = -0.95f, r = -0.90f;
  svtkm::Float32 b = +0.90f, t = +0.95f;

  for (unsigned int i = 0; i < this->ColorSwatchList.size(); ++i)
  {
    canvas.AddColorSwatch(l, b, l, t, r, t, r, b, this->ColorSwatchList[i]);
    b -= 0.07f;
    t -= 0.07f;
  }

  // reset positions
  l = -0.95f;
  r = -0.90f;
  b = +0.90f;
  t = +0.95f;

  while (this->Annot.size() < this->Labels.size())
  {
    this->Annot.push_back(
      std::unique_ptr<TextAnnotationScreen>(new svtkm::rendering::TextAnnotationScreen(
        "test", this->LabelColor, this->FontScale, svtkm::Vec2f_32(0, 0), 0)));
  }

  for (unsigned int i = 0; i < this->Annot.size(); ++i)
  {
    TextAnnotationScreen* txt = Annot[i].get();
    txt->SetText(Labels[i]);
    txt->SetPosition(r + .02f, (b + t) / 2.f);
    txt->SetAlignment(TextAnnotationScreen::Left, TextAnnotationScreen::VCenter);
    txt->Render(camera, annotator, canvas);
    b -= 0.07f;
    t -= 0.07f;
  }
}
}
} // namespace svtkm::rendering
