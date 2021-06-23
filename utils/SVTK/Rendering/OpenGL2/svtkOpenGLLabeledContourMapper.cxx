/*==============================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLLabeledContourMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/
#include "svtkOpenGLLabeledContourMapper.h"

#include "svtkActor.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLActor.h"
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLHelper.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLTexture.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextActor3D.h"

//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLLabeledContourMapper);

//------------------------------------------------------------------------------
void svtkOpenGLLabeledContourMapper::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkOpenGLLabeledContourMapper::svtkOpenGLLabeledContourMapper()
{
  this->StencilBO = new svtkOpenGLHelper;
  this->TempMatrix4 = svtkMatrix4x4::New();
}

//------------------------------------------------------------------------------
svtkOpenGLLabeledContourMapper::~svtkOpenGLLabeledContourMapper()
{
  delete this->StencilBO;
  this->StencilBO = nullptr;
  this->TempMatrix4->Delete();
}

//------------------------------------------------------------------------------
bool svtkOpenGLLabeledContourMapper::CreateLabels(svtkActor* actor)
{
  if (!this->Superclass::CreateLabels(actor))
  {
    return false;
  }

  if (svtkMatrix4x4* actorMatrix = actor->GetMatrix())
  {
    for (svtkIdType i = 0; i < this->NumberOfUsedTextActors; ++i)
    {
      svtkMatrix4x4* labelMatrix = this->TextActors[i]->GetUserMatrix();
      svtkMatrix4x4::Multiply4x4(actorMatrix, labelMatrix, labelMatrix);
      this->TextActors[i]->SetUserMatrix(labelMatrix);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void svtkOpenGLLabeledContourMapper::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->StencilBO->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
bool svtkOpenGLLabeledContourMapper::ApplyStencil(svtkRenderer* ren, svtkActor* act)
{
  // Draw stencil quads into stencil buffer:
  // compile and bind it if needed
  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(ren->GetSVTKWindow());
  svtkOpenGLState* ostate = renWin->GetState();

  if (!this->StencilBO->Program)
  {
    this->StencilBO->Program = renWin->GetShaderCache()->ReadyShaderProgram(
      // vertex shader
      "//SVTK::System::Dec\n"
      "in vec4 vertexMC;\n"
      "uniform mat4 MCDCMatrix;\n"
      "void main() { gl_Position = MCDCMatrix*vertexMC; }\n",
      // fragment shader
      "//SVTK::System::Dec\n"
      "//SVTK::Output::Dec\n"
      "void main() { gl_FragData[0] = vec4(1.0,1.0,1.0,1.0); }",
      // geometry shader
      "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->StencilBO->Program);
  }

  if (!this->StencilBO->Program)
  {
    return false;
  }

  // Save some state:
  {
    svtkOpenGLState::ScopedglColorMask cmsaver(ostate);
    svtkOpenGLState::ScopedglDepthMask dmsaver(ostate);

    // Enable rendering into the stencil buffer:
    ostate->svtkglEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glClearStencil(0);
    ostate->svtkglClear(GL_STENCIL_BUFFER_BIT);
    ostate->svtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    ostate->svtkglDepthMask(GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

    svtkOpenGLCamera* cam = (svtkOpenGLCamera*)(ren->GetActiveCamera());
    svtkMatrix4x4* wcdc;
    svtkMatrix4x4* wcvc;
    svtkMatrix3x3* norms;
    svtkMatrix4x4* vcdc;
    cam->GetKeyMatrices(ren, wcvc, norms, vcdc, wcdc);
    if (!act->GetIsIdentity())
    {
      svtkMatrix4x4* mcwc;
      svtkMatrix3x3* anorms;
      ((svtkOpenGLActor*)act)->GetKeyMatrices(mcwc, anorms);
      svtkMatrix4x4::Multiply4x4(mcwc, wcdc, this->TempMatrix4);
      this->StencilBO->Program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
    }
    else
    {
      this->StencilBO->Program->SetUniformMatrix("MCDCMatrix", wcdc);
    }

    svtkOpenGLRenderUtilities::RenderTriangles(this->StencilQuads, this->StencilQuadsSize / 3,
      this->StencilQuadIndices, this->StencilQuadIndicesSize, nullptr, this->StencilBO->Program,
      this->StencilBO->VAO);
  }

  // Setup GL to only draw in unstenciled regions:
  glStencilMask(0x00);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilFunc(GL_EQUAL, 0, 0xFF);

  svtkOpenGLCheckErrorMacro("failed after ApplyStencil()");

  return this->Superclass::ApplyStencil(ren, act);
}

//------------------------------------------------------------------------------
bool svtkOpenGLLabeledContourMapper::RemoveStencil(svtkRenderer* ren)
{
  static_cast<svtkOpenGLRenderWindow*>(ren->GetSVTKWindow())
    ->GetState()
    ->svtkglDisable(GL_STENCIL_TEST);
  svtkOpenGLCheckErrorMacro("failed after RemoveStencil()");
  return this->Superclass::RemoveStencil(ren);
}
