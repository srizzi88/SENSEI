/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSSAAPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSSAAPass
 * @brief   Implement Screen Space Anti Aliasing pass.
 *
 * Render to a larger image and then sample down
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * The delegate is used once.
 *
 * Its delegate is usually set to a svtkCameraPass or to a post-processing pass.
 *
 * @par Implementation:
 * As the filter is separable, it first blurs the image horizontally and then
 * vertically. This reduces the number of texture samples taken.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkSSAAPass_h
#define svtkSSAAPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLFramebufferObject;
class svtkOpenGLHelper;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkSSAAPass : public svtkRenderPass
{
public:
  static svtkSSAAPass* New();
  svtkTypeMacro(svtkSSAAPass, svtkRenderPass);
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
  svtkSSAAPass();

  /**
   * Destructor.
   */
  ~svtkSSAAPass() override;

  /**
   * Graphics resources.
   */
  svtkOpenGLFramebufferObject* FrameBufferObject;
  svtkTextureObject* Pass1; // render target for the scene
  svtkTextureObject* Pass2; // render target for the horizontal pass

  // Structures for the various cell types we render.
  svtkOpenGLHelper* SSAAProgram;

  svtkRenderPass* DelegatePass;

private:
  svtkSSAAPass(const svtkSSAAPass&) = delete;
  void operator=(const svtkSSAAPass&) = delete;
};

#endif
