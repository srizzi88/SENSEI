/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleMotionBlurPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSimpleMotionBlurPass
 * @brief   Avergae frames to simulate motion blur.
 *
 * A slow and simple approach that simply renders multiple frames
 * and accumulates them before displaying them. As such it causes the
 * render process to be SubFrames times slower than normal but handles
 * all types of motion correctly as it is actually rendering all the
 * sub frames.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkSimpleMotionBlurPass_h
#define svtkSimpleMotionBlurPass_h

#include "svtkDepthImageProcessingPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLFramebufferObject;
class svtkOpenGLHelper;
class svtkOpenGLRenderWindow;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkSimpleMotionBlurPass : public svtkDepthImageProcessingPass
{
public:
  static svtkSimpleMotionBlurPass* New();
  svtkTypeMacro(svtkSimpleMotionBlurPass, svtkDepthImageProcessingPass);
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

  //@{
  /**
   * Set the number of sub frames for doing motion blur.
   * Once this is set greater than one, you will no longer see a new frame
   * for every Render().  If you set this to five, you will need to do
   * five Render() invocations before seeing the result. This isn't
   * very impressive unless something is changing between the Renders.
   * Changing this value may reset the current subframe count.
   */
  svtkGetMacro(SubFrames, int);
  virtual void SetSubFrames(int subFrames);
  //@}

  /**
   *  Set the format to use for the depth texture
   * e.g. svtkTextureObject::Float32
   */
  svtkSetMacro(DepthFormat, int);

  /**
   *  Set the format to use for the color texture
   *  svtkTextureObject::Float16 svtkTextureObject::Float32
   *  and svtkTextureObject::Fixed8 are supported. Fixed8
   *  is the default.
   */
  svtkSetMacro(ColorFormat, int);

  // Get the depth texture object
  svtkGetObjectMacro(DepthTexture, svtkTextureObject);

  // Get the Color texture object
  svtkGetObjectMacro(ColorTexture, svtkTextureObject);

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  svtkSimpleMotionBlurPass();

  /**
   * Destructor.
   */
  ~svtkSimpleMotionBlurPass() override;

  /**
   * Graphics resources.
   */
  svtkOpenGLFramebufferObject* FrameBufferObject;
  svtkTextureObject* ColorTexture;           // render target for the scene
  svtkTextureObject* AccumulationTexture[2]; // where we add the colors
  svtkTextureObject* DepthTexture;           // render target for the depth

  //@{
  /**
   * Cache viewport values for depth peeling.
   */
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  //@}

  int DepthFormat;
  int ColorFormat;

  int SubFrames;       // number of sub frames
  int CurrentSubFrame; // what one are we on
  int ActiveAccumulationTexture;
  svtkOpenGLHelper* BlendProgram;

private:
  svtkSimpleMotionBlurPass(const svtkSimpleMotionBlurPass&) = delete;
  void operator=(const svtkSimpleMotionBlurPass&) = delete;
};

#endif
