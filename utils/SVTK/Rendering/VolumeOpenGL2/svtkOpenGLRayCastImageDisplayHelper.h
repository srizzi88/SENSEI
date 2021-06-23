/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRayCastImageDisplayHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLRayCastImageDisplayHelper
 * @brief   OpenGL subclass that draws the image to the screen
 *
 * This is the concrete implementation of a ray cast image display helper -
 * a helper class responsible for drawing the image to the screen.
 *
 * @sa
 * svtkRayCastImageDisplayHelper
 */

#ifndef svtkOpenGLRayCastImageDisplayHelper_h
#define svtkOpenGLRayCastImageDisplayHelper_h

#include "svtkRayCastImageDisplayHelper.h"
#include "svtkRenderingVolumeOpenGL2Module.h" // For export macro

class svtkFixedPointRayCastImage;
class svtkOpenGLHelper;
class svtkRenderer;
class svtkTextureObject;
class svtkVolume;
class svtkWindow;

class SVTKRENDERINGVOLUMEOPENGL2_EXPORT svtkOpenGLRayCastImageDisplayHelper
  : public svtkRayCastImageDisplayHelper
{
public:
  static svtkOpenGLRayCastImageDisplayHelper* New();
  svtkTypeMacro(svtkOpenGLRayCastImageDisplayHelper, svtkRayCastImageDisplayHelper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void RenderTexture(svtkVolume* vol, svtkRenderer* ren, int imageMemorySize[2],
    int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2], float requestedDepth,
    unsigned char* image) override;

  void RenderTexture(svtkVolume* vol, svtkRenderer* ren, int imageMemorySize[2],
    int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2], float requestedDepth,
    unsigned short* image) override;

  void RenderTexture(svtkVolume* vol, svtkRenderer* ren, svtkFixedPointRayCastImage* image,
    float requestedDepth) override;

  void ReleaseGraphicsResources(svtkWindow* win) override;

protected:
  svtkOpenGLRayCastImageDisplayHelper();
  ~svtkOpenGLRayCastImageDisplayHelper() override;

  void RenderTextureInternal(svtkVolume* vol, svtkRenderer* ren, int imageMemorySize[2],
    int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2], float requestedDepth,
    int imageScalarType, void* image);

  // used for copying to framebuffer
  svtkTextureObject* TextureObject;
  svtkOpenGLHelper* ShaderProgram;

private:
  svtkOpenGLRayCastImageDisplayHelper(const svtkOpenGLRayCastImageDisplayHelper&) = delete;
  void operator=(const svtkOpenGLRayCastImageDisplayHelper&) = delete;
};

#endif
