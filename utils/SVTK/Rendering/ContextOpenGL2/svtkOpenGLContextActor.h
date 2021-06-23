/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLContextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLContextActor
 * @brief   provides a svtkProp derived object.
 *
 * This object provides the entry point for the svtkContextScene to be rendered
 * in a svtkRenderer. Uses the RenderOverlay pass to render the 2D
 * svtkContextScene.
 */

#ifndef svtkOpenGLContextActor_h
#define svtkOpenGLContextActor_h

#include "svtkContextActor.h"
#include "svtkRenderingContextOpenGL2Module.h" // For export macro

class SVTKRENDERINGCONTEXTOPENGL2_EXPORT svtkOpenGLContextActor : public svtkContextActor
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkOpenGLContextActor, svtkContextActor);

  static svtkOpenGLContextActor* New();

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow* window) override;

  /**
   * We only render in the overlay for the context scene.
   */
  int RenderOverlay(svtkViewport* viewport) override;

protected:
  svtkOpenGLContextActor();
  ~svtkOpenGLContextActor() override;

  /**
   * Initialize the actor - right now we just decide which device to initialize.
   */
  void Initialize(svtkViewport* viewport) override;

private:
  svtkOpenGLContextActor(const svtkOpenGLContextActor&) = delete;
  void operator=(const svtkOpenGLContextActor&) = delete;
};

#endif
