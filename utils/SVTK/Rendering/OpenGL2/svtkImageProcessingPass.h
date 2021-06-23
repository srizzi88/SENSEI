/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageProcessingPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageProcessingPass
 * @brief   Convenient class for post-processing passes.
 * render pass.
 *
 * Abstract class with some convenient methods frequently used in subclasses.
 *
 *
 * @sa
 * svtkOpenGLRenderPass svtkGaussianBlurPass svtkSobelGradientMagnitudePass
 */

#ifndef svtkImageProcessingPass_h
#define svtkImageProcessingPass_h

#include "svtkOpenGLRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderWindow;
class svtkDepthPeelingPassLayerList; // Pimpl
class svtkOpenGLFramebufferObject;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkImageProcessingPass : public svtkOpenGLRenderPass
{
public:
  svtkTypeMacro(svtkImageProcessingPass, svtkOpenGLRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;

  //@{
  /**
   * Delegate for rendering the image to be processed.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a svtkCameraPass or to a post-processing pass.
   * Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(DelegatePass, svtkRenderPass);
  virtual void SetDelegatePass(svtkRenderPass* delegatePass);
  //@}

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  svtkImageProcessingPass();

  /**
   * Destructor.
   */
  ~svtkImageProcessingPass() override;

  /**
   * Render delegate with a image of different dimensions than the
   * original one.
   * \pre s_exists: s!=0
   * \pre fbo_exists: fbo!=0
   * \pre fbo_has_context: fbo->GetContext()!=0
   * \pre target_exists: target!=0
   * \pre target_has_context: target->GetContext()!=0
   */
  void RenderDelegate(const svtkRenderState* s, int width, int height, int newWidth, int newHeight,
    svtkOpenGLFramebufferObject* fbo, svtkTextureObject* target);

  svtkRenderPass* DelegatePass;

private:
  svtkImageProcessingPass(const svtkImageProcessingPass&) = delete;
  void operator=(const svtkImageProcessingPass&) = delete;
};

#endif
