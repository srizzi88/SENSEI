/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRenderUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLRenderUtilities
 * @brief   OpenGL rendering utility functions
 *
 * svtkOpenGLRenderUtilities provides functions to help render primitives.
 *
 * See also the svtkOpenGLQuadHelper class which may be easier to use.
 *
 */

#ifndef svtkOpenGLRenderUtilities_h
#define svtkOpenGLRenderUtilities_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

#include "svtk_glew.h" // Needed for GLuint.
#include <string>     // for std::string

class svtkOpenGLBufferObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLVertexArrayObject;
class svtkShaderProgram;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLRenderUtilities : public svtkObject
{
public:
  svtkTypeMacro(svtkOpenGLRenderUtilities, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Helper function that draws a quad on the screen
   * at the specified vertex coordinates and if
   * tcoords are not NULL with the specified
   * texture coordinates.
   */
  static void RenderQuad(
    float* verts, float* tcoords, svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao);
  static void RenderTriangles(float* verts, unsigned int numVerts, GLuint* indices,
    unsigned int numIndices, float* tcoords, svtkShaderProgram* program,
    svtkOpenGLVertexArrayObject* vao);
  //@}

  //@{
  /**
   * Draw a full-screen quad:
   *
   * * VertexShader and GeometryShader should be used as-is when building the
   * ShaderProgram.
   * * FragmentShaderTemplate supports the replacements //SVTK::FSQ::Decl and
   * //SVTK::FSQ::Impl for declaring variables and the shader body,
   * respectively.
   * * The varying texCoord is available to the fragment shader for texture
   * lookups into full-screen textures, ie. texture2D(textureName, texCoord).
   * * PrepFullScreenVAO initializes a new VAO for drawing a quad.
   * * DrawFullScreenQuad actually draws the quad.

   * Example usage:
   * @code
   * typedef svtkOpenGLRenderUtilities GLUtil;

   * // Prep fragment shader source:
   * std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
   * svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Decl",
   * "uniform sampler2D aTexture;");
   * svtkShaderProgram::Substitute(fragShader, "//SVTK::FSQ::Impl",
   * "gl_FragData[0] = texture2D(aTexture, texCoord);");

   * // Create shader program:
   * svtkShaderProgram *prog = shaderCache->ReadyShaderProgram(
   * GLUtil::GetFullScreenQuadVertexShader().c_str(),
   * fragShader.c_str(),
   * GLUtil::GetFullScreenQuadGeometryShader().c_str());

   * // Initialize new VAO/vertex buffer. This is only done once:
   * svtkNew<svtkOpenGLVertexArrayObject> vao;
   * GLUtil::PrepFullScreenVAO(renWin, vao.Get(), prog);

   * // Setup shader program to sample svtkTextureObject aTexture:
   * aTexture->Activate();
   * prog->SetUniformi("aTexture", aTexture->GetTextureUnit());

   * // Render the full-screen quad:
   * vao->Bind();
   * GLUtil::DrawFullScreenQuad();
   * vao->Release();
   * aTexture->Deactivate();
   * @endcode
   */
  static std::string GetFullScreenQuadVertexShader();
  static std::string GetFullScreenQuadFragmentShaderTemplate();
  static std::string GetFullScreenQuadGeometryShader();
  static bool PrepFullScreenVAO(
    svtkOpenGLRenderWindow* renWin, svtkOpenGLVertexArrayObject* vao, svtkShaderProgram* prog);
  static void DrawFullScreenQuad();
  //@}

  // older signsature, we suggest you use the newer signature above
  static bool PrepFullScreenVAO(
    svtkOpenGLBufferObject* verts, svtkOpenGLVertexArrayObject* vao, svtkShaderProgram* prog);

  /**
   * Pass a debugging mark to the render engine to assist development via tools
   * like apitrace. This calls glDebugMessageInsert to insert the event string
   * into the OpenGL command stream.
   *
   * Note that this method only works when glDebugMessageInsert is bound, which
   * it may not be on certain platforms.
   */
  static void MarkDebugEvent(const std::string& event);

protected:
  svtkOpenGLRenderUtilities();
  ~svtkOpenGLRenderUtilities() override;

private:
  svtkOpenGLRenderUtilities(const svtkOpenGLRenderUtilities&) = delete;
  void operator=(const svtkOpenGLRenderUtilities&) = delete;
};

#endif
