//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Camera_h
#define svtk_m_rendering_raytracing_Camera_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/raytracing/Ray.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class SVTKM_RENDERING_EXPORT Camera
{

private:
  struct PixelDataFunctor;
  svtkm::rendering::CanvasRayTracer Canvas;
  svtkm::Int32 Height;
  svtkm::Int32 Width;
  svtkm::Int32 SubsetWidth;
  svtkm::Int32 SubsetHeight;
  svtkm::Int32 SubsetMinX;
  svtkm::Int32 SubsetMinY;
  svtkm::Float32 FovX;
  svtkm::Float32 FovY;
  svtkm::Float32 Zoom;
  bool IsViewDirty;

  svtkm::Vec3f_32 Look;
  svtkm::Vec3f_32 Up;
  svtkm::Vec3f_32 LookAt;
  svtkm::Vec3f_32 Position;
  svtkm::rendering::Camera CameraView;
  svtkm::Matrix<svtkm::Float32, 4, 4> ViewProjectionMat;

public:
  SVTKM_CONT
  Camera();

  SVTKM_CONT
  ~Camera();

  // cuda does not compile if this is private
  class PerspectiveRayGen;
  class Ortho2DRayGen;

  std::string ToString();

  SVTKM_CONT
  void SetParameters(const svtkm::rendering::Camera& camera,
                     svtkm::rendering::CanvasRayTracer& canvas);


  SVTKM_CONT
  void SetHeight(const svtkm::Int32& height);

  SVTKM_CONT
  void WriteSettingsToLog();

  SVTKM_CONT
  svtkm::Int32 GetHeight() const;

  SVTKM_CONT
  void SetWidth(const svtkm::Int32& width);

  SVTKM_CONT
  svtkm::Int32 GetWidth() const;

  SVTKM_CONT
  svtkm::Int32 GetSubsetWidth() const;

  SVTKM_CONT
  svtkm::Int32 GetSubsetHeight() const;

  SVTKM_CONT
  void SetZoom(const svtkm::Float32& zoom);

  SVTKM_CONT
  svtkm::Float32 GetZoom() const;

  SVTKM_CONT
  void SetFieldOfView(const svtkm::Float32& degrees);

  SVTKM_CONT
  svtkm::Float32 GetFieldOfView() const;

  SVTKM_CONT
  void SetUp(const svtkm::Vec3f_32& up);

  SVTKM_CONT
  void SetPosition(const svtkm::Vec3f_32& position);

  SVTKM_CONT
  svtkm::Vec3f_32 GetPosition() const;

  SVTKM_CONT
  svtkm::Vec3f_32 GetUp() const;

  SVTKM_CONT
  void SetLookAt(const svtkm::Vec3f_32& lookAt);

  SVTKM_CONT
  svtkm::Vec3f_32 GetLookAt() const;

  SVTKM_CONT
  void ResetIsViewDirty();

  SVTKM_CONT
  bool GetIsViewDirty() const;

  SVTKM_CONT
  void CreateRays(Ray<svtkm::Float32>& rays, svtkm::Bounds bounds);

  SVTKM_CONT
  void CreateRays(Ray<svtkm::Float64>& rays, svtkm::Bounds bounds);

  SVTKM_CONT
  void GetPixelData(const svtkm::cont::CoordinateSystem& coords,
                    svtkm::Int32& activePixels,
                    svtkm::Float32& aveRayDistance);

  template <typename Precision>
  SVTKM_CONT void CreateRaysImpl(Ray<Precision>& rays, const svtkm::Bounds boundingBox);

  void CreateDebugRay(svtkm::Vec2i_32 pixel, Ray<svtkm::Float32>& rays);

  void CreateDebugRay(svtkm::Vec2i_32 pixel, Ray<svtkm::Float64>& rays);

  bool operator==(const Camera& other) const;

private:
  template <typename Precision>
  void CreateDebugRayImp(svtkm::Vec2i_32 pixel, Ray<Precision>& rays);
  SVTKM_CONT
  void FindSubset(const svtkm::Bounds& bounds);

  template <typename Precision>
  SVTKM_CONT void UpdateDimensions(Ray<Precision>& rays,
                                  const svtkm::Bounds& boundingBox,
                                  bool ortho2D);

}; // class camera
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_Camera_h
