/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkToneMappingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkToneMappingPass.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLQuadHelper.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"

svtkStandardNewMacro(svtkToneMappingPass);

// ----------------------------------------------------------------------------
svtkToneMappingPass::~svtkToneMappingPass()
{
  if (this->FrameBufferObject)
  {
    svtkErrorMacro("FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->ColorTexture)
  {
    svtkErrorMacro("ColorTexture should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->QuadHelper)
  {
    svtkErrorMacro("QuadHelper should have been deleted in ReleaseGraphicsResources().");
  }
}

// ----------------------------------------------------------------------------
void svtkToneMappingPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FrameBufferObject:";
  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "ColorTexture:";
  if (this->ColorTexture != nullptr)
  {
    this->ColorTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
void svtkToneMappingPass::Render(const svtkRenderState* s)
{
  svtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  svtkRenderer* r = s->GetRenderer();
  svtkOpenGLRenderWindow* renWin = static_cast<svtkOpenGLRenderWindow*>(r->GetRenderWindow());
  svtkOpenGLState* ostate = renWin->GetState();

  svtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
  svtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  if (this->DelegatePass == nullptr)
  {
    svtkWarningMacro("no delegate in svtkToneMappingPass.");
    return;
  }

  // create FBO and texture
  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  if (this->ColorTexture == nullptr)
  {
    this->ColorTexture = svtkTextureObject::New();
    this->ColorTexture->SetContext(renWin);
    this->ColorTexture->SetMinificationFilter(svtkTextureObject::Linear);
    this->ColorTexture->SetMagnificationFilter(svtkTextureObject::Linear);
    this->ColorTexture->Allocate2D(w, h, 4, SVTK_FLOAT);
  }
  this->ColorTexture->Resize(w, h);

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = svtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  renWin->GetState()->PushFramebufferBindings();
  this->RenderDelegate(s, w, h, w, h, this->FrameBufferObject, this->ColorTexture);
  renWin->GetState()->PopFramebufferBindings();

  if (this->QuadHelper &&
    (static_cast<unsigned int>(this->ToneMappingType) != this->QuadHelper->ShaderChangeValue ||
      this->UseACES != this->UseACESChangeValue))
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
  }

  if (!this->QuadHelper)
  {
    std::string FSSource = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl",
      "uniform sampler2D source;\n"
      "//SVTK::FSQ::Decl");

    // Inverse gamma correction
    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
      "  vec4 pixel = texture2D(source, texCoord);\n"
      "  vec3 color = pow(pixel.rgb, vec3(2.2));\n" // to linear color space
      "//SVTK::FSQ::Impl");

    switch (this->ToneMappingType)
    {
      case Clamp:
        svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
          "  vec3 toned = min(color, vec3(1.0));\n"
          "//SVTK::FSQ::Impl");
        break;
      case Reinhard:
        svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
          "  vec3 toned = color / (color + 1.0);\n"
          "//SVTK::FSQ::Impl");
        break;
      case Exponential:
        svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl", "uniform float exposure;\n");
        svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
          "  vec3 toned = (1.0 - exp(-color*exposure));\n"
          "  //SVTK::FSQ::Impl");
        break;
      case GenericFilmic:
        svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl",
          "uniform float exposure;\n"
          "uniform float a;\n"
          "uniform float b;\n"
          "uniform float c;\n"
          "uniform float d;\n"
          "//SVTK::FSQ::Decl");

        if (this->UseACES)
        {
          svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl",
            "const mat3 acesInputMat = mat3(0.5972782409, 0.0760130499, 0.0284085382,\n"
            "0.3545713181, 0.9083220973, 0.1338243154,\n"
            "0.0482176639, 0.0156579968, 0.8375684636);\n"
            "const mat3 acesOutputMat = mat3( 1.6047539945, -0.1020831870, -0.0032670420,\n"
            "-0.5310794927, 1.1081322801, -0.0727552477,\n"
            "-0.0736720338, -0.0060518756, 1.0760219533);\n"
            "//SVTK::FSQ::Decl");
        }
        svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
          "  vec3 toned = color * exposure;\n"
          "//SVTK::FSQ::Impl");
        if (this->UseACES)
        {
          svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
            "  toned = acesInputMat * toned;\n"
            "//SVTK::FSQ::Impl");
        }
        svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
          "  toned = pow(toned, vec3(a)) / (pow(toned, vec3(a * d)) * b + c);\n"
          "//SVTK::FSQ::Impl");
        if (this->UseACES)
        {
          svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
            "  toned = acesOutputMat * toned;\n"
            "//SVTK::FSQ::Impl");
        }
        svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
          "  toned = clamp(toned, vec3(0.f), vec3(1.f));\n"
          "//SVTK::FSQ::Impl");
        break;
    }

    // Recorrect gamma and output
    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl",
      "  toned = pow(toned, vec3(1.0/2.2));\n" // to sRGB color space
      "  gl_FragData[0] = vec4(toned , pixel.a);\n"
      "//SVTK::FSQ::Impl");

    this->QuadHelper = new svtkOpenGLQuadHelper(renWin,
      svtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->QuadHelper->ShaderChangeValue = this->ToneMappingType;
    this->UseACESChangeValue = this->UseACES;
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
  }

  if (!this->QuadHelper->Program || !this->QuadHelper->Program->GetCompiled())
  {
    svtkErrorMacro("Couldn't build the shader program.");
    return;
  }

  this->ColorTexture->Activate();
  this->QuadHelper->Program->SetUniformi("source", this->ColorTexture->GetTextureUnit());

  // Precompute generic filmic parameters after each modification
  if (this->PreComputeMTime > this->GetMTime())
  {
    this->PreComputeAnchorCurveGenericFilmic();
    this->PreComputeMTime = this->GetMTime();
  }

  if (this->ToneMappingType == Exponential)
  {
    this->QuadHelper->Program->SetUniformf("exposure", this->Exposure);
  }
  else if (this->ToneMappingType == GenericFilmic)
  {
    this->QuadHelper->Program->SetUniformf("exposure", this->Exposure);
    this->QuadHelper->Program->SetUniformf("a", this->Contrast);
    this->QuadHelper->Program->SetUniformf("b", this->ClippingPoint);
    this->QuadHelper->Program->SetUniformf("c", this->ToeSpeed);
    this->QuadHelper->Program->SetUniformf("d", this->Shoulder);
  }

  ostate->svtkglDisable(GL_BLEND);
  ostate->svtkglDisable(GL_DEPTH_TEST);
  ostate->svtkglViewport(x, y, w, h);
  ostate->svtkglScissor(x, y, w, h);

  this->QuadHelper->Render();

  this->ColorTexture->Deactivate();

  svtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
void svtkToneMappingPass::ReleaseGraphicsResources(svtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->QuadHelper)
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
  }
  if (this->FrameBufferObject)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->ColorTexture)
  {
    this->ColorTexture->Delete();
    this->ColorTexture = nullptr;
  }
}

void svtkToneMappingPass::SetGenericFilmicDefaultPresets()
{
  this->Contrast = 1.6773;
  this->Shoulder = 0.9714;
  this->MidIn = 0.18;
  this->MidOut = 0.18;
  this->HdrMax = 11.0785;
  this->UseACES = true;

  this->Modified();
}

void svtkToneMappingPass::SetGenericFilmicUncharted2Presets()
{
  this->Contrast = 1.1759;
  this->Shoulder = 0.9746;
  this->MidIn = 0.18;
  this->MidOut = 0.18;
  this->HdrMax = 6.3704;
  this->UseACES = false;

  this->Modified();
}

void svtkToneMappingPass::PreComputeAnchorCurveGenericFilmic()
{
  const float& a = this->Contrast;
  const float& d = this->Shoulder;
  const float& m = this->MidIn;
  const float& n = this->MidOut;

  // Pre compute shape of the curve parameters
  this->ClippingPoint =
    -((powf(m, -a * d) *
        (-powf(m, a) +
          (n *
            (powf(m, a * d) * n * powf(this->HdrMax, a) - powf(m, a) * powf(this->HdrMax, a * d))) /
            (powf(m, a * d) * n - n * powf(this->HdrMax, a * d)))) /
      n);

  // Avoid discontinuous curve by clamping to 0
  this->ToeSpeed =
    std::max((powf(m, a * d) * n * powf(this->HdrMax, a) - powf(m, a) * powf(this->HdrMax, a * d)) /
        (powf(m, a * d) * n - n * powf(this->HdrMax, a * d)),
      0.f);
}
