/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkOpenGLHelper_h
#define svtkOpenGLHelper_h

#include "svtkRenderingOpenGL2Module.h" // for export macro
#include "svtkTimeStamp.h"

class svtkOpenGLIndexBufferObject;
class svtkOpenGLShaderCache;
class svtkOpenGLVertexArrayObject;
class svtkShaderProgram;
class svtkWindow;

// Store the shaders, program, and ibo in a common place
// as they are used together frequently.  This is just
// a convenience class.
class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLHelper
{
public:
  svtkShaderProgram* Program;
  svtkTimeStamp ShaderSourceTime;
  svtkOpenGLVertexArrayObject* VAO;
  svtkTimeStamp AttributeUpdateTime;
  int PrimitiveType;
  unsigned int ShaderChangeValue;

  svtkOpenGLIndexBufferObject* IBO;

  svtkOpenGLHelper();
  ~svtkOpenGLHelper();
  void ReleaseGraphicsResources(svtkWindow* win);

private:
  svtkOpenGLHelper(const svtkOpenGLHelper&) = delete;
  svtkOpenGLHelper& operator=(const svtkOpenGLHelper&) = delete;
};

#endif // svtkOpenGLHelper_h

// SVTK-HeaderTest-Exclude: svtkOpenGLHelper.h
