//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_Camera_h
#define svtk_m_rendering_Camera_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/Bounds.h>
#include <svtkm/Math.h>
#include <svtkm/Matrix.h>
#include <svtkm/Range.h>
#include <svtkm/Transform3D.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/rendering/MatrixHelpers.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT Camera
{
  struct Camera3DStruct
  {
  public:
    SVTKM_CONT
    Camera3DStruct()
      : LookAt(0.0f, 0.0f, 0.0f)
      , Position(0.0f, 0.0f, 1.0f)
      , ViewUp(0.0f, 1.0f, 0.0f)
      , FieldOfView(60.0f)
      , XPan(0.0f)
      , YPan(0.0f)
      , Zoom(1.0f)
    {
    }

    svtkm::Matrix<svtkm::Float32, 4, 4> CreateViewMatrix() const;

    svtkm::Matrix<svtkm::Float32, 4, 4> CreateProjectionMatrix(svtkm::Id width,
                                                             svtkm::Id height,
                                                             svtkm::Float32 nearPlane,
                                                             svtkm::Float32 farPlane) const;

    svtkm::Vec3f_32 LookAt;
    svtkm::Vec3f_32 Position;
    svtkm::Vec3f_32 ViewUp;
    svtkm::Float32 FieldOfView;
    svtkm::Float32 XPan;
    svtkm::Float32 YPan;
    svtkm::Float32 Zoom;
  };

  struct SVTKM_RENDERING_EXPORT Camera2DStruct
  {
  public:
    SVTKM_CONT
    Camera2DStruct()
      : Left(-1.0f)
      , Right(1.0f)
      , Bottom(-1.0f)
      , Top(1.0f)
      , XScale(1.0f)
      , XPan(0.0f)
      , YPan(0.0f)
      , Zoom(1.0f)
    {
    }

    svtkm::Matrix<svtkm::Float32, 4, 4> CreateViewMatrix() const;

    svtkm::Matrix<svtkm::Float32, 4, 4> CreateProjectionMatrix(svtkm::Float32 size,
                                                             svtkm::Float32 znear,
                                                             svtkm::Float32 zfar,
                                                             svtkm::Float32 aspect) const;

    svtkm::Float32 Left;
    svtkm::Float32 Right;
    svtkm::Float32 Bottom;
    svtkm::Float32 Top;
    svtkm::Float32 XScale;
    svtkm::Float32 XPan;
    svtkm::Float32 YPan;
    svtkm::Float32 Zoom;
  };

public:
  enum ModeEnum
  {
    MODE_2D,
    MODE_3D
  };
  SVTKM_CONT
  Camera(ModeEnum vtype = Camera::MODE_3D)
    : Mode(vtype)
    , NearPlane(0.01f)
    , FarPlane(1000.0f)
    , ViewportLeft(-1.0f)
    , ViewportRight(1.0f)
    , ViewportBottom(-1.0f)
    , ViewportTop(1.0f)
  {
  }

  svtkm::Matrix<svtkm::Float32, 4, 4> CreateViewMatrix() const;

  svtkm::Matrix<svtkm::Float32, 4, 4> CreateProjectionMatrix(svtkm::Id screenWidth,
                                                           svtkm::Id screenHeight) const;

  void GetRealViewport(svtkm::Id screenWidth,
                       svtkm::Id screenHeight,
                       svtkm::Float32& left,
                       svtkm::Float32& right,
                       svtkm::Float32& bottom,
                       svtkm::Float32& top) const;

  /// \brief The mode of the camera (2D or 3D).
  ///
  /// \c svtkm::Camera can be set to a 2D or 3D mode. 2D mode is used for
  /// looking at data in the x-y plane. 3D mode allows the camera to be
  /// positioned anywhere and pointing at any place in 3D.
  ///
  SVTKM_CONT
  svtkm::rendering::Camera::ModeEnum GetMode() const { return this->Mode; }
  SVTKM_CONT
  void SetMode(svtkm::rendering::Camera::ModeEnum mode) { this->Mode = mode; }
  SVTKM_CONT
  void SetModeTo3D() { this->SetMode(svtkm::rendering::Camera::MODE_3D); }
  SVTKM_CONT
  void SetModeTo2D() { this->SetMode(svtkm::rendering::Camera::MODE_2D); }

  /// \brief The clipping range of the camera.
  ///
  /// The clipping range establishes the near and far clipping planes. These
  /// clipping planes are parallel to the viewing plane. The planes are defined
  /// by simply specifying the distance from the viewpoint. Renderers can (and
  /// usually do) remove any geometry closer than the near plane and further
  /// than the far plane.
  ///
  /// For precision purposes, it is best to place the near plane as far away as
  /// possible (while still being in front of any geometry). The far plane
  /// usually has less effect on the depth precision, so can be placed well far
  /// behind the geometry.
  ///
  SVTKM_CONT
  svtkm::Range GetClippingRange() const { return svtkm::Range(this->NearPlane, this->FarPlane); }
  SVTKM_CONT
  void SetClippingRange(svtkm::Float32 nearPlane, svtkm::Float32 farPlane)
  {
    this->NearPlane = nearPlane;
    this->FarPlane = farPlane;
  }
  SVTKM_CONT
  void SetClippingRange(svtkm::Float64 nearPlane, svtkm::Float64 farPlane)
  {
    this->SetClippingRange(static_cast<svtkm::Float32>(nearPlane),
                           static_cast<svtkm::Float32>(farPlane));
  }
  SVTKM_CONT
  void SetClippingRange(const svtkm::Range& nearFarRange)
  {
    this->SetClippingRange(nearFarRange.Min, nearFarRange.Max);
  }

  /// \brief The viewport of the projection
  ///
  /// The projection of the camera can be offset to be centered around a subset
  /// of the rendered image. This is established with a "viewport," which is
  /// defined by the left/right and bottom/top of this viewport. The values of
  /// the viewport are relative to the rendered image's bounds. The left and
  /// bottom of the image are at -1 and the right and top are at 1.
  ///
  SVTKM_CONT
  void GetViewport(svtkm::Float32& left,
                   svtkm::Float32& right,
                   svtkm::Float32& bottom,
                   svtkm::Float32& top) const
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  SVTKM_CONT
  void GetViewport(svtkm::Float64& left,
                   svtkm::Float64& right,
                   svtkm::Float64& bottom,
                   svtkm::Float64& top) const
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  SVTKM_CONT
  svtkm::Bounds GetViewport() const
  {
    return svtkm::Bounds(
      this->ViewportLeft, this->ViewportRight, this->ViewportBottom, this->ViewportTop, 0.0, 0.0);
  }
  SVTKM_CONT
  void SetViewport(svtkm::Float32 left, svtkm::Float32 right, svtkm::Float32 bottom, svtkm::Float32 top)
  {
    this->ViewportLeft = left;
    this->ViewportRight = right;
    this->ViewportBottom = bottom;
    this->ViewportTop = top;
  }
  SVTKM_CONT
  void SetViewport(svtkm::Float64 left, svtkm::Float64 right, svtkm::Float64 bottom, svtkm::Float64 top)
  {
    this->SetViewport(static_cast<svtkm::Float32>(left),
                      static_cast<svtkm::Float32>(right),
                      static_cast<svtkm::Float32>(bottom),
                      static_cast<svtkm::Float32>(top));
  }
  SVTKM_CONT
  void SetViewport(const svtkm::Bounds& viewportBounds)
  {
    this->SetViewport(
      viewportBounds.X.Min, viewportBounds.X.Max, viewportBounds.Y.Min, viewportBounds.Y.Max);
  }

  /// \brief The focal point the camera is looking at in 3D mode
  ///
  /// When in 3D mode, the camera is set up to be facing the \c LookAt
  /// position. If \c LookAt is set, the mode is changed to 3D mode.
  ///
  SVTKM_CONT
  const svtkm::Vec3f_32& GetLookAt() const { return this->Camera3D.LookAt; }
  SVTKM_CONT
  void SetLookAt(const svtkm::Vec3f_32& lookAt)
  {
    this->SetModeTo3D();
    this->Camera3D.LookAt = lookAt;
  }
  SVTKM_CONT
  void SetLookAt(const svtkm::Vec<Float64, 3>& lookAt)
  {
    this->SetLookAt(svtkm::Vec<Float32, 3>(lookAt));
  }

  /// \brief The spatial position of the camera in 3D mode
  ///
  /// When in 3D mode, the camera is modeled to be at a particular location. If
  /// \c Position is set, the mode is changed to 3D mode.
  ///
  SVTKM_CONT
  const svtkm::Vec3f_32& GetPosition() const { return this->Camera3D.Position; }
  SVTKM_CONT
  void SetPosition(const svtkm::Vec3f_32& position)
  {
    this->SetModeTo3D();
    this->Camera3D.Position = position;
  }
  SVTKM_CONT
  void SetPosition(const svtkm::Vec3f_64& position) { this->SetPosition(svtkm::Vec3f_32(position)); }

  /// \brief The up orientation of the camera in 3D mode
  ///
  /// When in 3D mode, the camera is modeled to be at a particular location and
  /// looking at a particular spot. The view up vector orients the rotation of
  /// the image so that the top of the image is in the direction pointed to by
  /// view up. If \c ViewUp is set, the mode is changed to 3D mode.
  ///
  SVTKM_CONT
  const svtkm::Vec3f_32& GetViewUp() const { return this->Camera3D.ViewUp; }
  SVTKM_CONT
  void SetViewUp(const svtkm::Vec3f_32& viewUp)
  {
    this->SetModeTo3D();
    this->Camera3D.ViewUp = viewUp;
  }
  SVTKM_CONT
  void SetViewUp(const svtkm::Vec3f_64& viewUp) { this->SetViewUp(svtkm::Vec3f_32(viewUp)); }

  /// \brief The xscale of the camera
  ///
  /// The xscale forces the 2D curves to be full-frame
  ///
  /// Setting the xscale changes the mode to 2D.
  ///
  SVTKM_CONT
  svtkm::Float32 GetXScale() const { return this->Camera2D.XScale; }
  SVTKM_CONT
  void SetXScale(svtkm::Float32 xscale)
  {
    this->SetModeTo2D();
    this->Camera2D.XScale = xscale;
  }
  SVTKM_CONT
  void SetXScale(svtkm::Float64 xscale) { this->SetXScale(static_cast<svtkm::Float32>(xscale)); }

  /// \brief The field of view angle
  ///
  /// The field of view defines the angle (in degrees) that are visible from
  /// the camera position.
  ///
  /// Setting the field of view changes the mode to 3D.
  ///
  SVTKM_CONT
  svtkm::Float32 GetFieldOfView() const { return this->Camera3D.FieldOfView; }
  SVTKM_CONT
  void SetFieldOfView(svtkm::Float32 fov)
  {
    this->SetModeTo3D();
    this->Camera3D.FieldOfView = fov;
  }
  SVTKM_CONT
  void SetFieldOfView(svtkm::Float64 fov) { this->SetFieldOfView(static_cast<svtkm::Float32>(fov)); }

  /// \brief Pans the camera
  ///
  void Pan(svtkm::Float32 dx, svtkm::Float32 dy);

  /// \brief Pans the camera
  ///
  SVTKM_CONT
  void Pan(svtkm::Float64 dx, svtkm::Float64 dy)
  {
    this->Pan(static_cast<svtkm::Float32>(dx), static_cast<svtkm::Float32>(dy));
  }
  SVTKM_CONT
  void Pan(svtkm::Vec2f_32 direction) { this->Pan(direction[0], direction[1]); }

  SVTKM_CONT
  void Pan(svtkm::Vec2f_64 direction) { this->Pan(direction[0], direction[1]); }

  SVTKM_CONT
  svtkm::Vec2f_32 GetPan() const
  {
    svtkm::Vec2f_32 pan;
    pan[0] = this->Camera3D.XPan;
    pan[1] = this->Camera3D.YPan;
    return pan;
  }


  /// \brief Zooms the camera in or out
  ///
  /// Zooming the camera scales everything in the image up or down. Positive
  /// zoom makes the geometry look bigger or closer. Negative zoom has the
  /// opposite effect. A zoom of 0 has no effect.
  ///
  void Zoom(svtkm::Float32 zoom);

  SVTKM_CONT
  void Zoom(svtkm::Float64 zoom) { this->Zoom(static_cast<svtkm::Float32>(zoom)); }

  SVTKM_CONT
  svtkm::Float32 GetZoom() const { return this->Camera3D.Zoom; }

  /// \brief Moves the camera as if a point was dragged along a sphere.
  ///
  /// \c TrackballRotate takes the normalized screen coordinates (in the range
  /// -1 to 1) and rotates the camera around the \c LookAt position. The rotation
  /// first projects the points to a sphere around the \c LookAt position. The
  /// camera is then rotated as if the start point was dragged to the end point
  /// along with the world.
  ///
  /// \c TrackballRotate changes the mode to 3D.
  ///
  void TrackballRotate(svtkm::Float32 startX,
                       svtkm::Float32 startY,
                       svtkm::Float32 endX,
                       svtkm::Float32 endY);

  SVTKM_CONT
  void TrackballRotate(svtkm::Float64 startX,
                       svtkm::Float64 startY,
                       svtkm::Float64 endX,
                       svtkm::Float64 endY)
  {
    this->TrackballRotate(static_cast<svtkm::Float32>(startX),
                          static_cast<svtkm::Float32>(startY),
                          static_cast<svtkm::Float32>(endX),
                          static_cast<svtkm::Float32>(endY));
  }

  /// \brief Set up the camera to look at geometry
  ///
  /// \c ResetToBounds takes a \c Bounds structure containing the bounds in
  /// 3D space that contain the geometry being rendered. This method sets up
  /// the camera so that it is looking at this region in space. The view
  /// direction is preserved.
  ///
  void ResetToBounds(const svtkm::Bounds& dataBounds);

  /// \brief Set up the camera to look at geometry with padding
  ///
  /// \c ResetToBounds takes a \c Bounds structure containing the bounds in
  /// 3D space that contain the geometry being rendered and a \c Float64 value
  /// representing the percent that a view should be padded in x, y, and z.
  /// This method sets up the camera so that it is looking at this region in
  // space with the given padding percent. The view direction is preserved.
  ///
  void ResetToBounds(const svtkm::Bounds& dataBounds, svtkm::Float64 dataViewPadding);
  void ResetToBounds(const svtkm::Bounds& dataBounds,
                     svtkm::Float64 XDataViewPadding,
                     svtkm::Float64 YDataViewPadding,
                     svtkm::Float64 ZDataViewPadding);

  /// \brief Roll the camera
  ///
  /// Rotates the camera around the view direction by the given angle. The
  /// angle is given in degrees.
  ///
  /// Roll is currently only supported for 3D cameras.
  ///
  void Roll(svtkm::Float32 angleDegrees);

  SVTKM_CONT
  void Roll(svtkm::Float64 angleDegrees) { this->Roll(static_cast<svtkm::Float32>(angleDegrees)); }

  /// \brief Rotate the camera about the view up vector centered at the focal point.
  ///
  /// Note that the view up vector is whatever was set via SetViewUp, and is
  /// not necessarily perpendicular to the direction of projection. The angle is
  /// given in degrees.
  ///
  /// Azimuth only makes sense for 3D cameras, so the camera mode will be set
  /// to 3D when this method is called.
  ///
  void Azimuth(svtkm::Float32 angleDegrees);

  SVTKM_CONT
  void Azimuth(svtkm::Float64 angleDegrees)
  {
    this->Azimuth(static_cast<svtkm::Float32>(angleDegrees));
  }

  /// \brief Rotate the camera vertically around the focal point.
  ///
  /// Specifically, this rotates the camera about the cross product of the
  /// negative of the direction of projection and the view up vector, using the
  /// focal point (LookAt) as the center of rotation. The angle is given
  /// in degrees.
  ///
  /// Elevation only makes sense for 3D cameras, so the camera mode will be set
  /// to 3D when this method is called.
  ///
  void Elevation(svtkm::Float32 angleDegrees);

  SVTKM_CONT
  void Elevation(svtkm::Float64 angleDegrees)
  {
    this->Elevation(static_cast<svtkm::Float32>(angleDegrees));
  }

  /// \brief Move the camera toward or away from the focal point.
  ///
  /// Specifically, this divides the camera's distance from the focal point
  /// (LookAt) by the given value. Use a value greater than one to dolly in
  /// toward the focal point, and use a value less than one to dolly-out away
  /// from the focal point.
  ///
  /// Dolly only makes sense for 3D cameras, so the camera mode will be set to
  /// 3D when this method is called.
  ///
  void Dolly(svtkm::Float32 value);

  SVTKM_CONT
  void Dolly(svtkm::Float64 value) { this->Dolly(static_cast<svtkm::Float32>(value)); }

  /// \brief The viewable region in the x-y plane
  ///
  /// When the camera is in 2D, it is looking at some region of the x-y plane.
  /// The region being looked at is defined by the range in x (determined by
  /// the left and right sides) and by the range in y (determined by the bottom
  /// and top sides).
  ///
  /// \c SetViewRange2D changes the camera mode to 2D.
  ///
  SVTKM_CONT
  void GetViewRange2D(svtkm::Float32& left,
                      svtkm::Float32& right,
                      svtkm::Float32& bottom,
                      svtkm::Float32& top) const
  {
    left = this->Camera2D.Left;
    right = this->Camera2D.Right;
    bottom = this->Camera2D.Bottom;
    top = this->Camera2D.Top;
  }
  SVTKM_CONT
  svtkm::Bounds GetViewRange2D() const
  {
    return svtkm::Bounds(this->Camera2D.Left,
                        this->Camera2D.Right,
                        this->Camera2D.Bottom,
                        this->Camera2D.Top,
                        0.0,
                        0.0);
  }
  SVTKM_CONT
  void SetViewRange2D(svtkm::Float32 left,
                      svtkm::Float32 right,
                      svtkm::Float32 bottom,
                      svtkm::Float32 top)
  {
    this->SetModeTo2D();
    this->Camera2D.Left = left;
    this->Camera2D.Right = right;
    this->Camera2D.Bottom = bottom;
    this->Camera2D.Top = top;

    this->Camera2D.XPan = 0;
    this->Camera2D.YPan = 0;
    this->Camera2D.Zoom = 1;
  }
  SVTKM_CONT
  void SetViewRange2D(svtkm::Float64 left,
                      svtkm::Float64 right,
                      svtkm::Float64 bottom,
                      svtkm::Float64 top)
  {
    this->SetViewRange2D(static_cast<svtkm::Float32>(left),
                         static_cast<svtkm::Float32>(right),
                         static_cast<svtkm::Float32>(bottom),
                         static_cast<svtkm::Float32>(top));
  }
  SVTKM_CONT
  void SetViewRange2D(const svtkm::Range& xRange, const svtkm::Range& yRange)
  {
    this->SetViewRange2D(xRange.Min, xRange.Max, yRange.Min, yRange.Max);
  }
  SVTKM_CONT
  void SetViewRange2D(const svtkm::Bounds& viewRange)
  {
    this->SetViewRange2D(viewRange.X, viewRange.Y);
  }

  SVTKM_CONT
  void Print() const;

private:
  ModeEnum Mode;
  Camera3DStruct Camera3D;
  Camera2DStruct Camera2D;

  svtkm::Float32 NearPlane;
  svtkm::Float32 FarPlane;

  svtkm::Float32 ViewportLeft;
  svtkm::Float32 ViewportRight;
  svtkm::Float32 ViewportBottom;
  svtkm::Float32 ViewportTop;
};
}
} // namespace svtkm::rendering

#endif // svtk_m_rendering_Camera_h
