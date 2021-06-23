/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTextActor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLTextActor3D
 * @brief   OpenGL2 override for svtkTextActor3D.
 */

#ifndef svtkOpenGLTextActor3D_h
#define svtkOpenGLTextActor3D_h

#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkTextActor3D.h"

class svtkOpenGLGL2PSHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLTextActor3D : public svtkTextActor3D
{
public:
  static svtkOpenGLTextActor3D* New();
  svtkTypeMacro(svtkOpenGLTextActor3D, svtkTextActor3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;

protected:
  svtkOpenGLTextActor3D();
  ~svtkOpenGLTextActor3D() override;

  int RenderGL2PS(svtkViewport* vp, svtkOpenGLGL2PSHelper* gl2ps);

private:
  svtkOpenGLTextActor3D(const svtkOpenGLTextActor3D&) = delete;
  void operator=(const svtkOpenGLTextActor3D&) = delete;
};

#endif // svtkOpenGLTextActor3D_h
