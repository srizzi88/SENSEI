/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFollower.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFollower
 * @brief   a subclass of actor that always faces the camera
 *
 * svtkFollower is a subclass of svtkActor that always follows its specified
 * camera. More specifically it will not change its position or scale,
 * but it will continually update its orientation so that it is right side
 * up and facing the camera. This is typically used for text labels in a
 * scene. All of the adjustments that can be made to an actor also will
 * take effect with a follower.  So, if you change the orientation of the
 * follower by 90 degrees, then it will follow the camera, but be off by
 * 90 degrees.
 *
 * @sa
 * svtkActor svtkCamera svtkAxisFollower svtkProp3DFollower
 */

#ifndef svtkFollower_h
#define svtkFollower_h

#include "svtkActor.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkCamera;

class SVTKRENDERINGCORE_EXPORT svtkFollower : public svtkActor
{
public:
  svtkTypeMacro(svtkFollower, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a follower with no camera set
   */
  static svtkFollower* New();

  //@{
  /**
   * Set/Get the camera to follow. If this is not set, then the follower
   * won't know who to follow.
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
  virtual void Render(svtkRenderer* ren);
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
   * ComputeMatrix() method due to the special svtkFollower matrix operations.
   */
  void ComputeMatrix() override;

  /**
   * Shallow copy of a follower. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

protected:
  svtkFollower();
  ~svtkFollower() override;

  svtkCamera* Camera;
  svtkActor* Device;

  // Internal matrices to avoid New/Delete for performance reasons
  svtkMatrix4x4* InternalMatrix;

private:
  svtkFollower(const svtkFollower&) = delete;
  void operator=(const svtkFollower&) = delete;

  // hide the two parameter Render() method from the user and the compiler.
  void Render(svtkRenderer*, svtkMapper*) override {}
};

#endif
