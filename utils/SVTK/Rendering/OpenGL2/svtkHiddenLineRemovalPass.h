/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHiddenLineRemovalPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkHiddenLineRemovalPass
 * @brief   RenderPass for HLR.
 *
 *
 * This render pass renders wireframe polydata such that only the front
 * wireframe surface is drawn.
 */

#ifndef svtkHiddenLineRemovalPass_h
#define svtkHiddenLineRemovalPass_h

#include "svtkOpenGLRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

#include <vector> // For std::vector!

class svtkProp;
class svtkViewport;

class SVTKRENDERINGOPENGL2_EXPORT svtkHiddenLineRemovalPass : public svtkOpenGLRenderPass
{
public:
  static svtkHiddenLineRemovalPass* New();
  svtkTypeMacro(svtkHiddenLineRemovalPass, svtkOpenGLRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Render(const svtkRenderState* s) override;

  /**
   * Returns true if any of the nProps in propArray are rendered as wireframe.
   */
  static bool WireframePropsExist(svtkProp** propArray, int nProps);

protected:
  svtkHiddenLineRemovalPass();
  ~svtkHiddenLineRemovalPass() override;

  void SetRepresentation(std::vector<svtkProp*>& props, int repr);
  int RenderProps(std::vector<svtkProp*>& props, svtkViewport* vp);

private:
  svtkHiddenLineRemovalPass(const svtkHiddenLineRemovalPass&) = delete;
  void operator=(const svtkHiddenLineRemovalPass&) = delete;
};

#endif // svtkHiddenLineRemovalPass_h
