/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSSAOPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSSAOPass.h"

#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLPolyDataMapper.h"
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

#include <random>

svtkStandardNewMacro(svtkSSAOPass);

// ----------------------------------------------------------------------------
void svtkSSAOPass::PrintSelf(ostream& os, svtkIndent indent)
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
  os << indent << "PositionTexture:";
  if (this->PositionTexture != nullptr)
  {
    this->PositionTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "NormalTexture:";
  if (this->NormalTexture != nullptr)
  {
    this->NormalTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "SSAOTexture:";
  if (this->SSAOTexture != nullptr)
  {
    this->SSAOTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "DepthTexture:";
  if (this->DepthTexture != nullptr)
  {
    this->DepthTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
void svtkSSAOPass::InitializeGraphicsResources(svtkOpenGLRenderWindow* renWin, int w, int h)
{
  if (this->ColorTexture == nullptr)
  {
    this->ColorTexture = svtkTextureObject::New();
    this->ColorTexture->SetContext(renWin);
    this->ColorTexture->SetFormat(GL_RGBA);
    this->ColorTexture->SetInternalFormat(GL_RGBA32F);
    this->ColorTexture->SetDataType(GL_FLOAT);
    this->ColorTexture->SetMinificationFilter(svtkTextureObject::Linear);
    this->ColorTexture->SetMagnificationFilter(svtkTextureObject::Linear);
    this->ColorTexture->Allocate2D(w, h, 4, SVTK_FLOAT);
  }

  if (this->PositionTexture == nullptr)
  {
    // This texture needs mipmapping levels in order to improve
    // texture sampling performances
    // see "Scalable ambient obscurance"
    this->PositionTexture = svtkTextureObject::New();
    this->PositionTexture->SetContext(renWin);
    this->PositionTexture->SetFormat(GL_RGB);
    this->PositionTexture->SetInternalFormat(GL_RGB16F);
    this->PositionTexture->SetDataType(GL_FLOAT);
    this->PositionTexture->SetWrapS(svtkTextureObject::ClampToEdge);
    this->PositionTexture->SetWrapT(svtkTextureObject::ClampToEdge);
    this->PositionTexture->SetMinificationFilter(svtkTextureObject::NearestMipmapNearest);
    this->PositionTexture->SetMaxLevel(10);
    this->PositionTexture->Allocate2D(w, h, 3, SVTK_FLOAT);
  }

  if (this->NormalTexture == nullptr)
  {
    this->NormalTexture = svtkTextureObject::New();
    this->NormalTexture->SetContext(renWin);
    this->NormalTexture->SetFormat(GL_RGB);
    this->NormalTexture->SetInternalFormat(GL_RGB16F);
    this->NormalTexture->SetDataType(GL_FLOAT);
    this->NormalTexture->SetWrapS(svtkTextureObject::ClampToEdge);
    this->NormalTexture->SetWrapT(svtkTextureObject::ClampToEdge);
    this->NormalTexture->Allocate2D(w, h, 3, SVTK_FLOAT);
  }

  if (this->SSAOTexture == nullptr)
  {
    this->SSAOTexture = svtkTextureObject::New();
    this->SSAOTexture->SetContext(renWin);
    this->SSAOTexture->SetFormat(GL_RED);
    this->SSAOTexture->SetInternalFormat(GL_R8);
    this->SSAOTexture->SetDataType(GL_UNSIGNED_BYTE);
    this->SSAOTexture->Allocate2D(w, h, 1, SVTK_UNSIGNED_CHAR);
  }

  if (this->DepthTexture == nullptr)
  {
    this->DepthTexture = svtkTextureObject::New();
    this->DepthTexture->SetContext(renWin);
    this->DepthTexture->AllocateDepth(w, h, svtkTextureObject::Float32);
  }

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = svtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }
}

// ----------------------------------------------------------------------------
void svtkSSAOPass::ComputeKernel()
{
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
  std::default_random_engine generator;

  this->Kernel.resize(3 * this->KernelSize);

  for (unsigned int i = 0; i < this->KernelSize; ++i)
  {
    float sample[3] = { randomFloats(generator) * 2.f - 1.f, randomFloats(generator) * 2.f - 1.f,
      randomFloats(generator) };

    // reject the sample if not in the hemisphere
    if (svtkMath::Norm(sample) > 1.0)
    {
      i--;
      continue;
    }

    // more samples closer to the point
    float scale = i / static_cast<float>(this->KernelSize);
    scale = 0.1f + 0.9f * scale * scale; // lerp
    svtkMath::MultiplyScalar(sample, scale);

    this->Kernel[3 * i] = sample[0];
    this->Kernel[3 * i + 1] = sample[1];
    this->Kernel[3 * i + 2] = sample[2];
  }
}

// ----------------------------------------------------------------------------
bool svtkSSAOPass::SetShaderParameters(svtkShaderProgram* svtkNotUsed(program),
  svtkAbstractMapper* mapper, svtkProp* svtkNotUsed(prop), svtkOpenGLVertexArrayObject* svtkNotUsed(VAO))
{
  if (svtkOpenGLPolyDataMapper::SafeDownCast(mapper) != nullptr)
  {
    this->FrameBufferObject->ActivateDrawBuffers(3);
  }
  else
  {
    this->FrameBufferObject->ActivateDrawBuffers(1);
  }

  return true;
}

// ----------------------------------------------------------------------------
void svtkSSAOPass::RenderDelegate(const svtkRenderState* s, int w, int h)
{
  this->PreRender(s);

  this->FrameBufferObject->GetContext()->GetState()->PushFramebufferBindings();
  this->FrameBufferObject->Bind();

  this->FrameBufferObject->AddColorAttachment(0, this->ColorTexture);
  this->FrameBufferObject->AddColorAttachment(1, this->PositionTexture);
  this->FrameBufferObject->AddColorAttachment(2, this->NormalTexture);
  this->FrameBufferObject->ActivateDrawBuffers(3);
  this->FrameBufferObject->AddDepthAttachment(this->DepthTexture);
  this->FrameBufferObject->StartNonOrtho(w, h);

  this->DelegatePass->Render(s);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

  this->FrameBufferObject->GetContext()->GetState()->PopFramebufferBindings();

  this->PostRender(s);
}

// ----------------------------------------------------------------------------
void svtkSSAOPass::RenderSSAO(svtkOpenGLRenderWindow* renWin, svtkMatrix4x4* projection, int w, int h)
{
  if (this->SSAOQuadHelper && this->SSAOQuadHelper->ShaderChangeValue < this->GetMTime())
  {
    delete this->SSAOQuadHelper;
    this->SSAOQuadHelper = nullptr;
  }

  if (!this->SSAOQuadHelper)
  {
    this->ComputeKernel();

    std::string FSSource = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    std::stringstream ssDecl;
    ssDecl << "uniform sampler2D texPosition;\n"
              "uniform sampler2D texNormal;\n"
              "uniform sampler2D texNoise;\n"
              "uniform sampler2D texDepth;\n"
              "uniform float kernelRadius;\n"
              "uniform float kernelBias;\n"
              "uniform vec3 samples["
           << this->KernelSize
           << "];\n"
              "uniform mat4 matProjection;\n"
              "uniform ivec2 size;\n";

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl", ssDecl.str());

    std::stringstream ssImpl;
    ssImpl
      << "\n"
         "  float occlusion = 0.0;\n"
         "  float depth = texture(texDepth, texCoord).r;\n"
         "  if (depth < 1.0)\n"
         "  {\n"
         "    vec3 fragPosVC = texture(texPosition, texCoord).xyz;\n"
         "    vec4 fragPosDC = matProjection * vec4(fragPosVC, 1.0);\n"
         "    fragPosDC.xyz /= fragPosDC.w;\n"
         "    fragPosDC.xyz = fragPosDC.xyz * 0.5 + 0.5;\n"
         "    if (fragPosDC.z - depth < 0.0001)\n"
         "    {\n"
         "      vec3 normal = texture(texNormal, texCoord).rgb;\n"
         "      vec2 tilingShift = size / textureSize(texNoise, 0);\n"
         "      float randomAngle = 6.283185 * texture(texNoise, texCoord * tilingShift).r;\n"
         "      vec3 randomVec = vec3(cos(randomAngle), sin(randomAngle), 0.0);\n"
         "      vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));\n"
         "      vec3 bitangent = cross(normal, tangent);\n"
         "      mat3 TBN = mat3(tangent, bitangent, normal);\n"
         "      const int kernelSize = "
      << this->KernelSize
      << ";\n"
         "      for (int i = 0; i < kernelSize; i++)\n"
         "      {\n"
         "        vec3 sampleVC = TBN * samples[i];\n"
         "        sampleVC = fragPosVC + sampleVC * kernelRadius;\n"
         "        vec4 sampleDC = matProjection * vec4(sampleVC, 1.0);\n"
         "        sampleDC.xyz /= sampleDC.w;\n"
         "        sampleDC.xyz = sampleDC.xyz * 0.5 + 0.5;\n" // to clip space
         "        float sampleDepth = textureLod(texPosition, sampleDC.xy, 40.0 * "
         "distance(fragPosDC.xy, sampleDC.xy)).z;\n"
         "        float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(fragPosVC.z - "
         "sampleDepth));\n"
         "        occlusion += (sampleDepth >= sampleVC.z + kernelBias ? 1.0 : 0.0) * rangeCheck;\n"
         "      }\n"
         "      occlusion = occlusion / float(kernelSize);\n"
         "    }\n"
         "  }\n"
         "  gl_FragData[0] = vec4(vec3(1.0 - occlusion), 1.0);\n";

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl", ssImpl.str());

    this->SSAOQuadHelper = new svtkOpenGLQuadHelper(renWin,
      svtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->SSAOQuadHelper->ShaderChangeValue = this->GetMTime();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->SSAOQuadHelper->Program);
  }

  if (!this->SSAOQuadHelper->Program || !this->SSAOQuadHelper->Program->GetCompiled())
  {
    svtkErrorMacro("Couldn't build the SSAO shader program.");
    return;
  }

  this->PositionTexture->Activate();
  this->NormalTexture->Activate();
  this->DepthTexture->Activate();
  this->SSAOQuadHelper->Program->SetUniformi(
    "texPosition", this->PositionTexture->GetTextureUnit());
  this->SSAOQuadHelper->Program->SetUniformi("texNormal", this->NormalTexture->GetTextureUnit());
  this->SSAOQuadHelper->Program->SetUniform3fv("samples", this->KernelSize, this->Kernel.data());
  this->SSAOQuadHelper->Program->SetUniformi("texNoise", renWin->GetNoiseTextureUnit());
  this->SSAOQuadHelper->Program->SetUniformi("texDepth", this->DepthTexture->GetTextureUnit());
  this->SSAOQuadHelper->Program->SetUniformf("kernelRadius", this->Radius);
  this->SSAOQuadHelper->Program->SetUniformf("kernelBias", this->Bias);
  this->SSAOQuadHelper->Program->SetUniformMatrix("matProjection", projection);

  int size[2] = { w, h };
  this->SSAOQuadHelper->Program->SetUniform2i("size", size);

  this->FrameBufferObject->GetContext()->GetState()->PushFramebufferBindings();
  this->FrameBufferObject->Bind();

  this->FrameBufferObject->AddColorAttachment(0, this->SSAOTexture);
  this->FrameBufferObject->ActivateDrawBuffers(1);
  this->FrameBufferObject->StartNonOrtho(w, h);

  this->SSAOQuadHelper->Render();

  this->FrameBufferObject->GetContext()->GetState()->PopFramebufferBindings();

  this->DepthTexture->Deactivate();
  this->PositionTexture->Deactivate();
  this->NormalTexture->Deactivate();
}

// ----------------------------------------------------------------------------
void svtkSSAOPass::RenderCombine(svtkOpenGLRenderWindow* renWin)
{
  svtkOpenGLState* ostate = renWin->GetState();

  if (this->CombineQuadHelper && this->CombineQuadHelper->ShaderChangeValue < this->GetMTime())
  {
    delete this->CombineQuadHelper;
    this->CombineQuadHelper = nullptr;
  }

  if (!this->CombineQuadHelper)
  {
    std::string FSSource = svtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    std::stringstream ssDecl;
    ssDecl << "uniform sampler2D texColor;\n"
              "uniform sampler2D texSSAO;\n"
              "uniform sampler2D texDepth;\n"
              "//SVTK::FSQ::Decl";

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Decl", ssDecl.str());

    std::stringstream ssImpl;
    ssImpl << "  vec4 col = texture(texColor, texCoord);\n";

    if (this->Blur)
    {
      ssImpl << "  ivec2 size = textureSize(texSSAO, 0);"
                "  float ao = 0.195346 * texture(texSSAO, texCoord).r + \n"
                "    0.077847	* texture(texSSAO, texCoord + vec2(-1, -1) / size).r +\n"
                "    0.077847 * texture(texSSAO, texCoord + vec2(-1, 1) / size).r +\n"
                "    0.077847 * texture(texSSAO, texCoord + vec2(1, -1) / size).r +\n"
                "    0.077847 * texture(texSSAO, texCoord + vec2(1, 1) / size).r +\n"
                "    0.123317 * texture(texSSAO, texCoord + vec2(-1, 0) / size).r +\n"
                "    0.123317 * texture(texSSAO, texCoord + vec2(1, 0) / size).r +\n"
                "    0.123317 * texture(texSSAO, texCoord + vec2(0, -1) / size).r +\n"
                "    0.123317 * texture(texSSAO, texCoord + vec2(0, 1) / size).r;\n";
    }
    else
    {
      ssImpl << "  float ao = texture(texSSAO, texCoord).r;\n";
    }
    ssImpl << "  gl_FragData[0] = vec4(col.rgb * ao, col.a);\n"
              "  gl_FragDepth = texture(texDepth, texCoord).r;\n";

    svtkShaderProgram::Substitute(FSSource, "//SVTK::FSQ::Impl", ssImpl.str());

    this->CombineQuadHelper = new svtkOpenGLQuadHelper(renWin,
      svtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->CombineQuadHelper->ShaderChangeValue = this->GetMTime();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->CombineQuadHelper->Program);
  }

  if (!this->CombineQuadHelper->Program || !this->CombineQuadHelper->Program->GetCompiled())
  {
    svtkErrorMacro("Couldn't build the SSAO Combine shader program.");
    return;
  }

  this->ColorTexture->Activate();
  this->SSAOTexture->Activate();
  this->DepthTexture->Activate();
  this->CombineQuadHelper->Program->SetUniformi("texColor", this->ColorTexture->GetTextureUnit());
  this->CombineQuadHelper->Program->SetUniformi("texSSAO", this->SSAOTexture->GetTextureUnit());
  this->CombineQuadHelper->Program->SetUniformi("texDepth", this->DepthTexture->GetTextureUnit());

  ostate->svtkglEnable(GL_DEPTH_TEST);
  ostate->svtkglClear(GL_DEPTH_BUFFER_BIT);

  this->CombineQuadHelper->Render();

  this->DepthTexture->Deactivate();
  this->ColorTexture->Deactivate();
  this->SSAOTexture->Deactivate();
}

// ----------------------------------------------------------------------------
void svtkSSAOPass::Render(const svtkRenderState* s)
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
    svtkWarningMacro("no delegate in svtkSSAOPass.");
    return;
  }

  // create FBO and texture
  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  this->InitializeGraphicsResources(renWin, w, h);

  this->ColorTexture->Resize(w, h);
  this->PositionTexture->Resize(w, h);
  this->NormalTexture->Resize(w, h);
  this->SSAOTexture->Resize(w, h);
  this->DepthTexture->Resize(w, h);

  ostate->svtkglViewport(x, y, w, h);
  ostate->svtkglScissor(x, y, w, h);

  this->RenderDelegate(s, w, h);

  ostate->svtkglDisable(GL_BLEND);
  ostate->svtkglDisable(GL_DEPTH_TEST);

  // generate mipmap levels
  this->PositionTexture->Bind();
  glGenerateMipmap(GL_TEXTURE_2D);

  svtkOpenGLCamera* cam = (svtkOpenGLCamera*)(r->GetActiveCamera());
  svtkMatrix4x4* projection = cam->GetProjectionTransformMatrix(r->GetTiledAspectRatio(), -1, 1);
  projection->Transpose();

  this->RenderSSAO(renWin, projection, w, h);
  this->RenderCombine(renWin);

  svtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
bool svtkSSAOPass::PreReplaceShaderValues(std::string& svtkNotUsed(vertexShader),
  std::string& svtkNotUsed(geometryShader), std::string& fragmentShader, svtkAbstractMapper* mapper,
  svtkProp* svtkNotUsed(prop))
{
  if (svtkOpenGLPolyDataMapper::SafeDownCast(mapper) != nullptr)
  {
    // apply SSAO after lighting
    svtkShaderProgram::Substitute(fragmentShader, "//SVTK::Light::Impl",
      "//SVTK::Light::Impl\n"
      "  //SVTK::SSAO::Impl\n",
      false);
  }

  return true;
}

// ----------------------------------------------------------------------------
bool svtkSSAOPass::PostReplaceShaderValues(std::string& svtkNotUsed(vertexShader),
  std::string& svtkNotUsed(geometryShader), std::string& fragmentShader, svtkAbstractMapper* mapper,
  svtkProp* svtkNotUsed(prop))
{
  if (svtkOpenGLPolyDataMapper::SafeDownCast(mapper) != nullptr)
  {
    if (fragmentShader.find("vertexVC") != std::string::npos &&
      fragmentShader.find("normalVCVSOutput") != std::string::npos)
    {
      svtkShaderProgram::Substitute(fragmentShader, "  //SVTK::SSAO::Impl",
        "  gl_FragData[1] = vec4(vertexVC.xyz, 1.0);\n"
        "  gl_FragData[2] = vec4(normalVCVSOutput, 1.0);\n"
        "\n",
        false);
    }
    else
    {
      svtkShaderProgram::Substitute(fragmentShader, "  //SVTK::SSAO::Impl",
        "  gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "  gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "\n",
        false);
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
void svtkSSAOPass::ReleaseGraphicsResources(svtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->SSAOQuadHelper)
  {
    delete this->SSAOQuadHelper;
    this->SSAOQuadHelper = nullptr;
  }
  if (this->CombineQuadHelper)
  {
    delete this->CombineQuadHelper;
    this->CombineQuadHelper = nullptr;
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
  if (this->PositionTexture)
  {
    this->PositionTexture->Delete();
    this->PositionTexture = nullptr;
  }
  if (this->NormalTexture)
  {
    this->NormalTexture->Delete();
    this->NormalTexture = nullptr;
  }
  if (this->SSAOTexture)
  {
    this->SSAOTexture->Delete();
    this->SSAOTexture = nullptr;
  }
  if (this->DepthTexture)
  {
    this->DepthTexture->Delete();
    this->DepthTexture = nullptr;
  }
}
