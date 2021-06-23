//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Ray_h
#define svtk_m_rendering_raytracing_Ray_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/rendering/raytracing/ChannelBuffer.h>
#include <svtkm/rendering/raytracing/Worklets.h>

#include <vector>

#define RAY_ACTIVE 0
#define RAY_COMPLETE 1
#define RAY_TERMINATED 2
#define RAY_EXITED_MESH 3
#define RAY_EXITED_DOMAIN 4
#define RAY_LOST 5
#define RAY_ABANDONED 6
#define RAY_TUG_EPSILON 0.001

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

template <typename Precision>
class Ray
{
protected:
  bool IntersectionDataEnabled;

public:
  // composite vectors to hold array handles
  typename //tell the compiler we have a dependent type
    svtkm::cont::ArrayHandleCompositeVector<svtkm::cont::ArrayHandle<Precision>,
                                           svtkm::cont::ArrayHandle<Precision>,
                                           svtkm::cont::ArrayHandle<Precision>>
      Intersection;

  typename //tell the compiler we have a dependent type
    svtkm::cont::ArrayHandleCompositeVector<svtkm::cont::ArrayHandle<Precision>,
                                           svtkm::cont::ArrayHandle<Precision>,
                                           svtkm::cont::ArrayHandle<Precision>>
      Normal;

  typename //tell the compiler we have a dependent type
    svtkm::cont::ArrayHandleCompositeVector<svtkm::cont::ArrayHandle<Precision>,
                                           svtkm::cont::ArrayHandle<Precision>,
                                           svtkm::cont::ArrayHandle<Precision>>
      Origin;

  typename //tell the compiler we have a dependent type
    svtkm::cont::ArrayHandleCompositeVector<svtkm::cont::ArrayHandle<Precision>,
                                           svtkm::cont::ArrayHandle<Precision>,
                                           svtkm::cont::ArrayHandle<Precision>>
      Dir;

  svtkm::cont::ArrayHandle<Precision> IntersectionX; //ray Intersection
  svtkm::cont::ArrayHandle<Precision> IntersectionY;
  svtkm::cont::ArrayHandle<Precision> IntersectionZ;


  svtkm::cont::ArrayHandle<Precision> OriginX; //ray Origin
  svtkm::cont::ArrayHandle<Precision> OriginY;
  svtkm::cont::ArrayHandle<Precision> OriginZ;

  svtkm::cont::ArrayHandle<Precision> DirX; //ray Dir
  svtkm::cont::ArrayHandle<Precision> DirY;
  svtkm::cont::ArrayHandle<Precision> DirZ;

  svtkm::cont::ArrayHandle<Precision> U; //barycentric coordinates
  svtkm::cont::ArrayHandle<Precision> V;
  svtkm::cont::ArrayHandle<Precision> NormalX; //ray Normal
  svtkm::cont::ArrayHandle<Precision> NormalY;
  svtkm::cont::ArrayHandle<Precision> NormalZ;
  svtkm::cont::ArrayHandle<Precision> Scalar; //scalar

  svtkm::cont::ArrayHandle<Precision> Distance; //distance to hit

  svtkm::cont::ArrayHandle<svtkm::Id> HitIdx;
  svtkm::cont::ArrayHandle<svtkm::Id> PixelIdx;

  svtkm::cont::ArrayHandle<Precision> MinDistance; // distance to hit
  svtkm::cont::ArrayHandle<Precision> MaxDistance; // distance to hit
  svtkm::cont::ArrayHandle<svtkm::UInt8> Status;    // 0 = active 1 = miss 2 = lost

  std::vector<ChannelBuffer<Precision>> Buffers;
  svtkm::Id DebugWidth;
  svtkm::Id DebugHeight;
  svtkm::Id NumRays;

  SVTKM_CONT
  Ray()
  {
    IntersectionDataEnabled = false;
    NumRays = 0;
    Intersection =
      svtkm::cont::make_ArrayHandleCompositeVector(IntersectionX, IntersectionY, IntersectionZ);
    Normal = svtkm::cont::make_ArrayHandleCompositeVector(NormalX, NormalY, NormalZ);
    Origin = svtkm::cont::make_ArrayHandleCompositeVector(OriginX, OriginY, OriginZ);
    Dir = svtkm::cont::make_ArrayHandleCompositeVector(DirX, DirY, DirZ);

    ChannelBuffer<Precision> buffer;
    buffer.Resize(NumRays);
    Buffers.push_back(buffer);
    DebugWidth = -1;
    DebugHeight = -1;
  }


  struct EnableIntersectionDataFunctor
  {
    template <typename Device>
    SVTKM_CONT bool operator()(Device, Ray<Precision>* self)
    {
      SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
      self->EnableIntersectionData(Device());
      return true;
    }
  };

  void EnableIntersectionData() { svtkm::cont::TryExecute(EnableIntersectionDataFunctor(), this); }

  template <typename Device>
  void EnableIntersectionData(Device)
  {
    if (IntersectionDataEnabled)
    {
      return;
    }

    IntersectionDataEnabled = true;
    IntersectionX.PrepareForOutput(NumRays, Device());
    IntersectionY.PrepareForOutput(NumRays, Device());
    IntersectionZ.PrepareForOutput(NumRays, Device());
    U.PrepareForOutput(NumRays, Device());
    V.PrepareForOutput(NumRays, Device());
    Scalar.PrepareForOutput(NumRays, Device());

    NormalX.PrepareForOutput(NumRays, Device());
    NormalY.PrepareForOutput(NumRays, Device());
    NormalZ.PrepareForOutput(NumRays, Device());
  }

  void DisableIntersectionData()
  {
    if (!IntersectionDataEnabled)
    {
      return;
    }

    IntersectionDataEnabled = false;
    IntersectionX.ReleaseResources();
    IntersectionY.ReleaseResources();
    IntersectionZ.ReleaseResources();
    U.ReleaseResources();
    V.ReleaseResources();
    Scalar.ReleaseResources();

    NormalX.ReleaseResources();
    NormalY.ReleaseResources();
    NormalZ.ReleaseResources();
  }

  template <typename Device>
  SVTKM_CONT Ray(const svtkm::Int32 size, Device, bool enableIntersectionData = false)
  {
    NumRays = size;
    IntersectionDataEnabled = enableIntersectionData;

    ChannelBuffer<Precision> buffer;
    this->Buffers.push_back(buffer);

    DebugWidth = -1;
    DebugHeight = -1;

    this->Resize(size, Device());
  }

  struct ResizeFunctor
  {
    template <typename Device>
    SVTKM_CONT bool operator()(Device, Ray<Precision>* self, const svtkm::Int32 size)
    {
      SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
      self->Resize(size, Device());
      return true;
    }
  };

  SVTKM_CONT void Resize(const svtkm::Int32 size) { svtkm::cont::TryExecute(ResizeFunctor(), size); }

  template <typename Device>
  SVTKM_CONT void Resize(const svtkm::Int32 size, Device)
  {
    NumRays = size;

    if (IntersectionDataEnabled)
    {
      IntersectionX.PrepareForOutput(NumRays, Device());
      IntersectionY.PrepareForOutput(NumRays, Device());
      IntersectionZ.PrepareForOutput(NumRays, Device());

      U.PrepareForOutput(NumRays, Device());
      V.PrepareForOutput(NumRays, Device());

      Scalar.PrepareForOutput(NumRays, Device());

      NormalX.PrepareForOutput(NumRays, Device());
      NormalY.PrepareForOutput(NumRays, Device());
      NormalZ.PrepareForOutput(NumRays, Device());
    }

    OriginX.PrepareForOutput(NumRays, Device());
    OriginY.PrepareForOutput(NumRays, Device());
    OriginZ.PrepareForOutput(NumRays, Device());

    DirX.PrepareForOutput(NumRays, Device());
    DirY.PrepareForOutput(NumRays, Device());
    DirZ.PrepareForOutput(NumRays, Device());

    Distance.PrepareForOutput(NumRays, Device());

    MinDistance.PrepareForOutput(NumRays, Device());
    MaxDistance.PrepareForOutput(NumRays, Device());
    Status.PrepareForOutput(NumRays, Device());

    HitIdx.PrepareForOutput(NumRays, Device());
    PixelIdx.PrepareForOutput(NumRays, Device());

    Intersection =
      svtkm::cont::make_ArrayHandleCompositeVector(IntersectionX, IntersectionY, IntersectionZ);
    Normal = svtkm::cont::make_ArrayHandleCompositeVector(NormalX, NormalY, NormalZ);
    Origin = svtkm::cont::make_ArrayHandleCompositeVector(OriginX, OriginY, OriginZ);
    Dir = svtkm::cont::make_ArrayHandleCompositeVector(DirX, DirY, DirZ);

    const size_t numBuffers = this->Buffers.size();
    for (size_t i = 0; i < numBuffers; ++i)
    {
      this->Buffers[i].Resize(NumRays, Device());
    }
  }

  SVTKM_CONT
  void AddBuffer(const svtkm::Int32 numChannels, const std::string name)
  {

    ChannelBuffer<Precision> buffer(numChannels, this->NumRays);
    buffer.SetName(name);
    this->Buffers.push_back(buffer);
  }

  SVTKM_CONT
  bool HasBuffer(const std::string name)
  {
    size_t numBuffers = this->Buffers.size();
    bool found = false;
    for (size_t i = 0; i < numBuffers; ++i)
    {
      if (this->Buffers[i].GetName() == name)
      {
        found = true;
        break;
      }
    }
    return found;
  }

  SVTKM_CONT
  ChannelBuffer<Precision>& GetBuffer(const std::string name)
  {
    const size_t numBuffers = this->Buffers.size();
    bool found = false;
    size_t index = 0;
    for (size_t i = 0; i < numBuffers; ++i)
    {
      if (this->Buffers[i].GetName() == name)
      {
        found = true;
        index = i;
      }
    }
    if (found)
    {
      return this->Buffers.at(index);
    }
    else
    {
      throw svtkm::cont::ErrorBadValue("No channel buffer with requested name: " + name);
    }
  }

  void PrintRay(svtkm::Id pixelId)
  {
    for (svtkm::Id i = 0; i < NumRays; ++i)
    {
      if (PixelIdx.GetPortalControl().Get(i) == pixelId)
      {
        std::cout << "Ray " << pixelId << "\n";
        std::cout << "Origin "
                  << "[" << OriginX.GetPortalControl().Get(i) << ","
                  << OriginY.GetPortalControl().Get(i) << "," << OriginZ.GetPortalControl().Get(i)
                  << "]\n";
        std::cout << "Dir "
                  << "[" << DirX.GetPortalControl().Get(i) << "," << DirY.GetPortalControl().Get(i)
                  << "," << DirZ.GetPortalControl().Get(i) << "]\n";
      }
    }
  }

  friend class RayOperations;
}; // class ray
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_Ray_h
