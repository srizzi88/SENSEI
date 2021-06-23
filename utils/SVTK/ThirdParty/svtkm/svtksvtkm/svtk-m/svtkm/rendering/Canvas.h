//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_rendering_Canvas_h
#define svtk_m_rendering_Canvas_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/Matrix.h>
#include <svtkm/Types.h>
#include <svtkm/cont/ColorTable.h>
#include <svtkm/rendering/BitmapFont.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/Texture2D.h>

#define SVTKM_DEFAULT_CANVAS_DEPTH 1.001f

namespace svtkm
{
namespace rendering
{

class WorldAnnotator;

class SVTKM_RENDERING_EXPORT Canvas
{
public:
  using ColorBufferType = svtkm::cont::ArrayHandle<svtkm::Vec4f_32>;
  using DepthBufferType = svtkm::cont::ArrayHandle<svtkm::Float32>;
  using FontTextureType = svtkm::rendering::Texture2D<1>;

  Canvas(svtkm::Id width = 1024, svtkm::Id height = 1024);
  virtual ~Canvas();

  virtual svtkm::rendering::Canvas* NewCopy() const;

  virtual void Initialize();

  virtual void Activate();

  virtual void Clear();

  virtual void Finish();

  virtual void BlendBackground();

  SVTKM_CONT
  svtkm::Id GetWidth() const;

  SVTKM_CONT
  svtkm::Id GetHeight() const;

  SVTKM_CONT
  const ColorBufferType& GetColorBuffer() const;

  SVTKM_CONT
  ColorBufferType& GetColorBuffer();

  SVTKM_CONT
  const DepthBufferType& GetDepthBuffer() const;

  SVTKM_CONT
  DepthBufferType& GetDepthBuffer();

  SVTKM_CONT
  void ResizeBuffers(svtkm::Id width, svtkm::Id height);

  SVTKM_CONT
  const svtkm::rendering::Color& GetBackgroundColor() const;

  SVTKM_CONT
  void SetBackgroundColor(const svtkm::rendering::Color& color);

  SVTKM_CONT
  const svtkm::rendering::Color& GetForegroundColor() const;

  SVTKM_CONT
  void SetForegroundColor(const svtkm::rendering::Color& color);

  SVTKM_CONT
  svtkm::Id2 GetScreenPoint(svtkm::Float32 x,
                           svtkm::Float32 y,
                           svtkm::Float32 z,
                           const svtkm::Matrix<svtkm::Float32, 4, 4>& transfor) const;

  // If a subclass uses a system that renderers to different buffers, then
  // these should be overridden to copy the data to the buffers.
  virtual void RefreshColorBuffer() const {}
  virtual void RefreshDepthBuffer() const {}

  virtual void SetViewToWorldSpace(const svtkm::rendering::Camera& camera, bool clip);
  virtual void SetViewToScreenSpace(const svtkm::rendering::Camera& camera, bool clip);
  virtual void SetViewportClipping(const svtkm::rendering::Camera&, bool) {}

  virtual void SaveAs(const std::string& fileName) const;

  /// Creates a WorldAnnotator of a type that is paired with this Canvas. Other
  /// types of world annotators might work, but this provides a default.
  ///
  /// The WorldAnnotator is created with the C++ new keyword (so it should be
  /// deleted with delete later). A pointer to the created WorldAnnotator is
  /// returned.
  ///
  virtual svtkm::rendering::WorldAnnotator* CreateWorldAnnotator() const;

  SVTKM_CONT
  virtual void AddColorSwatch(const svtkm::Vec2f_64& point0,
                              const svtkm::Vec2f_64& point1,
                              const svtkm::Vec2f_64& point2,
                              const svtkm::Vec2f_64& point3,
                              const svtkm::rendering::Color& color) const;

  SVTKM_CONT
  void AddColorSwatch(const svtkm::Float64 x0,
                      const svtkm::Float64 y0,
                      const svtkm::Float64 x1,
                      const svtkm::Float64 y1,
                      const svtkm::Float64 x2,
                      const svtkm::Float64 y2,
                      const svtkm::Float64 x3,
                      const svtkm::Float64 y3,
                      const svtkm::rendering::Color& color) const;

  SVTKM_CONT
  virtual void AddLine(const svtkm::Vec2f_64& point0,
                       const svtkm::Vec2f_64& point1,
                       svtkm::Float32 linewidth,
                       const svtkm::rendering::Color& color) const;

  SVTKM_CONT
  void AddLine(svtkm::Float64 x0,
               svtkm::Float64 y0,
               svtkm::Float64 x1,
               svtkm::Float64 y1,
               svtkm::Float32 linewidth,
               const svtkm::rendering::Color& color) const;

  SVTKM_CONT
  virtual void AddColorBar(const svtkm::Bounds& bounds,
                           const svtkm::cont::ColorTable& colorTable,
                           bool horizontal) const;

  SVTKM_CONT
  void AddColorBar(svtkm::Float32 x,
                   svtkm::Float32 y,
                   svtkm::Float32 width,
                   svtkm::Float32 height,
                   const svtkm::cont::ColorTable& colorTable,
                   bool horizontal) const;

  virtual void AddText(const svtkm::Vec2f_32& position,
                       svtkm::Float32 scale,
                       svtkm::Float32 angle,
                       svtkm::Float32 windowAspect,
                       const svtkm::Vec2f_32& anchor,
                       const svtkm::rendering::Color& color,
                       const std::string& text) const;

  SVTKM_CONT
  void AddText(svtkm::Float32 x,
               svtkm::Float32 y,
               svtkm::Float32 scale,
               svtkm::Float32 angle,
               svtkm::Float32 windowAspect,
               svtkm::Float32 anchorX,
               svtkm::Float32 anchorY,
               const svtkm::rendering::Color& color,
               const std::string& text) const;

  SVTKM_CONT
  void AddText(const svtkm::Matrix<svtkm::Float32, 4, 4>& transform,
               svtkm::Float32 scale,
               const svtkm::Vec2f_32& anchor,
               const svtkm::rendering::Color& color,
               const std::string& text,
               const svtkm::Float32& depth = 0) const;


  friend class AxisAnnotation2D;
  friend class ColorBarAnnotation;
  friend class ColorLegendAnnotation;
  friend class TextAnnotationScreen;
  friend class TextRenderer;
  friend class WorldAnnotator;

private:
  bool LoadFont() const;

  const svtkm::Matrix<svtkm::Float32, 4, 4>& GetModelView() const;

  const svtkm::Matrix<svtkm::Float32, 4, 4>& GetProjection() const;

  struct CanvasInternals;
  std::shared_ptr<CanvasInternals> Internals;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_Canvas_h
