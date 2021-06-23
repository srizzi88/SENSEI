/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRModel.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenVRRay.h"

#include "svtkObjectFactory.h"
#include "svtkOpenVRRenderWindow.h"
// #include "svtkOpenVRCamera.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkOpenGLVertexBufferObject.h"
// #include "svtkOpenGLIndexBufferObject.h"
#include "svtkMatrix4x4.h"
#include "svtkOpenGLHelper.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkRendererCollection.h"
#include "svtkShaderProgram.h"
// #include "svtkRenderWindowInteractor.h"
// #include "svtkInteractorObserver.h"

/*=========================================================================
svtkOpenVRRay
=========================================================================*/
svtkStandardNewMacro(svtkOpenVRRay);

svtkOpenVRRay::svtkOpenVRRay()
{
  this->Show = false;
  this->Length = 1.0f;
  this->Loaded = false;
  this->Color[0] = 1.f;
  this->Color[1] = 0.f;
  this->Color[2] = 0.f;
  this->RayVBO = svtkOpenGLVertexBufferObject::New();
};

svtkOpenVRRay::~svtkOpenVRRay()
{
  this->RayVBO->Delete();
  this->RayVBO = nullptr;
}

void svtkOpenVRRay::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Loaded " << (this->Loaded ? "On\n" : "Off\n");
}

void svtkOpenVRRay::ReleaseGraphicsResources(svtkRenderWindow* win)
{
  this->RayVBO->ReleaseGraphicsResources();
  this->RayHelper.ReleaseGraphicsResources(win);
}

bool svtkOpenVRRay::Build(svtkOpenGLRenderWindow* win)
{
  // Ray geometry
  float vert[] = { 0, 0, 0, 0, 0, -1 };

  this->RayVBO->Upload(vert, 2 * 3, svtkOpenGLBufferObject::ArrayBuffer);

  this->RayHelper.Program = win->GetShaderCache()->ReadyShaderProgram(

    // vertex shader
    "//SVTK::System::Dec\n"
    "uniform mat4 matrix;\n"
    "uniform float scale;\n"
    "in vec3 position;\n"
    "void main()\n"
    "{\n"
    " gl_Position =  matrix * vec4(scale * position, 1.0);\n"
    "}\n",

    // fragment shader
    "//SVTK::System::Dec\n"
    "//SVTK::Output::Dec\n"
    "uniform vec3 color;\n"
    "void main()\n"
    "{\n"
    "   gl_FragData[0] = vec4(color, 1.0);\n"
    "}\n",

    // geom shader
    "");
  svtkShaderProgram* program = this->RayHelper.Program;
  this->RayHelper.VAO->Bind();
  if (!this->RayHelper.VAO->AddAttributeArray(
        program, this->RayVBO, "position", 0, 3 * sizeof(float), SVTK_FLOAT, 3, false))
  {
    svtkErrorMacro(<< "Error setting position in shader VAO.");
  }

  return true;
}

void svtkOpenVRRay::Render(svtkOpenGLRenderWindow* win, svtkMatrix4x4* poseMatrix)
{
  // Load ray
  if (!this->Loaded)
  {
    if (!this->Build(win))
    {
      svtkErrorMacro("Unable to build controller ray ");
    }
    this->Loaded = true;
  }

  // Render ray
  win->GetState()->svtkglDepthMask(GL_TRUE);
  win->GetShaderCache()->ReadyShaderProgram(this->RayHelper.Program);
  this->RayHelper.VAO->Bind();

  svtkRenderer* ren = static_cast<svtkRenderer*>(win->GetRenderers()->GetItemAsObject(0));
  if (!ren)
  {
    svtkErrorMacro("Unable get renderer");
    return;
  }

  double unitV[4] = { 0, 0, 0, 1 };
  double scaleFactor = svtkMath::Norm(poseMatrix->MultiplyDoublePoint(unitV));

  this->RayHelper.Program->SetUniformf("scale", this->Length / scaleFactor);
  this->RayHelper.Program->SetUniform3f("color", this->Color);

  this->RayHelper.Program->SetUniformMatrix("matrix", poseMatrix);

  glDrawArrays(GL_LINES, 0, 6);
}
