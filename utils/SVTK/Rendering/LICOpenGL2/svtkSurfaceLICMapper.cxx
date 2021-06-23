/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSurfaceLICMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSurfaceLICMapper.h"

#include "svtkSurfaceLICInterface.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkOpenGLVertexBufferObjectGroup.h"
#include "svtkPainterCommunicator.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"

// use parallel timer for benchmarks and scaling
// if not defined svtkTimerLOG is used.
// #define svtkSurfaceLICMapperTIME
#if !defined(svtkSurfaceLICMapperTIME)
#include "svtkTimerLog.h"
#endif
#define svtkSurfaceLICMapperDEBUG 0

//----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkSurfaceLICMapper);

//----------------------------------------------------------------------------
svtkSurfaceLICMapper::svtkSurfaceLICMapper()
{
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);

  this->LICInterface = svtkSurfaceLICInterface::New();
}

//----------------------------------------------------------------------------
svtkSurfaceLICMapper::~svtkSurfaceLICMapper()
{
#if svtkSurfaceLICMapperDEBUG >= 1
  cerr << "=====svtkSurfaceLICMapper::~svtkSurfaceLICMapper" << endl;
#endif

  this->LICInterface->Delete();
  this->LICInterface = nullptr;
}

void svtkSurfaceLICMapper::ShallowCopy(svtkAbstractMapper* mapper)
{
  svtkSurfaceLICMapper* m = svtkSurfaceLICMapper::SafeDownCast(mapper);
  this->LICInterface->ShallowCopy(m->GetLICInterface());

  this->SetInputArrayToProcess(0, m->GetInputArrayInformation(0));
  this->SetScalarVisibility(m->GetScalarVisibility());

  // Now do superclass
  this->svtkOpenGLPolyDataMapper::ShallowCopy(mapper);
}

//----------------------------------------------------------------------------
void svtkSurfaceLICMapper::ReleaseGraphicsResources(svtkWindow* win)
{
  this->LICInterface->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

void svtkSurfaceLICMapper::ReplaceShaderValues(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* actor)
{
  std::string VSSource = shaders[svtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[svtkShader::Fragment]->GetSource();

  // add some code to handle the LIC vectors and mask
  svtkShaderProgram::Substitute(VSSource, "//SVTK::TCoord::Dec",
    "in vec3 vecsMC;\n"
    "out vec3 tcoordVCVSOutput;\n");

  svtkShaderProgram::Substitute(VSSource, "//SVTK::TCoord::Impl", "tcoordVCVSOutput = vecsMC;");

  svtkShaderProgram::Substitute(FSSource, "//SVTK::TCoord::Dec",
    // 0/1, when 1 V is projected to surface for |V| computation.
    "uniform int uMaskOnSurface;\n"
    "uniform mat3 normalMatrix;\n"
    "in vec3 tcoordVCVSOutput;");

  svtkShaderProgram::Substitute(FSSource, "//SVTK::TCoord::Impl",
    // projected vectors
    "  vec3 tcoordLIC = normalMatrix * tcoordVCVSOutput;\n"
    "  vec3 normN = normalize(normalVCVSOutput);\n"
    "  float k = dot(tcoordLIC, normN);\n"
    "  tcoordLIC = (tcoordLIC - k*normN);\n"
    "  gl_FragData[1] = vec4(tcoordLIC.x, tcoordLIC.y, 0.0 , gl_FragCoord.z);\n"
    //   "  gl_FragData[1] = vec4(tcoordVC.xyz, gl_FragCoord.z);\n"
    // vectors for fragment masking
    "  if (uMaskOnSurface == 0)\n"
    "    {\n"
    "    gl_FragData[2] = vec4(tcoordVCVSOutput, gl_FragCoord.z);\n"
    "    }\n"
    "  else\n"
    "    {\n"
    "    gl_FragData[2] = vec4(tcoordLIC.x, tcoordLIC.y, 0.0 , gl_FragCoord.z);\n"
    "    }\n"
    //   "  gl_FragData[2] = vec4(19.0, 19.0, tcoordVC.x, gl_FragCoord.z);\n"
    ,
    false);

  shaders[svtkShader::Vertex]->SetSource(VSSource);
  shaders[svtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderValues(shaders, ren, actor);
}

void svtkSurfaceLICMapper::SetMapperShaderParameters(
  svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
  cellBO.Program->SetUniformi("uMaskOnSurface", this->LICInterface->GetMaskOnSurface());
}

//----------------------------------------------------------------------------
void svtkSurfaceLICMapper::RenderPiece(svtkRenderer* renderer, svtkActor* actor)
{
#ifdef svtkSurfaceLICMapperTIME
  this->StartTimerEvent("svtkSurfaceLICMapper::RenderInternal");
#else
  svtkSmartPointer<svtkTimerLog> timer = svtkSmartPointer<svtkTimerLog>::New();
  timer->StartTimer();
#endif

  svtkOpenGLClearErrorMacro();

  this->LICInterface->ValidateContext(renderer);

  this->LICInterface->UpdateCommunicator(renderer, actor, this->GetInput());

  svtkPainterCommunicator* comm = this->LICInterface->GetCommunicator();

  if (comm->GetIsNull())
  {
    // other rank's may have some visible data but we
    // have none and should not participate further
    return;
  }

  this->CurrentInput = this->GetInput();
  svtkDataArray* vectors = this->GetInputArrayToProcess(0, this->CurrentInput);
  this->LICInterface->SetHasVectors(vectors != nullptr ? true : false);

  if (!this->LICInterface->CanRenderSurfaceLIC(actor))
  {
    // we've determined that there's no work for us, or that the
    // requisite opengl extensions are not available. pass control on
    // to delegate renderer and return.
    this->Superclass::RenderPiece(renderer, actor);
#ifdef svtkSurfaceLICMapperTIME
    this->EndTimerEvent("svtkSurfaceLICMapper::RenderInternal");
#endif
    return;
  }

  // Before start rendering LIC, capture some essential state so we can restore
  // it.
  svtkOpenGLRenderWindow* rw = svtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  svtkOpenGLState* ostate = rw->GetState();
  svtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);

  svtkNew<svtkOpenGLFramebufferObject> fbo;
  fbo->SetContext(rw);
  ostate->PushFramebufferBindings();

  // allocate rendering resources, initialize or update
  // textures and shaders.
  this->LICInterface->InitializeResources();

  // draw the geometry
  this->LICInterface->PrepareForGeometry();
  this->RenderPieceStart(renderer, actor);
  this->RenderPieceDraw(renderer, actor);
  this->RenderPieceFinish(renderer, actor);
  this->LICInterface->CompletedGeometry();

  // --------------------------------------------- compoiste vectors for parallel LIC
  this->LICInterface->GatherVectors();

  // ------------------------------------------- LIC on screen
  this->LICInterface->ApplyLIC();

  // ------------------------------------------- combine scalar colors + LIC
  this->LICInterface->CombineColorsAndLIC();

  // ----------------------------------------------- depth test and copy to screen
  this->LICInterface->CopyToScreen();

  ostate->PopFramebufferBindings();

  // clear opengl error flags and be absolutely certain that nothing failed.
  svtkOpenGLCheckErrorMacro("failed during surface lic painter");

#ifdef svtkSurfaceLICMapperTIME
  this->EndTimerEvent("svtkSurfaceLICMapper::RenderInternal");
#else
  timer->StopTimer();
#endif
}

//-------------------------------------------------------------------------
void svtkSurfaceLICMapper::BuildBufferObjects(svtkRenderer* ren, svtkActor* act)
{
  if (this->LICInterface->GetHasVectors())
  {
    svtkDataArray* vectors = this->GetInputArrayToProcess(0, this->CurrentInput);
    this->VBOs->CacheDataArray("vecsMC", vectors, ren, SVTK_FLOAT);
  }

  this->Superclass::BuildBufferObjects(ren, act);
}

//----------------------------------------------------------------------------
void svtkSurfaceLICMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
