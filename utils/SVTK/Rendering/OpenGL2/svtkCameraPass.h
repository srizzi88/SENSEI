/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCameraPass
 * @brief   Implement the camera render pass.
 *
 * Render the camera.
 *
 * It setups the projection and modelview matrices and can clear the background
 * It calls its delegate once.
 * After its delegate returns, it restore the modelview matrix stack.
 *
 * Its delegate is usually set to a svtkSequencePass with a svtkLigthsPass and
 * a list of passes for the geometry.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkCameraPass_h
#define svtkCameraPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkCameraPass : public svtkRenderPass
{
public:
  static svtkCameraPass* New();
  svtkTypeMacro(svtkCameraPass, svtkRenderPass);
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
   * Delegate for rendering the geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a svtkSequencePass with a svtkLigthsPass and
   * a list of passes for the geometry.
   * Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(DelegatePass, svtkRenderPass);
  virtual void SetDelegatePass(svtkRenderPass* delegatePass);
  //@}

  //@{
  /**
   * Used to override the aspect ratio used when computing the projection
   * matrix. This is useful when rendering for tile-displays for example.
   */
  svtkSetMacro(AspectRatioOverride, double);
  svtkGetMacro(AspectRatioOverride, double);

protected:
  //@}
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  svtkCameraPass();

  //@{
  /**
   * Destructor.
   */
  ~svtkCameraPass() override;
  virtual void GetTiledSizeAndOrigin(
    const svtkRenderState* render_state, int* width, int* height, int* originX, int* originY);
  //@}

  svtkRenderPass* DelegatePass;

  double AspectRatioOverride;

private:
  svtkCameraPass(const svtkCameraPass&) = delete;
  void operator=(const svtkCameraPass&) = delete;
};

#endif
