/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayRendererNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include "svtkOSPRayRendererNode.h"

#include "svtkAbstractVolumeMapper.h"
#include "svtkBoundingBox.h"
#include "svtkCamera.h"
#include "svtkCollectionIterator.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkInformationStringKey.h"
#include "svtkLight.h"
#include "svtkMapper.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkOSPRayActorNode.h"
#include "svtkOSPRayCameraNode.h"
#include "svtkOSPRayLightNode.h"
#include "svtkOSPRayMaterialHelpers.h"
#include "svtkOSPRayMaterialLibrary.h"
#include "svtkOSPRayVolumeNode.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTexture.h"
#include "svtkTransform.h"
#include "svtkViewNodeCollection.h"
#include "svtkVolume.h"
#include "svtkVolumeCollection.h"
#include "svtkWeakPointer.h"

#include "RTWrapper/RTWrapper.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace ospray
{
namespace opengl
{

// code borrowed from ospray::modules::opengl to facilitate updating
// and linking
// todo: use ospray's copy instead of this
inline osp::vec3f operator*(const osp::vec3f& a, const osp::vec3f& b)
{
  return osp::vec3f{ a.x * b.x, a.y * b.y, a.z * b.z };
}
inline osp::vec3f operator*(const osp::vec3f& a, float b)
{
  return osp::vec3f{ a.x * b, a.y * b, a.z * b };
}
inline osp::vec3f operator/(const osp::vec3f& a, float b)
{
  return osp::vec3f{ a.x / b, a.y / b, a.z / b };
}
inline osp::vec3f operator*(float b, const osp::vec3f& a)
{
  return osp::vec3f{ a.x * b, a.y * b, a.z * b };
}
inline osp::vec3f operator*=(osp::vec3f a, float b)
{
  return osp::vec3f{ a.x * b, a.y * b, a.z * b };
}
inline osp::vec3f operator-(const osp::vec3f& a, const osp::vec3f& b)
{
  return osp::vec3f{ a.x - b.x, a.y - b.y, a.z - b.z };
}
inline osp::vec3f operator+(const osp::vec3f& a, const osp::vec3f& b)
{
  return osp::vec3f{ a.x + b.x, a.y + b.y, a.z + b.z };
}
inline osp::vec3f cross(const osp::vec3f& a, const osp::vec3f& b)
{
  return osp::vec3f{ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

inline float dot(const osp::vec3f& a, const osp::vec3f& b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline osp::vec3f normalize(const osp::vec3f& v)
{
  return v / sqrtf(dot(v, v));
}

/*! \brief Compute and return OpenGL depth values from the depth component of the given
  OSPRay framebuffer, using parameters of the current OpenGL context and assuming a
  perspective projection.

  This function automatically determines the parameters of the OpenGL perspective
  projection and camera direction / up vectors. It assumes these values match those
  provided to OSPRay (fovy, aspect, camera direction / up vectors). It then maps the
  OSPRay depth buffer and transforms it to OpenGL depth values according to the OpenGL
  perspective projection.

  The OSPRay frame buffer object must have been constructed with the OSP_FB_DEPTH flag.
*/
OSPTexture getOSPDepthTextureFromOpenGLPerspective(const double& fovy, const double& aspect,
  const double& zNear, const double& zFar, const osp::vec3f& _cameraDir,
  const osp::vec3f& _cameraUp, const float* glDepthBuffer, float* ospDepthBuffer,
  const size_t& glDepthBufferWidth, const size_t& glDepthBufferHeight, RTW::Backend* backend)
{
  osp::vec3f cameraDir = (osp::vec3f&)_cameraDir;
  osp::vec3f cameraUp = (osp::vec3f&)_cameraUp;
  // this should later be done in ISPC...

  // transform OpenGL depth to linear depth
  for (size_t i = 0; i < glDepthBufferWidth * glDepthBufferHeight; i++)
  {
    const double z_n = 2.0 * glDepthBuffer[i] - 1.0;
    ospDepthBuffer[i] = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
    if (svtkMath::IsNan(ospDepthBuffer[i]))
    {
      ospDepthBuffer[i] = FLT_MAX;
    }
  }

  // transform from orthogonal Z depth to ray distance t
  osp::vec3f dir_du = normalize(cross(cameraDir, cameraUp));
  osp::vec3f dir_dv = normalize(cross(dir_du, cameraDir));

  const float imagePlaneSizeY = 2.f * tanf(fovy / 2.f * M_PI / 180.f);
  const float imagePlaneSizeX = imagePlaneSizeY * aspect;

  dir_du *= imagePlaneSizeX;
  dir_dv *= imagePlaneSizeY;

  const osp::vec3f dir_00 = cameraDir - .5f * dir_du - .5f * dir_dv;

  for (size_t j = 0; j < glDepthBufferHeight; j++)
  {
    for (size_t i = 0; i < glDepthBufferWidth; i++)
    {
      const osp::vec3f dir_ij =
        normalize(dir_00 + float(i) / float(glDepthBufferWidth - 1) * dir_du +
          float(j) / float(glDepthBufferHeight - 1) * dir_dv);

      const float t = ospDepthBuffer[j * glDepthBufferWidth + i] / dot(cameraDir, dir_ij);
      ospDepthBuffer[j * glDepthBufferWidth + i] = t;
    }
  }

  // nearest texture filtering required for depth textures -- we don't want interpolation of depth
  // values...
  osp::vec2i texSize = { static_cast<int>(glDepthBufferWidth),
    static_cast<int>(glDepthBufferHeight) };
  OSPTexture depthTexture = svtkOSPRayMaterialHelpers::NewTexture2D(backend, (osp::vec2i&)texSize,
    OSP_TEXTURE_R32F, ospDepthBuffer, OSP_TEXTURE_FILTER_NEAREST, sizeof(float));

  return depthTexture;
}
}
}

svtkInformationKeyMacro(svtkOSPRayRendererNode, SAMPLES_PER_PIXEL, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, MAX_CONTRIBUTION, Double);
svtkInformationKeyMacro(svtkOSPRayRendererNode, MAX_DEPTH, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, MIN_CONTRIBUTION, Double);
svtkInformationKeyMacro(svtkOSPRayRendererNode, ROULETTE_DEPTH, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, VARIANCE_THRESHOLD, Double);
svtkInformationKeyMacro(svtkOSPRayRendererNode, MAX_FRAMES, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, AMBIENT_SAMPLES, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, COMPOSITE_ON_GL, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, RENDERER_TYPE, String);
svtkInformationKeyMacro(svtkOSPRayRendererNode, NORTH_POLE, DoubleVector);
svtkInformationKeyMacro(svtkOSPRayRendererNode, EAST_POLE, DoubleVector);
svtkInformationKeyMacro(svtkOSPRayRendererNode, MATERIAL_LIBRARY, ObjectBase);
svtkInformationKeyMacro(svtkOSPRayRendererNode, VIEW_TIME, Double);
svtkInformationKeyMacro(svtkOSPRayRendererNode, TIME_CACHE_SIZE, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, DENOISER_THRESHOLD, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, ENABLE_DENOISER, Integer);
svtkInformationKeyMacro(svtkOSPRayRendererNode, BACKGROUND_MODE, Integer);

class svtkOSPRayRendererNodeInternals
{
  // todo: move the rest of the internal data here too
public:
  svtkOSPRayRendererNodeInternals(svtkOSPRayRendererNode* _owner)
    : Owner(_owner){};

  ~svtkOSPRayRendererNodeInternals() {}

  bool CanReuseBG(bool forbackplate)
  {
    bool retval = true;
    int index = (forbackplate ? 0 : 1);
    svtkRenderer* ren = svtkRenderer::SafeDownCast(this->Owner->GetRenderable());
    bool useTexture =
      (forbackplate ? ren->GetTexturedBackground() : ren->GetUseImageBasedLighting());
    if (this->lUseTexture[index] != useTexture)
    {
      this->lUseTexture[index] = useTexture;
      retval = false;
    }
    svtkTexture* envTexture =
      (forbackplate ? ren->GetBackgroundTexture() : ren->GetEnvironmentTexture());
    svtkMTimeType envTextureTime = 0;
    if (envTexture)
    {
      envTextureTime = envTexture->GetMTime();
    }
    if (this->lTexture[index] != envTexture || envTextureTime > this->lTextureTime[index])
    {
      this->lTexture[index] = envTexture;
      this->lTextureTime[index] = envTextureTime;
      retval = false;
    }
    bool useGradient =
      (forbackplate ? ren->GetGradientBackground() : ren->GetGradientEnvironmentalBG());
    if (this->lUseGradient[index] != useGradient)
    {
      this->lUseGradient[index] = useGradient;
      retval = false;
    }
    double* color1 = (forbackplate ? ren->GetBackground() : ren->GetEnvironmentalBG());
    double* color2 = (forbackplate ? ren->GetBackground2() : ren->GetEnvironmentalBG2());
    if (this->lColor1[index][0] != color1[0] || this->lColor1[index][1] != color1[1] ||
      this->lColor1[index][2] != color1[2] || this->lColor2[index][0] != color2[0] ||
      this->lColor2[index][1] != color2[1] || this->lColor2[index][2] != color2[2])
    {
      this->lColor1[index][0] = color1[0];
      this->lColor1[index][1] = color1[1];
      this->lColor1[index][2] = color1[2];
      this->lColor2[index][0] = color2[0];
      this->lColor2[index][1] = color2[1];
      this->lColor2[index][2] = color2[2];
      retval = false;
    }
    if (!forbackplate)
    {
      double* up = svtkOSPRayRendererNode::GetNorthPole(ren);
      if (!up)
      {
        up = ren->GetEnvironmentUp();
      }
      if (this->lup[0] != up[0] || this->lup[1] != up[1] || this->lup[2] != up[2])
      {
        this->lup[0] = up[0];
        this->lup[1] = up[1];
        this->lup[2] = up[2];
        retval = false;
      }

      double* east = svtkOSPRayRendererNode::GetEastPole(ren);
      if (!east)
      {
        east = ren->GetEnvironmentRight();
      }
      if (this->least[0] != east[0] || this->least[1] != east[1] || this->least[2] != east[2])
      {
        this->least[0] = east[0];
        this->least[1] = east[1];
        this->least[2] = east[2];
        retval = false;
      }
    }
    return retval;
  }

  bool SetupPathTraceBG(bool forbackplate, RTW::Backend* backend, OSPRenderer oRenderer)
  {
    svtkRenderer* ren = svtkRenderer::SafeDownCast(this->Owner->GetRenderable());
    if (std::string(this->Owner->GetRendererType(ren)).find(std::string("pathtracer")) ==
      std::string::npos)
    {
      return true;
    }
    OSPTexture t2d = nullptr;
    int bgMode = svtkOSPRayRendererNode::GetBackgroundMode(ren);
    bool reuseable = this->CanReuseBG(forbackplate) && (bgMode == this->lBackgroundMode);
    if (!reuseable)
    {
      svtkTexture* text =
        (forbackplate ? ren->GetBackgroundTexture() : ren->GetEnvironmentTexture());
      if (text && (forbackplate ? ren->GetTexturedBackground() : ren->GetUseImageBasedLighting()))
      {
        svtkImageData* vColorTextureMap = text->GetInput();

        // todo: if the imageData is empty, we should download the texture from the GPU
        if (vColorTextureMap)
        {
          t2d = svtkOSPRayMaterialHelpers::SVTKToOSPTexture(backend, vColorTextureMap);
        }
      }

      if (t2d == nullptr)
      {
        std::vector<unsigned char> ochars;
        double* bg1 = (forbackplate ? ren->GetBackground() : ren->GetEnvironmentalBG());
        int isize = 1;
        int jsize = 1;

        if (forbackplate ? ren->GetGradientBackground() : ren->GetGradientEnvironmentalBG())
        {
          double* bg2 = (forbackplate ? ren->GetBackground2() : ren->GetEnvironmentalBG2());
          isize = 256; // todo: configurable
          jsize = 2;
          ochars.resize(isize * jsize * 3);
          unsigned char* oc = ochars.data();
          for (int i = 0; i < isize; i++)
          {
            double frac = (double)i / (double)isize;
            *(oc + 0) = (bg1[0] * (1.0 - frac) + bg2[0] * frac) * 255;
            *(oc + 1) = (bg1[1] * (1.0 - frac) + bg2[1] * frac) * 255;
            *(oc + 2) = (bg1[2] * (1.0 - frac) + bg2[2] * frac) * 255;
            *(oc + 3) = *(oc + 0);
            *(oc + 4) = *(oc + 1);
            *(oc + 5) = *(oc + 2);
            oc += 6;
          }
        }
        else
        {
          ochars.resize(3);
          ochars[0] = bg1[0] * 255;
          ochars[1] = bg1[1] * 255;
          ochars[2] = bg1[2] * 255;
        }

        t2d = svtkOSPRayMaterialHelpers::NewTexture2D(backend, osp::vec2i{ jsize, isize },
          OSP_TEXTURE_RGB8, ochars.data(), 0, 3 * sizeof(char));
      }

      if (forbackplate)
      {
        if (bgMode & 0x1)
        {
          ospSetData(oRenderer, "backplate", t2d);
        }
        else
        {
          ospSetData(oRenderer, "backplate", nullptr);
        }
      }
      else
      {

        OSPLight ospLight = ospNewLight3("hdri");
        ospSetObject(ospLight, "map", t2d);
        ospRelease(t2d);

        double* up = svtkOSPRayRendererNode::GetNorthPole(ren);
        if (!up)
        {
          up = ren->GetEnvironmentUp();
        }
        ospSet3f(ospLight, "up", (float)up[0], (float)up[1], (float)up[2]);

        double* east = svtkOSPRayRendererNode::GetEastPole(ren);
        if (!east)
        {
          east = ren->GetEnvironmentRight();
        }
        ospSet3f(ospLight, "dir", (float)east[0], (float)east[1], (float)east[2]);

        ospCommit(t2d);
        ospCommit(ospLight);
        this->BGLight = ospLight;
      }
    }

    if (!forbackplate && (bgMode & 0x2))
    {
      this->Owner->AddLight(this->BGLight);
    }

    return reuseable;
  }

  std::map<svtkProp3D*, svtkAbstractMapper3D*> LastMapperFor;
  svtkOSPRayRendererNode* Owner;

  int lBackgroundMode = 0;
  double lColor1[2][3] = { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } };
  bool lUseGradient[2] = { false, false };
  double lColor2[2][3] = { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } };
  bool lUseTexture[2] = { false, false };
  svtkWeakPointer<svtkTexture> lTexture[2] = { nullptr, nullptr };
  svtkMTimeType lTextureTime[2] = { 0, 0 };
  double lup[3];
  double least[3];

  double LastViewPort[2];
  double LastParallelScale = 0.0;
  double LastFocalDisk = -1.0;
  double LastFocalDistance = -1.0;

  OSPLight BGLight;
  RTW::Backend* Backend = nullptr;
};

//============================================================================
svtkStandardNewMacro(svtkOSPRayRendererNode);

//----------------------------------------------------------------------------
svtkOSPRayRendererNode::svtkOSPRayRendererNode()
{
  this->ColorBufferTex = 0;
  this->DepthBufferTex = 0;
  this->OModel = nullptr;
  this->ORenderer = nullptr;
  this->OLightArray = nullptr;
  this->NumActors = 0;
  this->ComputeDepth = true;
  this->OFrameBuffer = nullptr;
  this->ImageX = this->ImageY = -1;
  this->CompositeOnGL = false;
  this->Accumulate = true;
  this->AccumulateCount = 0;
  this->ActorCount = 0;
  this->AccumulateTime = 0;
  this->AccumulateMatrix = svtkMatrix4x4::New();
  this->Internal = new svtkOSPRayRendererNodeInternals(this);
  this->PreviousType = "none";

#ifdef SVTKOSPRAY_ENABLE_DENOISER
  this->DenoiserDevice = oidn::newDevice();
  this->DenoiserDevice.commit();
  this->DenoiserFilter = this->DenoiserDevice.newFilter("RT");
#endif
}

//----------------------------------------------------------------------------
svtkOSPRayRendererNode::~svtkOSPRayRendererNode()
{
  if (this->Internal->Backend != nullptr)
  {
    RTW::Backend* backend = this->Internal->Backend;
    ospRelease((OSPModel)this->OModel);
    ospRelease((OSPRenderer)this->ORenderer);
    ospRelease(this->OFrameBuffer);
  }
  this->AccumulateMatrix->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetSamplesPerPixel(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::SAMPLES_PER_PIXEL(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetSamplesPerPixel(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::SAMPLES_PER_PIXEL()))
  {
    return (info->Get(svtkOSPRayRendererNode::SAMPLES_PER_PIXEL()));
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetMaxContribution(double value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::MAX_CONTRIBUTION(), value);
}

//----------------------------------------------------------------------------
double svtkOSPRayRendererNode::GetMaxContribution(svtkRenderer* renderer)
{
  constexpr double DEFAULT_MAX_CONTRIBUTION = 2.0;
  if (!renderer)
  {
    return DEFAULT_MAX_CONTRIBUTION;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::MAX_CONTRIBUTION()))
  {
    return (info->Get(svtkOSPRayRendererNode::MAX_CONTRIBUTION()));
  }
  return DEFAULT_MAX_CONTRIBUTION;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetMaxDepth(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::MAX_DEPTH(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetMaxDepth(svtkRenderer* renderer)
{
  constexpr int DEFAULT_MAX_DEPTH = 20;
  if (!renderer)
  {
    return DEFAULT_MAX_DEPTH;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::MAX_DEPTH()))
  {
    return (info->Get(svtkOSPRayRendererNode::MAX_DEPTH()));
  }
  return DEFAULT_MAX_DEPTH;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetMinContribution(double value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::MIN_CONTRIBUTION(), value);
}

//----------------------------------------------------------------------------
double svtkOSPRayRendererNode::GetMinContribution(svtkRenderer* renderer)
{
  constexpr double DEFAULT_MIN_CONTRIBUTION = 0.01;
  if (!renderer)
  {
    return DEFAULT_MIN_CONTRIBUTION;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::MIN_CONTRIBUTION()))
  {
    return (info->Get(svtkOSPRayRendererNode::MIN_CONTRIBUTION()));
  }
  return DEFAULT_MIN_CONTRIBUTION;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetRouletteDepth(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::ROULETTE_DEPTH(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetRouletteDepth(svtkRenderer* renderer)
{
  constexpr int DEFAULT_ROULETTE_DEPTH = 5;
  if (!renderer)
  {
    return DEFAULT_ROULETTE_DEPTH;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::ROULETTE_DEPTH()))
  {
    return (info->Get(svtkOSPRayRendererNode::ROULETTE_DEPTH()));
  }
  return DEFAULT_ROULETTE_DEPTH;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetVarianceThreshold(double value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::VARIANCE_THRESHOLD(), value);
}

//----------------------------------------------------------------------------
double svtkOSPRayRendererNode::GetVarianceThreshold(svtkRenderer* renderer)
{
  constexpr double DEFAULT_VARIANCE_THRESHOLD = 0.3;
  if (!renderer)
  {
    return DEFAULT_VARIANCE_THRESHOLD;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::VARIANCE_THRESHOLD()))
  {
    return (info->Get(svtkOSPRayRendererNode::VARIANCE_THRESHOLD()));
  }
  return DEFAULT_VARIANCE_THRESHOLD;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetMaterialLibrary(
  svtkOSPRayMaterialLibrary* value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::MATERIAL_LIBRARY(), value);
}

//----------------------------------------------------------------------------
svtkOSPRayMaterialLibrary* svtkOSPRayRendererNode::GetMaterialLibrary(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return nullptr;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::MATERIAL_LIBRARY()))
  {
    svtkObjectBase* obj = info->Get(svtkOSPRayRendererNode::MATERIAL_LIBRARY());
    return (svtkOSPRayMaterialLibrary::SafeDownCast(obj));
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetMaxFrames(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::MAX_FRAMES(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetMaxFrames(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::MAX_FRAMES()))
  {
    return (info->Get(svtkOSPRayRendererNode::MAX_FRAMES()));
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetRendererType(std::string name, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();

#ifdef SVTK_ENABLE_OSPRAY
  if ("scivis" == name || "OSPRay raycaster" == name)
  {
    info->Set(svtkOSPRayRendererNode::RENDERER_TYPE(), "scivis");
  }
  if ("pathtracer" == name || "OSPRay pathtracer" == name)
  {
    info->Set(svtkOSPRayRendererNode::RENDERER_TYPE(), "pathtracer");
  }
#endif

#ifdef SVTK_ENABLE_VISRTX
  if ("optix pathtracer" == name || "OptiX pathtracer" == name)
  {
    info->Set(svtkOSPRayRendererNode::RENDERER_TYPE(), "optix pathtracer");
  }
#endif
}

//----------------------------------------------------------------------------
std::string svtkOSPRayRendererNode::GetRendererType(svtkRenderer* renderer)
{
  if (!renderer)
  {
#ifdef SVTK_ENABLE_OSPRAY
    return std::string("scivis");
#else
    return std::string("optix pathtracer");
#endif
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::RENDERER_TYPE()))
  {
    return (info->Get(svtkOSPRayRendererNode::RENDERER_TYPE()));
  }
#ifdef SVTK_ENABLE_OSPRAY
  return std::string("scivis");
#else
  return std::string("optix pathtracer");
#endif
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetAmbientSamples(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::AMBIENT_SAMPLES(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetAmbientSamples(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::AMBIENT_SAMPLES()))
  {
    return (info->Get(svtkOSPRayRendererNode::AMBIENT_SAMPLES()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetCompositeOnGL(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::COMPOSITE_ON_GL(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetCompositeOnGL(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::COMPOSITE_ON_GL()))
  {
    return (info->Get(svtkOSPRayRendererNode::COMPOSITE_ON_GL()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetNorthPole(double* value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::NORTH_POLE(), value, 3);
}

//----------------------------------------------------------------------------
double* svtkOSPRayRendererNode::GetNorthPole(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return nullptr;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::NORTH_POLE()))
  {
    return (info->Get(svtkOSPRayRendererNode::NORTH_POLE()));
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetEastPole(double* value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::EAST_POLE(), value, 3);
}

//----------------------------------------------------------------------------
double* svtkOSPRayRendererNode::GetEastPole(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return nullptr;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::EAST_POLE()))
  {
    return (info->Get(svtkOSPRayRendererNode::EAST_POLE()));
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetViewTime(double value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::VIEW_TIME(), value);
}

//----------------------------------------------------------------------------
double svtkOSPRayRendererNode::GetViewTime(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::VIEW_TIME()))
  {
    return (info->Get(svtkOSPRayRendererNode::VIEW_TIME()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetTimeCacheSize(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::TIME_CACHE_SIZE(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetTimeCacheSize(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::TIME_CACHE_SIZE()))
  {
    return (info->Get(svtkOSPRayRendererNode::TIME_CACHE_SIZE()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetDenoiserThreshold(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::DENOISER_THRESHOLD(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetDenoiserThreshold(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 4;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::DENOISER_THRESHOLD()))
  {
    return (info->Get(svtkOSPRayRendererNode::DENOISER_THRESHOLD()));
  }
  return 4;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetEnableDenoiser(int value, svtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::ENABLE_DENOISER(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetEnableDenoiser(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::ENABLE_DENOISER()))
  {
    return (info->Get(svtkOSPRayRendererNode::ENABLE_DENOISER()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::SetBackgroundMode(int value, svtkRenderer* renderer)
{
  if (!renderer || value < 0 || value > 3)
  {
    return;
  }
  svtkInformation* info = renderer->GetInformation();
  info->Set(svtkOSPRayRendererNode::BACKGROUND_MODE(), value);
}

//----------------------------------------------------------------------------
int svtkOSPRayRendererNode::GetBackgroundMode(svtkRenderer* renderer)
{
  if (!renderer)
  {
    return 2;
  }
  svtkInformation* info = renderer->GetInformation();
  if (info && info->Has(svtkOSPRayRendererNode::BACKGROUND_MODE()))
  {
    return (info->Get(svtkOSPRayRendererNode::BACKGROUND_MODE()));
  }
  return 2;
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::Traverse(int operation)
{
  // do not override other passes
  if (operation != render)
  {
    this->Superclass::Traverse(operation);
    return;
  }

  this->Apply(operation, true);

  OSPRenderer oRenderer = this->ORenderer;

  // camera
  // TODO: this repeated traversal to find things of particular types
  // is bad, find something smarter
  svtkViewNodeCollection* nodes = this->GetChildren();
  svtkCollectionIterator* it = nodes->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
  {
    svtkOSPRayCameraNode* child = svtkOSPRayCameraNode::SafeDownCast(it->GetCurrentObject());
    if (child)
    {
      child->Traverse(operation);
      break;
    }
    it->GoToNextItem();
  }

  // lights
  this->Lights.clear();
  it->InitTraversal();
  bool hasAmbient = false;
  while (!it->IsDoneWithTraversal())
  {
    svtkOSPRayLightNode* child = svtkOSPRayLightNode::SafeDownCast(it->GetCurrentObject());
    if (child)
    {
      child->Traverse(operation);
      if (child->GetIsAmbient(svtkLight::SafeDownCast(child->GetRenderable())))
      {
        hasAmbient = true;
      }
    }
    it->GoToNextItem();
  }

  RTW::Backend* backend = this->Internal->Backend;
  if (backend == nullptr)
    return;

  if (!hasAmbient && (this->GetAmbientSamples(static_cast<svtkRenderer*>(this->Renderable)) > 0))
  {
    // hardcode an ambient light for AO since OSP 1.2 stopped doing so.
    OSPLight ospAmbient = ospNewLight3("AmbientLight");
    ospSetString(ospAmbient, "name", "default_ambient");
    ospSet3f(ospAmbient, "color", 1.f, 1.f, 1.f);
    ospSet1f(ospAmbient, "intensity", 0.13f * svtkOSPRayLightNode::GetLightScale() * svtkMath::Pi());
    ospCommit(ospAmbient);
    this->Lights.push_back(ospAmbient);
  }

  bool bpreused = this->Internal->SetupPathTraceBG(true, backend, oRenderer);
  bool envreused = this->Internal->SetupPathTraceBG(false, backend, oRenderer);
  this->Internal->lBackgroundMode = svtkOSPRayRendererNode::GetBackgroundMode(
    static_cast<svtkRenderer*>(this->Renderable)); // save it only once both of the above check
  bool bgreused = envreused && bpreused;
  ospRelease(this->OLightArray);
  this->OLightArray = ospNewData(
    this->Lights.size(), OSP_OBJECT, (this->Lights.size() ? &this->Lights[0] : nullptr), 0);
  ospSetData(oRenderer, "lights", this->OLightArray);

  // actors
  OSPModel oModel = nullptr;
  it->InitTraversal();
  // since we have to spatially sort everything
  // let's see if we can avoid that in the common case when
  // the objects have not changed. Note we also cache in actornodes
  // to reuse already created ospray meshes
  svtkMTimeType recent = 0;
  int numAct = 0; // catches removed actors
  while (!it->IsDoneWithTraversal())
  {
    svtkOSPRayActorNode* child = svtkOSPRayActorNode::SafeDownCast(it->GetCurrentObject());
    svtkOSPRayVolumeNode* vchild = svtkOSPRayVolumeNode::SafeDownCast(it->GetCurrentObject());
    if (child)
    {
      numAct++;
      recent = std::max(recent, child->GetMTime());
    }
    if (vchild)
    {
      numAct++;
      recent = std::max(recent, vchild->GetMTime());
    }

    it->GoToNextItem();
  }

  bool enable_cache = true; // turn off to force rebuilds for debugging
  if (!this->OModel || !enable_cache || (recent > this->RenderTime) || (numAct != this->NumActors))
  {
    this->NumActors = numAct;
    ospRelease((OSPModel)this->OModel);
    oModel = ospNewModel();
    this->OModel = oModel;
    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
    {
      svtkOSPRayActorNode* child = svtkOSPRayActorNode::SafeDownCast(it->GetCurrentObject());
      if (child)
      {
        child->Traverse(operation);
      }
      svtkOSPRayVolumeNode* vchild = svtkOSPRayVolumeNode::SafeDownCast(it->GetCurrentObject());
      if (vchild)
      {
        vchild->Traverse(operation);
      }
      it->GoToNextItem();
    }
    this->RenderTime = recent;
    ospCommit(oModel);
    ospSetObject(oRenderer, "model", oModel);
    ospCommit(oRenderer);
  }
  else
  {
    oModel = (OSPModel)this->OModel;
    ospSetObject(oRenderer, "model", oModel);
    ospCommit(oRenderer);
  }
  it->Delete();

  if (!bgreused)
  {
    // hack to ensure progressive rendering resets when background changes
    this->AccumulateTime = 0;
  }
  this->Apply(operation, false);
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::Build(bool prepass)
{
  if (prepass)
  {
    svtkRenderer* aren = svtkRenderer::SafeDownCast(this->Renderable);
    // make sure we have a camera
    if (!aren->IsActiveCameraCreated())
    {
      aren->ResetCamera();
    }
  }
  this->Superclass::Build(prepass);
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::Render(bool prepass)
{
  svtkRenderer* ren = svtkRenderer::SafeDownCast(this->GetRenderable());
  if (!ren)
  {
    return;
  }

  RTW::Backend* backend = this->Internal->Backend;

  if (prepass)
  {
    OSPRenderer oRenderer = nullptr;

    std::string type = this->GetRendererType(static_cast<svtkRenderer*>(this->Renderable));
    if (!this->ORenderer || this->PreviousType != type)
    {
      this->Traverse(invalidate);
      // std::cerr << "initializing backend...\n";
      // std::set<RTWBackendType> availableBackends = rtwGetAvailableBackends();
      // for (RTWBackendType backend : availableBackends)
      //{
      //    switch (backend)
      //    {
      //    case OSP_BACKEND_OSPRAY:
      //        std::cerr << "OSPRay backend is available\n";
      //        break;
      //    case OSP_BACKEND_VISRTX:
      //        std::cerr << "VisRTX backend is available\n";
      //        break;
      //    default:
      //        std::cerr << "Unknown backend type listed as available \n";
      //    }
      //}
      this->Internal->Backend = rtwSwitch(type.c_str());
      if (this->Internal->Backend == nullptr)
      {
        return;
      }
      backend = this->Internal->Backend;

      oRenderer = ospNewRenderer(type.c_str());
      this->ORenderer = oRenderer;
      this->PreviousType = type;
    }
    else
    {
      oRenderer = this->ORenderer;
    }

    ospSet1f(this->ORenderer, "maxContribution", this->GetMaxContribution(ren));
    ospSet1f(this->ORenderer, "minContribution", this->GetMinContribution(ren));
    ospSet1i(this->ORenderer, "maxDepth", this->GetMaxDepth(ren));
    ospSet1i(this->ORenderer, "rouletteDepth", this->GetRouletteDepth(ren));
    ospSet1f(this->ORenderer, "varianceThreshold", this->GetVarianceThreshold(ren));
    ospCommit(this->ORenderer);

    if (ren->GetUseShadows())
    {
      ospSet1i(oRenderer, "shadowsEnabled", 1);
    }
    else
    {
      ospSet1i(oRenderer, "shadowsEnabled", 0);
    }

    // todo: this can be expensive and should be cached
    // also the user might want to control
    svtkBoundingBox bbox(ren->ComputeVisiblePropBounds());
    if (bbox.IsValid())
    {
      float diam = static_cast<float>(bbox.GetDiagonalLength());
      float logDiam = log(diam);
      if (logDiam < 0.f)
      {
        logDiam = 1.f / (fabs(logDiam));
      }
      float epsilon = 1e-5 * logDiam;
      ospSet1f(oRenderer, "epsilon", epsilon);
      ospSet1f(oRenderer, "aoDistance", diam * 0.3);
      ospSet1i(oRenderer, "autoEpsilon", 0);
    }
    else
    {
      ospSet1f(oRenderer, "epsilon", 0.001f);
    }

    svtkVolumeCollection* vc = ren->GetVolumes();
    if (vc->GetNumberOfItems())
    {
      ospSet1i(oRenderer, "aoTransparencyEnabled", 1);
    }

    ospSet1i(oRenderer, "aoSamples", this->GetAmbientSamples(ren));
    ospSet1i(oRenderer, "spp", this->GetSamplesPerPixel(ren));
    this->CompositeOnGL = (this->GetCompositeOnGL(ren) != 0);

    double* bg = ren->GetBackground();
    ospSet4f(oRenderer, "bgColor", bg[0], bg[1], bg[2], ren->GetBackgroundAlpha());
  }
  else
  {
    OSPRenderer oRenderer = this->ORenderer;
    ospCommit(oRenderer);

    osp::vec2i isize = { this->Size[0], this->Size[1] };
    if (this->ImageX != this->Size[0] || this->ImageY != this->Size[1])
    {
      this->ImageX = this->Size[0];
      this->ImageY = this->Size[1];
      const size_t size = this->ImageX * this->ImageY;
      ospRelease(this->OFrameBuffer);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
      this->OFrameBuffer = ospNewFrameBuffer(isize,
#ifdef SVTKOSPRAY_ENABLE_DENOISER
        OSP_FB_RGBA32F,
#else
        OSP_FB_RGBA8,
#endif
        OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0) |
          (this->Accumulate ? OSP_FB_ACCUM : 0)
#ifdef SVTKOSPRAY_ENABLE_DENOISER
          | OSP_FB_NORMAL | OSP_FB_ALBEDO
#endif
      );
      this->DenoisedBuffer.resize(size);
      this->ColorBuffer.resize(size);
      this->NormalBuffer.resize(size);
      this->AlbedoBuffer.resize(size);
      this->DenoiserDirty = true;
      ospSet1f(this->OFrameBuffer, "gamma", 1.0f);
      ospCommit(this->OFrameBuffer);
      ospFrameBufferClear(this->OFrameBuffer,
        OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0) |
          (this->Accumulate ? OSP_FB_ACCUM : 0));
#pragma GCC diagnostic pop
      this->Buffer.resize(this->Size[0] * this->Size[1] * 4);
      this->ZBuffer.resize(this->Size[0] * this->Size[1]);
      if (this->CompositeOnGL)
      {
        this->ODepthBuffer.resize(this->Size[0] * this->Size[1]);
      }
    }
    else if (this->Accumulate)
    {
      // check if something has changed
      // if so we clear and start over, otherwise we continue to accumulate
      bool canReuse = true;

      // TODO: these all need some work as checks are not necessarily fast
      // nor sufficient for all cases that matter

      // check for stereo and disable so don't get left in right
      svtkRenderWindow* rwin = svtkRenderWindow::SafeDownCast(ren->GetSVTKWindow());
      if (rwin && rwin->GetStereoRender())
      {
        canReuse = false;
      }

      // check for tiling, ie typically putting together large images to save high res pictures
      double* vp = rwin->GetTileViewport();
      if (this->Internal->LastViewPort[0] != vp[0] || this->Internal->LastViewPort[1] != vp[1])
      {
        canReuse = false;
        this->Internal->LastViewPort[0] = vp[0];
        this->Internal->LastViewPort[1] = vp[1];
      }

      // check actors (and time)
      svtkMTimeType m = 0;
      svtkActorCollection* ac = ren->GetActors();
      int nitems = ac->GetNumberOfItems();
      if (nitems != this->ActorCount)
      {
        // TODO: need a hash or something to really check for added/deleted
        this->ActorCount = nitems;
        this->AccumulateCount = 0;
        canReuse = false;
      }
      if (canReuse)
      {
        ac->InitTraversal();
        svtkActor* nac = ac->GetNextActor();
        while (nac)
        {
          if (nac->GetRedrawMTime() > m)
          {
            m = nac->GetRedrawMTime();
          }
          if (this->Internal->LastMapperFor[nac] != nac->GetMapper())
          {
            // a check to ensure svtkPVLODActor restarts on LOD swap
            this->Internal->LastMapperFor[nac] = nac->GetMapper();
            canReuse = false;
          }
          nac = ac->GetNextActor();
        }
        if (this->AccumulateTime < m)
        {
          this->AccumulateTime = m;
          canReuse = false;
        }
      }

      if (canReuse)
      {
        m = 0;
        svtkVolumeCollection* vc = ren->GetVolumes();
        vc->InitTraversal();
        svtkVolume* nvol = vc->GetNextVolume();
        while (nvol)
        {
          if (nvol->GetRedrawMTime() > m)
          {
            m = nvol->GetRedrawMTime();
          }
          if (this->Internal->LastMapperFor[nvol] != nvol->GetMapper())
          {
            // a check to ensure svtkPVLODActor restarts on LOD swap
            this->Internal->LastMapperFor[nvol] = nvol->GetMapper();
            canReuse = false;
          }
          nvol = vc->GetNextVolume();
        };
        if (this->AccumulateTime < m)
        {
          this->AccumulateTime = m;
          canReuse = false;
        }
      }

      if (canReuse)
      {
        // check camera
        // Why not cam->mtime?
        // cam->mtime is bumped by synch after this in parallel so never reuses
        // Why not cam->MVTO->mtime?
        //  cam set's elements directly, so the mtime doesn't bump with motion
        svtkMatrix4x4* camnow = ren->GetActiveCamera()->GetModelViewTransformObject()->GetMatrix();
        for (int i = 0; i < 4; i++)
        {
          for (int j = 0; j < 4; j++)
          {
            if (this->AccumulateMatrix->GetElement(i, j) != camnow->GetElement(i, j))
            {
              this->AccumulateMatrix->DeepCopy(camnow);
              canReuse = false;
              i = 4;
              j = 4;
            }
          }
        }
        if (this->Internal->LastParallelScale != ren->GetActiveCamera()->GetParallelScale())
        {
          this->Internal->LastParallelScale = ren->GetActiveCamera()->GetParallelScale();
          canReuse = false;
        }

        if (this->Internal->LastFocalDisk != ren->GetActiveCamera()->GetFocalDisk())
        {
          this->Internal->LastFocalDisk = ren->GetActiveCamera()->GetFocalDisk();
          canReuse = false;
        }

        if (this->Internal->LastFocalDistance != ren->GetActiveCamera()->GetFocalDistance())
        {
          this->Internal->LastFocalDistance = ren->GetActiveCamera()->GetFocalDistance();
          canReuse = false;
        }
      }
      if (!canReuse)
      {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
        ospFrameBufferClear(this->OFrameBuffer,
          OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0) | OSP_FB_ACCUM);
#pragma GCC diagnostic pop
        this->AccumulateCount = 0;
      }
    }
    else if (!this->Accumulate)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
      ospFrameBufferClear(
        this->OFrameBuffer, OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0));
#pragma GCC diagnostic pop
    }

    svtkCamera* cam = svtkRenderer::SafeDownCast(this->Renderable)->GetActiveCamera();

    ospSet1i(oRenderer, "backgroundEnabled", ren->GetErase());
    if (this->CompositeOnGL && backend->IsSupported(RTW_DEPTH_COMPOSITING))
    {
      svtkRenderWindow* rwin = svtkRenderWindow::SafeDownCast(ren->GetSVTKWindow());
      int viewportX, viewportY;
      int viewportWidth, viewportHeight;
      ren->GetTiledSizeAndOrigin(&viewportWidth, &viewportHeight, &viewportX, &viewportY);
      rwin->GetZbufferData(viewportX, viewportY, viewportX + viewportWidth - 1,
        viewportY + viewportHeight - 1, this->GetZBuffer());

      double zNear, zFar;
      double fovy, aspect;
      fovy = cam->GetViewAngle();
      aspect = double(viewportWidth) / double(viewportHeight);
      cam->GetClippingRange(zNear, zFar);
      double camUp[3];
      double camDir[3];
      cam->GetViewUp(camUp);
      cam->GetFocalPoint(camDir);
      osp::vec3f cameraUp = { static_cast<float>(camUp[0]), static_cast<float>(camUp[1]),
        static_cast<float>(camUp[2]) };
      osp::vec3f cameraDir = { static_cast<float>(camDir[0]), static_cast<float>(camDir[1]),
        static_cast<float>(camDir[2]) };
      double cameraPos[3];
      cam->GetPosition(cameraPos);
      cameraDir.x -= cameraPos[0];
      cameraDir.y -= cameraPos[1];
      cameraDir.z -= cameraPos[2];
      cameraDir = ospray::opengl::normalize(cameraDir);

      OSPTexture glDepthTex = ospray::opengl::getOSPDepthTextureFromOpenGLPerspective(fovy, aspect,
        zNear, zFar, (osp::vec3f&)cameraDir, (osp::vec3f&)cameraUp, this->GetZBuffer(),
        this->ODepthBuffer.data(), viewportWidth, viewportHeight, this->Internal->Backend);

      ospSetObject(oRenderer, "maxDepthTexture", glDepthTex);
    }
    else
    {
      ospSetObject(oRenderer, "maxDepthTexture", 0);
    }

    // Enable VisRTX denoiser
    this->AccumulateCount += this->GetSamplesPerPixel(ren);
    bool useDenoiser =
      this->GetEnableDenoiser(ren) && (this->AccumulateCount >= this->GetDenoiserThreshold(ren));
    ospSet1i(oRenderer, "denoise", useDenoiser ? 1 : 0); // for VisRTX backend only

    ospCommit(oRenderer);

    const bool backendDepthNormalization = backend->IsSupported(RTW_DEPTH_NORMALIZATION);
    if (backendDepthNormalization)
    {
      const double* clipValues = cam->GetClippingRange();
      const double clipMin = clipValues[0];
      const double clipMax = clipValues[1];
      backend->SetDepthNormalizationGL(this->OFrameBuffer, clipMin, clipMax);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
    ospRenderFrame(this->OFrameBuffer, oRenderer,
      OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0) | (this->Accumulate ? OSP_FB_ACCUM : 0)
#ifdef SVTKOSPRAY_ENABLE_DENOISER
        | OSP_FB_NORMAL | OSP_FB_ALBEDO
#endif
    );
#pragma GCC diagnostic pop

    // Check if backend can do direct OpenGL display using textures
    bool useOpenGLInterop = backend->IsSupported(RTW_OPENGL_INTEROP);

    // Only layer 0 can currently display using OpenGL
    if (ren->GetLayer() != 0)
      useOpenGLInterop = false;

    if (useOpenGLInterop)
    {
      // Check if we actually have an OpenGL window
      svtkRenderWindow* rwin = svtkRenderWindow::SafeDownCast(ren->GetSVTKWindow());
      svtkOpenGLRenderWindow* windowOpenGL = svtkOpenGLRenderWindow::SafeDownCast(rwin);
      if (windowOpenGL != nullptr)
      {
        windowOpenGL->MakeCurrent();
        this->ColorBufferTex = backend->GetColorTextureGL(this->OFrameBuffer);
        this->DepthBufferTex = backend->GetDepthTextureGL(this->OFrameBuffer);

        useOpenGLInterop = (this->ColorBufferTex != 0 && this->DepthBufferTex != 0);
      }
      else
      {
        useOpenGLInterop = false;
      }
    }

    if (!useOpenGLInterop)
    {
      const void* rgba = ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_COLOR);
#ifdef SVTKOSPRAY_ENABLE_DENOISER
      const osp::vec4f* rgba4f = reinterpret_cast<const osp::vec4f*>(rgba);
      this->ColorBuffer.assign(rgba4f, rgba4f + this->Size[0] * this->Size[1]);
      if (useDenoiser)
      {
        this->Denoise();
      }
      const float* color = reinterpret_cast<const float*>(this->ColorBuffer.data());
      this->Buffer.assign(color, color + this->ImageX * this->ImageY * 4);
#else
      const unsigned char* rgbauc = reinterpret_cast<const unsigned char*>(rgba);
      this->Buffer.assign(rgbauc, rgbauc + this->Size[0] * this->Size[1] * 4);
#endif
      ospUnmapFrameBuffer(rgba, this->OFrameBuffer);

      if (this->ComputeDepth)
      {
        const float* Z =
          reinterpret_cast<const float*>(ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_DEPTH));

        if (backendDepthNormalization)
        {
          this->ZBuffer.assign(Z, Z + this->Size[0] * this->Size[1]);
        }
        else
        {
          double* clipValues = cam->GetClippingRange();
          double clipMin = clipValues[0];
          double clipMax = clipValues[1];
          double clipDiv = 1.0 / (clipMax - clipMin);

          const float* s = Z;
          float* d = this->ZBuffer.data();
          for (int i = 0; i < (this->Size[0] * this->Size[1]); i++, s++, d++)
          {
            *d = (*s < clipMin ? 1.0 : (*s - clipMin) * clipDiv);
          }
        }

        ospUnmapFrameBuffer(Z, this->OFrameBuffer);
      }
    }
  }
}

void svtkOSPRayRendererNode::Denoise()
{
#ifdef SVTKOSPRAY_ENABLE_DENOISER
  RTW::Backend* backend = this->Internal->Backend;
  this->DenoisedBuffer = this->ColorBuffer;
  if (this->DenoiserDirty)
  {
    this->DenoiserFilter.setImage("color", (void*)this->ColorBuffer.data(), oidn::Format::Float3,
      this->ImageX, this->ImageY, 0, sizeof(osp::vec4f));

    this->DenoiserFilter.setImage("normal", (void*)this->NormalBuffer.data(), oidn::Format::Float3,
      this->ImageX, this->ImageY, 0, sizeof(osp::vec3f));

    this->DenoiserFilter.setImage("albedo", (void*)this->AlbedoBuffer.data(), oidn::Format::Float3,
      this->ImageX, this->ImageY, 0, sizeof(osp::vec3f));

    this->DenoiserFilter.setImage("output", (void*)this->DenoisedBuffer.data(),
      oidn::Format::Float3, this->ImageX, this->ImageY, 0, sizeof(osp::vec4f));

    this->DenoiserFilter.commit();
    this->DenoiserDirty = false;
  }

  const auto size = this->ImageX * this->ImageY;
  const osp::vec4f* rgba =
    static_cast<const osp::vec4f*>(ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_COLOR));
  std::copy(rgba, rgba + size, this->ColorBuffer.begin());
  ospUnmapFrameBuffer(rgba, this->OFrameBuffer);
  const osp::vec3f* normal =
    static_cast<const osp::vec3f*>(ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_NORMAL));
  std::copy(normal, normal + size, this->NormalBuffer.begin());
  ospUnmapFrameBuffer(normal, this->OFrameBuffer);
  const osp::vec3f* albedo =
    static_cast<const osp::vec3f*>(ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_ALBEDO));
  std::copy(albedo, albedo + size, this->AlbedoBuffer.begin());
  ospUnmapFrameBuffer(albedo, this->OFrameBuffer);

  this->DenoiserFilter.execute();
  // Carson: not sure we need two buffers
  this->ColorBuffer = this->DenoisedBuffer;
#endif
}

//----------------------------------------------------------------------------
void svtkOSPRayRendererNode::WriteLayer(
  unsigned char* buffer, float* Z, int buffx, int buffy, int layer)
{
  if (layer == 0)
  {
    for (int j = 0; j < buffy && j < this->Size[1]; j++)
    {
#ifdef SVTKOSPRAY_ENABLE_DENOISER
      float* iptr = this->Buffer.data() + j * this->Size[0] * 4;
#else
      unsigned char* iptr = this->Buffer.data() + j * this->Size[0] * 4;
#endif
      float* zptr = this->ZBuffer.data() + j * this->Size[0];
      unsigned char* optr = buffer + j * buffx * 4;
      float* ozptr = Z + j * buffx;
      for (int i = 0; i < buffx && i < this->Size[0]; i++)
      {
#ifdef SVTKOSPRAY_ENABLE_DENOISER
        *optr++ = static_cast<unsigned char>(svtkMath::ClampValue(*iptr++, 0.f, 1.f) * 255.f);
        *optr++ = static_cast<unsigned char>(svtkMath::ClampValue(*iptr++, 0.f, 1.f) * 255.f);
        *optr++ = static_cast<unsigned char>(svtkMath::ClampValue(*iptr++, 0.f, 1.f) * 255.f);
        *optr++ = static_cast<unsigned char>(svtkMath::ClampValue(*iptr++, 0.f, 1.f) * 255.f);
#else
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *optr++ = *iptr++;
#endif
        *ozptr++ = *zptr;
        zptr++;
      }
    }
  }
  else
  {
    for (int j = 0; j < buffy && j < this->Size[1]; j++)
    {
#ifdef SVTKOSPRAY_ENABLE_DENOISER
      float* iptr = this->Buffer.data() + j * this->Size[0] * 4;
#else
      unsigned char* iptr = this->Buffer.data() + j * this->Size[0] * 4;
#endif
      float* zptr = this->ZBuffer.data() + j * this->Size[0];
      unsigned char* optr = buffer + j * buffx * 4;
      float* ozptr = Z + j * buffx;
      for (int i = 0; i < buffx && i < this->Size[0]; i++)
      {
        if (*zptr < 1.0)
        {
          if (this->CompositeOnGL)
          {
            // ospray is cooperating with GL (osprayvolumemapper)
#ifdef SVTKOSPRAY_ENABLE_DENOISER
            float A = iptr[3];
            for (int h = 0; h < 3; h++)
            {
              *optr = static_cast<unsigned char>(
                (*iptr * 255.f) * (1 - A) + static_cast<float>(*optr) * (A));
              optr++;
              iptr++;
            }
#else
            float A = iptr[3] / 255.f;
            for (int h = 0; h < 3; h++)
            {
              *optr = static_cast<unsigned char>(
                static_cast<float>(*iptr) * (1 - A) + static_cast<float>(*optr) * (A));
              optr++;
              iptr++;
            }
#endif
            optr++;
            iptr++;
          }
          else
          {
            // ospray owns all layers in window
#ifdef SVTKOSPRAY_ENABLE_DENOISER
            *optr++ = static_cast<unsigned char>(svtkMath::ClampValue(*iptr++, 0.f, 1.f) * 255.f);
            *optr++ = static_cast<unsigned char>(svtkMath::ClampValue(*iptr++, 0.f, 1.f) * 255.f);
            *optr++ = static_cast<unsigned char>(svtkMath::ClampValue(*iptr++, 0.f, 1.f) * 255.f);
            *optr++ = static_cast<unsigned char>(svtkMath::ClampValue(*iptr++, 0.f, 1.f) * 255.f);
#else
            *optr++ = *iptr++;
            *optr++ = *iptr++;
            *optr++ = *iptr++;
            *optr++ = *iptr++;
#endif
          }
          *ozptr = *zptr;
        }
        else
        {
          optr += 4;
          iptr += 4;
        }
        ozptr++;
        zptr++;
      }
    }
  }
}

//------------------------------------------------------------------------------
svtkRenderer* svtkOSPRayRendererNode::GetRenderer()
{
  return svtkRenderer::SafeDownCast(this->GetRenderable());
}

//------------------------------------------------------------------------------
svtkOSPRayRendererNode* svtkOSPRayRendererNode::GetRendererNode(svtkViewNode* self)
{
  return static_cast<svtkOSPRayRendererNode*>(self->GetFirstAncestorOfType("svtkOSPRayRendererNode"));
}

//------------------------------------------------------------------------------
RTW::Backend* svtkOSPRayRendererNode::GetBackend()
{
  return this->Internal->Backend;
}
