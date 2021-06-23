/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLightActor
 * @brief   a cone and a frustum to represent a spotlight.
 *
 * svtkLightActor is a composite actor used to represent a spotlight. The cone
 * angle is equal to the spotlight angle, the cone apex is at the position of
 * the light, the direction of the light goes from the cone apex to the center
 * of the base of the cone. The square frustum position is the light position,
 * the frustum focal point is in the direction of the light direction. The
 * frustum vertical view angle (aperture) (this is also the horizontal view
 * angle as the frustum is square) is equal to twice the cone angle. The
 * clipping range of the frustum is arbitrary set by the user
 * (initially at 0.5,11.0).
 *
 * @warning
 * Right now only spotlight are supported but directional light might be
 * supported in the future.
 *
 * @sa
 * svtkLight svtkConeSource svtkFrustumSource svtkCameraActor
 */

#ifndef svtkLightActor_h
#define svtkLightActor_h

#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkLight;
class svtkConeSource;
class svtkPolyDataMapper;
class svtkActor;
class svtkCamera;
class svtkCameraActor;
class svtkBoundingBox;

class SVTKRENDERINGCORE_EXPORT svtkLightActor : public svtkProp3D
{
public:
  static svtkLightActor* New();
  svtkTypeMacro(svtkLightActor, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The spotlight to represent. Initial value is NULL.
   */
  void SetLight(svtkLight* light);
  svtkGetObjectMacro(Light, svtkLight);
  //@}

  //@{
  /**
   * Set/Get the location of the near and far clipping planes along the
   * direction of projection.  Both of these values must be positive.
   * Initial values are  (0.5,11.0)
   */
  void SetClippingRange(double dNear, double dFar);
  void SetClippingRange(const double a[2]);
  svtkGetVector2Macro(ClippingRange, double);
  //@}

  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;

  /**
   * Does this prop have some translucent polygonal geometry? No.
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  double* GetBounds() override;

  /**
   * Get the actors mtime plus consider its properties and texture if set.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkLightActor();
  ~svtkLightActor() override;

  void UpdateViewProps();

  svtkLight* Light;
  double ClippingRange[2];

  svtkConeSource* ConeSource;
  svtkPolyDataMapper* ConeMapper;
  svtkActor* ConeActor;

  svtkCamera* CameraLight;
  svtkCameraActor* FrustumActor;

  svtkBoundingBox* BoundingBox;

private:
  svtkLightActor(const svtkLightActor&) = delete;
  void operator=(const svtkLightActor&) = delete;
};

#endif
