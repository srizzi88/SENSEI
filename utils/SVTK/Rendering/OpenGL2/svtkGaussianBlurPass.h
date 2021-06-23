/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGaussianBlurPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGaussianBlurPass
 * @brief   Implement a post-processing Gaussian blur
 * render pass.
 *
 * Blur the image renderered by its delegate. Blurring uses a Gaussian low-pass
 * filter with a 5x5 kernel.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 * An opaque pass may have been performed right after the initialization.
 *
 * The delegate is used once.
 *
 * Its delegate is usually set to a svtkCameraPass or to a post-processing pass.
 *
 * This pass requires a OpenGL context that supports texture objects (TO),
 * framebuffer objects (FBO) and GLSL. If not, it will emit an error message
 * and will render its delegate and return.
 *
 * @par Implementation:
 * As the filter is separable, it first blurs the image horizontally and then
 * vertically. This reduces the number of texture sampling to 5 per pass.
 * In addition, as texture sampling can already blend texel values in linear
 * mode, by adjusting the texture coordinate accordingly, only 3 texture
 * sampling are actually necessary.
 * Reference: OpenGL Bloom Toturial by Philip Rideout, section
 * Exploit Hardware Filtering  http://prideout.net/bloom/index.php#Sneaky
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkGaussianBlurPass_h
#define svtkGaussianBlurPass_h

#include "svtkImageProcessingPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkDepthPeelingPassLayerList; // Pimpl
class svtkOpenGLFramebufferObject;
class svtkOpenGLHelper;
class svtkOpenGLRenderWindow;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkGaussianBlurPass : public svtkImageProcessingPass
{
public:
  static svtkGaussianBlurPass* New();
  svtkTypeMacro(svtkGaussianBlurPass, svtkImageProcessingPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  svtkGaussianBlurPass();

  /**
   * Destructor.
   */
  ~svtkGaussianBlurPass() override;

  /**
   * Graphics resources.
   */
  svtkOpenGLFramebufferObject* FrameBufferObject;
  svtkTextureObject* Pass1; // render target for the scene
  svtkTextureObject* Pass2; // render target for the horizontal pass

  // Structures for the various cell types we render.
  svtkOpenGLHelper* BlurProgram;

private:
  svtkGaussianBlurPass(const svtkGaussianBlurPass&) = delete;
  void operator=(const svtkGaussianBlurPass&) = delete;
};

#endif
