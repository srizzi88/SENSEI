/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClientServerCompositePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClientServerCompositePass
 *
 * svtkClientServerCompositePass is a render-pass that can handle client-server
 * image delivery. This is designed to be used in configurations in
 * two-processes configurations.
 */

#ifndef svtkClientServerCompositePass_h
#define svtkClientServerCompositePass_h

#include "svtkRenderPass.h"
#include "svtkRenderingParallelModule.h" // For export macro

class svtkMultiProcessController;

class SVTKRENDERINGPARALLEL_EXPORT svtkClientServerCompositePass : public svtkRenderPass
{
public:
  static svtkClientServerCompositePass* New();
  svtkTypeMacro(svtkClientServerCompositePass, svtkRenderPass);
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
   * Controller
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * Initial value is a NULL pointer.
   * This must be set to the socket controller used for communicating between
   * the client and the server.
   */
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  virtual void SetController(svtkMultiProcessController* controller);
  //@}

  //@{
  /**
   * Get/Set the render pass used to do the actual rendering.
   * When ServerSideRendering is true, the rendering-pass is called only on the
   * server side.
   */
  void SetRenderPass(svtkRenderPass*);
  svtkGetObjectMacro(RenderPass, svtkRenderPass);
  //@}

  //@{
  /**
   * Set/Get the optional post-fetch render pass.
   * On the client-process this is called after the server-side image is fetched
   * (if ServerSideRendering is true). On server-process, this is called after the
   * image rendered by this->RenderPass is delivered to the client (if
   * ServerSideRendering is true). This is optional, so you can set this either on
   * one of the two processes or both or neither.
   */
  void SetPostProcessingRenderPass(svtkRenderPass*);
  svtkGetObjectMacro(PostProcessingRenderPass, svtkRenderPass);
  //@}

  //@{
  /**
   * Set the current process type. This is needed since when using the socket
   * communicator there's no easy way of determining which process is the server
   * and which one is the client.
   */
  svtkSetMacro(ProcessIsServer, bool);
  svtkBooleanMacro(ProcessIsServer, bool);
  svtkGetMacro(ProcessIsServer, bool);
  //@}

  //@{
  /**
   * Enable/Disable fetching of the image from the server side to the client. If
   * this flag is disabled, then this pass just acts as a "pass-through" pass.
   * This flag must be set to the same value on both the processes.
   */
  svtkSetMacro(ServerSideRendering, bool);
  svtkBooleanMacro(ServerSideRendering, bool);
  svtkGetMacro(ServerSideRendering, bool);
  //@}

protected:
  svtkClientServerCompositePass();
  ~svtkClientServerCompositePass() override;

  svtkRenderPass* RenderPass;
  svtkRenderPass* PostProcessingRenderPass;
  svtkMultiProcessController* Controller;

  bool ProcessIsServer;
  bool ServerSideRendering;

private:
  svtkClientServerCompositePass(const svtkClientServerCompositePass&) = delete;
  void operator=(const svtkClientServerCompositePass&) = delete;
};

#endif
