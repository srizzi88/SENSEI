/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderPass
 * @brief   Perform part of the rendering of a svtkRenderer.
 *
 * svtkRenderPass is a deferred class with a simple deferred method Render.
 * This method performs a rendering pass of the scene described in
 * svtkRenderState.
 * Subclasses define what really happens during rendering.
 *
 * Directions to write a subclass of svtkRenderPass:
 * It is up to the subclass to decide if it needs to delegate part of its job
 * to some other svtkRenderPass objects ("delegates").
 * - The subclass has to define ivar to set/get its delegates.
 * - The documentation of the subclass has to describe:
 *  - what each delegate is supposed to perform
 *  - if a delegate is supposed to be used once or multiple times
 *  - what it expects to have in the framebuffer before starting (status
 * of colorbuffers, depth buffer, stencil buffer)
 *  - what it will change in the framebuffer.
 * - A pass cannot modify the svtkRenderState where it will perform but
 * it can build a new svtkRenderState (it can change the FrameBuffer, change the
 * prop array, changed the required prop properties keys (usually adding some
 * to a copy of the existing list) but it has to keep the same svtkRenderer
 * object), make it current and pass it to its delegate.
 * - at the end of the execution of Render, the pass has to ensure the
 * current svtkRenderState is the one it has in argument.
 * @sa
 * svtkRenderState svtkRenderer
 */

#ifndef svtkRenderPass_h
#define svtkRenderPass_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkFrameBufferObjectBase;
class svtkRenderState;
class svtkWindow;
class svtkRenderer;

class SVTKRENDERINGCORE_EXPORT svtkRenderPass : public svtkObject
{
public:
  svtkTypeMacro(svtkRenderPass, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * It modifies NumberOfRenderedProps.
   * \pre s_exists: s!=0
   */
  virtual void Render(const svtkRenderState* s) = 0;

  //@{
  /**
   * Number of props rendered at the last Render call.
   */
  svtkGetMacro(NumberOfRenderedProps, int);
  //@}

  /**
   * Release graphics resources and ask components to release their own
   * resources. Default implementation is empty.
   * \pre w_exists: w!=0
   */
  virtual void ReleaseGraphicsResources(svtkWindow* w);

protected:
  /**
   * Default constructor. Do nothing.
   */
  svtkRenderPass();

  /**
   * Destructor. Do nothing.
   */
  ~svtkRenderPass() override;

  /**
   * Call UpdateCamera() on Renderer. This ugly mechanism gives access to
   * a protected method of Renderer to subclasses of svtkRenderPass.
   * \pre renderer_exists: renderer!=0
   */
  void UpdateCamera(svtkRenderer* renderer);

  /**
   * Call ClearLights() on Renderer. See note about UpdateCamera().
   * \pre renderer_exists: renderer!=0
   */
  void ClearLights(svtkRenderer* renderer);

  /**
   * Call UpdateLightGeometry() on Renderer. See note about UpdateCamera().
   * \pre renderer_exists: renderer!=0
   */
  void UpdateLightGeometry(svtkRenderer* renderer);

  /**
   * Call UpdateLights() on Renderer. See note about UpdateCamera().
   * \pre renderer_exists: renderer!=0
   */
  void UpdateLights(svtkRenderer* renderer);

  /**
   * Call UpdateGeometry() on Renderer. See note about UpdateCamera().
   * \pre renderer_exists: renderer!=0
   */
  void UpdateGeometry(svtkRenderer* renderer, svtkFrameBufferObjectBase* fbo = nullptr);

  /**
   * Modify protected member LastRenderingUsedDepthPeeling on Renderer.
   * See note about UpdateCamera().
   * \pre renderer_exists: renderer!=0
   * Note - OpenGL1 specific, remove when completely switched to OpenGL2
   */
  void SetLastRenderingUsedDepthPeeling(svtkRenderer* renderer, bool value);

  int NumberOfRenderedProps;

private:
  svtkRenderPass(const svtkRenderPass&) = delete;
  void operator=(const svtkRenderPass&) = delete;
};

#endif
