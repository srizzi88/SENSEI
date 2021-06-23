/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFramebufferPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFramebufferPass
 * @brief   Render into a FO
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkFramebufferPass_h
#define svtkFramebufferPass_h

#include "svtkDepthImageProcessingPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLFramebufferObject;
class svtkOpenGLHelper;
class svtkOpenGLRenderWindow;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkFramebufferPass : public svtkDepthImageProcessingPass
{
public:
  static svtkFramebufferPass* New();
  svtkTypeMacro(svtkFramebufferPass, svtkDepthImageProcessingPass);
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
  svtkFramebufferPass();

  /**
   * Destructor.
   */
  ~svtkFramebufferPass() override;

  /**
   * Graphics resources.
   */
  svtkOpenGLFramebufferObject* FrameBufferObject;
  svtkTextureObject* ColorTexture; // render target for the scene
  svtkTextureObject* DepthTexture; // render target for the depth

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

private:
  svtkFramebufferPass(const svtkFramebufferPass&) = delete;
  void operator=(const svtkFramebufferPass&) = delete;
};

#endif
