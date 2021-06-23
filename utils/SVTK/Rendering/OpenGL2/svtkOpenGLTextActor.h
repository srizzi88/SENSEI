/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLTextActor
 * @brief   svtkTextActor override.
 */

#ifndef svtkOpenGLTextActor_h
#define svtkOpenGLTextActor_h

#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkTextActor.h"

class svtkOpenGLGL2PSHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLTextActor : public svtkTextActor
{
public:
  static svtkOpenGLTextActor* New();
  svtkTypeMacro(svtkOpenGLTextActor, svtkTextActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  int RenderOverlay(svtkViewport* viewport) override;

protected:
  svtkOpenGLTextActor();
  ~svtkOpenGLTextActor() override;

  int RenderGL2PS(svtkViewport* viewport, svtkOpenGLGL2PSHelper* gl2ps);

private:
  svtkOpenGLTextActor(const svtkOpenGLTextActor&) = delete;
  void operator=(const svtkOpenGLTextActor&) = delete;
};

#endif // svtkOpenGLTextActor_h
