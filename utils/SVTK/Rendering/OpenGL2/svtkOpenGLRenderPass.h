/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRenderPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLRenderPass
 * @brief   Abstract render pass with shader modifications.
 *
 *
 * Allows a render pass to update shader code using a new virtual API.
 */

#ifndef svtkOpenGLRenderPass_h
#define svtkOpenGLRenderPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

#include <string> // For std::string

class svtkAbstractMapper;
class svtkInformationObjectBaseVectorKey;
class svtkProp;
class svtkShaderProgram;
class svtkOpenGLVertexArrayObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLRenderPass : public svtkRenderPass
{
public:
  svtkTypeMacro(svtkOpenGLRenderPass, svtkRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Use svtkShaderProgram::Substitute to replace //SVTK::XXX:YYY declarations in
   * the shader sources. Gets called before other mapper shader replacements
   * Return false on error.
   */
  virtual bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop);

  /**
   * Use svtkShaderProgram::Substitute to replace //SVTK::XXX:YYY declarations in
   * the shader sources. Gets called after other mapper shader replacements.
   * Return false on error.
   */
  virtual bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop);

  /**
   * Update the uniforms of the shader program.
   * Return false on error.
   */
  virtual bool SetShaderParameters(svtkShaderProgram* program, svtkAbstractMapper* mapper,
    svtkProp* prop, svtkOpenGLVertexArrayObject* VAO = nullptr);

  /**
   * For multi-stage render passes that need to change shader code during a
   * single pass, use this method to notify a mapper that the shader needs to be
   * rebuilt (rather than reuse the last cached shader. This method should
   * return the last time that the shader stage changed, or 0 if the shader
   * is single-stage.
   */
  virtual svtkMTimeType GetShaderStageMTime();

  /**
   * Key containing information about the current pass.
   */
  static svtkInformationObjectBaseVectorKey* RenderPasses();

  /**
   * Number of active draw buffers.
   */
  svtkSetMacro(ActiveDrawBuffers, unsigned int);
  svtkGetMacro(ActiveDrawBuffers, unsigned int);

protected:
  svtkOpenGLRenderPass();
  ~svtkOpenGLRenderPass() override;

  /**
   * Call before rendering to update the actors' information keys.
   */
  void PreRender(const svtkRenderState* s);

  /**
   * Call after rendering to clean up the actors' information keys.
   */
  void PostRender(const svtkRenderState* s);

  unsigned int ActiveDrawBuffers = 0;

private:
  svtkOpenGLRenderPass(const svtkOpenGLRenderPass&) = delete;
  void operator=(const svtkOpenGLRenderPass&) = delete;
};

#endif // svtkOpenGLRenderPass_h
