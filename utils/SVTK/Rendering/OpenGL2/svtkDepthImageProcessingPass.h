/*=========================================================================

   Program: ParaView
   Module:    svtkDepthImageProcessingPass.h

  Copyright (c) Sandia Corporation, Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------
Acknowledgement:
This algorithm is the result of joint work by Electricité de France,
CNRS, Collège de France and Université J. Fourier as part of the
Ph.D. thesis of Christian BOUCHENY.
------------------------------------------------------------------------*/
/**
 * @class   svtkDepthImageProcessingPass
 * @brief   Convenient class for post-processing
 * passes. Based on svtkImageProcessingPass, but writes depth as well
 * in a texture
 *
 * Abstract class with some convenient methods frequently used in subclasses.
 *
 *
 * @sa
 * svtkRenderPass svtkDepthImageProcessingPass svtkEDLShading
 */

#ifndef svtkDepthImageProcessingPass_h
#define svtkDepthImageProcessingPass_h

#include "svtkImageProcessingPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderWindow;
class svtkDepthPeelingPassLayerList; // Pimpl
class svtkOpenGLFramebufferObject;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkDepthImageProcessingPass : public svtkImageProcessingPass
{
public:
  svtkTypeMacro(svtkDepthImageProcessingPass, svtkImageProcessingPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  svtkDepthImageProcessingPass();

  /**
   * Destructor.
   */
  ~svtkDepthImageProcessingPass() override;

  /**
   * Render delegate with a image of different dimensions than the
   * original one.
   * \pre s_exists: s!=0
   * \pre fbo_exists: fbo!=0
   * \pre fbo_has_context: fbo->GetContext()!=0
   * \pre colortarget_exists: colortarget!=0
   * \pre colortarget_has_context: colortarget->GetContext()!=0
   */
  virtual void RenderDelegate(const svtkRenderState* s, int width, int height, int newWidth,
    int newHeight, svtkOpenGLFramebufferObject* fbo, svtkTextureObject* colortarget,
    svtkTextureObject* depthtarget);

  /**
   * Read parent size - for sake of code clarity
   * This function is generic, can be useful in multiple image-based rendering classes
   * \pre s_exists: s!=0
   */
  void ReadWindowSize(const svtkRenderState* s);

  int Origin[2];   // Viewport origin
  int Width;       // parent window width
  int Height;      // parent window height
  int W;           // this width
  int H;           // this height
  int ExtraPixels; // w(h) = width(height) + 2*extrapixels

private:
  svtkDepthImageProcessingPass(const svtkDepthImageProcessingPass&) = delete;
  void operator=(const svtkDepthImageProcessingPass&) = delete;
};

#endif
