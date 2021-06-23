/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRenderUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLRenderUtilities.h"
#include "svtk_glew.h"

#include "svtkNew.h"
#include "svtkOpenGLBufferObject.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkRenderingOpenGLConfigure.h"
#include "svtkShaderProgram.h"

// ----------------------------------------------------------------------------
svtkOpenGLRenderUtilities::svtkOpenGLRenderUtilities() = default;

// ----------------------------------------------------------------------------
svtkOpenGLRenderUtilities::~svtkOpenGLRenderUtilities() = default;

void svtkOpenGLRenderUtilities::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ---------------------------------------------------------------------------
// a program must be bound
// a VAO must be bound
void svtkOpenGLRenderUtilities::RenderQuad(
  float* verts, float* tcoords, svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao)
{
  GLuint iboData[] = { 0, 1, 2, 0, 2, 3 };
  svtkOpenGLRenderUtilities::RenderTriangles(verts, 4, iboData, 6, tcoords, program, vao);
}

// ---------------------------------------------------------------------------
// a program must be bound
// a VAO must be bound
void svtkOpenGLRenderUtilities::RenderTriangles(float* verts, unsigned int numVerts, GLuint* iboData,
  unsigned int numIndices, float* tcoords, svtkShaderProgram* program,
  svtkOpenGLVertexArrayObject* vao)
{
  if (!program || !vao || !verts)
  {
    svtkGenericWarningMacro(<< "Error must have verts, program and vao");
    return;
  }

  if (!program->isBound())
  {
    svtkGenericWarningMacro("attempt to render to unbound program");
  }

  svtkNew<svtkOpenGLBufferObject> vbo;
  vbo->Upload(verts, numVerts * 3, svtkOpenGLBufferObject::ArrayBuffer);
  vao->Bind();
  if (!vao->AddAttributeArray(program, vbo, "vertexMC", 0, sizeof(float) * 3, SVTK_FLOAT, 3, false))
  {
    svtkGenericWarningMacro(<< "Error setting 'vertexMC' in shader VAO.");
  }

  svtkNew<svtkOpenGLBufferObject> tvbo;
  if (tcoords)
  {
    tvbo->Upload(tcoords, numVerts * 2, svtkOpenGLBufferObject::ArrayBuffer);
    if (!vao->AddAttributeArray(
          program, tvbo, "tcoordMC", 0, sizeof(float) * 2, SVTK_FLOAT, 2, false))
    {
      svtkGenericWarningMacro(<< "Error setting 'tcoordMC' in shader VAO.");
    }
  }

  svtkNew<svtkOpenGLBufferObject> ibo;
  vao->Bind();
  ibo->Upload(iboData, numIndices, svtkOpenGLBufferObject::ElementArrayBuffer);
  glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);
  ibo->Release();
  ibo->ReleaseGraphicsResources();
  vao->RemoveAttributeArray("vertexMC");
  vao->RemoveAttributeArray("tcoordMC");
  vao->Release();
  vbo->Release();
  vbo->ReleaseGraphicsResources();
  if (tcoords)
  {
    tvbo->Release();
    tvbo->ReleaseGraphicsResources();
  }
}

//------------------------------------------------------------------------------
std::string svtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader()
{
  // Pass through:
  return "//SVTK::System::Dec\n"
         "in vec4 ndCoordIn;\n"
         "in vec2 texCoordIn;\n"
         "out vec2 texCoord;\n"
         "void main()\n"
         "{\n"
         "  gl_Position = ndCoordIn;\n"
         "  texCoord = texCoordIn;\n"
         "}\n";
}

//------------------------------------------------------------------------------
std::string svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate()
{
  return "//SVTK::System::Dec\n"
         "//SVTK::Output::Dec\n"
         "in vec2 texCoord;\n"
         "//SVTK::FSQ::Decl\n"
         "void main()\n"
         "{\n"
         "//SVTK::FSQ::Impl\n"
         "}\n";
}

//------------------------------------------------------------------------------
std::string svtkOpenGLRenderUtilities::GetFullScreenQuadGeometryShader()
{
  return "";
}

//------------------------------------------------------------------------------
bool svtkOpenGLRenderUtilities::PrepFullScreenVAO(
  svtkOpenGLBufferObject* vertBuf, svtkOpenGLVertexArrayObject* vao, svtkShaderProgram* prog)
{
  bool res;

  // ndCoord_x, ndCoord_y, texCoord_x, texCoord_y
  float verts[16] = { 1.f, 1.f, 1.f, 1.f, -1.f, 1.f, 0.f, 1.f, 1.f, -1.f, 1.f, 0.f, -1.f, -1.f, 0.f,
    0.f };

  vertBuf->SetType(svtkOpenGLBufferObject::ArrayBuffer);
  res = vertBuf->Upload(verts, 16, svtkOpenGLBufferObject::ArrayBuffer);
  if (!res)
  {
    svtkGenericWarningMacro("Error uploading fullscreen quad vertex data.");
    return false;
  }

  vao->Bind();

  res =
    vao->AddAttributeArray(prog, vertBuf, "ndCoordIn", 0, 4 * sizeof(float), SVTK_FLOAT, 2, false);
  if (!res)
  {
    vao->Release();
    svtkGenericWarningMacro("Error binding ndCoords to VAO.");
    return false;
  }

  res = vao->AddAttributeArray(
    prog, vertBuf, "texCoordIn", 2 * sizeof(float), 4 * sizeof(float), SVTK_FLOAT, 2, false);
  if (!res)
  {
    vao->Release();
    svtkGenericWarningMacro("Error binding texCoords to VAO.");
    return false;
  }

  vao->Release();
  return true;
}

bool svtkOpenGLRenderUtilities::PrepFullScreenVAO(
  svtkOpenGLRenderWindow* renWin, svtkOpenGLVertexArrayObject* vao, svtkShaderProgram* prog)
{
  bool res;

  vao->Bind();

  svtkOpenGLBufferObject* vertBuf = renWin->GetTQuad2DVBO();
  res =
    vao->AddAttributeArray(prog, vertBuf, "ndCoordIn", 0, 4 * sizeof(float), SVTK_FLOAT, 2, false);
  if (!res)
  {
    vao->Release();
    svtkGenericWarningMacro("Error binding ndCoords to VAO.");
    return false;
  }

  res = vao->AddAttributeArray(
    prog, vertBuf, "texCoordIn", 2 * sizeof(float), 4 * sizeof(float), SVTK_FLOAT, 2, false);
  if (!res)
  {
    vao->Release();
    svtkGenericWarningMacro("Error binding texCoords to VAO.");
    return false;
  }

  vao->Release();
  return true;
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderUtilities::DrawFullScreenQuad()
{
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderUtilities::MarkDebugEvent(const std::string& event)
{
#ifndef SVTK_OPENGL_ENABLE_STREAM_ANNOTATIONS
  (void)event;
#else  // SVTK_OPENGL_ENABLE_STREAM_ANNOTATIONS
  svtkOpenGLStaticCheckErrorMacro("Error before glDebugMessageInsert.");
  glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0,
    GL_DEBUG_SEVERITY_NOTIFICATION, static_cast<GLsizei>(event.size()), event.c_str());
  svtkOpenGLClearErrorMacro();
#endif // SVTK_OPENGL_ENABLE_STREAM_ANNOTATIONS
}
