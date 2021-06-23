//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Ray_Operations_h
#define svtk_m_rendering_raytracing_Ray_Operations_h

#include <svtkm/Matrix.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/raytracing/ChannelBufferOperations.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/rendering/raytracing/Worklets.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class RayStatusFilter : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  RayStatusFilter() {}
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);
  SVTKM_EXEC
  void operator()(const svtkm::Id& hitIndex, svtkm::UInt8& rayStatus) const
  {
    if (hitIndex == -1)
      rayStatus = RAY_EXITED_DOMAIN;
    else if (rayStatus != RAY_EXITED_DOMAIN && rayStatus != RAY_TERMINATED)
      rayStatus = RAY_ACTIVE;
    //else printf("Bad status state %d \n",(int)rayStatus);
  }
}; //class RayStatusFileter

class RayMapCanvas : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Matrix<svtkm::Float32, 4, 4> InverseProjView;
  svtkm::Id Width;
  svtkm::Float32 DoubleInvHeight;
  svtkm::Float32 DoubleInvWidth;
  svtkm::Vec3f_32 Origin;

public:
  SVTKM_CONT
  RayMapCanvas(const svtkm::Matrix<svtkm::Float32, 4, 4>& inverseProjView,
               const svtkm::Id width,
               const svtkm::Id height,
               const svtkm::Vec3f_32& origin)
    : InverseProjView(inverseProjView)
    , Width(width)
    , Origin(origin)
  {
    SVTKM_ASSERT(width > 0);
    SVTKM_ASSERT(height > 0);
    DoubleInvHeight = 2.f / static_cast<svtkm::Float32>(height);
    DoubleInvWidth = 2.f / static_cast<svtkm::Float32>(width);
  }

  using ControlSignature = void(FieldIn, FieldInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename Precision, typename DepthPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& pixelId,
                            Precision& maxDistance,
                            const DepthPortalType& depths) const
  {
    svtkm::Vec4f_32 position;
    position[0] = static_cast<svtkm::Float32>(pixelId % Width);
    position[1] = static_cast<svtkm::Float32>(pixelId / Width);
    position[2] = static_cast<svtkm::Float32>(depths.Get(pixelId));
    position[3] = 1;
    // transform into normalized device coordinates (-1,1)
    position[0] = position[0] * DoubleInvWidth - 1.f;
    position[1] = position[1] * DoubleInvHeight - 1.f;
    position[2] = 2.f * position[2] - 1.f;
    // offset so we don't go all the way to the same point
    position[2] -= 0.00001f;
    position = svtkm::MatrixMultiply(InverseProjView, position);
    svtkm::Vec3f_32 p;
    p[0] = position[0] / position[3];
    p[1] = position[1] / position[3];
    p[2] = position[2] / position[3];
    p = p - Origin;

    maxDistance = svtkm::Magnitude(p);
  }
}; //class RayMapMinDistances

} // namespace detail
class RayOperations
{
public:
  template <typename T>
  static void ResetStatus(Ray<T>& rays, svtkm::UInt8 status)
  {
    svtkm::cont::ArrayHandleConstant<svtkm::UInt8> statusHandle(status, rays.NumRays);
    svtkm::cont::Algorithm::Copy(statusHandle, rays.Status);
  }

  //
  // Some worklets like triangle intersection do not set the
  // ray status, so this operation sets the status based on
  // the ray hit index
  //
  template <typename Device, typename T>
  static void UpdateRayStatus(Ray<T>& rays, Device)
  {
    svtkm::worklet::DispatcherMapField<detail::RayStatusFilter> dispatcher{ (
      detail::RayStatusFilter{}) };
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(rays.HitIdx, rays.Status);
  }

  template <typename T>
  static void UpdateRayStatus(Ray<T>& rays)
  {
    svtkm::worklet::DispatcherMapField<detail::RayStatusFilter> dispatcher{ (
      detail::RayStatusFilter{}) };
    dispatcher.Invoke(rays.HitIdx, rays.Status);
  }

  static void MapCanvasToRays(Ray<svtkm::Float32>& rays,
                              const svtkm::rendering::Camera& camera,
                              const svtkm::rendering::CanvasRayTracer& canvas);

  template <typename T>
  static svtkm::Id RaysInMesh(Ray<T>& rays)
  {
    svtkm::Vec<UInt8, 2> maskValues;
    maskValues[0] = RAY_ACTIVE;
    maskValues[1] = RAY_LOST;

    svtkm::cont::ArrayHandle<svtkm::UInt8> masks;

    svtkm::worklet::DispatcherMapField<ManyMask<svtkm::UInt8, 2>> dispatcher{ (
      ManyMask<svtkm::UInt8, 2>{ maskValues }) };
    dispatcher.Invoke(rays.Status, masks);
    svtkm::cont::ArrayHandleCast<svtkm::Id, svtkm::cont::ArrayHandle<svtkm::UInt8>> castedMasks(masks);
    const svtkm::Id initVal = 0;
    svtkm::Id count = svtkm::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static svtkm::Id GetStatusCount(Ray<T>& rays, svtkm::Id status)
  {
    svtkm::UInt8 statusUInt8;
    if (status < 0 || status > 255)
    {
      throw svtkm::cont::ErrorBadValue("Rays GetStatusCound: invalid status");
    }

    statusUInt8 = static_cast<svtkm::UInt8>(status);
    svtkm::cont::ArrayHandle<svtkm::UInt8> masks;

    svtkm::worklet::DispatcherMapField<Mask<svtkm::UInt8>> dispatcher{ (
      Mask<svtkm::UInt8>{ statusUInt8 }) };
    dispatcher.Invoke(rays.Status, masks);
    svtkm::cont::ArrayHandleCast<svtkm::Id, svtkm::cont::ArrayHandle<svtkm::UInt8>> castedMasks(masks);
    const svtkm::Id initVal = 0;
    svtkm::Id count = svtkm::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static svtkm::Id RaysProcessed(Ray<T>& rays)
  {
    svtkm::Vec<UInt8, 3> maskValues;
    maskValues[0] = RAY_TERMINATED;
    maskValues[1] = RAY_EXITED_DOMAIN;
    maskValues[2] = RAY_ABANDONED;

    svtkm::cont::ArrayHandle<svtkm::UInt8> masks;

    svtkm::worklet::DispatcherMapField<ManyMask<svtkm::UInt8, 3>> dispatcher{ (
      ManyMask<svtkm::UInt8, 3>{ maskValues }) };
    dispatcher.Invoke(rays.Status, masks);
    svtkm::cont::ArrayHandleCast<svtkm::Id, svtkm::cont::ArrayHandle<svtkm::UInt8>> castedMasks(masks);
    const svtkm::Id initVal = 0;
    svtkm::Id count = svtkm::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static svtkm::cont::ArrayHandle<svtkm::UInt8> CompactActiveRays(Ray<T>& rays)
  {
    svtkm::Vec<UInt8, 1> maskValues;
    maskValues[0] = RAY_ACTIVE;
    svtkm::UInt8 statusUInt8 = static_cast<svtkm::UInt8>(RAY_ACTIVE);
    svtkm::cont::ArrayHandle<svtkm::UInt8> masks;

    svtkm::worklet::DispatcherMapField<Mask<svtkm::UInt8>> dispatcher{ (
      Mask<svtkm::UInt8>{ statusUInt8 }) };
    dispatcher.Invoke(rays.Status, masks);

    svtkm::cont::ArrayHandle<T> emptyHandle;

    rays.Normal =
      svtkm::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);
    rays.Origin =
      svtkm::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);
    rays.Dir = svtkm::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);

    const svtkm::Int32 numFloatArrays = 18;
    svtkm::cont::ArrayHandle<T>* floatArrayPointers[numFloatArrays];
    floatArrayPointers[0] = &rays.OriginX;
    floatArrayPointers[1] = &rays.OriginY;
    floatArrayPointers[2] = &rays.OriginZ;
    floatArrayPointers[3] = &rays.DirX;
    floatArrayPointers[4] = &rays.DirY;
    floatArrayPointers[5] = &rays.DirZ;
    floatArrayPointers[6] = &rays.Distance;
    floatArrayPointers[7] = &rays.MinDistance;
    floatArrayPointers[8] = &rays.MaxDistance;

    floatArrayPointers[9] = &rays.Scalar;
    floatArrayPointers[10] = &rays.IntersectionX;
    floatArrayPointers[11] = &rays.IntersectionY;
    floatArrayPointers[12] = &rays.IntersectionZ;
    floatArrayPointers[13] = &rays.U;
    floatArrayPointers[14] = &rays.V;
    floatArrayPointers[15] = &rays.NormalX;
    floatArrayPointers[16] = &rays.NormalY;
    floatArrayPointers[17] = &rays.NormalZ;

    const int breakPoint = rays.IntersectionDataEnabled ? -1 : 9;
    for (int i = 0; i < numFloatArrays; ++i)
    {
      if (i == breakPoint)
      {
        break;
      }
      svtkm::cont::ArrayHandle<T> compacted;
      svtkm::cont::Algorithm::CopyIf(*floatArrayPointers[i], masks, compacted);
      *floatArrayPointers[i] = compacted;
    }

    //
    // restore the composite vectors
    //
    rays.Normal =
      svtkm::cont::make_ArrayHandleCompositeVector(rays.NormalX, rays.NormalY, rays.NormalZ);
    rays.Origin =
      svtkm::cont::make_ArrayHandleCompositeVector(rays.OriginX, rays.OriginY, rays.OriginZ);
    rays.Dir = svtkm::cont::make_ArrayHandleCompositeVector(rays.DirX, rays.DirY, rays.DirZ);

    svtkm::cont::ArrayHandle<svtkm::Id> compactedHits;
    svtkm::cont::Algorithm::CopyIf(rays.HitIdx, masks, compactedHits);
    rays.HitIdx = compactedHits;

    svtkm::cont::ArrayHandle<svtkm::Id> compactedPixels;
    svtkm::cont::Algorithm::CopyIf(rays.PixelIdx, masks, compactedPixels);
    rays.PixelIdx = compactedPixels;

    svtkm::cont::ArrayHandle<svtkm::UInt8> compactedStatus;
    svtkm::cont::Algorithm::CopyIf(rays.Status, masks, compactedStatus);
    rays.Status = compactedStatus;

    rays.NumRays = rays.Status.GetPortalConstControl().GetNumberOfValues();

    const size_t bufferCount = static_cast<size_t>(rays.Buffers.size());
    for (size_t i = 0; i < bufferCount; ++i)
    {
      ChannelBufferOperations::Compact(rays.Buffers[i], masks, rays.NumRays);
    }
    return masks;
  }

  template <typename Device, typename T>
  static void Resize(Ray<T>& rays, const svtkm::Int32 newSize, Device)
  {
    if (newSize == rays.NumRays)
      return; //nothing to do

    rays.NumRays = newSize;

    if (rays.IntersectionDataEnabled)
    {
      rays.IntersectionX.PrepareForOutput(rays.NumRays, Device());
      rays.IntersectionY.PrepareForOutput(rays.NumRays, Device());
      rays.IntersectionZ.PrepareForOutput(rays.NumRays, Device());
      rays.U.PrepareForOutput(rays.NumRays, Device());
      rays.V.PrepareForOutput(rays.NumRays, Device());
      rays.Scalar.PrepareForOutput(rays.NumRays, Device());

      rays.NormalX.PrepareForOutput(rays.NumRays, Device());
      rays.NormalY.PrepareForOutput(rays.NumRays, Device());
      rays.NormalZ.PrepareForOutput(rays.NumRays, Device());
    }

    rays.OriginX.PrepareForOutput(rays.NumRays, Device());
    rays.OriginY.PrepareForOutput(rays.NumRays, Device());
    rays.OriginZ.PrepareForOutput(rays.NumRays, Device());

    rays.DirX.PrepareForOutput(rays.NumRays, Device());
    rays.DirY.PrepareForOutput(rays.NumRays, Device());
    rays.DirZ.PrepareForOutput(rays.NumRays, Device());

    rays.Distance.PrepareForOutput(rays.NumRays, Device());
    rays.MinDistance.PrepareForOutput(rays.NumRays, Device());
    rays.MaxDistance.PrepareForOutput(rays.NumRays, Device());
    rays.Status.PrepareForOutput(rays.NumRays, Device());
    rays.HitIdx.PrepareForOutput(rays.NumRays, Device());
    rays.PixelIdx.PrepareForOutput(rays.NumRays, Device());

    const size_t bufferCount = static_cast<size_t>(rays.Buffers.size());
    for (size_t i = 0; i < bufferCount; ++i)
    {
      rays.Buffers[i].Resize(rays.NumRays, Device());
    }
  }

  template <typename T>
  static void CopyDistancesToMin(Ray<T> rays, const T offset = 0.f)
  {
    svtkm::worklet::DispatcherMapField<CopyAndOffsetMask<T>> dispatcher{ (
      CopyAndOffsetMask<T>{ offset, RAY_EXITED_MESH }) };
    dispatcher.Invoke(rays.Distance, rays.MinDistance, rays.Status);
  }
};
}
}
} // namespace vktm::rendering::raytracing
#endif
