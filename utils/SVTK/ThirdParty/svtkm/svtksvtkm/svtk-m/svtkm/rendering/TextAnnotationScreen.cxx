//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/TextAnnotationScreen.h>

namespace svtkm
{
namespace rendering
{

TextAnnotationScreen::TextAnnotationScreen(const std::string& text,
                                           const svtkm::rendering::Color& color,
                                           svtkm::Float32 scale,
                                           const svtkm::Vec2f_32& position,
                                           svtkm::Float32 angleDegrees)
  : TextAnnotation(text, color, scale)
  , Position(position)
  , Angle(angleDegrees)
{
}

TextAnnotationScreen::~TextAnnotationScreen()
{
}

void TextAnnotationScreen::SetPosition(const svtkm::Vec2f_32& position)
{
  this->Position = position;
}

void TextAnnotationScreen::SetPosition(svtkm::Float32 xpos, svtkm::Float32 ypos)
{
  this->SetPosition(svtkm::make_Vec(xpos, ypos));
}

void TextAnnotationScreen::Render(const svtkm::rendering::Camera& svtkmNotUsed(camera),
                                  const svtkm::rendering::WorldAnnotator& svtkmNotUsed(annotator),
                                  svtkm::rendering::Canvas& canvas) const
{
  svtkm::Float32 windowAspect = svtkm::Float32(canvas.GetWidth()) / svtkm::Float32(canvas.GetHeight());

  canvas.AddText(this->Position,
                 this->Scale,
                 this->Angle,
                 windowAspect,
                 this->Anchor,
                 this->TextColor,
                 this->Text);
}
}
} // namespace svtkm::rendering
