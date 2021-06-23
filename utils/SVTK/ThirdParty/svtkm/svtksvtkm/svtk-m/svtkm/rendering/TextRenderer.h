//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_rendering_TextRenderer_h
#define svtk_m_rendering_TextRenderer_h

#include <string>

#include <svtkm/rendering/BitmapFont.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/svtkm_rendering_export.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT TextRenderer
{
public:
  SVTKM_CONT
  TextRenderer(const svtkm::rendering::Canvas* canvas,
               const svtkm::rendering::BitmapFont& font,
               const svtkm::rendering::Canvas::FontTextureType& fontTexture);

  SVTKM_CONT
  void RenderText(const svtkm::Vec2f_32& position,
                  svtkm::Float32 scale,
                  svtkm::Float32 angle,
                  svtkm::Float32 windowAspect,
                  const svtkm::Vec2f_32& anchor,
                  const svtkm::rendering::Color& color,
                  const std::string& text);

  SVTKM_CONT
  void RenderText(const svtkm::Vec3f_32& origin,
                  const svtkm::Vec3f_32& right,
                  const svtkm::Vec3f_32& up,
                  svtkm::Float32 scale,
                  const svtkm::Vec2f_32& anchor,
                  const svtkm::rendering::Color& color,
                  const std::string& text);

  SVTKM_CONT
  void RenderText(const svtkm::Matrix<svtkm::Float32, 4, 4>& transform,
                  svtkm::Float32 scale,
                  const svtkm::Vec2f_32& anchor,
                  const svtkm::rendering::Color& color,
                  const std::string& text,
                  const svtkm::Float32& depth = 0.f);

private:
  const svtkm::rendering::Canvas* Canvas;
  svtkm::rendering::BitmapFont Font;
  svtkm::rendering::Canvas::FontTextureType FontTexture;
};
}
} // namespace svtkm::rendering

#endif // svtk_m_rendering_TextRenderer_h
