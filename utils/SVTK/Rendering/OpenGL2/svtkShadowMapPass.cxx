/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShadowMapPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkShadowMapPass.h"
#include "svtkObjectFactory.h"

#include "svtkAbstractTransform.h" // for helper classes stack and concatenation
#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImplicitSum.h"
#include "svtkInformation.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkLightsPass.h"
#include "svtkMath.h"
#include "svtkMatrixToLinearTransform.h"
#include "svtkNew.h"
#include "svtkOpaquePass.h"
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPerspectiveTransform.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderState.h"
#include "svtkSequencePass.h"
#include "svtkShaderProgram.h"
#include "svtkShadowMapBakerPass.h"
#include "svtkTextureObject.h"
#include "svtkTextureUnitManager.h"
#include "svtkTransform.h"
#include "svtk_glew.h"

// debugging
#include "svtkTimerLog.h"

#include "svtkStdString.h"
#include <cassert>
#include <sstream>

// to be able to dump intermediate passes into png files for debugging.
// only for svtkShadowMapPass developers.
//#define SVTK_SHADOW_MAP_PASS_DEBUG
//#define DONT_DUPLICATE_LIGHTS

svtkStandardNewMacro(svtkShadowMapPass);
svtkCxxSetObjectMacro(svtkShadowMapPass, ShadowMapBakerPass, svtkShadowMapBakerPass);
svtkCxxSetObjectMacro(svtkShadowMapPass, OpaqueSequence, svtkRenderPass);

svtkInformationKeyMacro(svtkShadowMapPass, ShadowMapPass, ObjectBase);

// ----------------------------------------------------------------------------
svtkShadowMapPass::svtkShadowMapPass()
{
  this->ShadowMapBakerPass = nullptr;

  svtkNew<svtkSequencePass> seqP;
  svtkNew<svtkLightsPass> lightP;
  svtkNew<svtkOpaquePass> opaqueP;
  svtkNew<svtkRenderPassCollection> rpc;
  rpc->AddItem(lightP);
  rpc->AddItem(opaqueP);
  seqP->SetPasses(rpc);

  this->OpaqueSequence = nullptr;
  this->SetOpaqueSequence(seqP);

  svtkNew<svtkShadowMapBakerPass> bp;
  this->ShadowMapBakerPass = nullptr;
  this->SetShadowMapBakerPass(bp);
}

// ----------------------------------------------------------------------------
svtkShadowMapPass::~svtkShadowMapPass()
{
  if (this->ShadowMapBakerPass != nullptr)
  {
    this->ShadowMapBakerPass->Delete();
  }
  if (this->OpaqueSequence != nullptr)
  {
    this->OpaqueSequence->Delete();
  }
}

// ----------------------------------------------------------------------------
void svtkShadowMapPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ShadowMapBackerPass: ";
  if (this->ShadowMapBakerPass != nullptr)
  {
    this->ShadowMapBakerPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "OpaqueSequence: ";
  if (this->OpaqueSequence != nullptr)
  {
    this->OpaqueSequence->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkShadowMapPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  svtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  svtkOpenGLRenderer* r = static_cast<svtkOpenGLRenderer*>(s->GetRenderer());
  svtkOpenGLCamera* cam = static_cast<svtkOpenGLCamera*>(r->GetActiveCamera());

  if (this->ShadowMapBakerPass != nullptr && this->OpaqueSequence != nullptr)
  {
    this->ShadowTextureUnits.clear();
    this->ShadowAttenuation.clear();
    this->ShadowParallel.clear();

    if (!this->ShadowMapBakerPass->GetHasShadows())
    {
      this->OpaqueSequence->Render(s);
      this->NumberOfRenderedProps += this->OpaqueSequence->GetNumberOfRenderedProps();
      return;
    }

    svtkLightCollection* lights = r->GetLights();
    this->ShadowTextureUnits.resize(lights->GetNumberOfItems());
    this->ShadowAttenuation.resize(lights->GetNumberOfItems());
    this->ShadowParallel.resize(lights->GetNumberOfItems());

    // get the shadow maps and activate them
    int shadowingLightIndex = 0;
    int lightIndex = 0;
    svtkLight* light = nullptr;
    for (lights->InitTraversal(), light = lights->GetNextItem(); light != nullptr;
         light = lights->GetNextItem(), lightIndex++)
    {
      this->ShadowTextureUnits[lightIndex] = -1;
      if (light->GetSwitch() && this->ShadowMapBakerPass->LightCreatesShadow(light))
      {
        svtkTextureObject* map =
          (*this->ShadowMapBakerPass->GetShadowMaps())[static_cast<size_t>(shadowingLightIndex)];
        // activate the texture map
        map->Activate();
        this->ShadowTextureUnits[lightIndex] = map->GetTextureUnit();
        this->ShadowAttenuation[lightIndex] = light->GetShadowAttenuation();
        this->ShadowParallel[lightIndex] = light->GetPositional() ? 0 : 1;
        shadowingLightIndex++;
      }
    }

    svtkMatrix4x4* tmp = svtkMatrix4x4::New();
    svtkMatrix4x4* mat = svtkMatrix4x4::New();
    svtkPerspectiveTransform* transform = svtkPerspectiveTransform::New();

    svtkMatrix4x4* wcdc;
    svtkMatrix4x4* wcvc;
    svtkMatrix3x3* norms;
    svtkMatrix4x4* vcdc;
    cam->GetKeyMatrices(r, wcvc, norms, vcdc, wcdc);

    mat->DeepCopy(wcvc);
    mat->Transpose();
    mat->Invert();

    svtkMatrixToLinearTransform* viewCamera_Inv = svtkMatrixToLinearTransform::New();
    viewCamera_Inv->SetInput(mat);
    mat->Delete();

    // identity. pre-multiply mode
    transform->Translate(0.5, 0.5, 0.5); // bias
    transform->Scale(0.5, 0.5, 0.5);     // scale

    this->ShadowTransforms.clear();
    shadowingLightIndex = 0;
    for (lights->InitTraversal(), light = lights->GetNextItem(), lightIndex = 0; light != nullptr;
         light = lights->GetNextItem(), lightIndex++)
    {
      if (this->ShadowTextureUnits[lightIndex] >= 0)
      {
        svtkCamera* lightCamera =
          (*this->ShadowMapBakerPass->GetLightCameras())[static_cast<size_t>(shadowingLightIndex)];
        transform->Push();
        transform->Concatenate(lightCamera->GetProjectionTransformObject(1, -1, 1));
        transform->Concatenate(lightCamera->GetViewTransformObject());
        transform->Concatenate(viewCamera_Inv);
        transform->GetMatrix(tmp);
        transform->Pop();
        tmp->Transpose();
        for (int i = 0; i < 4; i++)
        {
          for (int j = 0; j < 4; j++)
          {
            this->ShadowTransforms.push_back(tmp->Element[i][j]);
          }
        }
        ++shadowingLightIndex;
      }
    }

    // build the shader code
    this->BuildShaderCode();

    // Setup property keys for actors:
    this->PreRender(s);

    viewCamera_Inv->Delete();
    transform->Delete();
    tmp->Delete();

    // render with shadows
    // note this time we use the list of props after culling.
    this->OpaqueSequence->Render(s);
    this->NumberOfRenderedProps += this->OpaqueSequence->GetNumberOfRenderedProps();

    // now deactivate the shadow maps
    shadowingLightIndex = 0;
    for (lights->InitTraversal(), light = lights->GetNextItem(), lightIndex = 0; light != nullptr;
         light = lights->GetNextItem(), lightIndex++)
    {
      if (light->GetSwitch() && this->ShadowMapBakerPass->LightCreatesShadow(light))
      {
        svtkTextureObject* map =
          (*this->ShadowMapBakerPass->GetShadowMaps())[static_cast<size_t>(shadowingLightIndex)];
        // activate the texture map
        map->Deactivate();
        shadowingLightIndex++;
      }
    }

    this->PostRender(s);
  }
  else
  {
    svtkWarningMacro(<< " no ShadowMapBakerPass or no OpaqueSequence on the ShadowMapBakerPass.");
  }

  svtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
bool svtkShadowMapPass::SetShaderParameters(svtkShaderProgram* program, svtkAbstractMapper*, svtkProp*,
  svtkOpenGLVertexArrayObject* svtkNotUsed(VAO))
{
  size_t numLights = this->ShadowTextureUnits.size();

  // how many lights have shadow maps
  int numSMT = 0;
  float transform[16];
  std::ostringstream toString;

  program->SetUniformf("depthC", 11.0);
  for (size_t i = 0; i < numLights; i++)
  {
    if (this->ShadowTextureUnits[i] >= 0)
    {
      for (int j = 0; j < 16; j++)
      {
        transform[j] = this->ShadowTransforms[numSMT * 16 + j];
      }
      toString.str("");
      toString.clear();
      toString << numSMT;
      program->SetUniformf(
        std::string("shadowAttenuation" + toString.str()).c_str(), this->ShadowAttenuation[i]);
      program->SetUniformi(
        std::string("shadowMap" + toString.str()).c_str(), this->ShadowTextureUnits[i]);
      program->SetUniformMatrix4x4(
        std::string("shadowTransform" + toString.str()).c_str(), transform);

      program->SetUniformi(
        std::string("shadowParallel" + toString.str()).c_str(), this->ShadowParallel[i]);

      svtkCamera* lightCamera =
        (*this->ShadowMapBakerPass->GetLightCameras())[static_cast<size_t>(numSMT)];
      double* crange = lightCamera->GetClippingRange();
      program->SetUniformf(std::string("shadowNearZ" + toString.str()).c_str(), crange[0]);
      program->SetUniformf(std::string("shadowFarZ" + toString.str()).c_str(), crange[1]);

      numSMT++;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool svtkShadowMapPass::PreReplaceShaderValues(
  std::string&, std::string&, std::string& fragmentShader, svtkAbstractMapper*, svtkProp*)
{
  // build the values
  this->BuildShaderCode();

  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Light::Dec", this->GetFragmentDeclaration(), false);
  svtkShaderProgram::Substitute(
    fragmentShader, "//SVTK::Light::Impl", this->GetFragmentImplementation(), false);

  return true;
}

bool svtkShadowMapPass::PostReplaceShaderValues(
  std::string&, std::string&, std::string& fragmentShader, svtkAbstractMapper*, svtkProp*)
{
  size_t numLights = this->ShadowTextureUnits.size();

  for (size_t i = 0; i < numLights; ++i)
  {
    std::ostringstream toString1;
    std::ostringstream toString2;
    toString1 << "diffuse += (df * lightColor" << i << ");";
    toString2 << "diffuse += (df * factor" << i << ".r * lightColor" << i << ");";
    // example of using thickness for subsurface scattering below
    // toString2 << "diffuse += (max(0.2, dot(normalVCVSOutput, lightDirectionVC" << i << ")) *
    // ambientColorUniform * pow(0.5, 30.0*factor" << i << ".g - 0.1) + df * factor" << i << ".r *
    // lightColor" << i << ");";
    svtkShaderProgram::Substitute(fragmentShader, toString1.str(), toString2.str(), false);
    std::ostringstream toString3;
    std::ostringstream toString4;
    toString3 << "specular += (sf * lightColor" << i << ");";
    toString4 << "specular += (sf * factor" << i << ".r * lightColor" << i << ");";
    svtkShaderProgram::Substitute(fragmentShader, toString3.str(), toString4.str(), false);

    // for PBR
    std::ostringstream toString5;
    std::ostringstream toString6;
    toString5 << "radiance = lightColor" << i << ";";
    toString6 << "radiance = factor" << i << ".r * lightColor" << i << ";";
    svtkShaderProgram::Substitute(fragmentShader, toString5.str(), toString6.str(), false);
  }
  return true;
}

void svtkShadowMapPass::BuildShaderCode()
{
  size_t numLights = this->ShadowTextureUnits.size();

  // count how many lights have shadow maps
  int numSMT = 0;
  for (size_t i = 0; i < numLights; i++)
  {
    if (this->ShadowTextureUnits[i] >= 0)
    {
      numSMT++;
    }
  }

  std::ostringstream toString;
  toString.str("");
  toString.clear();
  toString << this->ShadowMapBakerPass->GetResolution();

  std::string fdec =
    "//SVTK::Light::Dec\n"
    "uniform float depthC;\n"
    //"float calcShadow(in vec4 vert,\n"
    "vec2 calcShadow(in vec4 vert,\n"
    "                  in sampler2D shadowMap,\n"
    "                  in mat4 shadowTransform,\n"
    "                  in float attenuation,\n"
    "                  in int shadowParallel,\n"
    "                  in float sNearZ, in float sFarZ)\n"
    "{\n"
    "  vec4 shadowCoord = shadowTransform*vert;\n"
    "  float expFactor = 8.0;\n"
    "  float thickness = 0.0;\n"
    "  if(shadowCoord.w > 0.0)\n"
    "    {\n"
    "    vec2 projected = shadowCoord.xy/shadowCoord.w;\n"
    "    if(projected.x >= 0.0 && projected.x <= 1.0\n"
    "       && projected.y >= 0.0 && projected.y <= 1.0)\n"
    "      {\n"
    "      float ldepth = shadowCoord.z;\n"
    "      if (shadowParallel == 0) { ldepth =  (shadowCoord.w - sNearZ)/(sFarZ - sNearZ); }\n"
    "      float depthCExpActual = exp(- depthC*ldepth);\n"
    "      float depthCExpBlured = texture2D(shadowMap,projected).r;\n"
    "      expFactor = depthCExpBlured * depthCExpActual;\n"
    "      float depth = log(depthCExpBlured)/depthC;\n"
    "      thickness = clamp(ldepth - depth, 0.0, 1.0)*(sFarZ - sNearZ);\n"
    "      if (expFactor > 1.0) { expFactor = 1.0; }\n"
    "      }\n"
    "    }\n"
    "  return vec2(1.0 - attenuation + attenuation*expFactor, thickness);\n"
    "}\n";

  for (int i = 0; i < numSMT; i++)
  {
    toString.str("");
    toString.clear();
    toString << i;
    fdec += "uniform int shadowParallel" + toString.str() +
      ";\n"
      "uniform float shadowNearZ" +
      toString.str() +
      ";\n"
      "uniform float shadowFarZ" +
      toString.str() +
      ";\n"
      "uniform float shadowAttenuation" +
      toString.str() +
      ";\n"
      "uniform sampler2D shadowMap" +
      toString.str() +
      ";\n"
      "uniform mat4 shadowTransform" +
      toString.str() + ";\n";
  }

  // build the code for the lighting factors
  toString.str("");
  toString.clear();
  numSMT = 0;
  for (size_t i = 0; i < numLights; i++)
  {
    //    toString << "float factor" << i << " = ";
    toString << "vec2 factor" << i << " = ";
    if (i < numLights && this->ShadowTextureUnits[i] >= 0)
    {
      std::ostringstream toString2;
      toString2 << numSMT;
      toString << "calcShadow(vertexVC, shadowMap" << toString2.str() << ", shadowTransform"
               << toString2.str() << ", shadowAttenuation" << toString2.str() << ", shadowParallel"
               << toString2.str() << ", shadowNearZ" << toString2.str() << ", shadowFarZ"
               << toString2.str() << ");\n";
      numSMT++;
    }
    else
    {
      toString << "vec2(1.0);\n";
    }
  }

  // compute the factors then do the normal lighting
  toString << "//SVTK::Light::Impl\n";
  this->FragmentDeclaration = fdec;
  this->FragmentImplementation = toString.str();
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkShadowMapPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  if (this->ShadowMapBakerPass != nullptr)
  {
    this->ShadowMapBakerPass->ReleaseGraphicsResources(w);
  }
}
