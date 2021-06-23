//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/Canvas.h>

#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/rendering/BitmapFontFactory.h>
#include <svtkm/rendering/DecodePNG.h>
#include <svtkm/rendering/LineRenderer.h>
#include <svtkm/rendering/TextRenderer.h>
#include <svtkm/rendering/WorldAnnotator.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/ColorTable.hxx>

#include <fstream>
#include <iostream>

namespace svtkm
{
namespace rendering
{
namespace internal
{

struct ClearBuffers : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2);

  SVTKM_CONT
  ClearBuffers() {}

  SVTKM_EXEC
  void operator()(svtkm::Vec4f_32& color, svtkm::Float32& depth) const
  {
    color[0] = 0.f;
    color[1] = 0.f;
    color[2] = 0.f;
    color[3] = 0.f;
    // The depth is set to slightly larger than 1.0f, ensuring this color value always fails a
    // depth check
    depth = SVTKM_DEFAULT_CANVAS_DEPTH;
  }
}; // struct ClearBuffers

struct BlendBackground : public svtkm::worklet::WorkletMapField
{
  svtkm::Vec4f_32 BackgroundColor;

  SVTKM_CONT
  BlendBackground(const svtkm::Vec4f_32& backgroundColor)
    : BackgroundColor(backgroundColor)
  {
  }

  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  SVTKM_EXEC void operator()(svtkm::Vec4f_32& color) const
  {
    if (color[3] >= 1.f)
      return;

    svtkm::Float32 alpha = BackgroundColor[3] * (1.f - color[3]);
    color[0] = color[0] + BackgroundColor[0] * alpha;
    color[1] = color[1] + BackgroundColor[1] * alpha;
    color[2] = color[2] + BackgroundColor[2] * alpha;
    color[3] = alpha + color[3];
  }
}; // struct BlendBackground

struct DrawColorSwatch : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2);

  SVTKM_CONT
  DrawColorSwatch(svtkm::Id2 dims, svtkm::Id2 xBounds, svtkm::Id2 yBounds, const svtkm::Vec4f_32 color)
    : Color(color)
  {
    ImageWidth = dims[0];
    ImageHeight = dims[1];
    SwatchBottomLeft[0] = xBounds[0];
    SwatchBottomLeft[1] = yBounds[0];
    SwatchWidth = xBounds[1] - xBounds[0];
    SwatchHeight = yBounds[1] - yBounds[0];
  }

  template <typename FrameBuffer>
  SVTKM_EXEC void operator()(const svtkm::Id& index, FrameBuffer& frameBuffer) const
  {
    // local bar coord
    svtkm::Id x = index % SwatchWidth;
    svtkm::Id y = index / SwatchWidth;

    // offset to global image coord
    x += SwatchBottomLeft[0];
    y += SwatchBottomLeft[1];

    svtkm::Id offset = y * ImageWidth + x;
    frameBuffer.Set(offset, Color);
  }

  svtkm::Id ImageWidth;
  svtkm::Id ImageHeight;
  svtkm::Id2 SwatchBottomLeft;
  svtkm::Id SwatchWidth;
  svtkm::Id SwatchHeight;
  const svtkm::Vec4f_32 Color;
}; // struct DrawColorSwatch

struct DrawColorBar : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);

  SVTKM_CONT
  DrawColorBar(svtkm::Id2 dims, svtkm::Id2 xBounds, svtkm::Id2 yBounds, bool horizontal)
    : Horizontal(horizontal)
  {
    ImageWidth = dims[0];
    ImageHeight = dims[1];
    BarBottomLeft[0] = xBounds[0];
    BarBottomLeft[1] = yBounds[0];
    BarWidth = xBounds[1] - xBounds[0];
    BarHeight = yBounds[1] - yBounds[0];
  }

  template <typename FrameBuffer, typename ColorMap>
  SVTKM_EXEC void operator()(const svtkm::Id& index,
                            FrameBuffer& frameBuffer,
                            const ColorMap& colorMap) const
  {
    // local bar coord
    svtkm::Id x = index % BarWidth;
    svtkm::Id y = index / BarWidth;
    svtkm::Id sample = Horizontal ? x : y;


    const svtkm::Vec4ui_8 color = colorMap.Get(sample);

    svtkm::Float32 normalizedHeight = Horizontal
      ? static_cast<svtkm::Float32>(y) / static_cast<svtkm::Float32>(BarHeight)
      : static_cast<svtkm::Float32>(x) / static_cast<svtkm::Float32>(BarWidth);
    // offset to global image coord
    x += BarBottomLeft[0];
    y += BarBottomLeft[1];

    svtkm::Id offset = y * ImageWidth + x;
    // If the colortable has alpha values, we blend each color sample with translucent white.
    // The height of the resultant translucent bar indicates the opacity.

    constexpr svtkm::Float32 conversionToFloatSpace = (1.0f / 255.0f);
    svtkm::Float32 alpha = color[3] * conversionToFloatSpace;
    if (alpha < 1 && normalizedHeight <= alpha)
    {
      constexpr svtkm::Float32 intensity = 0.4f;
      constexpr svtkm::Float32 inverseIntensity = (1.0f - intensity);
      alpha *= inverseIntensity;
      svtkm::Vec4f_32 blendedColor(1.0f * intensity + (color[0] * conversionToFloatSpace) * alpha,
                                  1.0f * intensity + (color[1] * conversionToFloatSpace) * alpha,
                                  1.0f * intensity + (color[2] * conversionToFloatSpace) * alpha,
                                  1.0f);
      frameBuffer.Set(offset, blendedColor);
    }
    else
    {
      // make sure this is opaque
      svtkm::Vec4f_32 fColor((color[0] * conversionToFloatSpace),
                            (color[1] * conversionToFloatSpace),
                            (color[2] * conversionToFloatSpace),
                            1.0f);
      frameBuffer.Set(offset, fColor);
    }
  }

  svtkm::Id ImageWidth;
  svtkm::Id ImageHeight;
  svtkm::Id2 BarBottomLeft;
  svtkm::Id BarWidth;
  svtkm::Id BarHeight;
  bool Horizontal;
}; // struct DrawColorBar

} // namespace internal

struct Canvas::CanvasInternals
{

  CanvasInternals(svtkm::Id width, svtkm::Id height)
    : Width(width)
    , Height(height)
  {
    BackgroundColor.Components[0] = 0.f;
    BackgroundColor.Components[1] = 0.f;
    BackgroundColor.Components[2] = 0.f;
    BackgroundColor.Components[3] = 1.f;

    ForegroundColor.Components[0] = 1.f;
    ForegroundColor.Components[1] = 1.f;
    ForegroundColor.Components[2] = 1.f;
    ForegroundColor.Components[3] = 1.f;
  }

  svtkm::Id Width;
  svtkm::Id Height;
  svtkm::rendering::Color BackgroundColor;
  svtkm::rendering::Color ForegroundColor;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;
  svtkm::rendering::BitmapFont Font;
  FontTextureType FontTexture;
  svtkm::Matrix<svtkm::Float32, 4, 4> ModelView;
  svtkm::Matrix<svtkm::Float32, 4, 4> Projection;
};

Canvas::Canvas(svtkm::Id width, svtkm::Id height)
  : Internals(new CanvasInternals(0, 0))
{
  svtkm::MatrixIdentity(Internals->ModelView);
  svtkm::MatrixIdentity(Internals->Projection);
  this->ResizeBuffers(width, height);
}

Canvas::~Canvas()
{
}

svtkm::rendering::Canvas* Canvas::NewCopy() const
{
  return new svtkm::rendering::Canvas(*this);
}

svtkm::Id Canvas::GetWidth() const
{
  return Internals->Width;
}

svtkm::Id Canvas::GetHeight() const
{
  return Internals->Height;
}

const Canvas::ColorBufferType& Canvas::GetColorBuffer() const
{
  return Internals->ColorBuffer;
}

Canvas::ColorBufferType& Canvas::GetColorBuffer()
{
  return Internals->ColorBuffer;
}

const Canvas::DepthBufferType& Canvas::GetDepthBuffer() const
{
  return Internals->DepthBuffer;
}

Canvas::DepthBufferType& Canvas::GetDepthBuffer()
{
  return Internals->DepthBuffer;
}

const svtkm::rendering::Color& Canvas::GetBackgroundColor() const
{
  return Internals->BackgroundColor;
}

void Canvas::SetBackgroundColor(const svtkm::rendering::Color& color)
{
  Internals->BackgroundColor = color;
}

const svtkm::rendering::Color& Canvas::GetForegroundColor() const
{
  return Internals->ForegroundColor;
}

void Canvas::SetForegroundColor(const svtkm::rendering::Color& color)
{
  Internals->ForegroundColor = color;
}

void Canvas::Initialize()
{
}

void Canvas::Activate()
{
}

void Canvas::Clear()
{
  internal::ClearBuffers worklet;
  svtkm::worklet::DispatcherMapField<internal::ClearBuffers> dispatcher(worklet);
  dispatcher.Invoke(this->GetColorBuffer(), this->GetDepthBuffer());
}

void Canvas::Finish()
{
}

void Canvas::BlendBackground()
{
  internal::BlendBackground worklet(GetBackgroundColor().Components);
  svtkm::worklet::DispatcherMapField<internal::BlendBackground> dispatcher(worklet);
  dispatcher.Invoke(this->GetColorBuffer());
}

void Canvas::ResizeBuffers(svtkm::Id width, svtkm::Id height)
{
  SVTKM_ASSERT(width >= 0);
  SVTKM_ASSERT(height >= 0);

  svtkm::Id numPixels = width * height;
  if (Internals->ColorBuffer.GetNumberOfValues() != numPixels)
  {
    Internals->ColorBuffer.Allocate(numPixels);
  }
  if (Internals->DepthBuffer.GetNumberOfValues() != numPixels)
  {
    Internals->DepthBuffer.Allocate(numPixels);
  }

  Internals->Width = width;
  Internals->Height = height;
}

void Canvas::AddColorSwatch(const svtkm::Vec2f_64& point0,
                            const svtkm::Vec2f_64& svtkmNotUsed(point1),
                            const svtkm::Vec2f_64& point2,
                            const svtkm::Vec2f_64& svtkmNotUsed(point3),
                            const svtkm::rendering::Color& color) const
{
  svtkm::Float64 width = static_cast<svtkm::Float64>(this->GetWidth());
  svtkm::Float64 height = static_cast<svtkm::Float64>(this->GetHeight());

  svtkm::Id2 x, y;
  x[0] = static_cast<svtkm::Id>(((point0[0] + 1.) / 2.) * width + .5);
  x[1] = static_cast<svtkm::Id>(((point2[0] + 1.) / 2.) * width + .5);
  y[0] = static_cast<svtkm::Id>(((point0[1] + 1.) / 2.) * height + .5);
  y[1] = static_cast<svtkm::Id>(((point2[1] + 1.) / 2.) * height + .5);

  svtkm::Id2 dims(this->GetWidth(), this->GetHeight());

  svtkm::Id totalPixels = (x[1] - x[0]) * (y[1] - y[0]);
  svtkm::cont::ArrayHandleCounting<svtkm::Id> iterator(0, 1, totalPixels);
  svtkm::worklet::DispatcherMapField<internal::DrawColorSwatch> dispatcher(
    internal::DrawColorSwatch(dims, x, y, color.Components));
  dispatcher.Invoke(iterator, this->GetColorBuffer());
}

void Canvas::AddColorSwatch(const svtkm::Float64 x0,
                            const svtkm::Float64 y0,
                            const svtkm::Float64 x1,
                            const svtkm::Float64 y1,
                            const svtkm::Float64 x2,
                            const svtkm::Float64 y2,
                            const svtkm::Float64 x3,
                            const svtkm::Float64 y3,
                            const svtkm::rendering::Color& color) const
{
  this->AddColorSwatch(svtkm::make_Vec(x0, y0),
                       svtkm::make_Vec(x1, y1),
                       svtkm::make_Vec(x2, y2),
                       svtkm::make_Vec(x3, y3),
                       color);
}

void Canvas::AddLine(const svtkm::Vec2f_64& point0,
                     const svtkm::Vec2f_64& point1,
                     svtkm::Float32 linewidth,
                     const svtkm::rendering::Color& color) const
{
  svtkm::rendering::Canvas* self = const_cast<svtkm::rendering::Canvas*>(this);
  LineRenderer renderer(self, svtkm::MatrixMultiply(Internals->Projection, Internals->ModelView));
  renderer.RenderLine(point0, point1, linewidth, color);
}

void Canvas::AddLine(svtkm::Float64 x0,
                     svtkm::Float64 y0,
                     svtkm::Float64 x1,
                     svtkm::Float64 y1,
                     svtkm::Float32 linewidth,
                     const svtkm::rendering::Color& color) const
{
  this->AddLine(svtkm::make_Vec(x0, y0), svtkm::make_Vec(x1, y1), linewidth, color);
}

void Canvas::AddColorBar(const svtkm::Bounds& bounds,
                         const svtkm::cont::ColorTable& colorTable,
                         bool horizontal) const
{
  svtkm::Float64 width = static_cast<svtkm::Float64>(this->GetWidth());
  svtkm::Float64 height = static_cast<svtkm::Float64>(this->GetHeight());

  svtkm::Id2 x, y;
  x[0] = static_cast<svtkm::Id>(((bounds.X.Min + 1.) / 2.) * width + .5);
  x[1] = static_cast<svtkm::Id>(((bounds.X.Max + 1.) / 2.) * width + .5);
  y[0] = static_cast<svtkm::Id>(((bounds.Y.Min + 1.) / 2.) * height + .5);
  y[1] = static_cast<svtkm::Id>(((bounds.Y.Max + 1.) / 2.) * height + .5);
  svtkm::Id barWidth = x[1] - x[0];
  svtkm::Id barHeight = y[1] - y[0];

  svtkm::Id numSamples = horizontal ? barWidth : barHeight;
  svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> colorMap;

  {
    svtkm::cont::ScopedRuntimeDeviceTracker tracker(svtkm::cont::DeviceAdapterTagSerial{});
    colorTable.Sample(static_cast<svtkm::Int32>(numSamples), colorMap);
  }

  svtkm::Id2 dims(this->GetWidth(), this->GetHeight());

  svtkm::Id totalPixels = (x[1] - x[0]) * (y[1] - y[0]);
  svtkm::cont::ArrayHandleCounting<svtkm::Id> iterator(0, 1, totalPixels);
  svtkm::worklet::DispatcherMapField<internal::DrawColorBar> dispatcher(
    internal::DrawColorBar(dims, x, y, horizontal));
  dispatcher.Invoke(iterator, this->GetColorBuffer(), colorMap);
}

void Canvas::AddColorBar(svtkm::Float32 x,
                         svtkm::Float32 y,
                         svtkm::Float32 width,
                         svtkm::Float32 height,
                         const svtkm::cont::ColorTable& colorTable,
                         bool horizontal) const
{
  this->AddColorBar(
    svtkm::Bounds(svtkm::Range(x, x + width), svtkm::Range(y, y + height), svtkm::Range(0, 0)),
    colorTable,
    horizontal);
}

svtkm::Id2 Canvas::GetScreenPoint(svtkm::Float32 x,
                                 svtkm::Float32 y,
                                 svtkm::Float32 z,
                                 const svtkm::Matrix<svtkm::Float32, 4, 4>& transform) const
{
  svtkm::Vec4f_32 point(x, y, z, 1.0f);
  point = svtkm::MatrixMultiply(transform, point);

  svtkm::Id2 pixelPos;
  svtkm::Float32 width = static_cast<svtkm::Float32>(Internals->Width);
  svtkm::Float32 height = static_cast<svtkm::Float32>(Internals->Height);
  pixelPos[0] = static_cast<svtkm::Id>(svtkm::Round((1.0f + point[0]) * width * 0.5f + 0.5f));
  pixelPos[1] = static_cast<svtkm::Id>(svtkm::Round((1.0f + point[1]) * height * 0.5f + 0.5f));
  return pixelPos;
}

void Canvas::AddText(const svtkm::Matrix<svtkm::Float32, 4, 4>& transform,
                     svtkm::Float32 scale,
                     const svtkm::Vec2f_32& anchor,
                     const svtkm::rendering::Color& color,
                     const std::string& text,
                     const svtkm::Float32& depth) const
{
  if (!Internals->FontTexture.IsValid())
  {
    if (!LoadFont())
    {
      return;
    }
  }

  svtkm::rendering::Canvas* self = const_cast<svtkm::rendering::Canvas*>(this);
  TextRenderer fontRenderer(self, Internals->Font, Internals->FontTexture);
  fontRenderer.RenderText(transform, scale, anchor, color, text, depth);
}

void Canvas::AddText(const svtkm::Vec2f_32& position,
                     svtkm::Float32 scale,
                     svtkm::Float32 angle,
                     svtkm::Float32 windowAspect,
                     const svtkm::Vec2f_32& anchor,
                     const svtkm::rendering::Color& color,
                     const std::string& text) const
{
  svtkm::Matrix<svtkm::Float32, 4, 4> translationMatrix =
    Transform3DTranslate(position[0], position[1], 0.f);
  svtkm::Matrix<svtkm::Float32, 4, 4> scaleMatrix = Transform3DScale(1.0f / windowAspect, 1.0f, 1.0f);
  svtkm::Vec3f_32 rotationAxis(0.0f, 0.0f, 1.0f);
  svtkm::Matrix<svtkm::Float32, 4, 4> rotationMatrix = Transform3DRotate(angle, rotationAxis);
  svtkm::Matrix<svtkm::Float32, 4, 4> transform =
    svtkm::MatrixMultiply(translationMatrix, svtkm::MatrixMultiply(scaleMatrix, rotationMatrix));

  this->AddText(transform, scale, anchor, color, text, 0.f);
}

void Canvas::AddText(svtkm::Float32 x,
                     svtkm::Float32 y,
                     svtkm::Float32 scale,
                     svtkm::Float32 angle,
                     svtkm::Float32 windowAspect,
                     svtkm::Float32 anchorX,
                     svtkm::Float32 anchorY,
                     const svtkm::rendering::Color& color,
                     const std::string& text) const
{
  this->AddText(svtkm::make_Vec(x, y),
                scale,
                angle,
                windowAspect,
                svtkm::make_Vec(anchorX, anchorY),
                color,
                text);
}

bool Canvas::LoadFont() const
{
  Internals->Font = BitmapFontFactory::CreateLiberation2Sans();
  const std::vector<unsigned char>& rawPNG = Internals->Font.GetRawImageData();
  std::vector<unsigned char> rgba;
  unsigned long textureWidth, textureHeight;
  auto error = DecodePNG(rgba, textureWidth, textureHeight, &rawPNG[0], rawPNG.size());
  if (error != 0)
  {
    return false;
  }
  std::size_t numValues = textureWidth * textureHeight;
  std::vector<unsigned char> alpha(numValues);
  for (std::size_t i = 0; i < numValues; ++i)
  {
    alpha[i] = rgba[i * 4 + 3];
  }
  svtkm::cont::ArrayHandle<svtkm::UInt8> textureHandle = svtkm::cont::make_ArrayHandle(alpha);
  Internals->FontTexture =
    FontTextureType(svtkm::Id(textureWidth), svtkm::Id(textureHeight), textureHandle);
  Internals->FontTexture.SetFilterMode(TextureFilterMode::Linear);
  Internals->FontTexture.SetWrapMode(TextureWrapMode::Clamp);
  return true;
}

const svtkm::Matrix<svtkm::Float32, 4, 4>& Canvas::GetModelView() const
{
  return Internals->ModelView;
}

const svtkm::Matrix<svtkm::Float32, 4, 4>& Canvas::GetProjection() const
{
  return Internals->Projection;
}

void Canvas::SetViewToWorldSpace(const svtkm::rendering::Camera& camera, bool svtkmNotUsed(clip))
{
  Internals->ModelView = camera.CreateViewMatrix();
  Internals->Projection = camera.CreateProjectionMatrix(GetWidth(), GetHeight());
}

void Canvas::SetViewToScreenSpace(const svtkm::rendering::Camera& svtkmNotUsed(camera),
                                  bool svtkmNotUsed(clip))
{
  svtkm::MatrixIdentity(Internals->ModelView);
  svtkm::MatrixIdentity(Internals->Projection);
  Internals->Projection[2][2] = -1.0f;
}

void Canvas::SaveAs(const std::string& fileName) const
{
  this->RefreshColorBuffer();
  std::ofstream of(fileName.c_str(), std::ios_base::binary | std::ios_base::out);
  svtkm::Id width = GetWidth();
  svtkm::Id height = GetHeight();
  of << "P6" << std::endl << width << " " << height << std::endl << 255 << std::endl;
  ColorBufferType::PortalConstControl colorPortal = GetColorBuffer().GetPortalConstControl();
  for (svtkm::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (svtkm::Id xIndex = 0; xIndex < width; xIndex++)
    {
      svtkm::Vec4f_32 tuple = colorPortal.Get(yIndex * width + xIndex);
      of << (unsigned char)(tuple[0] * 255);
      of << (unsigned char)(tuple[1] * 255);
      of << (unsigned char)(tuple[2] * 255);
    }
  }
  of.close();
}

svtkm::rendering::WorldAnnotator* Canvas::CreateWorldAnnotator() const
{
  return new svtkm::rendering::WorldAnnotator(this);
}
}
} // svtkm::rendering
