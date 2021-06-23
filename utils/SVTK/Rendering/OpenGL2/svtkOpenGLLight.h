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
 * @class   svtkOpenGLLight
 * @brief   OpenGL light
 *
 * svtkOpenGLLight is a concrete implementation of the abstract class svtkLight.
 * svtkOpenGLLight interfaces to the OpenGL rendering library.
 */

#ifndef svtkOpenGLLight_h
#define svtkOpenGLLight_h

#include "svtkLight.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderer;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLLight : public svtkLight
{
public:
  static svtkOpenGLLight* New();
  svtkTypeMacro(svtkOpenGLLight, svtkLight);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(svtkRenderer* ren, int light_index) override;

protected:
  svtkOpenGLLight() {}
  ~svtkOpenGLLight() override {}

private:
  svtkOpenGLLight(const svtkOpenGLLight&) = delete;
  void operator=(const svtkOpenGLLight&) = delete;
};

#endif
