/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTextMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLTextMapper
 * @brief   svtkTextMapper override for OpenGL2.
 */

#ifndef svtkOpenGLTextMapper_h
#define svtkOpenGLTextMapper_h

#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkTextMapper.h"

class svtkOpenGLGL2PSHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLTextMapper : public svtkTextMapper
{
public:
  static svtkOpenGLTextMapper* New();
  svtkTypeMacro(svtkOpenGLTextMapper, svtkTextMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void RenderOverlay(svtkViewport* vp, svtkActor2D* act) override;

protected:
  svtkOpenGLTextMapper();
  ~svtkOpenGLTextMapper() override;

  void RenderGL2PS(svtkViewport* vp, svtkActor2D* act, svtkOpenGLGL2PSHelper* gl2ps);

private:
  svtkOpenGLTextMapper(const svtkOpenGLTextMapper&) = delete;
  void operator=(const svtkOpenGLTextMapper&) = delete;
};

#endif // svtkOpenGLTextMapper_h
