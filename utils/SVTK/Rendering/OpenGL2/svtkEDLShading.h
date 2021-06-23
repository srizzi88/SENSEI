/*=========================================================================

   Program: SVTK
   Module:  svtkEDLShading.h

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
 * @class   svtkEDLShading
 *
 *
 * Implement an EDL offscreen shading.
 * Shade the image renderered by its delegate. Two image resolutions are used
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
 */

#ifndef svtkEDLShading_h
#define svtkEDLShading_h

#define EDL_HIGH_RESOLUTION_ON 1
#define EDL_LOW_RESOLUTION_ON 1

#include "svtkDepthImageProcessingPass.h"
#include "svtkOpenGLHelper.h"           // used for ivars
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkSmartPointer.h"           // needed for svtkSmartPointer

class svtkOpenGLRenderWindow;
class svtkOpenGLFramebufferObject;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkEDLShading : public svtkDepthImageProcessingPass
{
public:
  static svtkEDLShading* New();
  svtkTypeMacro(svtkEDLShading, svtkDepthImageProcessingPass);
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
  svtkEDLShading();

  /**
   * Destructor.
   */
  ~svtkEDLShading() override;

  /**
   * Initialization of required framebuffer objects
   */
  void EDLInitializeFramebuffers(svtkRenderState& s);

  /**
   * Initialization of required GLSL shaders
   */
  void EDLInitializeShaders(svtkOpenGLRenderWindow*);

  /**
   * Render EDL in full resolution buffer
   */
  bool EDLShadeHigh(svtkRenderState& s, svtkOpenGLRenderWindow*);

  /**
   * Render EDL in middle resolution buffer
   */
  bool EDLShadeLow(svtkRenderState& s, svtkOpenGLRenderWindow*);

  /**
   * Render EDL in middle resolution buffer
   */
  bool EDLBlurLow(svtkRenderState& s, svtkOpenGLRenderWindow*);

  /**
   * Compose color and shaded images
   */
  bool EDLCompose(const svtkRenderState* s, svtkOpenGLRenderWindow*);

  //@{
  /**
   * Framebuffer object and textures for initial projection
   */
  svtkOpenGLFramebufferObject* ProjectionFBO;
  // used to record scene data
  svtkTextureObject* ProjectionColorTexture;
  // color render target for projection pass
  svtkTextureObject* ProjectionDepthTexture;
  // depth render target for projection pass
  //@}

  // Framebuffer objects and textures for EDL
  svtkOpenGLFramebufferObject* EDLHighFBO;
  // for EDL full res shading
  svtkTextureObject* EDLHighShadeTexture;
  // color render target for EDL full res pass
  svtkOpenGLFramebufferObject* EDLLowFBO;
  // for EDL low res shading (image size/4)
  svtkTextureObject* EDLLowShadeTexture;
  // color render target for EDL low res pass
  svtkTextureObject* EDLLowBlurTexture;
  // color render target for EDL low res
  // bilateral filter pass

  // Shader prohrams
  svtkOpenGLHelper EDLShadeProgram;
  svtkOpenGLHelper EDLComposeProgram;
  svtkOpenGLHelper BilateralProgram;

  float EDLNeighbours[8][4];
  bool EDLIsFiltered;
  int EDLLowResFactor; // basically 4

  float Zn; // near clipping plane
  float Zf; // far clipping plane

private:
  svtkEDLShading(const svtkEDLShading&) = delete;
  void operator=(const svtkEDLShading&) = delete;
};

#endif
