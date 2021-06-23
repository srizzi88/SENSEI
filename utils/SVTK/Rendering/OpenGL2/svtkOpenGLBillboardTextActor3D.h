/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLBillboardTextActor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkOpenGLBillboardTextActor3D
 * @brief Handles GL2PS capture of billboard text.
 */

#ifndef svtkOpenGLBillboardTextActor3D_h
#define svtkOpenGLBillboardTextActor3D_h

#include "svtkBillboardTextActor3D.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLGL2PSHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLBillboardTextActor3D : public svtkBillboardTextActor3D
{
public:
  static svtkOpenGLBillboardTextActor3D* New();
  svtkTypeMacro(svtkOpenGLBillboardTextActor3D, svtkBillboardTextActor3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  int RenderTranslucentPolygonalGeometry(svtkViewport* vp) override;

protected:
  svtkOpenGLBillboardTextActor3D();
  ~svtkOpenGLBillboardTextActor3D() override;

  int RenderGL2PS(svtkViewport* viewport, svtkOpenGLGL2PSHelper* gl2ps);

private:
  svtkOpenGLBillboardTextActor3D(const svtkOpenGLBillboardTextActor3D&) = delete;
  void operator=(const svtkOpenGLBillboardTextActor3D&) = delete;
};

#endif // svtkOpenGLBillboardTextActor3D_h
