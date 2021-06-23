/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLQuadHelper
 * @brief   Class to make rendering a full screen quad easier
 *
 * svtkOpenGLQuadHelper is designed to be used by classes in SVTK that
 * need to render a quad to the screen with a shader program. This happens
 * often with render passes or other advanced rendering techniques.
 *
 * Note that when releasing graphics resources instances of this
 * class should be destroyed. A common use pattern is to conditionally
 * create the instance where used and delete it in ReleaseGraphicsResources
 * and the destructor.
 *
 * Example usage:
 * @code
 * if (!this->QuadHelper)
 * {
 *   this->QuadHelper = svtkOpenGLQualHelper(renWin, vs, fs, gs);
 * }
 * renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
 * aTexture->Activate();
 * this->QuadHelper->Program->SetUniformi("aTexture", aTexture->GetTextureUnit());
 * this->QuadHelper->Render();
 * aTexture->Deactivate();
 * @endcode
 *
 * @sa svtkOpenGLRenderUtilities
 */

#ifndef svtkOpenGLQuadHelper_h
#define svtkOpenGLQuadHelper_h

#include "svtkRenderingOpenGL2Module.h" // for export macro
#include "svtkTimeStamp.h"
#include <memory> // for std::unique_ptr

class svtkOpenGLRenderWindow;
class svtkOpenGLVertexArrayObject;
class svtkShaderProgram;
class svtkGenericOpenGLResourceFreeCallback;
class svtkWindow;

// Helper class to render full screen quads
class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLQuadHelper
{
public:
  svtkShaderProgram* Program;
  svtkTimeStamp ShaderSourceTime;
  svtkOpenGLVertexArrayObject* VAO;
  unsigned int ShaderChangeValue;

  // create a quadhelper with the provided shaders
  // if the vertex is nullptr
  // then the default is used. Note that this
  // class should be destroyed upon
  // ReleaseGraphicsResources
  svtkOpenGLQuadHelper(svtkOpenGLRenderWindow*, const char* vs, const char* fs, const char* gs);

  ~svtkOpenGLQuadHelper();

  // Draw the Quad, will bind the VAO for you
  void Render();

  /**
   * Release graphics resources. In general, there's no need to call this
   * explicitly, since svtkOpenGLQuadHelper will invoke it appropriately when
   * needed.
   */
  void ReleaseGraphicsResources(svtkWindow*);

private:
  svtkOpenGLQuadHelper(const svtkOpenGLQuadHelper&) = delete;
  svtkOpenGLQuadHelper& operator=(const svtkOpenGLQuadHelper&) = delete;
  std::unique_ptr<svtkGenericOpenGLResourceFreeCallback> ResourceCallback;
};

#endif // svtkOpenGLQuadHelper_h

// SVTK-HeaderTest-Exclude: svtkOpenGLQuadHelper.h
