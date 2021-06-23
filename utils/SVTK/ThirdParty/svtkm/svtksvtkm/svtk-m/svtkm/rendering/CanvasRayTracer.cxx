//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/CanvasRayTracer.h>

#include <svtkm/cont/TryExecute.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{
namespace internal
{

class SurfaceConverter : public svtkm::worklet::WorkletMapField
{
  svtkm::Matrix<svtkm::Float32, 4, 4> ViewProjMat;

public:
  SVTKM_CONT
  SurfaceConverter(const svtkm::Matrix<svtkm::Float32, 4, 4> viewProjMat)
    : ViewProjMat(viewProjMat)
  {
  }

  using ControlSignature =
    void(FieldIn, WholeArrayInOut, FieldIn, FieldIn, FieldIn, WholeArrayOut, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, WorkIndex);
  template <typename Precision,
            typename ColorPortalType,
            typename DepthBufferPortalType,
            typename ColorBufferPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& pixelIndex,
                            ColorPortalType& colorBufferIn,
                            const Precision& inDepth,
                            const svtkm::Vec<Precision, 3>& origin,
                            const svtkm::Vec<Precision, 3>& dir,
                            DepthBufferPortalType& depthBuffer,
                            ColorBufferPortalType& colorBuffer,
                            const svtkm::Id& index) const
  {
    svtkm::Vec<Precision, 3> intersection = origin + inDepth * dir;
    svtkm::Vec4f_32 point;
    point[0] = static_cast<svtkm::Float32>(intersection[0]);
    point[1] = static_cast<svtkm::Float32>(intersection[1]);
    point[2] = static_cast<svtkm::Float32>(intersection[2]);
    point[3] = 1.f;

    svtkm::Vec4f_32 newpoint;
    newpoint = svtkm::MatrixMultiply(this->ViewProjMat, point);
    newpoint[0] = newpoint[0] / newpoint[3];
    newpoint[1] = newpoint[1] / newpoint[3];
    newpoint[2] = newpoint[2] / newpoint[3];

    svtkm::Float32 depth = newpoint[2];

    depth = 0.5f * (depth) + 0.5f;
    svtkm::Vec4f_32 color;
    color[0] = static_cast<svtkm::Float32>(colorBufferIn.Get(index * 4 + 0));
    color[1] = static_cast<svtkm::Float32>(colorBufferIn.Get(index * 4 + 1));
    color[2] = static_cast<svtkm::Float32>(colorBufferIn.Get(index * 4 + 2));
    color[3] = static_cast<svtkm::Float32>(colorBufferIn.Get(index * 4 + 3));
    // blend the mapped color with existing canvas color
    svtkm::Vec4f_32 inColor = colorBuffer.Get(pixelIndex);

    // if transparency exists, all alphas have been pre-multiplied
    svtkm::Float32 alpha = (1.f - color[3]);
    color[0] = color[0] + inColor[0] * alpha;
    color[1] = color[1] + inColor[1] * alpha;
    color[2] = color[2] + inColor[2] * alpha;
    color[3] = inColor[3] * alpha + color[3];

    // clamp
    for (svtkm::Int32 i = 0; i < 4; ++i)
    {
      color[i] = svtkm::Min(1.f, svtkm::Max(color[i], 0.f));
    }
    // The existing depth should already been feed into the ray mapper
    // so no color contribution will exist past the existing depth.

    depthBuffer.Set(pixelIndex, depth);
    colorBuffer.Set(pixelIndex, color);
  }
}; //class SurfaceConverter

template <typename Precision>
SVTKM_CONT void WriteToCanvas(const svtkm::rendering::raytracing::Ray<Precision>& rays,
                             const svtkm::cont::ArrayHandle<Precision>& colors,
                             const svtkm::rendering::Camera& camera,
                             svtkm::rendering::CanvasRayTracer* canvas)
{
  svtkm::Matrix<svtkm::Float32, 4, 4> viewProjMat =
    svtkm::MatrixMultiply(camera.CreateProjectionMatrix(canvas->GetWidth(), canvas->GetHeight()),
                         camera.CreateViewMatrix());

  svtkm::worklet::DispatcherMapField<SurfaceConverter>(SurfaceConverter(viewProjMat))
    .Invoke(rays.PixelIdx,
            colors,
            rays.Distance,
            rays.Origin,
            rays.Dir,
            canvas->GetDepthBuffer(),
            canvas->GetColorBuffer());

  //Force the transfer so the vectors contain data from device
  canvas->GetColorBuffer().GetPortalControl().Get(0);
  canvas->GetDepthBuffer().GetPortalControl().Get(0);
}

} // namespace internal

CanvasRayTracer::CanvasRayTracer(svtkm::Id width, svtkm::Id height)
  : Canvas(width, height)
{
}

CanvasRayTracer::~CanvasRayTracer()
{
}

void CanvasRayTracer::WriteToCanvas(const svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays,
                                    const svtkm::cont::ArrayHandle<svtkm::Float32>& colors,
                                    const svtkm::rendering::Camera& camera)
{
  internal::WriteToCanvas(rays, colors, camera, this);
}

void CanvasRayTracer::WriteToCanvas(const svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays,
                                    const svtkm::cont::ArrayHandle<svtkm::Float64>& colors,
                                    const svtkm::rendering::Camera& camera)
{
  internal::WriteToCanvas(rays, colors, camera, this);
}

svtkm::rendering::Canvas* CanvasRayTracer::NewCopy() const
{
  return new svtkm::rendering::CanvasRayTracer(*this);
}
}
}
