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
 * @class   svtkOpenGLCamera
 * @brief   OpenGL camera
 *
 * svtkOpenGLCamera is a concrete implementation of the abstract class
 * svtkCamera.  svtkOpenGLCamera interfaces to the OpenGL rendering library.
 */

#ifndef svtkOpenGLCamera_h
#define svtkOpenGLCamera_h

#include "svtkCamera.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderer;
class svtkMatrix3x3;
class svtkMatrix4x4;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLCamera : public svtkCamera
{
public:
  static svtkOpenGLCamera* New();
  svtkTypeMacro(svtkOpenGLCamera, svtkCamera);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(svtkRenderer* ren) override;

  void UpdateViewport(svtkRenderer* ren) override;

  virtual void GetKeyMatrices(svtkRenderer* ren, svtkMatrix4x4*& WCVCMatrix,
    svtkMatrix3x3*& normalMatrix, svtkMatrix4x4*& VCDCMatrix, svtkMatrix4x4*& WCDCMatrix);

protected:
  svtkOpenGLCamera();
  ~svtkOpenGLCamera() override;

  svtkMatrix4x4* WCDCMatrix;
  svtkMatrix4x4* WCVCMatrix;
  svtkMatrix3x3* NormalMatrix;
  svtkMatrix4x4* VCDCMatrix;
  svtkTimeStamp KeyMatrixTime;
  svtkRenderer* LastRenderer;

private:
  svtkOpenGLCamera(const svtkOpenGLCamera&) = delete;
  void operator=(const svtkOpenGLCamera&) = delete;
};

#endif
