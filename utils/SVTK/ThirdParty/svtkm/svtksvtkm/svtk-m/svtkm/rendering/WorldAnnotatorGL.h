//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_WorldAnnotatorGL_h
#define svtk_m_rendering_WorldAnnotatorGL_h

#include <svtkm/rendering/WorldAnnotator.h>

#include <svtkm/rendering/BitmapFont.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/TextureGL.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT WorldAnnotatorGL : public WorldAnnotator
{
public:
  WorldAnnotatorGL(const svtkm::rendering::Canvas* canvas);

  ~WorldAnnotatorGL();

  void AddLine(const svtkm::Vec3f_64& point0,
               const svtkm::Vec3f_64& point1,
               svtkm::Float32 lineWidth,
               const svtkm::rendering::Color& color,
               bool inFront) const override;

  void AddText(const svtkm::Vec3f_32& origin,
               const svtkm::Vec3f_32& right,
               const svtkm::Vec3f_32& up,
               svtkm::Float32 scale,
               const svtkm::Vec2f_32& anchor,
               const svtkm::rendering::Color& color,
               const std::string& text,
               const svtkm::Float32 depth = 0.f) const override;

private:
  BitmapFont Font;
  TextureGL FontTexture;

  void RenderText(svtkm::Float32 scale,
                  svtkm::Float32 anchorx,
                  svtkm::Float32 anchory,
                  std::string text) const;
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_WorldAnnotatorGL_h
