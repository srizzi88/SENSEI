/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSobelGradientMagnitudePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSobelGradientMagnitudePass
 * @brief   Implement a post-processing edge
 * detection with a Sobel gradient magnitude render pass.
 *
 * Detect the edges of the image renderered by its delegate. Edge-detection
 * uses a Sobel high-pass filter (3x3 kernel).
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
 * To compute the gradient magnitude, the x and y components of the gradient
 * (Gx and Gy) have to be computed first. Each computation of Gx and Gy uses
 * a separable filter.
 * The first pass takes the image from the delegate as the single input
 * texture.
 * The first pass has two outputs, one for the first part of Gx, Gx1, result of
 * a convolution with (-1 0 1), one for the first part of Gy, Gy1, result of a
 * convolution with (1 2 1).
 * The second pass has two inputs, Gx1 and Gy1. Kernel (1 2 1)^T is applied
 * to Gx1 and kernel (-1 0 1)^T is applied to Gx2. It gives the values for
 * Gx and Gy. Those values are then used to compute the magnitude of the
 * gradient which is stored in the render target.
 * The gradient computation happens per component (R,G,B). A is arbitrarily set
 * to 1 (full opacity).
 *
 * @par Implementation:
 * \image html svtkSobelGradientMagnitudePassFigure.png
 * \image latex svtkSobelGradientMagnitudePassFigure.eps
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkSobelGradientMagnitudePass_h
#define svtkSobelGradientMagnitudePass_h

#include "svtkImageProcessingPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkDepthPeelingPassLayerList; // Pimpl
class svtkOpenGLFramebufferObject;
class svtkOpenGLHelper;
class svtkOpenGLRenderWindow;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkSobelGradientMagnitudePass : public svtkImageProcessingPass
{
public:
  static svtkSobelGradientMagnitudePass* New();
  svtkTypeMacro(svtkSobelGradientMagnitudePass, svtkImageProcessingPass);
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
  svtkSobelGradientMagnitudePass();

  /**
   * Destructor.
   */
  ~svtkSobelGradientMagnitudePass() override;

  /**
   * Graphics resources.
   */
  svtkOpenGLFramebufferObject* FrameBufferObject;
  svtkTextureObject* Pass1; // render target for the scene
  svtkTextureObject* Gx1;   // render target 0 for the first shader
  svtkTextureObject* Gy1;   // render target 1 for the first shader

  // Structures for the various cell types we render.
  svtkOpenGLHelper* Program1; // shader to compute Gx1 and Gy1
  svtkOpenGLHelper* Program2; // shader to compute |G| from Gx1 and Gy1

private:
  svtkSobelGradientMagnitudePass(const svtkSobelGradientMagnitudePass&) = delete;
  void operator=(const svtkSobelGradientMagnitudePass&) = delete;
};

#endif
