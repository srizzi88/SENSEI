/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExternalOpenGLCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExternalOpenGLCamera
 * @brief   OpenGL camera
 *
 * svtkExternalOpenGLCamera is a concrete implementation of the abstract class
 * svtkCamera.  svtkExternalOpenGLCamera interfaces to the OpenGL rendering library.
 * This class extends svtkOpenGLCamera by introducing API wherein the camera
 * matrices can be set explicitly by the application.
 */

#ifndef svtkExternalOpenGLCamera_h
#define svtkExternalOpenGLCamera_h

#include "svtkOpenGLCamera.h"
#include "svtkRenderingExternalModule.h" // For export macro

class SVTKRENDERINGEXTERNAL_EXPORT svtkExternalOpenGLCamera : public svtkOpenGLCamera
{
public:
  static svtkExternalOpenGLCamera* New();
  svtkTypeMacro(svtkExternalOpenGLCamera, svtkOpenGLCamera);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the view transform matrix
   */
  void SetViewTransformMatrix(const double elements[16]);

  /**
   * Set the projection matrix
   */
  void SetProjectionTransformMatrix(const double elements[16]);

protected:
  svtkExternalOpenGLCamera();
  ~svtkExternalOpenGLCamera() override {}

  /**
   * These methods should only be used within svtkCamera.cxx.
   * Bypass computation if user provided the view transform
   */
  void ComputeViewTransform() override;

private:
  bool UserProvidedViewTransform;

  svtkExternalOpenGLCamera(const svtkExternalOpenGLCamera&) = delete;
  void operator=(const svtkExternalOpenGLCamera&) = delete;
};

#endif
