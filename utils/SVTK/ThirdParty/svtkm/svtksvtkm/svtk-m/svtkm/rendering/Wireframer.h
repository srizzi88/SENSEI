//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_rendering_Wireframer_h
#define svtk_m_rendering_Wireframer_h

#include <svtkm/Assert.h>
#include <svtkm/Math.h>
#include <svtkm/Swap.h>
#include <svtkm/Types.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/AtomicArray.h>
#include <svtkm/rendering/MatrixHelpers.h>
#include <svtkm/rendering/Triangulator.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{
namespace
{

using ColorMapHandle = svtkm::cont::ArrayHandle<svtkm::Vec4f_32>;
using IndicesHandle = svtkm::cont::ArrayHandle<svtkm::Id2>;
using PackedFrameBufferHandle = svtkm::cont::ArrayHandle<svtkm::Int64>;

// Depth value of 1.0f
const svtkm::Int64 ClearDepth = 0x3F800000;
// Packed frame buffer value with color set as black and depth as 1.0f
const svtkm::Int64 ClearValue = 0x3F800000000000FF;

SVTKM_EXEC_CONT
svtkm::Float32 IntegerPart(svtkm::Float32 x)
{
  return svtkm::Floor(x);
}

SVTKM_EXEC_CONT
svtkm::Float32 FractionalPart(svtkm::Float32 x)
{
  return x - svtkm::Floor(x);
}

SVTKM_EXEC_CONT
svtkm::Float32 ReverseFractionalPart(svtkm::Float32 x)
{
  return 1.0f - FractionalPart(x);
}

SVTKM_EXEC_CONT
svtkm::UInt32 ScaleColorComponent(svtkm::Float32 c)
{
  svtkm::Int32 t = svtkm::Int32(c * 256.0f);
  return svtkm::UInt32(t < 0 ? 0 : (t > 255 ? 255 : t));
}

SVTKM_EXEC_CONT
svtkm::UInt32 PackColor(svtkm::Float32 r, svtkm::Float32 g, svtkm::Float32 b, svtkm::Float32 a);

SVTKM_EXEC_CONT
svtkm::UInt32 PackColor(const svtkm::Vec4f_32& color)
{
  return PackColor(color[0], color[1], color[2], color[3]);
}

SVTKM_EXEC_CONT
svtkm::UInt32 PackColor(svtkm::Float32 r, svtkm::Float32 g, svtkm::Float32 b, svtkm::Float32 a)
{
  svtkm::UInt32 packed = (ScaleColorComponent(r) << 24);
  packed |= (ScaleColorComponent(g) << 16);
  packed |= (ScaleColorComponent(b) << 8);
  packed |= ScaleColorComponent(a);
  return packed;
}

SVTKM_EXEC_CONT
void UnpackColor(svtkm::UInt32 color,
                 svtkm::Float32& r,
                 svtkm::Float32& g,
                 svtkm::Float32& b,
                 svtkm::Float32& a);

SVTKM_EXEC_CONT
void UnpackColor(svtkm::UInt32 packedColor, svtkm::Vec4f_32& color)
{
  UnpackColor(packedColor, color[0], color[1], color[2], color[3]);
}

SVTKM_EXEC_CONT
void UnpackColor(svtkm::UInt32 color,
                 svtkm::Float32& r,
                 svtkm::Float32& g,
                 svtkm::Float32& b,
                 svtkm::Float32& a)
{
  r = svtkm::Float32((color & 0xFF000000) >> 24) / 255.0f;
  g = svtkm::Float32((color & 0x00FF0000) >> 16) / 255.0f;
  b = svtkm::Float32((color & 0x0000FF00) >> 8) / 255.0f;
  a = svtkm::Float32((color & 0x000000FF)) / 255.0f;
}

union PackedValue {
  struct PackedFloats
  {
    svtkm::Float32 Color;
    svtkm::Float32 Depth;
  } Floats;
  struct PackedInts
  {
    svtkm::UInt32 Color;
    svtkm::UInt32 Depth;
  } Ints;
  svtkm::Int64 Raw;
}; // union PackedValue

struct CopyIntoFrameBuffer : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  SVTKM_CONT
  CopyIntoFrameBuffer() {}

  SVTKM_EXEC
  void operator()(const svtkm::Vec4f_32& color,
                  const svtkm::Float32& depth,
                  svtkm::Int64& outValue) const
  {
    PackedValue packed;
    packed.Ints.Color = PackColor(color);
    packed.Floats.Depth = depth;
    outValue = packed.Raw;
  }
}; //struct CopyIntoFrameBuffer

template <typename DeviceTag>
class EdgePlotter : public svtkm::worklet::WorkletMapField
{
public:
  using AtomicPackedFrameBufferHandle =
    svtkm::exec::AtomicArrayExecutionObject<svtkm::Int64, DeviceTag>;
  using AtomicPackedFrameBuffer = svtkm::cont::AtomicArray<svtkm::Int64>;

  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  SVTKM_CONT
  EdgePlotter(const svtkm::Matrix<svtkm::Float32, 4, 4>& worldToProjection,
              svtkm::Id width,
              svtkm::Id height,
              svtkm::Id subsetWidth,
              svtkm::Id subsetHeight,
              svtkm::Id xOffset,
              svtkm::Id yOffset,
              bool assocPoints,
              const svtkm::Range& fieldRange,
              const ColorMapHandle& colorMap,
              const AtomicPackedFrameBuffer& frameBuffer,
              const svtkm::Range& clippingRange)
    : WorldToProjection(worldToProjection)
    , Width(width)
    , Height(height)
    , SubsetWidth(subsetWidth)
    , SubsetHeight(subsetHeight)
    , XOffset(xOffset)
    , YOffset(yOffset)
    , AssocPoints(assocPoints)
    , ColorMap(colorMap.PrepareForInput(DeviceTag()))
    , ColorMapSize(svtkm::Float32(colorMap.GetNumberOfValues() - 1))
    , FrameBuffer(frameBuffer.PrepareForExecution(DeviceTag()))
    , FieldMin(svtkm::Float32(fieldRange.Min))
  {
    InverseFieldDelta = 1.0f / svtkm::Float32(fieldRange.Length());
    Offset = svtkm::Max(0.03f / svtkm::Float32(clippingRange.Length()), 0.0001f);
  }

  template <typename CoordinatesPortalType, typename ScalarFieldPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id2& edgeIndices,
                            const CoordinatesPortalType& coordsPortal,
                            const ScalarFieldPortalType& fieldPortal) const
  {
    svtkm::Id point1Idx = edgeIndices[0];
    svtkm::Id point2Idx = edgeIndices[1];

    svtkm::Vec3f_32 point1 = coordsPortal.Get(edgeIndices[0]);
    svtkm::Vec3f_32 point2 = coordsPortal.Get(edgeIndices[1]);

    TransformWorldToViewport(point1);
    TransformWorldToViewport(point2);

    svtkm::Float32 x1 = svtkm::Round(point1[0]);
    svtkm::Float32 y1 = svtkm::Round(point1[1]);
    svtkm::Float32 z1 = point1[2];
    svtkm::Float32 x2 = svtkm::Round(point2[0]);
    svtkm::Float32 y2 = svtkm::Round(point2[1]);
    svtkm::Float32 z2 = point2[2];
    // If the line is steep, i.e., the height is greater than the width, then
    // transpose the co-ordinates to prevent "holes" in the line. This ensures
    // that we pick the co-ordinate which grows at a lesser rate than the other.
    bool transposed = svtkm::Abs(y2 - y1) > svtkm::Abs(x2 - x1);
    if (transposed)
    {
      svtkm::Swap(x1, y1);
      svtkm::Swap(x2, y2);
    }

    // Ensure we are always going from left to right
    if (x1 > x2)
    {
      svtkm::Swap(x1, x2);
      svtkm::Swap(y1, y2);
      svtkm::Swap(z1, z2);
    }

    svtkm::Float32 dx = x2 - x1;
    svtkm::Float32 dy = y2 - y1;
    svtkm::Float32 gradient = (dx == 0.0) ? 1.0f : (dy / dx);

    svtkm::Float32 xEnd = svtkm::Round(x1);
    svtkm::Float32 yEnd = y1 + gradient * (xEnd - x1);
    svtkm::Float32 xPxl1 = xEnd, yPxl1 = IntegerPart(yEnd);
    svtkm::Float32 zPxl1 = svtkm::Lerp(z1, z2, (xPxl1 - x1) / dx);
    svtkm::Float64 point1Field = fieldPortal.Get(point1Idx);
    svtkm::Float64 point2Field;
    if (AssocPoints)
    {
      point2Field = fieldPortal.Get(point2Idx);
    }
    else
    {
      // cell associated field has a solid line color
      point2Field = point1Field;
    }

    // Plot first endpoint
    svtkm::Vec4f_32 color = GetColor(point1Field);
    if (transposed)
    {
      Plot(yPxl1, xPxl1, zPxl1, color, 1.0f);
    }
    else
    {
      Plot(xPxl1, yPxl1, zPxl1, color, 1.0f);
    }

    svtkm::Float32 interY = yEnd + gradient;
    xEnd = svtkm::Round(x2);
    yEnd = y2 + gradient * (xEnd - x2);
    svtkm::Float32 xPxl2 = xEnd, yPxl2 = IntegerPart(yEnd);
    svtkm::Float32 zPxl2 = svtkm::Lerp(z1, z2, (xPxl2 - x1) / dx);

    // Plot second endpoint
    color = GetColor(point2Field);
    if (transposed)
    {
      Plot(yPxl2, xPxl2, zPxl2, color, 1.0f);
    }
    else
    {
      Plot(xPxl2, yPxl2, zPxl2, color, 1.0f);
    }

    // Plot rest of the line
    if (transposed)
    {
      for (svtkm::Float32 x = xPxl1 + 1; x <= xPxl2 - 1; ++x)
      {
        svtkm::Float32 t = IntegerPart(interY);
        svtkm::Float32 factor = (x - x1) / dx;
        svtkm::Float32 depth = svtkm::Lerp(zPxl1, zPxl2, factor);
        svtkm::Float64 fieldValue = svtkm::Lerp(point1Field, point2Field, factor);
        color = GetColor(fieldValue);
        Plot(t, x, depth, color, ReverseFractionalPart(interY));
        Plot(t + 1, x, depth, color, FractionalPart(interY));
        interY += gradient;
      }
    }
    else
    {
      for (svtkm::Float32 x = xPxl1 + 1; x <= xPxl2 - 1; ++x)
      {
        svtkm::Float32 t = IntegerPart(interY);
        svtkm::Float32 factor = (x - x1) / dx;
        svtkm::Float32 depth = svtkm::Lerp(zPxl1, zPxl2, factor);
        svtkm::Float64 fieldValue = svtkm::Lerp(point1Field, point2Field, factor);
        color = GetColor(fieldValue);
        Plot(x, t, depth, color, ReverseFractionalPart(interY));
        Plot(x, t + 1, depth, color, FractionalPart(interY));
        interY += gradient;
      }
    }
  }

private:
  using ColorMapPortalConst = typename ColorMapHandle::ExecutionTypes<DeviceTag>::PortalConst;

  SVTKM_EXEC
  void TransformWorldToViewport(svtkm::Vec3f_32& point) const
  {
    svtkm::Vec4f_32 temp(point[0], point[1], point[2], 1.0f);
    temp = svtkm::MatrixMultiply(WorldToProjection, temp);
    for (svtkm::IdComponent i = 0; i < 3; ++i)
    {
      point[i] = temp[i] / temp[3];
    }
    // Scale to canvas width and height
    point[0] = (point[0] * 0.5f + 0.5f) * svtkm::Float32(SubsetWidth) + svtkm::Float32(XOffset);
    point[1] = (point[1] * 0.5f + 0.5f) * svtkm::Float32(SubsetHeight) + svtkm::Float32(YOffset);
    // Convert from -1/+1 to 0/+1 range
    point[2] = point[2] * 0.5f + 0.5f;
    // Offset the point to a bit towards the camera. This is to ensure that the front faces of
    // the wireframe wins the z-depth check against the surface render, and is in addition to the
    // existing camera space offset.
    point[2] -= Offset;
  }

  SVTKM_EXEC svtkm::Vec4f_32 GetColor(svtkm::Float64 fieldValue) const
  {
    svtkm::Int32 colorIdx =
      svtkm::Int32((svtkm::Float32(fieldValue) - FieldMin) * ColorMapSize * InverseFieldDelta);
    colorIdx = svtkm::Min(svtkm::Int32(ColorMap.GetNumberOfValues() - 1), svtkm::Max(0, colorIdx));
    return ColorMap.Get(colorIdx);
  }

  SVTKM_EXEC
  void Plot(svtkm::Float32 x,
            svtkm::Float32 y,
            svtkm::Float32 depth,
            const svtkm::Vec4f_32& color,
            svtkm::Float32 intensity) const
  {
    svtkm::Id xi = static_cast<svtkm::Id>(x), yi = static_cast<svtkm::Id>(y);
    if (xi < 0 || xi >= Width || yi < 0 || yi >= Height)
    {
      return;
    }
    svtkm::Id index = yi * Width + xi;
    PackedValue current, next;
    current.Raw = ClearValue;
    next.Floats.Depth = depth;
    svtkm::Vec4f_32 blendedColor;
    svtkm::Vec4f_32 srcColor;
    do
    {
      UnpackColor(current.Ints.Color, srcColor);
      svtkm::Float32 inverseIntensity = (1.0f - intensity);
      svtkm::Float32 alpha = srcColor[3] * inverseIntensity;
      blendedColor[0] = color[0] * intensity + srcColor[0] * alpha;
      blendedColor[1] = color[1] * intensity + srcColor[1] * alpha;
      blendedColor[2] = color[2] * intensity + srcColor[2] * alpha;
      blendedColor[3] = alpha + intensity;
      next.Ints.Color = PackColor(blendedColor);
      current.Raw = FrameBuffer.CompareAndSwap(index, next.Raw, current.Raw);
    } while (current.Floats.Depth > next.Floats.Depth);
  }

  svtkm::Matrix<svtkm::Float32, 4, 4> WorldToProjection;
  svtkm::Id Width;
  svtkm::Id Height;
  svtkm::Id SubsetWidth;
  svtkm::Id SubsetHeight;
  svtkm::Id XOffset;
  svtkm::Id YOffset;
  bool AssocPoints;
  ColorMapPortalConst ColorMap;
  svtkm::Float32 ColorMapSize;
  AtomicPackedFrameBufferHandle FrameBuffer;
  svtkm::Float32 FieldMin;
  svtkm::Float32 InverseFieldDelta;
  svtkm::Float32 Offset;
};

struct BufferConverter : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  BufferConverter() {}

  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);

  template <typename DepthBufferPortalType, typename ColorBufferPortalType>
  SVTKM_EXEC void operator()(const svtkm::Int64& packedValue,
                            DepthBufferPortalType& depthBuffer,
                            ColorBufferPortalType& colorBuffer,
                            const svtkm::Id& index) const
  {
    PackedValue packed;
    packed.Raw = packedValue;
    float depth = packed.Floats.Depth;
    if (depth <= depthBuffer.Get(index))
    {
      svtkm::Vec4f_32 color;
      UnpackColor(packed.Ints.Color, color);
      colorBuffer.Set(index, color);
      depthBuffer.Set(index, depth);
    }
  }
};

} // namespace

class Wireframer
{
public:
  SVTKM_CONT
  Wireframer(svtkm::rendering::Canvas* canvas, bool showInternalZones, bool isOverlay)
    : Canvas(canvas)
    , ShowInternalZones(showInternalZones)
    , IsOverlay(isOverlay)
  {
  }

  SVTKM_CONT
  void SetCamera(const svtkm::rendering::Camera& camera) { this->Camera = camera; }

  SVTKM_CONT
  void SetColorMap(const ColorMapHandle& colorMap) { this->ColorMap = colorMap; }

  SVTKM_CONT
  void SetSolidDepthBuffer(const svtkm::cont::ArrayHandle<svtkm::Float32> depthBuffer)
  {
    this->SolidDepthBuffer = depthBuffer;
  }

  SVTKM_CONT
  void SetData(const svtkm::cont::CoordinateSystem& coords,
               const IndicesHandle& endPointIndices,
               const svtkm::cont::Field& field,
               const svtkm::Range& fieldRange)
  {
    this->Bounds = coords.GetBounds();
    this->Coordinates = coords;
    this->PointIndices = endPointIndices;
    this->ScalarField = field;
    this->ScalarFieldRange = fieldRange;
  }

  SVTKM_CONT
  void Render()
  {
    RenderWithDeviceFunctor functor(this);
    svtkm::cont::TryExecute(functor);
  }

private:
  template <typename DeviceTag>
  SVTKM_CONT void RenderWithDevice(DeviceTag)
  {

    // The wireframe should appear on top of any prerendered data, and hide away the internal
    // zones if `ShowInternalZones` is set to false. Since the prerendered data (or the solid
    // depth buffer) could cause z-fighting with the wireframe, we will offset all the edges in Z
    // by a small amount, proportional to distance between the near and far camera planes, in the
    // camera space.
    svtkm::Range clippingRange = Camera.GetClippingRange();
    svtkm::Float64 offset1 = (clippingRange.Max - clippingRange.Min) / 1.0e4;
    svtkm::Float64 offset2 = clippingRange.Min / 2.0;
    svtkm::Float32 offset = static_cast<svtkm::Float32>(svtkm::Min(offset1, offset2));
    svtkm::Matrix<svtkm::Float32, 4, 4> modelMatrix;
    svtkm::MatrixIdentity(modelMatrix);
    modelMatrix[2][3] = offset;
    svtkm::Matrix<svtkm::Float32, 4, 4> worldToCamera =
      svtkm::MatrixMultiply(modelMatrix, Camera.CreateViewMatrix());
    svtkm::Matrix<svtkm::Float32, 4, 4> WorldToProjection = svtkm::MatrixMultiply(
      Camera.CreateProjectionMatrix(Canvas->GetWidth(), Canvas->GetHeight()), worldToCamera);

    svtkm::Id width = static_cast<svtkm::Id>(Canvas->GetWidth());
    svtkm::Id height = static_cast<svtkm::Id>(Canvas->GetHeight());
    svtkm::Id pixelCount = width * height;
    FrameBuffer.PrepareForOutput(pixelCount, DeviceTag());

    if (ShowInternalZones && !IsOverlay)
    {
      svtkm::cont::ArrayHandleConstant<svtkm::Int64> clear(ClearValue, pixelCount);
      svtkm::cont::Algorithm::Copy(clear, FrameBuffer);
    }
    else
    {
      SVTKM_ASSERT(SolidDepthBuffer.GetNumberOfValues() == pixelCount);
      CopyIntoFrameBuffer bufferCopy;
      svtkm::worklet::DispatcherMapField<CopyIntoFrameBuffer>(bufferCopy)
        .Invoke(Canvas->GetColorBuffer(), SolidDepthBuffer, FrameBuffer);
    }
    //
    // detect a 2D camera and set the correct viewport.
    // The View port specifies what the region of the screen
    // to draw to which baiscally modifies the width and the
    // height of the "canvas"
    //
    svtkm::Id xOffset = 0;
    svtkm::Id yOffset = 0;
    svtkm::Id subsetWidth = width;
    svtkm::Id subsetHeight = height;

    bool ortho2d = Camera.GetMode() == svtkm::rendering::Camera::MODE_2D;
    if (ortho2d)
    {
      svtkm::Float32 vl, vr, vb, vt;
      Camera.GetRealViewport(width, height, vl, vr, vb, vt);
      svtkm::Float32 _x = static_cast<svtkm::Float32>(width) * (1.f + vl) / 2.f;
      svtkm::Float32 _y = static_cast<svtkm::Float32>(height) * (1.f + vb) / 2.f;
      svtkm::Float32 _w = static_cast<svtkm::Float32>(width) * (vr - vl) / 2.f;
      svtkm::Float32 _h = static_cast<svtkm::Float32>(height) * (vt - vb) / 2.f;

      subsetWidth = static_cast<svtkm::Id>(_w);
      subsetHeight = static_cast<svtkm::Id>(_h);
      yOffset = static_cast<svtkm::Id>(_y);
      xOffset = static_cast<svtkm::Id>(_x);
    }

    const bool isSupportedField = ScalarField.IsFieldCell() || ScalarField.IsFieldPoint();
    if (!isSupportedField)
    {
      throw svtkm::cont::ErrorBadValue("Field not associated with cell set or points");
    }
    const bool isAssocPoints = ScalarField.IsFieldPoint();

    EdgePlotter<DeviceTag> plotter(WorldToProjection,
                                   width,
                                   height,
                                   subsetWidth,
                                   subsetHeight,
                                   xOffset,
                                   yOffset,
                                   isAssocPoints,
                                   ScalarFieldRange,
                                   ColorMap,
                                   FrameBuffer,
                                   Camera.GetClippingRange());
    svtkm::worklet::DispatcherMapField<EdgePlotter<DeviceTag>> plotterDispatcher(plotter);
    plotterDispatcher.SetDevice(DeviceTag());
    plotterDispatcher.Invoke(
      PointIndices, Coordinates, ScalarField.GetData().ResetTypes(svtkm::TypeListFieldScalar()));

    BufferConverter converter;
    svtkm::worklet::DispatcherMapField<BufferConverter> converterDispatcher(converter);
    converterDispatcher.SetDevice(DeviceTag());
    converterDispatcher.Invoke(FrameBuffer, Canvas->GetDepthBuffer(), Canvas->GetColorBuffer());
  }

  SVTKM_CONT
  struct RenderWithDeviceFunctor
  {
    Wireframer* Renderer;

    RenderWithDeviceFunctor(Wireframer* renderer)
      : Renderer(renderer)
    {
    }

    template <typename DeviceTag>
    SVTKM_CONT bool operator()(DeviceTag)
    {
      SVTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);
      Renderer->RenderWithDevice(DeviceTag());
      return true;
    }
  };

  svtkm::Bounds Bounds;
  svtkm::rendering::Camera Camera;
  svtkm::rendering::Canvas* Canvas;
  bool ShowInternalZones;
  bool IsOverlay;
  ColorMapHandle ColorMap;
  svtkm::cont::CoordinateSystem Coordinates;
  IndicesHandle PointIndices;
  svtkm::cont::Field ScalarField;
  svtkm::Range ScalarFieldRange;
  svtkm::cont::ArrayHandle<svtkm::Float32> SolidDepthBuffer;
  PackedFrameBufferHandle FrameBuffer;
}; // class Wireframer
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_Wireframer_h
