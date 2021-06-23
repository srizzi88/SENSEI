/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCameraActor
 * @brief   a frustum to represent a camera.
 *
 * svtkCameraActor is an actor used to represent a camera by its wireframe
 * frustum.
 *
 * @sa
 * svtkLight svtkConeSource svtkFrustumSource svtkCameraActor
 */

#ifndef svtkCameraActor_h
#define svtkCameraActor_h

#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkCamera;
class svtkFrustumSource;
class svtkPolyDataMapper;
class svtkActor;
class svtkProperty;

class SVTKRENDERINGCORE_EXPORT svtkCameraActor : public svtkProp3D
{
public:
  static svtkCameraActor* New();
  svtkTypeMacro(svtkCameraActor, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The camera to represent. Initial value is NULL.
   */
  void SetCamera(svtkCamera* camera);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  //@{
  /**
   * Ratio between the width and the height of the frustum. Initial value is
   * 1.0 (square)
   */
  svtkSetMacro(WidthByHeightRatio, double);
  svtkGetMacro(WidthByHeightRatio, double);
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

  /**
   * Get property of the internal actor.
   */
  svtkProperty* GetProperty();

  /**
   * Set property of the internal actor.
   */
  void SetProperty(svtkProperty* p);

protected:
  svtkCameraActor();
  ~svtkCameraActor() override;

  void UpdateViewProps();

  svtkCamera* Camera;
  double WidthByHeightRatio;

  svtkFrustumSource* FrustumSource;
  svtkPolyDataMapper* FrustumMapper;
  svtkActor* FrustumActor;

private:
  svtkCameraActor(const svtkCameraActor&) = delete;
  void operator=(const svtkCameraActor&) = delete;
};

#endif
