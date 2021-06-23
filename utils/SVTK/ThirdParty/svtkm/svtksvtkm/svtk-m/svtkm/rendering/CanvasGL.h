//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_CanvasGL_h
#define svtk_m_rendering_CanvasGL_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/BitmapFont.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/TextureGL.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT CanvasGL : public Canvas
{
public:
  CanvasGL(svtkm::Id width = 1024, svtkm::Id height = 1024);

  ~CanvasGL();

  void Initialize() override;

  void Activate() override;

  void Clear() override;

  void Finish() override;

  svtkm::rendering::Canvas* NewCopy() const override;

  void SetViewToWorldSpace(const svtkm::rendering::Camera& camera, bool clip) override;

  void SetViewToScreenSpace(const svtkm::rendering::Camera& camera, bool clip) override;

  void SetViewportClipping(const svtkm::rendering::Camera& camera, bool clip) override;

  void RefreshColorBuffer() const override;

  virtual void RefreshDepthBuffer() const override;

  svtkm::rendering::WorldAnnotator* CreateWorldAnnotator() const override;

protected:
  void AddLine(const svtkm::Vec2f_64& point0,
               const svtkm::Vec2f_64& point1,
               svtkm::Float32 linewidth,
               const svtkm::rendering::Color& color) const override;

  void AddColorBar(const svtkm::Bounds& bounds,
                   const svtkm::cont::ColorTable& colorTable,
                   bool horizontal) const override;

  void AddColorSwatch(const svtkm::Vec2f_64& point0,
                      const svtkm::Vec2f_64& point1,
                      const svtkm::Vec2f_64& point2,
                      const svtkm::Vec2f_64& point3,
                      const svtkm::rendering::Color& color) const override;

  void AddText(const svtkm::Vec2f_32& position,
               svtkm::Float32 scale,
               svtkm::Float32 angle,
               svtkm::Float32 windowAspect,
               const svtkm::Vec2f_32& anchor,
               const svtkm::rendering::Color& color,
               const std::string& text) const override;

private:
  svtkm::rendering::BitmapFont Font;
  svtkm::rendering::TextureGL FontTexture;

  void RenderText(svtkm::Float32 scale, const svtkm::Vec2f_32& anchor, const std::string& text) const;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_CanvasGL_h
