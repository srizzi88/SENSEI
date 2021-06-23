/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLImageAlgorithmHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLImageAlgorithmHelper
 * @brief   Help image algorithms use the GPU
 *
 * Designed to make it easier to accelerate an image algorithm on the GPU
 */

#ifndef svtkOpenGLImageAlgorithmHelper_h
#define svtkOpenGLImageAlgorithmHelper_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

#include "svtkOpenGLHelper.h" // used for ivars
#include "svtkSmartPointer.h" // for ivar

class svtkOpenGLRenderWindow;
class svtkRenderWindow;
class svtkImageData;
class svtkDataArray;

class svtkOpenGLImageAlgorithmCallback
{
public:
  virtual void InitializeShaderUniforms(svtkShaderProgram* /* program */) {}
  virtual void UpdateShaderUniforms(svtkShaderProgram* /* program */, int /* zExtent */) {}
  virtual ~svtkOpenGLImageAlgorithmCallback() {}
  svtkOpenGLImageAlgorithmCallback() {}

private:
  svtkOpenGLImageAlgorithmCallback(const svtkOpenGLImageAlgorithmCallback&) = delete;
  void operator=(const svtkOpenGLImageAlgorithmCallback&) = delete;
};

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLImageAlgorithmHelper : public svtkObject
{
public:
  static svtkOpenGLImageAlgorithmHelper* New();
  svtkTypeMacro(svtkOpenGLImageAlgorithmHelper, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Execute(svtkOpenGLImageAlgorithmCallback* cb, svtkImageData* inImage, svtkDataArray* inData,
    svtkImageData* outData, int outExt[6], const char* vertexCode, const char* fragmentCode,
    const char* geometryCode);

  /**
   * Set the render window to get the OpenGL resources from
   */
  void SetRenderWindow(svtkRenderWindow* renWin);

protected:
  svtkOpenGLImageAlgorithmHelper();
  ~svtkOpenGLImageAlgorithmHelper() override;

  svtkSmartPointer<svtkOpenGLRenderWindow> RenderWindow;
  svtkOpenGLHelper Quad;

private:
  svtkOpenGLImageAlgorithmHelper(const svtkOpenGLImageAlgorithmHelper&) = delete;
  void operator=(const svtkOpenGLImageAlgorithmHelper&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkOpenGLImageAlgorithmHelper.h
