/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLProperty
 * @brief   OpenGL property
 *
 * svtkOpenGLProperty is a concrete implementation of the abstract class
 * svtkProperty. svtkOpenGLProperty interfaces to the OpenGL rendering library.
 */

#ifndef svtkOpenGLProperty_h
#define svtkOpenGLProperty_h

#include "svtkProperty.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLProperty : public svtkProperty
{
public:
  static svtkOpenGLProperty* New();
  svtkTypeMacro(svtkOpenGLProperty, svtkProperty);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(svtkActor* a, svtkRenderer* ren) override;

  /**
   * Implement base class method.
   */
  void BackfaceRender(svtkActor* a, svtkRenderer* ren) override;

  /**
   * This method is called after the actor has been rendered.
   * Don't call this directly. This method cleans up
   * any shaders allocated.
   */
  void PostRender(svtkActor* a, svtkRenderer* r) override;

  /**
   * Release any graphics resources that are being consumed by this
   * property. The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow* win) override;

protected:
  svtkOpenGLProperty();
  ~svtkOpenGLProperty() override;

  /**
   * Method called in svtkOpenGLProperty::Render() to render textures.
   * Last argument is the value returned from RenderShaders() call.
   */
  bool RenderTextures(svtkActor* actor, svtkRenderer* renderer);

private:
  svtkOpenGLProperty(const svtkOpenGLProperty&) = delete;
  void operator=(const svtkOpenGLProperty&) = delete;
};

#endif
