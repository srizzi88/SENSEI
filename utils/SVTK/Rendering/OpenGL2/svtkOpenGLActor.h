/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLActor
 * @brief   OpenGL actor
 *
 * svtkOpenGLActor is a concrete implementation of the abstract class svtkActor.
 * svtkOpenGLActor interfaces to the OpenGL rendering library.
 */

#ifndef svtkOpenGLActor_h
#define svtkOpenGLActor_h

#include "svtkActor.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkInformationIntegerKey;
class svtkOpenGLRenderer;
class svtkMatrix4x4;
class svtkMatrix3x3;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLActor : public svtkActor
{
public:
  static svtkOpenGLActor* New();
  svtkTypeMacro(svtkOpenGLActor, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Actual actor render method.
   */
  void Render(svtkRenderer* ren, svtkMapper* mapper) override;

  virtual void GetKeyMatrices(svtkMatrix4x4*& WCVCMatrix, svtkMatrix3x3*& normalMatrix);

  /**
   * If this key is set in GetPropertyKeys(), the glDepthMask will be adjusted
   * prior to rendering translucent objects. This is useful for e.g. depth
   * peeling.

   * If GetIsOpaque() == true, the depth mask is always enabled, regardless of
   * this key. Otherwise, the depth mask is disabled for default alpha blending
   * unless this key is set.

   * If this key is set, the integer value has the following meanings:
   * 0: glDepthMask(GL_FALSE)
   * 1: glDepthMask(GL_TRUE)
   * Anything else: No change to depth mask.
   */
  static svtkInformationIntegerKey* GLDepthMaskOverride();

protected:
  svtkOpenGLActor();
  ~svtkOpenGLActor() override;

  svtkMatrix4x4* MCWCMatrix;
  svtkMatrix3x3* NormalMatrix;
  svtkTransform* NormalTransform;
  svtkTimeStamp KeyMatrixTime;

private:
  svtkOpenGLActor(const svtkOpenGLActor&) = delete;
  void operator=(const svtkOpenGLActor&) = delete;
};

#endif
