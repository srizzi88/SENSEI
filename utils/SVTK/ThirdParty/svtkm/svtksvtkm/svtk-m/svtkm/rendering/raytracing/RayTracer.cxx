//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/rendering/raytracing/RayTracer.h>

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/ColorTable.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class SurfaceColor
{
public:
  class Shade : public svtkm::worklet::WorkletMapField
  {
  private:
    svtkm::Vec3f_32 LightPosition;
    svtkm::Vec3f_32 LightAbmient;
    svtkm::Vec3f_32 LightDiffuse;
    svtkm::Vec3f_32 LightSpecular;
    svtkm::Float32 SpecularExponent;
    svtkm::Vec3f_32 CameraPosition;
    svtkm::Vec3f_32 LookAt;

  public:
    SVTKM_CONT
    Shade(const svtkm::Vec3f_32& lightPosition,
          const svtkm::Vec3f_32& cameraPosition,
          const svtkm::Vec3f_32& lookAt)
      : LightPosition(lightPosition)
      , CameraPosition(cameraPosition)
      , LookAt(lookAt)
    {
      //Set up some default lighting parameters for now
      LightAbmient[0] = .5f;
      LightAbmient[1] = .5f;
      LightAbmient[2] = .5f;
      LightDiffuse[0] = .7f;
      LightDiffuse[1] = .7f;
      LightDiffuse[2] = .7f;
      LightSpecular[0] = .7f;
      LightSpecular[1] = .7f;
      LightSpecular[2] = .7f;
      SpecularExponent = 20.f;
    }

    using ControlSignature =
      void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

    template <typename ColorPortalType, typename Precision, typename ColorMapPortalType>
    SVTKM_EXEC void operator()(const svtkm::Id& hitIdx,
                              const Precision& scalar,
                              const svtkm::Vec<Precision, 3>& normal,
                              const svtkm::Vec<Precision, 3>& intersection,
                              ColorPortalType& colors,
                              ColorMapPortalType colorMap,
                              const svtkm::Id& idx) const
    {
      svtkm::Vec<Precision, 4> color;
      svtkm::Id offset = idx * 4;

      if (hitIdx < 0)
      {
        return;
      }

      color[0] = colors.Get(offset + 0);
      color[1] = colors.Get(offset + 1);
      color[2] = colors.Get(offset + 2);
      color[3] = colors.Get(offset + 3);

      svtkm::Vec<Precision, 3> lightDir = LightPosition - intersection;
      svtkm::Vec<Precision, 3> viewDir = CameraPosition - LookAt;
      svtkm::Normalize(lightDir);
      svtkm::Normalize(viewDir);
      //Diffuse lighting
      Precision cosTheta = svtkm::dot(normal, lightDir);
      //clamp tp [0,1]
      const Precision zero = 0.f;
      const Precision one = 1.f;
      cosTheta = svtkm::Min(svtkm::Max(cosTheta, zero), one);
      //Specular lighting
      svtkm::Vec<Precision, 3> reflect = 2.f * svtkm::dot(lightDir, normal) * normal - lightDir;
      svtkm::Normalize(reflect);
      Precision cosPhi = svtkm::dot(reflect, viewDir);
      Precision specularConstant =
        svtkm::Pow(svtkm::Max(cosPhi, zero), static_cast<Precision>(SpecularExponent));
      svtkm::Int32 colorMapSize = static_cast<svtkm::Int32>(colorMap.GetNumberOfValues());
      svtkm::Int32 colorIdx = svtkm::Int32(scalar * Precision(colorMapSize - 1));

      // clamp color index
      colorIdx = svtkm::Max(0, colorIdx);
      colorIdx = svtkm::Min(colorMapSize - 1, colorIdx);
      color = colorMap.Get(colorIdx);

      color[0] *= svtkm::Min(
        LightAbmient[0] + LightDiffuse[0] * cosTheta + LightSpecular[0] * specularConstant, one);
      color[1] *= svtkm::Min(
        LightAbmient[1] + LightDiffuse[1] * cosTheta + LightSpecular[1] * specularConstant, one);
      color[2] *= svtkm::Min(
        LightAbmient[2] + LightDiffuse[2] * cosTheta + LightSpecular[2] * specularConstant, one);

      colors.Set(offset + 0, color[0]);
      colors.Set(offset + 1, color[1]);
      colors.Set(offset + 2, color[2]);
      colors.Set(offset + 3, color[3]);
    }

  }; //class Shade

  class MapScalarToColor : public svtkm::worklet::WorkletMapField
  {
  public:
    SVTKM_CONT
    MapScalarToColor() {}

    using ControlSignature = void(FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
    using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);

    template <typename ColorPortalType, typename Precision, typename ColorMapPortalType>
    SVTKM_EXEC void operator()(const svtkm::Id& hitIdx,
                              const Precision& scalar,
                              ColorPortalType& colors,
                              ColorMapPortalType colorMap,
                              const svtkm::Id& idx) const
    {

      if (hitIdx < 0)
      {
        return;
      }

      svtkm::Vec<Precision, 4> color;
      svtkm::Id offset = idx * 4;

      svtkm::Int32 colorMapSize = static_cast<svtkm::Int32>(colorMap.GetNumberOfValues());
      svtkm::Int32 colorIdx = svtkm::Int32(scalar * Precision(colorMapSize - 1));

      // clamp color index
      colorIdx = svtkm::Max(0, colorIdx);
      colorIdx = svtkm::Min(colorMapSize - 1, colorIdx);
      color = colorMap.Get(colorIdx);

      colors.Set(offset + 0, color[0]);
      colors.Set(offset + 1, color[1]);
      colors.Set(offset + 2, color[2]);
      colors.Set(offset + 3, color[3]);
    }

  }; //class MapScalarToColor

  template <typename Precision>
  SVTKM_CONT void run(Ray<Precision>& rays,
                     svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colorMap,
                     const svtkm::rendering::raytracing::Camera& camera,
                     bool shade)
  {
    if (shade)
    {
      // TODO: support light positions
      svtkm::Vec3f_32 scale(2, 2, 2);
      svtkm::Vec3f_32 lightPosition = camera.GetPosition() + scale * camera.GetUp();
      svtkm::worklet::DispatcherMapField<Shade>(
        Shade(lightPosition, camera.GetPosition(), camera.GetLookAt()))
        .Invoke(rays.HitIdx,
                rays.Scalar,
                rays.Normal,
                rays.Intersection,
                rays.Buffers.at(0).Buffer,
                colorMap);
    }
    else
    {
      svtkm::worklet::DispatcherMapField<MapScalarToColor>(MapScalarToColor())
        .Invoke(rays.HitIdx, rays.Scalar, rays.Buffers.at(0).Buffer, colorMap);
    }
  }
}; // class SurfaceColor

} // namespace detail

RayTracer::RayTracer()
  : NumberOfShapes(0)
  , Shade(true)
{
}

RayTracer::~RayTracer()
{
  Clear();
}

Camera& RayTracer::GetCamera()
{
  return camera;
}


void RayTracer::AddShapeIntersector(std::shared_ptr<ShapeIntersector> intersector)
{
  NumberOfShapes += intersector->GetNumberOfShapes();
  Intersectors.push_back(intersector);
}

void RayTracer::SetField(const svtkm::cont::Field& scalarField, const svtkm::Range& scalarRange)
{
  ScalarField = scalarField;
  ScalarRange = scalarRange;
}

void RayTracer::SetColorMap(const svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void RayTracer::Render(Ray<svtkm::Float32>& rays)
{
  RenderOnDevice(rays);
}

void RayTracer::Render(Ray<svtkm::Float64>& rays)
{
  RenderOnDevice(rays);
}

void RayTracer::SetShadingOn(bool on)
{
  Shade = on;
}

svtkm::Id RayTracer::GetNumberOfShapes() const
{
  return NumberOfShapes;
}

void RayTracer::Clear()
{
  Intersectors.clear();
}

template <typename Precision>
void RayTracer::RenderOnDevice(Ray<Precision>& rays)
{
  using Timer = svtkm::cont::Timer;

  Logger* logger = Logger::GetInstance();
  Timer renderTimer;
  renderTimer.Start();
  svtkm::Float64 time = 0.;
  logger->OpenLogEntry("ray_tracer");
  logger->AddLogData("device", GetDeviceString());

  logger->AddLogData("shapes", NumberOfShapes);
  logger->AddLogData("num_rays", rays.NumRays);
  size_t numShapes = Intersectors.size();
  if (NumberOfShapes > 0)
  {
    Timer timer;
    timer.Start();

    for (size_t i = 0; i < numShapes; ++i)
    {
      Intersectors[i]->IntersectRays(rays);
      time = timer.GetElapsedTime();
      logger->AddLogData("intersect", time);

      timer.Start();
      Intersectors[i]->IntersectionData(rays, ScalarField, ScalarRange);
      time = timer.GetElapsedTime();
      logger->AddLogData("intersection_data", time);
      timer.Start();

      // Calculate the color at the intersection  point
      detail::SurfaceColor surfaceColor;
      surfaceColor.run(rays, ColorMap, camera, this->Shade);

      time = timer.GetElapsedTime();
      logger->AddLogData("shade", time);
    }
  }

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} // RenderOnDevice
}
}
} // namespace svtkm::rendering::raytracing
