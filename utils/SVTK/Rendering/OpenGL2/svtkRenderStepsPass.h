/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderStepsPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderStepsPass
 * @brief   Execute render passes sequentially.
 *
 * svtkRenderStepsPass executes a standard list of render passes sequentially.
 * This class allows to define a sequence of render passes at run time.
 * You can set a step to NULL in order to skip that step. Likewise you
 * can replace any of the default steps with your own step. Typically in
 * such a case you would get the current step, replace it with your own
 * and likely have your step call the current step as a delegate. For example
 * to replace the translucent step with a depthpeeling step you would get the
 * current tranlucent step and set it as a delegate on the depthpeeling step.
 * Then set this classes translparent step to the depthpeelnig step.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkRenderStepsPass_h
#define svtkRenderStepsPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkSequencePass;
class svtkCameraPass;

class SVTKRENDERINGOPENGL2_EXPORT svtkRenderStepsPass : public svtkRenderPass
{
public:
  static svtkRenderStepsPass* New();
  svtkTypeMacro(svtkRenderStepsPass, svtkRenderPass);
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
   * Get the RenderPass used for the Camera Step
   */
  svtkGetObjectMacro(CameraPass, svtkCameraPass);
  void SetCameraPass(svtkCameraPass*);
  //@}

  //@{
  /**
   * Get the RenderPass used for the Lights Step
   */
  svtkGetObjectMacro(LightsPass, svtkRenderPass);
  void SetLightsPass(svtkRenderPass*);
  //@}

  //@{
  /**
   * Get the RenderPass used for the Opaque Step
   */
  svtkGetObjectMacro(OpaquePass, svtkRenderPass);
  void SetOpaquePass(svtkRenderPass*);
  //@}

  //@{
  /**
   * Get the RenderPass used for the translucent Step
   */
  svtkGetObjectMacro(TranslucentPass, svtkRenderPass);
  void SetTranslucentPass(svtkRenderPass*);
  //@}

  //@{
  /**
   * Get the RenderPass used for the Volume Step
   */
  svtkGetObjectMacro(VolumetricPass, svtkRenderPass);
  void SetVolumetricPass(svtkRenderPass*);
  //@}

  //@{
  /**
   * Get the RenderPass used for the Overlay Step
   */
  svtkGetObjectMacro(OverlayPass, svtkRenderPass);
  void SetOverlayPass(svtkRenderPass*);
  //@}

  //@{
  /**
   * Get the RenderPass used for the PostProcess Step
   */
  svtkGetObjectMacro(PostProcessPass, svtkRenderPass);
  void SetPostProcessPass(svtkRenderPass*);
  //@}

protected:
  svtkRenderStepsPass();
  ~svtkRenderStepsPass() override;

  svtkCameraPass* CameraPass;
  svtkRenderPass* LightsPass;
  svtkRenderPass* OpaquePass;
  svtkRenderPass* TranslucentPass;
  svtkRenderPass* VolumetricPass;
  svtkRenderPass* OverlayPass;
  svtkRenderPass* PostProcessPass;
  svtkSequencePass* SequencePass;

private:
  svtkRenderStepsPass(const svtkRenderStepsPass&) = delete;
  void operator=(const svtkRenderStepsPass&) = delete;
};

#endif
