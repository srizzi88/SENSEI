/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkContextActor
 * @brief   provides a svtkProp derived object.
 *
 * This object provides the entry point for the svtkContextScene to be rendered
 * in a svtkRenderer. Uses the RenderOverlay pass to render the 2D
 * svtkContextScene.
 */

#ifndef svtkContextActor_h
#define svtkContextActor_h

#include "svtkNew.h" // For ivars
#include "svtkProp.h"
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkSmartPointer.h"             // For ivars

class svtkContext2D;
class svtkContext3D;
class svtkContextDevice2D;
class svtkContextScene;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextActor : public svtkProp
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkContextActor, svtkProp);

  static svtkContextActor* New();

  /**
   * We only render in the overlay for the context scene.
   */
  int RenderOverlay(svtkViewport* viewport) override;

  //@{
  /**
   * Get the svtkContext2D for the actor.
   */
  svtkGetNewMacro(Context, svtkContext2D);
  //@}

  /**
   * Get the chart object for the actor.
   */
  svtkContextScene* GetScene();

  /**
   * Set the scene for the actor.
   */
  void SetScene(svtkContextScene* scene);

  /**
   * Force rendering to a specific device. If left NULL, a default
   * device will be created.
   * @{
   */
  void SetForceDevice(svtkContextDevice2D* dev);
  svtkGetObjectMacro(ForceDevice, svtkContextDevice2D);
  /**@}*/

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow* window) override;

protected:
  svtkContextActor();
  ~svtkContextActor() override;

  /**
   * Initialize the actor - right now we just decide which device to initialize.
   */
  virtual void Initialize(svtkViewport* viewport);

  svtkSmartPointer<svtkContextScene> Scene;
  svtkNew<svtkContext2D> Context;
  svtkNew<svtkContext3D> Context3D;
  svtkContextDevice2D* ForceDevice;
  bool Initialized;

private:
  svtkContextActor(const svtkContextActor&) = delete;
  void operator=(const svtkContextActor&) = delete;
};

#endif
