/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLFXAAPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLFXAAPass
 * @brief   Render pass calling the FXAA filter
 *
 * svtkOpenGLFXAAPass is an image post processing render pass. It is a fast anti aliasing
 * filter.
 *
 * This pass usually takes the camera pass as its delegate pass.
 *
 * @note Currently, this pass wraps the existing FXAA implementation. It copies the pixels
 * from the framebuffer to a texture. A better approach would be to use the usual render pass
 * workflow to create a framebuffer drawing directly on the texture.
 *
 * @sa
 * svtkRenderPass svtkDefaultPass
 */

#ifndef svtkOpenGLFXAAPass_h
#define svtkOpenGLFXAAPass_h

#include "svtkImageProcessingPass.h"

#include "svtkNew.h"                    // For svtkNew
#include "svtkOpenGLFXAAFilter.h"       // For svtkOpenGLFXAAFilter
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLFXAAPass : public svtkImageProcessingPass
{
public:
  static svtkOpenGLFXAAPass* New();
  svtkTypeMacro(svtkOpenGLFXAAPass, svtkImageProcessingPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const svtkRenderState* s) override;

protected:
  svtkOpenGLFXAAPass() = default;
  ~svtkOpenGLFXAAPass() override = default;

  /**
   * Graphics resources.
   */
  svtkNew<svtkOpenGLFXAAFilter> FXAAFilter;

private:
  svtkOpenGLFXAAPass(const svtkOpenGLFXAAPass&) = delete;
  void operator=(const svtkOpenGLFXAAPass&) = delete;
};

#endif
