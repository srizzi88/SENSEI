/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightsPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLightsPass
 * @brief   Implement the lights render pass.
 *
 * Render the lights.
 *
 * This pass expects an initialized camera.
 * It disables all the lights, apply transformations for lights following the
 * camera, and turn on the enables lights.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkLightsPass_h
#define svtkLightsPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkLightsPass : public svtkRenderPass
{
public:
  static svtkLightsPass* New();
  svtkTypeMacro(svtkLightsPass, svtkRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

protected:
  /**
   * Default constructor.
   */
  svtkLightsPass();

  /**
   * Destructor.
   */
  ~svtkLightsPass() override;

private:
  svtkLightsPass(const svtkLightsPass&) = delete;
  void operator=(const svtkLightsPass&) = delete;
};

#endif
