/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOrderIndependentTranslucentPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOrderIndependentTranslucentPass
 * @brief   Implement OIT rendering using average color
 *
 * Simple version that uses average alpha weighted color
 * and correct final computed alpha. Single pass approach.
 *
 * @sa
 * svtkRenderPass, svtkTranslucentPass, svtkFramebufferPass
 */

#ifndef svtkOrderIndependentTranslucentPass_h
#define svtkOrderIndependentTranslucentPass_h

#include "svtkOpenGLRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLFramebufferObject;
class svtkTextureObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLState;
class svtkOpenGLQuadHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkOrderIndependentTranslucentPass : public svtkOpenGLRenderPass
{
public:
  static svtkOrderIndependentTranslucentPass* New();
  svtkTypeMacro(svtkOrderIndependentTranslucentPass, svtkOpenGLRenderPass);
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
   * Delegate for rendering the translucent polygonal geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a svtkTranslucentPass.
   * Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(TranslucentPass, svtkRenderPass);
  virtual void SetTranslucentPass(svtkRenderPass* translucentPass);
  //@}

  // svtkOpenGLRenderPass virtuals:
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop) override;

protected:
  /**
   * Default constructor. TranslucentPass is set to NULL.
   */
  svtkOrderIndependentTranslucentPass();

  /**
   * Destructor.
   */
  ~svtkOrderIndependentTranslucentPass() override;

  svtkRenderPass* TranslucentPass;

  //@{
  /**
   * Cache viewport values for depth peeling.
   */
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  //@}

  svtkOpenGLFramebufferObject* Framebuffer;
  svtkOpenGLQuadHelper* FinalBlend;

  svtkTextureObject* TranslucentRGBATexture;
  svtkTextureObject* TranslucentRTexture;
  svtkTextureObject* TranslucentZTexture;

  void BlendFinalPeel(svtkOpenGLRenderWindow* renWin);

  // useful to store
  svtkOpenGLState* State;

private:
  svtkOrderIndependentTranslucentPass(const svtkOrderIndependentTranslucentPass&) = delete;
  void operator=(const svtkOrderIndependentTranslucentPass&) = delete;
};

#endif
