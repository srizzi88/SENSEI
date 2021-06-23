/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLQuadHelper.h"

#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLResourceFreeCallback.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkShaderProgram.h"
#include "svtk_glew.h"

svtkOpenGLQuadHelper::svtkOpenGLQuadHelper(
  svtkOpenGLRenderWindow* renWin, const char* vs, const char* fs, const char* gs)
  : Program(nullptr)
  , VAO(nullptr)
  , ResourceCallback(new svtkOpenGLResourceFreeCallback<svtkOpenGLQuadHelper>(
      this, &svtkOpenGLQuadHelper::ReleaseGraphicsResources))
{
  if (!fs)
  {
    svtkGenericWarningMacro("A fragment shader is required");
    return;
  }

  this->ResourceCallback->RegisterGraphicsResources(renWin);

  static const char* defaultVS = "//SVTK::System::Dec\n"
                                 "in vec4 ndCoordIn;\n"
                                 "in vec2 texCoordIn;\n"
                                 "out vec2 texCoord;\n"
                                 "void main()\n"
                                 "{\n"
                                 "  gl_Position = ndCoordIn;\n"
                                 "  texCoord = texCoordIn;\n"
                                 "}\n";

  this->Program =
    renWin->GetShaderCache()->ReadyShaderProgram((vs ? vs : defaultVS), fs, (gs ? gs : ""));

  this->VAO = svtkOpenGLVertexArrayObject::New();
  this->ShaderChangeValue = 0;

  this->VAO->Bind();

  svtkOpenGLBufferObject* vertBuf = renWin->GetTQuad2DVBO();
  bool res = this->VAO->AddAttributeArray(
    this->Program, vertBuf, "ndCoordIn", 0, 4 * sizeof(float), SVTK_FLOAT, 2, false);
  if (!res)
  {
    this->VAO->Release();
    svtkGenericWarningMacro("Error binding ndCoords to VAO.");
    return;
  }

  res = this->VAO->AddAttributeArray(this->Program, vertBuf, "texCoordIn", 2 * sizeof(float),
    4 * sizeof(float), SVTK_FLOAT, 2, false);
  if (!res)
  {
    this->VAO->Release();
    svtkGenericWarningMacro("Error binding texCoords to VAO.");
    return;
  }

  this->VAO->Release();
}

svtkOpenGLQuadHelper::~svtkOpenGLQuadHelper()
{
  this->ResourceCallback->Release();
  if (this->VAO)
  {
    this->VAO->Delete();
    this->VAO = nullptr;
  }
}

//------------------------------------------------------------------------------
void svtkOpenGLQuadHelper::ReleaseGraphicsResources(svtkWindow*)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  if (this->VAO)
  {
    this->VAO->ReleaseGraphicsResources();
  }
}

//------------------------------------------------------------------------------
void svtkOpenGLQuadHelper::Render()
{
  if (this->VAO)
  {
    this->VAO->Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    this->VAO->Release();
  }
}
