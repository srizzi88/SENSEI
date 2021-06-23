/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLImageMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLImageMapper
 * @brief   2D image display support for OpenGL
 *
 * svtkOpenGLImageMapper is a concrete subclass of svtkImageMapper that
 * renders images under OpenGL
 *
 * @warning
 * svtkOpenGLImageMapper does not support svtkBitArray, you have to convert the array first
 * to svtkUnsignedCharArray (for example)
 *
 * @sa
 * svtkImageMapper
 */

#ifndef svtkOpenGLImageMapper_h
#define svtkOpenGLImageMapper_h

#include "svtkImageMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkActor2D;
class svtkTexturedActor2D;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLImageMapper : public svtkImageMapper
{
public:
  static svtkOpenGLImageMapper* New();
  svtkTypeMacro(svtkOpenGLImageMapper, svtkImageMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Handle the render method.
   */
  void RenderOverlay(svtkViewport* viewport, svtkActor2D* actor) override
  {
    this->RenderStart(viewport, actor);
  }

  /**
   * Called by the Render function in svtkImageMapper.  Actually draws
   * the image to the screen.
   */
  void RenderData(svtkViewport* viewport, svtkImageData* data, svtkActor2D* actor) override;

  /**
   * draw the data once it has been converted to uchar, windowed leveled
   * used internally by the templated functions
   */
  void DrawPixels(svtkViewport* vp, int width, int height, int numComponents, void* data);

  /**
   * Release any graphics resources that are being consumed by this
   * mapper, the image texture in particular.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkOpenGLImageMapper();
  ~svtkOpenGLImageMapper() override;

  svtkTexturedActor2D* Actor;

private:
  svtkOpenGLImageMapper(const svtkOpenGLImageMapper&) = delete;
  void operator=(const svtkOpenGLImageMapper&) = delete;
};

#endif
