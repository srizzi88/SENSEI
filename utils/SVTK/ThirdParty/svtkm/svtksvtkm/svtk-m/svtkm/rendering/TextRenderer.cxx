//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/TextRenderer.h>

#include <svtkm/Transform3D.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{
namespace internal
{

struct RenderBitmapFont : public svtkm::worklet::WorkletMapField
{
  using ColorBufferType = svtkm::rendering::Canvas::ColorBufferType;
  using DepthBufferType = svtkm::rendering::Canvas::DepthBufferType;
  using FontTextureType = svtkm::rendering::Canvas::FontTextureType;

  using ControlSignature = void(FieldIn, FieldIn, ExecObject, WholeArrayInOut, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  SVTKM_CONT
  RenderBitmapFont() {}

  SVTKM_CONT
  RenderBitmapFont(const svtkm::Vec4f_32& color,
                   svtkm::Id width,
                   svtkm::Id height,
                   svtkm::Float32 depth)
    : Color(color)
    , Width(width)
    , Height(height)
    , Depth(depth)
  {
  }

  template <typename ColorBufferPortal, typename FontTexture, typename DepthBufferPortal>
  SVTKM_EXEC void operator()(const svtkm::Vec4f_32& screenCoords,
                            const svtkm::Vec4f_32& textureCoords,
                            const FontTexture& fontTexture,
                            ColorBufferPortal& colorBuffer,
                            DepthBufferPortal& depthBuffer) const
  {
    svtkm::Float32 x0 = Clamp(screenCoords[0], 0.0f, static_cast<svtkm::Float32>(Width - 1));
    svtkm::Float32 x1 = Clamp(screenCoords[2], 0.0f, static_cast<svtkm::Float32>(Width - 1));
    svtkm::Float32 y0 = Clamp(screenCoords[1], 0.0f, static_cast<svtkm::Float32>(Height - 1));
    svtkm::Float32 y1 = Clamp(screenCoords[3], 0.0f, static_cast<svtkm::Float32>(Height - 1));
    // For crisp text rendering, we sample the font texture at points smaller than the pixel
    // sizes. Here we sample at increments of 0.25f, and scale the reported intensities accordingly
    svtkm::Float32 dx = x1 - x0, dy = y1 - y0;
    for (svtkm::Float32 x = x0; x <= x1; x += 0.25f)
    {
      for (svtkm::Float32 y = y0; y <= y1; y += 0.25f)
      {
        svtkm::Float32 tu = x1 == x0 ? 1.0f : (x - x0) / dx;
        svtkm::Float32 tv = y1 == y0 ? 1.0f : (y - y0) / dy;
        svtkm::Float32 u = svtkm::Lerp(textureCoords[0], textureCoords[2], tu);
        svtkm::Float32 v = svtkm::Lerp(textureCoords[1], textureCoords[3], tv);
        svtkm::Float32 intensity = fontTexture.GetColor(u, v)[0] * 0.25f;
        Plot(x, y, intensity, colorBuffer, depthBuffer);
      }
    }
  }

  template <typename ColorBufferPortal, typename DepthBufferPortal>
  SVTKM_EXEC void Plot(svtkm::Float32 x,
                      svtkm::Float32 y,
                      svtkm::Float32 intensity,
                      ColorBufferPortal& colorBuffer,
                      DepthBufferPortal& depthBuffer) const
  {
    svtkm::Id index =
      static_cast<svtkm::Id>(svtkm::Round(y)) * Width + static_cast<svtkm::Id>(svtkm::Round(x));
    svtkm::Vec4f_32 srcColor = colorBuffer.Get(index);
    svtkm::Float32 currentDepth = depthBuffer.Get(index);
    bool swap = Depth > currentDepth;

    intensity = intensity * Color[3];
    svtkm::Vec4f_32 color = intensity * Color;
    color[3] = intensity;
    svtkm::Vec4f_32 front = color;
    svtkm::Vec4f_32 back = srcColor;

    if (swap)
    {
      front = srcColor;
      back = color;
    }

    svtkm::Vec4f_32 blendedColor;
    svtkm::Float32 alpha = (1.f - front[3]);
    blendedColor[0] = front[0] + back[0] * alpha;
    blendedColor[1] = front[1] + back[1] * alpha;
    blendedColor[2] = front[2] + back[2] * alpha;
    blendedColor[3] = back[3] * alpha + front[3];

    colorBuffer.Set(index, blendedColor);
  }

  SVTKM_EXEC
  svtkm::Float32 Clamp(svtkm::Float32 v, svtkm::Float32 min, svtkm::Float32 max) const
  {
    return svtkm::Min(svtkm::Max(v, min), max);
  }

  svtkm::Vec4f_32 Color;
  svtkm::Id Width;
  svtkm::Id Height;
  svtkm::Float32 Depth;
}; // struct RenderBitmapFont

struct RenderBitmapFontExecutor
{
  using ColorBufferType = svtkm::rendering::Canvas::ColorBufferType;
  using DepthBufferType = svtkm::rendering::Canvas::DepthBufferType;
  using FontTextureType = svtkm::rendering::Canvas::FontTextureType;
  using ScreenCoordsArrayHandle = svtkm::cont::ArrayHandle<svtkm::Id4>;
  using TextureCoordsArrayHandle = svtkm::cont::ArrayHandle<svtkm::Vec4f_32>;

  SVTKM_CONT
  RenderBitmapFontExecutor(const ScreenCoordsArrayHandle& screenCoords,
                           const TextureCoordsArrayHandle& textureCoords,
                           const FontTextureType& fontTexture,
                           const svtkm::Vec4f_32& color,
                           const ColorBufferType& colorBuffer,
                           const DepthBufferType& depthBuffer,
                           svtkm::Id width,
                           svtkm::Id height,
                           svtkm::Float32 depth)
    : ScreenCoords(screenCoords)
    , TextureCoords(textureCoords)
    , FontTexture(fontTexture)
    , ColorBuffer(colorBuffer)
    , DepthBuffer(depthBuffer)
    , Worklet(color, width, height, depth)
  {
  }

  template <typename Device>
  SVTKM_CONT bool operator()(Device) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);

    svtkm::worklet::DispatcherMapField<RenderBitmapFont> dispatcher(Worklet);
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(
      ScreenCoords, TextureCoords, FontTexture.GetExecObjectFactory(), ColorBuffer, DepthBuffer);
    return true;
  }

  ScreenCoordsArrayHandle ScreenCoords;
  TextureCoordsArrayHandle TextureCoords;
  FontTextureType FontTexture;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;
  RenderBitmapFont Worklet;
}; // struct RenderBitmapFontExecutor
} // namespace internal

TextRenderer::TextRenderer(const svtkm::rendering::Canvas* canvas,
                           const svtkm::rendering::BitmapFont& font,
                           const svtkm::rendering::Canvas::FontTextureType& fontTexture)
  : Canvas(canvas)
  , Font(font)
  , FontTexture(fontTexture)
{
}

void TextRenderer::RenderText(const svtkm::Vec2f_32& position,
                              svtkm::Float32 scale,
                              svtkm::Float32 angle,
                              svtkm::Float32 windowAspect,
                              const svtkm::Vec2f_32& anchor,
                              const svtkm::rendering::Color& color,
                              const std::string& text)
{
  svtkm::Matrix<svtkm::Float32, 4, 4> translationMatrix =
    Transform3DTranslate(position[0], position[1], 0.f);
  svtkm::Matrix<svtkm::Float32, 4, 4> scaleMatrix = Transform3DScale(1.0f / windowAspect, 1.0f, 1.0f);
  svtkm::Vec3f_32 rotationAxis(0.0f, 0.0f, 1.0f);
  svtkm::Matrix<svtkm::Float32, 4, 4> rotationMatrix = Transform3DRotate(angle, rotationAxis);
  svtkm::Matrix<svtkm::Float32, 4, 4> transform =
    svtkm::MatrixMultiply(translationMatrix, svtkm::MatrixMultiply(scaleMatrix, rotationMatrix));
  RenderText(transform, scale, anchor, color, text);
}

void TextRenderer::RenderText(const svtkm::Vec3f_32& origin,
                              const svtkm::Vec3f_32& right,
                              const svtkm::Vec3f_32& up,
                              svtkm::Float32 scale,
                              const svtkm::Vec2f_32& anchor,
                              const svtkm::rendering::Color& color,
                              const std::string& text)
{
  svtkm::Vec3f_32 n = svtkm::Cross(right, up);
  svtkm::Normalize(n);

  svtkm::Matrix<svtkm::Float32, 4, 4> transform = MatrixHelpers::WorldMatrix(origin, right, up, n);
  transform = svtkm::MatrixMultiply(Canvas->GetModelView(), transform);
  transform = svtkm::MatrixMultiply(Canvas->GetProjection(), transform);
  RenderText(transform, scale, anchor, color, text);
}

void TextRenderer::RenderText(const svtkm::Matrix<svtkm::Float32, 4, 4>& transform,
                              svtkm::Float32 scale,
                              const svtkm::Vec2f_32& anchor,
                              const svtkm::rendering::Color& color,
                              const std::string& text,
                              const svtkm::Float32& depth)
{
  svtkm::Float32 textWidth = this->Font.GetTextWidth(text);
  svtkm::Float32 fx = -(0.5f + 0.5f * anchor[0]) * textWidth;
  svtkm::Float32 fy = -(0.5f + 0.5f * anchor[1]);
  svtkm::Float32 fz = 0;

  using ScreenCoordsArrayHandle = internal::RenderBitmapFontExecutor::ScreenCoordsArrayHandle;
  using TextureCoordsArrayHandle = internal::RenderBitmapFontExecutor::TextureCoordsArrayHandle;
  ScreenCoordsArrayHandle screenCoords;
  TextureCoordsArrayHandle textureCoords;
  screenCoords.Allocate(static_cast<svtkm::Id>(text.length()));
  textureCoords.Allocate(static_cast<svtkm::Id>(text.length()));
  ScreenCoordsArrayHandle::PortalControl screenCoordsPortal = screenCoords.GetPortalControl();
  TextureCoordsArrayHandle::PortalControl textureCoordsPortal = textureCoords.GetPortalControl();
  svtkm::Vec4f_32 charVertices, charUVs, charCoords;
  for (std::size_t i = 0; i < text.length(); ++i)
  {
    char c = text[i];
    char nextchar = (i < text.length() - 1) ? text[i + 1] : 0;
    Font.GetCharPolygon(c,
                        fx,
                        fy,
                        charVertices[0],
                        charVertices[2],
                        charVertices[3],
                        charVertices[1],
                        charUVs[0],
                        charUVs[2],
                        charUVs[3],
                        charUVs[1],
                        nextchar);
    charVertices = charVertices * scale;
    svtkm::Id2 p0 = Canvas->GetScreenPoint(charVertices[0], charVertices[3], fz, transform);
    svtkm::Id2 p1 = Canvas->GetScreenPoint(charVertices[2], charVertices[1], fz, transform);
    charCoords = svtkm::Id4(p0[0], p1[1], p1[0], p0[1]);
    screenCoordsPortal.Set(static_cast<svtkm::Id>(i), charCoords);
    textureCoordsPortal.Set(static_cast<svtkm::Id>(i), charUVs);
  }

  svtkm::cont::TryExecute(internal::RenderBitmapFontExecutor(screenCoords,
                                                            textureCoords,
                                                            FontTexture,
                                                            color.Components,
                                                            Canvas->GetColorBuffer(),
                                                            Canvas->GetDepthBuffer(),
                                                            Canvas->GetWidth(),
                                                            Canvas->GetHeight(),
                                                            depth));
}
}
} // namespace svtkm::rendering
