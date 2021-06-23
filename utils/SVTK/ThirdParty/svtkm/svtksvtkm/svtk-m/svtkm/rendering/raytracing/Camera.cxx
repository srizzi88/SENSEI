//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/VectorAnalysis.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/TryExecute.h>

#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <svtkm/rendering/raytracing/Sampler.h>
#include <svtkm/rendering/raytracing/Worklets.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <limits>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class PixelData : public svtkm::worklet::WorkletMapField
{
public:
  svtkm::Int32 w;
  svtkm::Int32 h;
  svtkm::Int32 Minx;
  svtkm::Int32 Miny;
  svtkm::Int32 SubsetWidth;
  svtkm::Vec3f_32 nlook; // normalized look
  svtkm::Vec3f_32 delta_x;
  svtkm::Vec3f_32 delta_y;
  svtkm::Vec3f_32 Origin;
  svtkm::Bounds BoundingBox;
  SVTKM_CONT
  PixelData(svtkm::Int32 width,
            svtkm::Int32 height,
            svtkm::Float32 fovX,
            svtkm::Float32 fovY,
            svtkm::Vec3f_32 look,
            svtkm::Vec3f_32 up,
            svtkm::Float32 _zoom,
            svtkm::Int32 subsetWidth,
            svtkm::Int32 minx,
            svtkm::Int32 miny,
            svtkm::Vec3f_32 origin,
            svtkm::Bounds boundingBox)
    : w(width)
    , h(height)
    , Minx(minx)
    , Miny(miny)
    , SubsetWidth(subsetWidth)
    , Origin(origin)
    , BoundingBox(boundingBox)
  {
    svtkm::Float32 thx = tanf((fovX * svtkm::Pi_180f()) * .5f);
    svtkm::Float32 thy = tanf((fovY * svtkm::Pi_180f()) * .5f);
    svtkm::Vec3f_32 ru = svtkm::Cross(look, up);
    svtkm::Normalize(ru);

    svtkm::Vec3f_32 rv = svtkm::Cross(ru, look);
    svtkm::Normalize(rv);
    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
    }
    nlook = look;
    svtkm::Normalize(nlook);
  }

  SVTKM_EXEC inline svtkm::Float32 rcp(svtkm::Float32 f) const { return 1.0f / f; }

  SVTKM_EXEC inline svtkm::Float32 rcp_safe(svtkm::Float32 f) const
  {
    return rcp((fabs(f) < 1e-8f) ? 1e-8f : f);
  }

  using ControlSignature = void(FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2);
  SVTKM_EXEC
  void operator()(const svtkm::Id idx, svtkm::Int32& hit, svtkm::Float32& distance) const
  {
    svtkm::Vec3f_32 ray_dir;
    int i = svtkm::Int32(idx) % SubsetWidth;
    int j = svtkm::Int32(idx) / SubsetWidth;
    i += Minx;
    j += Miny;
    // Write out the global pixelId
    ray_dir = nlook + delta_x * ((2.f * svtkm::Float32(i) - svtkm::Float32(w)) / 2.0f) +
      delta_y * ((2.f * svtkm::Float32(j) - svtkm::Float32(h)) / 2.0f);

    svtkm::Float32 dot = svtkm::Dot(ray_dir, ray_dir);
    svtkm::Float32 sq_mag = svtkm::Sqrt(dot);

    ray_dir[0] = ray_dir[0] / sq_mag;
    ray_dir[1] = ray_dir[1] / sq_mag;
    ray_dir[2] = ray_dir[2] / sq_mag;

    svtkm::Float32 invDirx = rcp_safe(ray_dir[0]);
    svtkm::Float32 invDiry = rcp_safe(ray_dir[1]);
    svtkm::Float32 invDirz = rcp_safe(ray_dir[2]);

    svtkm::Float32 odirx = Origin[0] * invDirx;
    svtkm::Float32 odiry = Origin[1] * invDiry;
    svtkm::Float32 odirz = Origin[2] * invDirz;

    svtkm::Float32 xmin = svtkm::Float32(BoundingBox.X.Min) * invDirx - odirx;
    svtkm::Float32 ymin = svtkm::Float32(BoundingBox.Y.Min) * invDiry - odiry;
    svtkm::Float32 zmin = svtkm::Float32(BoundingBox.Z.Min) * invDirz - odirz;
    svtkm::Float32 xmax = svtkm::Float32(BoundingBox.X.Max) * invDirx - odirx;
    svtkm::Float32 ymax = svtkm::Float32(BoundingBox.Y.Max) * invDiry - odiry;
    svtkm::Float32 zmax = svtkm::Float32(BoundingBox.Z.Max) * invDirz - odirz;
    svtkm::Float32 mind = svtkm::Max(
      svtkm::Max(svtkm::Max(svtkm::Min(ymin, ymax), svtkm::Min(xmin, xmax)), svtkm::Min(zmin, zmax)),
      0.f);
    svtkm::Float32 maxd =
      svtkm::Min(svtkm::Min(svtkm::Max(ymin, ymax), svtkm::Max(xmin, xmax)), svtkm::Max(zmin, zmax));
    if (maxd < mind)
    {
      hit = 0;
      distance = 0;
    }
    else
    {
      distance = maxd - mind;
      hit = 1;
    }
  }

}; // class pixelData

class PerspectiveRayGenJitter : public svtkm::worklet::WorkletMapField
{
public:
  svtkm::Int32 w;
  svtkm::Int32 h;
  svtkm::Vec3f_32 nlook; // normalized look
  svtkm::Vec3f_32 delta_x;
  svtkm::Vec3f_32 delta_y;
  svtkm::Int32 CurrentSample;
  SVTKM_CONT_EXPORT
  PerspectiveRayGenJitter(svtkm::Int32 width,
                          svtkm::Int32 height,
                          svtkm::Float32 fovX,
                          svtkm::Float32 fovY,
                          svtkm::Vec3f_32 look,
                          svtkm::Vec3f_32 up,
                          svtkm::Float32 _zoom,
                          svtkm::Int32 currentSample)
    : w(width)
    , h(height)
  {
    svtkm::Float32 thx = tanf((fovX * 3.1415926f / 180.f) * .5f);
    svtkm::Float32 thy = tanf((fovY * 3.1415926f / 180.f) * .5f);
    svtkm::Vec3f_32 ru = svtkm::Cross(up, look);
    svtkm::Normalize(ru);

    svtkm::Vec3f_32 rv = svtkm::Cross(ru, look);
    svtkm::Normalize(rv);

    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
    }
    nlook = look;
    svtkm::Normalize(nlook);
    CurrentSample = currentSample;
  }

  typedef void ControlSignature(FieldOut, FieldOut, FieldOut, FieldIn);

  typedef void ExecutionSignature(WorkIndex, _1, _2, _3, _4);
  SVTKM_EXEC
  void operator()(svtkm::Id idx,
                  svtkm::Float32& rayDirX,
                  svtkm::Float32& rayDirY,
                  svtkm::Float32& rayDirZ,
                  const svtkm::Int32& seed) const
  {
    svtkm::Vec2f_32 xy;
    Halton2D<3>(CurrentSample + seed, xy);
    xy[0] -= .5f;
    xy[1] -= .5f;

    svtkm::Vec3f_32 ray_dir(rayDirX, rayDirY, rayDirZ);
    svtkm::Float32 i = static_cast<svtkm::Float32>(svtkm::Int32(idx) % w);
    svtkm::Float32 j = static_cast<svtkm::Float32>(svtkm::Int32(idx) / w);
    i += xy[0];
    j += xy[1];
    ray_dir = nlook + delta_x * ((2.f * i - svtkm::Float32(w)) / 2.0f) +
      delta_y * ((2.f * j - svtkm::Float32(h)) / 2.0f);
    svtkm::Normalize(ray_dir);
    rayDirX = ray_dir[0];
    rayDirY = ray_dir[1];
    rayDirZ = ray_dir[2];
  }

}; // class perspective ray gen jitter

class Camera::Ortho2DRayGen : public svtkm::worklet::WorkletMapField
{
public:
  svtkm::Int32 w;
  svtkm::Int32 h;
  svtkm::Int32 Minx;
  svtkm::Int32 Miny;
  svtkm::Int32 SubsetWidth;
  svtkm::Vec3f_32 nlook; // normalized look
  svtkm::Vec3f_32 PixelDelta;
  svtkm::Vec3f_32 delta_y;
  svtkm::Vec3f_32 StartOffset;

  SVTKM_CONT
  Ortho2DRayGen(svtkm::Int32 width,
                svtkm::Int32 height,
                svtkm::Float32 svtkmNotUsed(_zoom),
                svtkm::Int32 subsetWidth,
                svtkm::Int32 minx,
                svtkm::Int32 miny,
                const svtkm::rendering::Camera& camera)
    : w(width)
    , h(height)
    , Minx(minx)
    , Miny(miny)
    , SubsetWidth(subsetWidth)
  {
    svtkm::Float32 left, right, bottom, top;
    camera.GetViewRange2D(left, right, bottom, top);

    svtkm::Float32 vl, vr, vb, vt;
    camera.GetRealViewport(width, height, vl, vr, vb, vt);
    svtkm::Float32 _w = static_cast<svtkm::Float32>(width) * (vr - vl) / 2.f;
    svtkm::Float32 _h = static_cast<svtkm::Float32>(height) * (vt - vb) / 2.f;
    svtkm::Vec2f_32 minPoint(left, bottom);
    svtkm::Vec2f_32 maxPoint(right, top);
    svtkm::Vec2f_32 delta = maxPoint - minPoint;
    //delta[0] /= svtkm::Float32(width);
    //delta[1] /= svtkm::Float32(height);
    delta[0] /= svtkm::Float32(_w);
    delta[1] /= svtkm::Float32(_h);
    PixelDelta[0] = delta[0];
    PixelDelta[1] = delta[1];
    PixelDelta[2] = 0.f;

    svtkm::Vec2f_32 startOffset = minPoint + delta / 2.f;
    StartOffset[0] = startOffset[0];
    StartOffset[1] = startOffset[1];
    // always push the rays back from the origin
    StartOffset[2] = -1.f;


    svtkm::Normalize(nlook);
  }

  using ControlSignature =
    void(FieldOut, FieldOut, FieldOut, FieldOut, FieldOut, FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5, _6, _7);
  template <typename Precision>
  SVTKM_EXEC void operator()(svtkm::Id idx,
                            Precision& rayDirX,
                            Precision& rayDirY,
                            Precision& rayDirZ,
                            Precision& rayOriginX,
                            Precision& rayOriginY,
                            Precision& rayOriginZ,
                            svtkm::Id& pixelIndex) const
  {
    // this is 2d so always look down z
    rayDirX = 0.f;
    rayDirY = 0.f;
    rayDirZ = 1.f;
    //
    // Pixel subset is the pixels in the 2d viewport
    // not where the rays might intersect data like
    // the perspective ray gen
    //
    int i = svtkm::Int32(idx) % SubsetWidth;
    int j = svtkm::Int32(idx) / SubsetWidth;

    svtkm::Vec3f_32 pos;
    pos[0] = svtkm::Float32(i);
    pos[1] = svtkm::Float32(j);
    pos[2] = 0.f;
    svtkm::Vec3f_32 origin = StartOffset + pos * PixelDelta;
    rayOriginX = origin[0];
    rayOriginY = origin[1];
    rayOriginZ = origin[2];
    i += Minx;
    j += Miny;
    pixelIndex = static_cast<svtkm::Id>(j * w + i);
  }

}; // class perspective ray gen

class Camera::PerspectiveRayGen : public svtkm::worklet::WorkletMapField
{
public:
  svtkm::Int32 w;
  svtkm::Int32 h;
  svtkm::Int32 Minx;
  svtkm::Int32 Miny;
  svtkm::Int32 SubsetWidth;
  svtkm::Vec3f_32 nlook; // normalized look
  svtkm::Vec3f_32 delta_x;
  svtkm::Vec3f_32 delta_y;
  SVTKM_CONT
  PerspectiveRayGen(svtkm::Int32 width,
                    svtkm::Int32 height,
                    svtkm::Float32 fovX,
                    svtkm::Float32 fovY,
                    svtkm::Vec3f_32 look,
                    svtkm::Vec3f_32 up,
                    svtkm::Float32 _zoom,
                    svtkm::Int32 subsetWidth,
                    svtkm::Int32 minx,
                    svtkm::Int32 miny)
    : w(width)
    , h(height)
    , Minx(minx)
    , Miny(miny)
    , SubsetWidth(subsetWidth)
  {
    svtkm::Float32 thx = tanf((fovX * svtkm::Pi_180f()) * .5f);
    svtkm::Float32 thy = tanf((fovY * svtkm::Pi_180f()) * .5f);
    svtkm::Vec3f_32 ru = svtkm::Cross(look, up);
    svtkm::Normalize(ru);

    svtkm::Vec3f_32 rv = svtkm::Cross(ru, look);
    svtkm::Normalize(rv);
    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
    }
    nlook = look;
    svtkm::Normalize(nlook);
  }

  using ControlSignature = void(FieldOut, FieldOut, FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);
  template <typename Precision>
  SVTKM_EXEC void operator()(svtkm::Id idx,
                            Precision& rayDirX,
                            Precision& rayDirY,
                            Precision& rayDirZ,
                            svtkm::Id& pixelIndex) const
  {
    svtkm::Vec<Precision, 3> ray_dir(rayDirX, rayDirY, rayDirZ);
    int i = svtkm::Int32(idx) % SubsetWidth;
    int j = svtkm::Int32(idx) / SubsetWidth;
    i += Minx;
    j += Miny;
    // Write out the global pixelId
    pixelIndex = static_cast<svtkm::Id>(j * w + i);
    ray_dir = nlook + delta_x * ((2.f * Precision(i) - Precision(w)) / 2.0f) +
      delta_y * ((2.f * Precision(j) - Precision(h)) / 2.0f);
    // avoid some numerical issues
    for (svtkm::Int32 d = 0; d < 3; ++d)
    {
      if (ray_dir[d] == 0.f)
        ray_dir[d] += 0.0000001f;
    }
    Precision dot = svtkm::Dot(ray_dir, ray_dir);
    Precision sq_mag = svtkm::Sqrt(dot);

    rayDirX = ray_dir[0] / sq_mag;
    rayDirY = ray_dir[1] / sq_mag;
    rayDirZ = ray_dir[2] / sq_mag;
  }

}; // class perspective ray gen

bool Camera::operator==(const Camera& other) const
{

  if (this->Height != other.Height)
    return false;
  if (this->Width != other.Width)
    return false;
  if (this->SubsetWidth != other.SubsetWidth)
    return false;
  if (this->SubsetHeight != other.SubsetHeight)
    return false;
  if (this->SubsetMinX != other.SubsetMinX)
    return false;
  if (this->SubsetMinY != other.SubsetMinY)
    return false;
  if (this->FovY != other.FovY)
    return false;
  if (this->FovX != other.FovX)
    return false;
  if (this->Zoom != other.Zoom)
    return false;
  if (this->Look[0] != other.Look[0])
    return false;
  if (this->Look[1] != other.Look[1])
    return false;
  if (this->Look[2] != other.Look[2])
    return false;
  if (this->LookAt[0] != other.LookAt[0])
    return false;
  if (this->LookAt[1] != other.LookAt[1])
    return false;
  if (this->LookAt[2] != other.LookAt[2])
    return false;
  if (this->Up[0] != other.Up[0])
    return false;
  if (this->Up[1] != other.Up[1])
    return false;
  if (this->Up[2] != other.Up[2])
    return false;
  if (this->Position[0] != other.Position[0])
    return false;
  if (this->Position[1] != other.Position[1])
    return false;
  if (this->Position[2] != other.Position[2])
    return false;
  return true;
}


SVTKM_CONT
Camera::Camera()
{
  this->Height = 500;
  this->Width = 500;
  this->SubsetWidth = 500;
  this->SubsetHeight = 500;
  this->SubsetMinX = 0;
  this->SubsetMinY = 0;
  this->FovY = 30.f;
  this->FovX = 30.f;
  this->Zoom = 1.f;
  this->Look[0] = 0.f;
  this->Look[1] = 0.f;
  this->Look[2] = -1.f;
  this->LookAt[0] = 0.f;
  this->LookAt[1] = 0.f;
  this->LookAt[2] = -1.f;
  this->Up[0] = 0.f;
  this->Up[1] = 1.f;
  this->Up[2] = 0.f;
  this->Position[0] = 0.f;
  this->Position[1] = 0.f;
  this->Position[2] = 0.f;
  this->IsViewDirty = true;
}

SVTKM_CONT
Camera::~Camera()
{
}

SVTKM_CONT
void Camera::SetParameters(const svtkm::rendering::Camera& camera,
                           svtkm::rendering::CanvasRayTracer& canvas)
{
  this->SetUp(camera.GetViewUp());
  this->SetLookAt(camera.GetLookAt());
  this->SetPosition(camera.GetPosition());
  this->SetZoom(camera.GetZoom());
  this->SetFieldOfView(camera.GetFieldOfView());
  this->SetHeight(static_cast<svtkm::Int32>(canvas.GetHeight()));
  this->SetWidth(static_cast<svtkm::Int32>(canvas.GetWidth()));
  this->CameraView = camera;
  Canvas = canvas;
}


SVTKM_CONT
void Camera::SetHeight(const svtkm::Int32& height)
{
  if (height <= 0)
  {
    throw svtkm::cont::ErrorBadValue("Camera height must be greater than zero.");
  }
  if (Height != height)
  {
    this->Height = height;
    this->SetFieldOfView(this->FovY);
  }
}

SVTKM_CONT
svtkm::Int32 Camera::GetHeight() const
{
  return this->Height;
}

SVTKM_CONT
void Camera::SetWidth(const svtkm::Int32& width)
{
  if (width <= 0)
  {
    throw svtkm::cont::ErrorBadValue("Camera width must be greater than zero.");
  }
  if (this->Width != width)
  {
    this->Width = width;
    this->SetFieldOfView(this->FovY);
  }
}

SVTKM_CONT
svtkm::Int32 Camera::GetWidth() const
{
  return this->Width;
}

SVTKM_CONT
svtkm::Int32 Camera::GetSubsetWidth() const
{
  return this->SubsetWidth;
}

SVTKM_CONT
svtkm::Int32 Camera::GetSubsetHeight() const
{
  return this->SubsetHeight;
}

SVTKM_CONT
void Camera::SetZoom(const svtkm::Float32& zoom)
{
  if (zoom <= 0)
  {
    throw svtkm::cont::ErrorBadValue("Camera zoom must be greater than zero.");
  }
  if (this->Zoom != zoom)
  {
    this->IsViewDirty = true;
    this->Zoom = zoom;
  }
}

SVTKM_CONT
svtkm::Float32 Camera::GetZoom() const
{
  return this->Zoom;
}

SVTKM_CONT
void Camera::SetFieldOfView(const svtkm::Float32& degrees)
{
  if (degrees <= 0)
  {
    throw svtkm::cont::ErrorBadValue("Camera feild of view must be greater than zero.");
  }
  if (degrees > 180)
  {
    throw svtkm::cont::ErrorBadValue("Camera feild of view must be less than 180.");
  }

  svtkm::Float32 newFOVY = degrees;
  svtkm::Float32 newFOVX;

  if (this->Width != this->Height)
  {
    svtkm::Float32 fovyRad = newFOVY * svtkm::Pi_180f();

    // Use the tan function to find the distance from the center of the image to the top (or
    // bottom). (Actually, we are finding the ratio of this distance to the near plane distance,
    // but since we scale everything by the near plane distance, we can use this ratio as a scaled
    // proxy of the distances we need.)
    svtkm::Float32 verticalDistance = svtkm::Tan(0.5f * fovyRad);

    // Scale the vertical distance by the aspect ratio to get the horizontal distance.
    svtkm::Float32 aspectRatio = svtkm::Float32(this->Width) / svtkm::Float32(this->Height);
    svtkm::Float32 horizontalDistance = aspectRatio * verticalDistance;

    // Now use the arctan function to get the proper field of view in the x direction.
    svtkm::Float32 fovxRad = 2.0f * svtkm::ATan(horizontalDistance);
    newFOVX = fovxRad / svtkm::Pi_180f();
  }
  else
  {
    newFOVX = newFOVY;
  }

  if (newFOVX != this->FovX)
  {
    this->IsViewDirty = true;
  }
  if (newFOVY != this->FovY)
  {
    this->IsViewDirty = true;
  }
  this->FovX = newFOVX;
  this->FovY = newFOVY;
  this->CameraView.SetFieldOfView(this->FovY);
}

SVTKM_CONT
svtkm::Float32 Camera::GetFieldOfView() const
{
  return this->FovY;
}

SVTKM_CONT
void Camera::SetUp(const svtkm::Vec3f_32& up)
{
  if (this->Up != up)
  {
    this->Up = up;
    svtkm::Normalize(this->Up);
    this->IsViewDirty = true;
  }
}

SVTKM_CONT
svtkm::Vec3f_32 Camera::GetUp() const
{
  return this->Up;
}

SVTKM_CONT
void Camera::SetLookAt(const svtkm::Vec3f_32& lookAt)
{
  if (this->LookAt != lookAt)
  {
    this->LookAt = lookAt;
    this->IsViewDirty = true;
  }
}

SVTKM_CONT
svtkm::Vec3f_32 Camera::GetLookAt() const
{
  return this->LookAt;
}

SVTKM_CONT
void Camera::SetPosition(const svtkm::Vec3f_32& position)
{
  if (this->Position != position)
  {
    this->Position = position;
    this->IsViewDirty = true;
  }
}

SVTKM_CONT
svtkm::Vec3f_32 Camera::GetPosition() const
{
  return this->Position;
}

SVTKM_CONT
void Camera::ResetIsViewDirty()
{
  this->IsViewDirty = false;
}

SVTKM_CONT
bool Camera::GetIsViewDirty() const
{
  return this->IsViewDirty;
}

void Camera::GetPixelData(const svtkm::cont::CoordinateSystem& coords,
                          svtkm::Int32& activePixels,
                          svtkm::Float32& aveRayDistance)
{
  svtkm::Bounds boundingBox = coords.GetBounds();
  this->FindSubset(boundingBox);
  //Reset the camera look vector
  this->Look = this->LookAt - this->Position;
  svtkm::Normalize(this->Look);
  const int size = this->SubsetWidth * this->SubsetHeight;
  svtkm::cont::ArrayHandle<svtkm::Float32> dists;
  svtkm::cont::ArrayHandle<svtkm::Int32> hits;
  dists.Allocate(size);
  hits.Allocate(size);

  //Create the ray direction
  svtkm::worklet::DispatcherMapField<PixelData>(PixelData(this->Width,
                                                         this->Height,
                                                         this->FovX,
                                                         this->FovY,
                                                         this->Look,
                                                         this->Up,
                                                         this->Zoom,
                                                         this->SubsetWidth,
                                                         this->SubsetMinX,
                                                         this->SubsetMinY,
                                                         this->Position,
                                                         boundingBox))
    .Invoke(hits, dists); //X Y Z
  activePixels = svtkm::cont::Algorithm::Reduce(hits, svtkm::Int32(0));
  aveRayDistance =
    svtkm::cont::Algorithm::Reduce(dists, svtkm::Float32(0)) / svtkm::Float32(activePixels);
}

SVTKM_CONT
void Camera::CreateRays(Ray<svtkm::Float32>& rays, svtkm::Bounds bounds)
{
  CreateRaysImpl(rays, bounds);
}

SVTKM_CONT
void Camera::CreateRays(Ray<svtkm::Float64>& rays, svtkm::Bounds bounds)
{
  CreateRaysImpl(rays, bounds);
}

template <typename Precision>
SVTKM_CONT void Camera::CreateRaysImpl(Ray<Precision>& rays, const svtkm::Bounds boundingBox)
{
  Logger* logger = Logger::GetInstance();
  svtkm::cont::Timer createTimer;
  createTimer.Start();
  logger->OpenLogEntry("ray_camera");

  bool ortho = this->CameraView.GetMode() == svtkm::rendering::Camera::MODE_2D;
  this->UpdateDimensions(rays, boundingBox, ortho);
  this->WriteSettingsToLog();
  svtkm::cont::Timer timer;
  timer.Start();
  //Set the origin of the ray back to the camera position

  Precision infinity;
  GetInfinity(infinity);

  svtkm::cont::ArrayHandleConstant<Precision> inf(infinity, rays.NumRays);
  svtkm::cont::Algorithm::Copy(inf, rays.MaxDistance);

  svtkm::cont::ArrayHandleConstant<Precision> zero(0, rays.NumRays);
  svtkm::cont::Algorithm::Copy(zero, rays.MinDistance);
  svtkm::cont::Algorithm::Copy(zero, rays.Distance);

  svtkm::cont::ArrayHandleConstant<svtkm::Id> initHit(-2, rays.NumRays);
  svtkm::cont::Algorithm::Copy(initHit, rays.HitIdx);

  svtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("camera_memset", time);
  timer.Start();

  //Reset the camera look vector
  this->Look = this->LookAt - this->Position;
  svtkm::Normalize(this->Look);
  if (ortho)
  {

    svtkm::worklet::DispatcherMapField<Ortho2DRayGen> dispatcher(Ortho2DRayGen(this->Width,
                                                                              this->Height,
                                                                              this->Zoom,
                                                                              this->SubsetWidth,
                                                                              this->SubsetMinX,
                                                                              this->SubsetMinY,
                                                                              this->CameraView));
    dispatcher.Invoke(rays.DirX,
                      rays.DirY,
                      rays.DirZ,
                      rays.OriginX,
                      rays.OriginY,
                      rays.OriginZ,
                      rays.PixelIdx); //X Y Z
  }
  else
  {
    //Create the ray direction
    svtkm::worklet::DispatcherMapField<PerspectiveRayGen> dispatcher(
      PerspectiveRayGen(this->Width,
                        this->Height,
                        this->FovX,
                        this->FovY,
                        this->Look,
                        this->Up,
                        this->Zoom,
                        this->SubsetWidth,
                        this->SubsetMinX,
                        this->SubsetMinY));
    dispatcher.Invoke(rays.DirX, rays.DirY, rays.DirZ, rays.PixelIdx); //X Y Z

    svtkm::cont::ArrayHandleConstant<Precision> posX(this->Position[0], rays.NumRays);
    svtkm::cont::Algorithm::Copy(posX, rays.OriginX);

    svtkm::cont::ArrayHandleConstant<Precision> posY(this->Position[1], rays.NumRays);
    svtkm::cont::Algorithm::Copy(posY, rays.OriginY);

    svtkm::cont::ArrayHandleConstant<Precision> posZ(this->Position[2], rays.NumRays);
    svtkm::cont::Algorithm::Copy(posZ, rays.OriginZ);
  }

  time = timer.GetElapsedTime();
  logger->AddLogData("ray_gen", time);
  time = createTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} //create rays

SVTKM_CONT
void Camera::FindSubset(const svtkm::Bounds& bounds)
{
  this->ViewProjectionMat =
    svtkm::MatrixMultiply(this->CameraView.CreateProjectionMatrix(this->Width, this->Height),
                         this->CameraView.CreateViewMatrix());
  svtkm::Float32 x[2], y[2], z[2];
  x[0] = static_cast<svtkm::Float32>(bounds.X.Min);
  x[1] = static_cast<svtkm::Float32>(bounds.X.Max);
  y[0] = static_cast<svtkm::Float32>(bounds.Y.Min);
  y[1] = static_cast<svtkm::Float32>(bounds.Y.Max);
  z[0] = static_cast<svtkm::Float32>(bounds.Z.Min);
  z[1] = static_cast<svtkm::Float32>(bounds.Z.Max);
  //Inise the data bounds
  if (this->Position[0] >= x[0] && this->Position[0] <= x[1] && this->Position[1] >= y[0] &&
      this->Position[1] <= y[1] && this->Position[2] >= z[0] && this->Position[2] <= z[1])
  {
    this->SubsetWidth = this->Width;
    this->SubsetHeight = this->Height;
    this->SubsetMinY = 0;
    this->SubsetMinX = 0;
    return;
  }

  svtkm::Float32 xmin, ymin, xmax, ymax, zmin, zmax;
  xmin = svtkm::Infinity32();
  ymin = svtkm::Infinity32();
  zmin = svtkm::Infinity32();
  xmax = svtkm::NegativeInfinity32();
  ymax = svtkm::NegativeInfinity32();
  zmax = svtkm::NegativeInfinity32();
  svtkm::Vec4f_32 extentPoint;
  for (svtkm::Int32 i = 0; i < 2; ++i)
    for (svtkm::Int32 j = 0; j < 2; ++j)
      for (svtkm::Int32 k = 0; k < 2; ++k)
      {
        extentPoint[0] = x[i];
        extentPoint[1] = y[j];
        extentPoint[2] = z[k];
        extentPoint[3] = 1.f;
        svtkm::Vec4f_32 transformed = svtkm::MatrixMultiply(this->ViewProjectionMat, extentPoint);
        // perform the perspective divide
        for (svtkm::Int32 a = 0; a < 3; ++a)
        {
          transformed[a] = transformed[a] / transformed[3];
        }

        transformed[0] = (transformed[0] * 0.5f + 0.5f) * static_cast<svtkm::Float32>(Width);
        transformed[1] = (transformed[1] * 0.5f + 0.5f) * static_cast<svtkm::Float32>(Height);
        transformed[2] = (transformed[2] * 0.5f + 0.5f);
        zmin = svtkm::Min(zmin, transformed[2]);
        zmax = svtkm::Max(zmax, transformed[2]);
        if (transformed[2] < 0 || transformed[2] > 1)
        {
          continue;
        }
        xmin = svtkm::Min(xmin, transformed[0]);
        ymin = svtkm::Min(ymin, transformed[1]);
        xmax = svtkm::Max(xmax, transformed[0]);
        ymax = svtkm::Max(ymax, transformed[1]);
      }

  xmin -= .001f;
  xmax += .001f;
  ymin -= .001f;
  ymax += .001f;
  xmin = svtkm::Floor(svtkm::Min(svtkm::Max(0.f, xmin), svtkm::Float32(Width)));
  xmax = svtkm::Ceil(svtkm::Min(svtkm::Max(0.f, xmax), svtkm::Float32(Width)));
  ymin = svtkm::Floor(svtkm::Min(svtkm::Max(0.f, ymin), svtkm::Float32(Height)));
  ymax = svtkm::Ceil(svtkm::Min(svtkm::Max(0.f, ymax), svtkm::Float32(Height)));

  Logger* logger = Logger::GetInstance();
  std::stringstream ss;
  ss << "(" << xmin << "," << ymin << "," << zmin << ")-";
  ss << "(" << xmax << "," << ymax << "," << zmax << ")";
  logger->AddLogData("pixel_range", ss.str());

  svtkm::Int32 dx = svtkm::Int32(xmax) - svtkm::Int32(xmin);
  svtkm::Int32 dy = svtkm::Int32(ymax) - svtkm::Int32(ymin);
  //
  //  scene is behind the camera
  //
  if (zmax < 0 || xmin >= xmax || ymin >= ymax)
  {
    this->SubsetWidth = 1;
    this->SubsetHeight = 1;
    this->SubsetMinX = 0;
    this->SubsetMinY = 0;
  }
  else
  {
    this->SubsetWidth = dx;
    this->SubsetHeight = dy;
    this->SubsetMinX = svtkm::Int32(xmin);
    this->SubsetMinY = svtkm::Int32(ymin);
  }
  logger->AddLogData("subset_width", dx);
  logger->AddLogData("subset_height", dy);
}

template <typename Precision>
SVTKM_CONT void Camera::UpdateDimensions(Ray<Precision>& rays,
                                        const svtkm::Bounds& boundingBox,
                                        bool ortho2D)
{
  // If bounds have been provided, only cast rays that could hit the data
  bool imageSubsetModeOn = boundingBox.IsNonEmpty();

  //Find the pixel footprint
  if (imageSubsetModeOn && !ortho2D)
  {
    //Create a transform matrix using the rendering::camera class
    svtkm::rendering::Camera camera = this->CameraView;
    camera.SetFieldOfView(this->GetFieldOfView());
    camera.SetLookAt(this->GetLookAt());
    camera.SetPosition(this->GetPosition());
    camera.SetViewUp(this->GetUp());
    //
    // Just create come clipping range, we ignore the zmax value in subsetting
    //
    svtkm::Float64 maxDim = svtkm::Max(
      boundingBox.X.Max - boundingBox.X.Min,
      svtkm::Max(boundingBox.Y.Max - boundingBox.Y.Min, boundingBox.Z.Max - boundingBox.Z.Min));

    maxDim *= 100;
    camera.SetClippingRange(.0001, maxDim);
    //Update our ViewProjection matrix
    this->ViewProjectionMat =
      svtkm::MatrixMultiply(this->CameraView.CreateProjectionMatrix(this->Width, this->Height),
                           this->CameraView.CreateViewMatrix());
    this->FindSubset(boundingBox);
  }
  else if (ortho2D)
  {
    // 2D rendering has a viewport that represents the area of the canvas where the image
    // is drawn. Thus, we have to create rays cooresponding to that region of the
    // canvas, so annotations are correctly rendered
    svtkm::Float32 vl, vr, vb, vt;
    this->CameraView.GetRealViewport(this->GetWidth(), this->GetHeight(), vl, vr, vb, vt);
    svtkm::Float32 _x = static_cast<svtkm::Float32>(this->GetWidth()) * (1.f + vl) / 2.f;
    svtkm::Float32 _y = static_cast<svtkm::Float32>(this->GetHeight()) * (1.f + vb) / 2.f;
    svtkm::Float32 _w = static_cast<svtkm::Float32>(this->GetWidth()) * (vr - vl) / 2.f;
    svtkm::Float32 _h = static_cast<svtkm::Float32>(this->GetHeight()) * (vt - vb) / 2.f;

    this->SubsetWidth = static_cast<svtkm::Int32>(_w);
    this->SubsetHeight = static_cast<svtkm::Int32>(_h);
    this->SubsetMinY = static_cast<svtkm::Int32>(_y);
    this->SubsetMinX = static_cast<svtkm::Int32>(_x);
  }
  else
  {
    //Update the image dimensions
    this->SubsetWidth = this->Width;
    this->SubsetHeight = this->Height;
    this->SubsetMinY = 0;
    this->SubsetMinX = 0;
  }

  // resize rays and buffers
  if (rays.NumRays != SubsetWidth * SubsetHeight)
  {
    RayOperations::Resize(
      rays, this->SubsetHeight * this->SubsetWidth, svtkm::cont::DeviceAdapterTagSerial());
  }
}

void Camera::CreateDebugRay(svtkm::Vec2i_32 pixel, Ray<svtkm::Float64>& rays)
{
  CreateDebugRayImp(pixel, rays);
}

void Camera::CreateDebugRay(svtkm::Vec2i_32 pixel, Ray<svtkm::Float32>& rays)
{
  CreateDebugRayImp(pixel, rays);
}

template <typename Precision>
void Camera::CreateDebugRayImp(svtkm::Vec2i_32 pixel, Ray<Precision>& rays)
{
  RayOperations::Resize(rays, 1, svtkm::cont::DeviceAdapterTagSerial());
  svtkm::Int32 pixelIndex = this->Width * (this->Height - pixel[1]) + pixel[0];
  rays.PixelIdx.GetPortalControl().Set(0, pixelIndex);
  rays.OriginX.GetPortalControl().Set(0, this->Position[0]);
  rays.OriginY.GetPortalControl().Set(0, this->Position[1]);
  rays.OriginZ.GetPortalControl().Set(0, this->Position[2]);


  svtkm::Float32 infinity;
  GetInfinity(infinity);

  rays.MaxDistance.GetPortalControl().Set(0, infinity);
  rays.MinDistance.GetPortalControl().Set(0, 0.f);
  rays.HitIdx.GetPortalControl().Set(0, -2);

  svtkm::Float32 thx = tanf((this->FovX * svtkm::Pi_180f()) * .5f);
  svtkm::Float32 thy = tanf((this->FovY * svtkm::Pi_180f()) * .5f);
  svtkm::Vec3f_32 ru = svtkm::Cross(this->Look, this->Up);
  svtkm::Normalize(ru);

  svtkm::Vec3f_32 rv = svtkm::Cross(ru, this->Look);
  svtkm::Vec3f_32 delta_x, delta_y;
  svtkm::Normalize(rv);
  delta_x = ru * (2 * thx / (float)this->Width);
  delta_y = rv * (2 * thy / (float)this->Height);

  if (this->Zoom > 0)
  {
    svtkm::Float32 _zoom = this->Zoom;
    delta_x[0] = delta_x[0] / _zoom;
    delta_x[1] = delta_x[1] / _zoom;
    delta_x[2] = delta_x[2] / _zoom;
    delta_y[0] = delta_y[0] / _zoom;
    delta_y[1] = delta_y[1] / _zoom;
    delta_y[2] = delta_y[2] / _zoom;
  }
  svtkm::Vec3f_32 nlook = this->Look;
  svtkm::Normalize(nlook);

  svtkm::Vec<Precision, 3> ray_dir;
  int i = svtkm::Int32(pixelIndex) % this->Width;
  int j = svtkm::Int32(pixelIndex) / this->Height;
  ray_dir = nlook + delta_x * ((2.f * Precision(i) - Precision(this->Width)) / 2.0f) +
    delta_y * ((2.f * Precision(j) - Precision(this->Height)) / 2.0f);

  Precision dot = svtkm::Dot(ray_dir, ray_dir);
  Precision sq_mag = svtkm::Sqrt(dot);

  ray_dir[0] = ray_dir[0] / sq_mag;
  ray_dir[1] = ray_dir[1] / sq_mag;
  ray_dir[2] = ray_dir[2] / sq_mag;
  rays.DirX.GetPortalControl().Set(0, ray_dir[0]);
  rays.DirY.GetPortalControl().Set(0, ray_dir[1]);
  rays.DirZ.GetPortalControl().Set(0, ray_dir[2]);
}

void Camera::WriteSettingsToLog()
{
  Logger* logger = Logger::GetInstance();
  logger->AddLogData("position_x", Position[0]);
  logger->AddLogData("position_y", Position[1]);
  logger->AddLogData("position_z", Position[2]);

  logger->AddLogData("lookat_x", LookAt[0]);
  logger->AddLogData("lookat_y", LookAt[1]);
  logger->AddLogData("lookat_z", LookAt[2]);

  logger->AddLogData("up_x", Up[0]);
  logger->AddLogData("up_y", Up[1]);
  logger->AddLogData("up_z", Up[2]);

  logger->AddLogData("fov_x", FovX);
  logger->AddLogData("fov_y", FovY);
  logger->AddLogData("width", Width);
  logger->AddLogData("height", Height);
  logger->AddLogData("subset_height", SubsetHeight);
  logger->AddLogData("subset_width", SubsetWidth);
  logger->AddLogData("num_rays", SubsetWidth * SubsetHeight);
}

std::string Camera::ToString()
{
  std::stringstream sstream;
  sstream << "------------------------------------------------------------\n";
  sstream << "Position : [" << this->Position[0] << ",";
  sstream << this->Position[1] << ",";
  sstream << this->Position[2] << "]\n";
  sstream << "LookAt   : [" << this->LookAt[0] << ",";
  sstream << this->LookAt[1] << ",";
  sstream << this->LookAt[2] << "]\n";
  sstream << "FOV_X    : " << this->FovX << "\n";
  sstream << "Up       : [" << this->Up[0] << ",";
  sstream << this->Up[1] << ",";
  sstream << this->Up[2] << "]\n";
  sstream << "Width    : " << this->Width << "\n";
  sstream << "Height   : " << this->Height << "\n";
  sstream << "------------------------------------------------------------\n";
  return sstream.str();
}
}
}
} //namespace svtkm::rendering::raytracing
