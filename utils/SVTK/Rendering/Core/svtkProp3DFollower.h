/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp3DFollower.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProp3DFollower
 * @brief   a svtkProp3D that always faces the camera
 *
 * svtkProp3DFollower is a type of svtkProp3D that always faces the camera.
 * More specifically it will not change its position or scale,
 * but it will continually update its orientation so that it is right side
 * up and facing the camera. This is typically used for complex billboards
 * or props that need to face the viewer at all times.
 *
 * Note: All of the transformations that can be made to a svtkProp3D will take
 * effect with the follower. Thus, if you change the orientation of the
 * follower by 90 degrees, then it will follow the camera, but be off by 90
 * degrees.
 *
 * @sa
 * svtkFollower svtkProp3D svtkCamera svtkProp3DAxisFollower
 */

#ifndef svtkProp3DFollower_h
#define svtkProp3DFollower_h

#include "svtkProp3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkCamera;
class svtkMapper;

class SVTKRENDERINGCORE_EXPORT svtkProp3DFollower : public svtkProp3D
{
public:
  /**
   * Creates a follower with no camera set.
   */
  static svtkProp3DFollower* New();

  //@{
  /**
   * Standard SVTK methods for type and printing.
   */
  svtkTypeMacro(svtkProp3DFollower, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the svtkProp3D to control (i.e., face the camera).
   */
  virtual void SetProp3D(svtkProp3D* prop);
  virtual svtkProp3D* GetProp3D();
  //@}

  //@{
  /**
   * Set/Get the camera to follow. If this is not set, then the follower
   * won't know what to follow and will act like a normal svtkProp3D.
   */
  virtual void SetCamera(svtkCamera*);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  //@{
  /**
   * This causes the actor to be rendered. It in turn will render the actor's
   * property, texture map and then mapper. If a property hasn't been
   * assigned, then the actor will create one automatically.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  int RenderVolumetricGeometry(svtkViewport* viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources associated with this svtkProp3DFollower.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Generate the matrix based on ivars. This method overloads its superclasses
   * ComputeMatrix() method due to the special svtkProp3DFollower matrix operations.
   */
  void ComputeMatrix() override;

  /**
   * Shallow copy of a follower. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * Return the bounds of this svtkProp3D.
   */
  double* GetBounds() override;

  //@{
  /**
   * Overload svtkProp's method for setting up assembly paths. See
   * the documentation for svtkProp.
   */
  void InitPathTraversal() override;
  svtkAssemblyPath* GetNextPath() override;
  //@}

protected:
  svtkProp3DFollower();
  ~svtkProp3DFollower() override;

  svtkCamera* Camera;
  svtkProp3D* Device;

  // Internal matrices to avoid New/Delete for performance reasons
  svtkMatrix4x4* InternalMatrix;

private:
  svtkProp3DFollower(const svtkProp3DFollower&) = delete;
  void operator=(const svtkProp3DFollower&) = delete;
};

#endif
