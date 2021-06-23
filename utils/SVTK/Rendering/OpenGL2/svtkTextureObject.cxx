/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextureObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTextureObject.h"

#include "svtk_glew.h"

#include "svtkObjectFactory.h"

#include "svtkNew.h"
#include "svtkOpenGLBufferObject.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLResourceFreeCallback.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLTexture.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkPixelBufferObject.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"

#include "svtkOpenGLHelper.h"

#include <cassert>

//#define SVTK_TO_DEBUG
//#define SVTK_TO_TIMING

#ifdef SVTK_TO_TIMING
#include "svtkTimerLog.h"
#endif

#include "svtkTextureObjectFS.h"
#include "svtkTextureObjectVS.h" // a pass through shader

#define BUFFER_OFFSET(i) (reinterpret_cast<char*>(i))

// Mapping from DepthTextureCompareFunction values to OpenGL values.
//----------------------------------------------------------------------------
static GLint OpenGLDepthTextureCompareFunction[8] = { GL_LEQUAL, GL_GEQUAL, GL_LESS, GL_GREATER,
  GL_EQUAL, GL_NOTEQUAL, GL_ALWAYS, GL_NEVER };

//----------------------------------------------------------------------------
static const char* DepthTextureCompareFunctionAsString[8] = { "Lequal", "Gequal", "Less", "Greater",
  "Equal", "NotEqual", "AlwaysTrue", "Never" };

// Mapping from Wrap values to OpenGL values
#ifndef GL_ES_VERSION_3_0
//--------------------------------------------------------------------------
static GLint OpenGLWrap[4] = { GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRRORED_REPEAT,
  GL_CLAMP_TO_BORDER };

//--------------------------------------------------------------------------
static const char* WrapAsString[4] = { "ClampToEdge", "Repeat", "MirroredRepeat", "ClampToBorder" };

#else
//--------------------------------------------------------------------------
static GLint OpenGLWrap[3] = { GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRRORED_REPEAT };

//--------------------------------------------------------------------------
static const char* WrapAsString[3] = { "ClampToEdge", "Repeat", "MirroredRepeat" };

#endif

// Mapping MinificationFilter values to OpenGL values.
//----------------------------------------------------------------------------
static GLint OpenGLMinFilter[6] = { GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
  GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR };

// Mapping MagnificationFilter values to OpenGL values.
//----------------------------------------------------------------------------
static GLint OpenGLMagFilter[6] = { GL_NEAREST, GL_LINEAR };

//----------------------------------------------------------------------------
static const char* MinMagFilterAsString[6] = { "Nearest", "Linear", "NearestMipmapNearest",
  "NearestMipmapLinear", "LinearMipmapNearest", "LinearMipmapLinear" };

//----------------------------------------------------------------------------
static GLenum OpenGLDepthInternalFormat[7] = {
  GL_DEPTH_COMPONENT,   // native
  GL_DEPTH_COMPONENT,   // fixed8
  GL_DEPTH_COMPONENT16, // fixed16
#ifdef GL_DEPTH_COMPONENT24
  GL_DEPTH_COMPONENT24, // fixed24
#else
  GL_DEPTH_COMPONENT16,
#endif
#ifdef GL_DEPTH_COMPONENT32
  GL_DEPTH_COMPONENT32, // fixed32
#else
  GL_DEPTH_COMPONENT16,
#endif
#ifdef GL_DEPTH_COMPONENT32F
  GL_DEPTH_COMPONENT32F, // float16
  GL_DEPTH_COMPONENT32F  // float32
#else
  GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT16
#endif
};

//----------------------------------------------------------------------------
static GLenum OpenGLDepthInternalFormatType[7] = {
  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT,
#ifdef GL_DEPTH_COMPONENT32F
  GL_FLOAT, GL_FLOAT
#else
  GL_UNSIGNED_INT, GL_UNSIGNED_INT
#endif
};

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkTextureObject);

//----------------------------------------------------------------------------
svtkTextureObject::svtkTextureObject()
{
  this->Context = nullptr;
  this->Handle = 0;
  this->OwnHandle = false;
  this->NumberOfDimensions = 0;
  this->Target = 0;
  this->Components = 0;
  this->Width = 0;
  this->Height = 0;
  this->Depth = 0;
  this->Samples = 0;
  this->RequireTextureInteger = false;
  this->SupportsTextureInteger = false;
  this->RequireTextureFloat = false;
  this->SupportsTextureFloat = false;
  this->RequireDepthBufferFloat = false;
  this->SupportsDepthBufferFloat = false;
  this->AutoParameters = 1;
  this->WrapS = Repeat;
  this->WrapT = Repeat;
  this->WrapR = Repeat;
  this->MinificationFilter = Nearest;
  this->MagnificationFilter = Nearest;
  this->MinLOD = -1000.0f;
  this->MaxLOD = 1000.0f;
  this->BaseLevel = 0;
  this->MaxLevel = 0;
  this->DepthTextureCompare = false;
  this->DepthTextureCompareFunction = Lequal;
  this->GenerateMipmap = false;
  this->ShaderProgram = nullptr;
  this->BorderColor[0] = 0.0f;
  this->BorderColor[1] = 0.0f;
  this->BorderColor[2] = 0.0f;
  this->BorderColor[3] = 0.0f;
  this->MaximumAnisotropicFiltering = 1.0;
  this->BufferObject = nullptr;
  this->UseSRGBColorSpace = false;

  this->ResourceCallback = new svtkOpenGLResourceFreeCallback<svtkTextureObject>(
    this, &svtkTextureObject::ReleaseGraphicsResources);

  this->ResetFormatAndType();
}

//----------------------------------------------------------------------------
svtkTextureObject::~svtkTextureObject()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = nullptr;
  }
  if (this->ShaderProgram)
  {
    delete this->ShaderProgram;
    this->ShaderProgram = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkTextureObject::SetContext(svtkOpenGLRenderWindow* renWin)
{
  this->ResourceCallback->RegisterGraphicsResources(renWin);

  // avoid pointless reassignment
  if (this->Context == renWin)
  {
    return;
  }

  this->ResetFormatAndType();

  this->Context = nullptr;
  this->Modified();
  // all done if assigned null
  if (!renWin)
  {
    return;
  }

  // initialize
  this->Context = renWin;
  this->Context->MakeCurrent();
}

//----------------------------------------------------------------------------
svtkOpenGLRenderWindow* svtkTextureObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void svtkTextureObject::DestroyTexture()
{
  // deactivate it first
  this->Deactivate();

  // because we don't hold a reference to the render
  // context we don't have any control on when it is
  // destroyed. In fact it may be destroyed before
  // we are(eg smart pointers), in which case we should
  // do nothing.
  if (this->Context && this->Handle)
  {
    GLuint tex = this->Handle;
    glDeleteTextures(1, &tex);
    svtkOpenGLCheckErrorMacro("failed at glDeleteTexture");
  }
  this->Handle = 0;
  this->NumberOfDimensions = 0;
  this->Target = 0;
  this->Components = 0;
  this->Width = this->Height = this->Depth = 0;
  this->ResetFormatAndType();
}

void svtkTextureObject::AssignToExistingTexture(unsigned int handle, unsigned int target)
{
  if (this->Handle == handle && this->Target == target)
  {
    return;
  }

  this->Handle = handle;
  this->Target = target;
  this->OwnHandle = false;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkTextureObject::CreateTexture()
{
  assert(this->Context);

  this->ResourceCallback->RegisterGraphicsResources(this->Context);

  // reuse the existing handle if we have one
  if (!this->Handle)
  {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    this->OwnHandle = true;
    svtkOpenGLCheckErrorMacro("failed at glGenTextures");
    this->Handle = tex;

#if defined(GL_TEXTURE_BUFFER)
    if (this->Target && this->Target != GL_TEXTURE_BUFFER)
#else
    if (this->Target)
#endif
    {
      glBindTexture(this->Target, this->Handle);
      svtkOpenGLCheckErrorMacro("failed at glBindTexture");

      // See: http://www.opengl.org/wiki/Common_Mistakes#Creating_a_complete_texture
      // turn off mip map filter or set the base and max level correctly. here
      // both are done.
#ifdef GL_TEXTURE_2D_MULTISAMPLE
      if (this->Target != GL_TEXTURE_2D_MULTISAMPLE)
#endif
      {
        glTexParameteri(this->Target, GL_TEXTURE_MIN_FILTER,
          this->GetMinificationFilterMode(this->MinificationFilter));
        glTexParameteri(this->Target, GL_TEXTURE_MAG_FILTER,
          this->GetMagnificationFilterMode(this->MagnificationFilter));

        glTexParameteri(this->Target, GL_TEXTURE_WRAP_S, this->GetWrapSMode(this->WrapS));
        glTexParameteri(this->Target, GL_TEXTURE_WRAP_T, this->GetWrapTMode(this->WrapT));

#if defined(GL_TEXTURE_3D)
        if (this->Target == GL_TEXTURE_3D)
        {
          glTexParameteri(this->Target, GL_TEXTURE_WRAP_R, this->GetWrapRMode(this->WrapR));
        }
#endif
      }

      if (this->Target == GL_TEXTURE_2D) // maybe expand later on
      {
        glTexParameteri(this->Target, GL_TEXTURE_BASE_LEVEL, this->BaseLevel);
        glTexParameteri(this->Target, GL_TEXTURE_MAX_LEVEL, this->MaxLevel);
      }

      glBindTexture(this->Target, 0);
    }
  }
}

//---------------------------------------------------------------------------
int svtkTextureObject::GetTextureUnit()
{
  if (this->Context)
  {
    return this->Context->GetTextureUnitForTexture(this);
  }
  return -1;
}

//---------------------------------------------------------------------------
void svtkTextureObject::Activate()
{
  // activate a free texture unit for this texture
  this->Context->ActivateTexture(this);
  this->Bind();
}

//---------------------------------------------------------------------------
void svtkTextureObject::Deactivate()
{
  if (this->Context)
  {
    this->Context->DeactivateTexture(this);
  }
}

//---------------------------------------------------------------------------
void svtkTextureObject::ReleaseGraphicsResources(svtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  // Ensure that the context is current before releasing any graphics
  // resources tied to it.
  if (this->Handle)
  {
    svtkOpenGLRenderWindow* rwin = svtkOpenGLRenderWindow::SafeDownCast(win);
    // you can commewnt out the next line to look for textures left active
    rwin->DeactivateTexture(this);
    if (this->OwnHandle)
    {
      GLuint tex = this->Handle;
      glDeleteTextures(1, &tex);
      this->OwnHandle = false;
    }
    this->Handle = 0;
    this->NumberOfDimensions = 0;
    this->Target = 0;
    this->InternalFormat = 0;
    this->Format = 0;
    this->Type = 0;
    this->Components = 0;
    this->Width = this->Height = this->Depth = 0;
  }
  if (this->ShaderProgram)
  {
    this->ShaderProgram->ReleaseGraphicsResources(win);
    delete this->ShaderProgram;
    this->ShaderProgram = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkTextureObject::Bind()
{
  assert(this->Context);
  assert(this->Handle);

  glBindTexture(this->Target, this->Handle);
  svtkOpenGLCheckErrorMacro("failed at glBindTexture");

  if (this->AutoParameters && (this->GetMTime() > this->SendParametersTime))
  {
    this->SendParameters();
  }
}

//----------------------------------------------------------------------------
bool svtkTextureObject::IsBound()
{
  bool result = false;
  if (this->Context && this->Handle)
  {
    GLenum target = 0; // to avoid warnings.
    switch (this->Target)
    {
#if defined(GL_TEXTURE_1D) && defined(GL_TEXTURE_BINDING_1D)
      case GL_TEXTURE_1D:
        target = GL_TEXTURE_BINDING_1D;
        break;
#endif
      case GL_TEXTURE_2D:
        target = GL_TEXTURE_BINDING_2D;
        break;
#if defined(GL_TEXTURE_2D_MULTISAMPLE) && defined(GL_TEXTURE_BINDING_2D_MULTISAMPLE)
      case GL_TEXTURE_2D_MULTISAMPLE:
        target = GL_TEXTURE_BINDING_2D_MULTISAMPLE;
        break;
#endif
#if defined(GL_TEXTURE_3D) && defined(GL_TEXTURE_BINDING_3D)
      case GL_TEXTURE_3D:
        target = GL_TEXTURE_BINDING_3D;
        break;
#endif
#if defined(GL_TEXTURE_BUFFER) && defined(GL_TEXTURE_BINDING_BUFFER)
      case GL_TEXTURE_BUFFER:
        target = GL_TEXTURE_BINDING_BUFFER;
        break;
#endif
#if defined(GL_TEXTURE_CUBE_MAP) && defined(GL_TEXTURE_BINDING_CUBE_MAP)
      case GL_TEXTURE_CUBE_MAP:
        target = GL_TEXTURE_BINDING_CUBE_MAP;
        break;
#endif
      default:
        assert("check: impossible case" && 0);
        break;
    }
    GLint objectId;
    glGetIntegerv(target, &objectId);
    result = static_cast<GLuint>(objectId) == this->Handle;
  }
  return result;
}

//----------------------------------------------------------------------------
void svtkTextureObject::SendParameters()
{
  assert("pre: is_bound" && this->IsBound());

#if defined(GL_TEXTURE_BUFFER)
  if (this->Target == GL_TEXTURE_BUFFER)
  {
    return;
  }
#endif

#ifdef GL_TEXTURE_2D_MULTISAMPLE
  if (this->Target == GL_TEXTURE_2D_MULTISAMPLE)
  {
    return;
  }
#endif

  glTexParameteri(this->Target, GL_TEXTURE_WRAP_S, OpenGLWrap[this->WrapS]);
  glTexParameteri(this->Target, GL_TEXTURE_WRAP_T, OpenGLWrap[this->WrapT]);

#ifdef GL_TEXTURE_WRAP_R
  glTexParameteri(this->Target, GL_TEXTURE_WRAP_R, OpenGLWrap[this->WrapR]);
#endif

  glTexParameteri(this->Target, GL_TEXTURE_MIN_FILTER, OpenGLMinFilter[this->MinificationFilter]);

  glTexParameteri(this->Target, GL_TEXTURE_MAG_FILTER, OpenGLMagFilter[this->MagnificationFilter]);

#ifndef GL_ES_VERSION_3_0
  glTexParameterfv(this->Target, GL_TEXTURE_BORDER_COLOR, this->BorderColor);

  if (this->DepthTextureCompare)
  {
    glTexParameteri(this->Target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
  }
  else
  {
    glTexParameteri(this->Target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
  }
#endif

  // if mipmaps are requested also turn on anisotropic if available
#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
  if (GLEW_EXT_texture_filter_anisotropic)
  {
    float aniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
    if (aniso > this->MaximumAnisotropicFiltering)
    {
      aniso = this->MaximumAnisotropicFiltering;
    }
    glTexParameterf(this->Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
  }
#endif

  glTexParameterf(this->Target, GL_TEXTURE_MIN_LOD, this->MinLOD);
  glTexParameterf(this->Target, GL_TEXTURE_MAX_LOD, this->MaxLOD);
  glTexParameteri(this->Target, GL_TEXTURE_BASE_LEVEL, this->BaseLevel);
  glTexParameteri(this->Target, GL_TEXTURE_MAX_LEVEL, this->MaxLevel);

  glTexParameteri(this->Target, GL_TEXTURE_COMPARE_FUNC,
    OpenGLDepthTextureCompareFunction[this->DepthTextureCompareFunction]);

  svtkOpenGLCheckErrorMacro("failed after SendParameters");
  this->SendParametersTime.Modified();
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetInternalFormat(
  int svtktype, int numComps, bool shaderSupportsTextureInt)
{
  if (this->InternalFormat)
  {
    return this->InternalFormat;
  }

  // pre-condition
  if (svtktype == SVTK_VOID && numComps != 1)
  {
    svtkErrorMacro(
      "Depth component texture must have 1 component only (" << numComps << " requested");
    this->InternalFormat = 0;
    return this->InternalFormat;
  }

  this->InternalFormat =
    this->GetDefaultInternalFormat(svtktype, numComps, shaderSupportsTextureInt);

  if (!this->InternalFormat)
  {
    svtkDebugMacro("Unable to find suitable internal format for T="
      << svtktype << " NC=" << numComps << " SSTI=" << shaderSupportsTextureInt);
  }

  return this->InternalFormat;
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetDefaultInternalFormat(
  int svtktype, int numComps, bool shaderSupportsTextureInt)
{
  GLenum result = 0;

  // if shader supports int textures try that first
  if (shaderSupportsTextureInt)
  {
    result = this->Context->GetDefaultTextureInternalFormat(
      svtktype, numComps, true, false, this->UseSRGBColorSpace);
    if (!result)
    {
      svtkDebugMacro("Unsupported internal texture type!");
    }
    return result;
  }

  // try default next
  result = this->Context->GetDefaultTextureInternalFormat(
    svtktype, numComps, false, false, this->UseSRGBColorSpace);
  if (result)
  {
    return result;
  }

  // try floating point
  result = this->Context->GetDefaultTextureInternalFormat(
    svtktype, numComps, false, true, this->UseSRGBColorSpace);

  if (!result)
  {
    svtkDebugMacro("Unsupported internal texture type!");
    svtkDebugMacro("Unable to find suitable internal format for T="
      << svtktype << " NC=" << numComps << " SSTI=" << shaderSupportsTextureInt);
  }

  return result;
}

//----------------------------------------------------------------------------
void svtkTextureObject::SetInternalFormat(unsigned int glInternalFormat)
{
  if (this->InternalFormat != glInternalFormat)
  {
    this->InternalFormat = glInternalFormat;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
static int svtkGetSVTKType(GLenum gltype)
{
  // DON'T DEAL with SVTK_CHAR as this is platform dependent.
  switch (gltype)
  {
    case GL_BYTE:
      return SVTK_SIGNED_CHAR;

    case GL_UNSIGNED_BYTE:
      return SVTK_UNSIGNED_CHAR;

    case GL_SHORT:
      return SVTK_SHORT;

    case GL_UNSIGNED_SHORT:
      return SVTK_UNSIGNED_SHORT;

    case GL_INT:
      return SVTK_INT;

    case GL_UNSIGNED_INT:
      return SVTK_UNSIGNED_INT;

    case GL_FLOAT:
      return SVTK_FLOAT;
  }

  return 0;
}

void svtkTextureObject::GetShiftAndScale(float& shift, float& scale)
{
  shift = 1.0;
  scale = 1.0;

  // check to see if this is an int format
  GLenum iresult = this->Context->GetDefaultTextureInternalFormat(
    svtkGetSVTKType(this->Type), this->Components, true, false, this->UseSRGBColorSpace);

  // using an int texture format, no shift scale
  if (iresult == this->InternalFormat)
  {
    return;
  }

  // for all float type internal formats
  switch (this->Type)
  {
    case GL_BYTE:
      scale = (SVTK_SIGNED_CHAR_MAX - SVTK_SIGNED_CHAR_MIN) / 2.0;
      shift = scale + SVTK_SIGNED_CHAR_MIN;
      break;
    case GL_UNSIGNED_BYTE:
      scale = SVTK_UNSIGNED_CHAR_MAX;
      shift = 0.0;
      break;
    case GL_SHORT:
      // this may be off a tad
      scale = (SVTK_SHORT_MAX - SVTK_SHORT_MIN) / 2.0;
      shift = scale + SVTK_SHORT_MIN;
      break;
    case GL_UNSIGNED_SHORT:
      scale = SVTK_UNSIGNED_SHORT_MAX;
      shift = 0.0;
      break;
    case GL_INT:
      // this may be off a tad
      scale = (1.0 * SVTK_INT_MAX - SVTK_INT_MIN) / 2.0;
      shift = scale + SVTK_INT_MIN;
      break;
    case GL_UNSIGNED_INT:
      scale = SVTK_UNSIGNED_INT_MAX;
      shift = 0.0;
      break;
    case GL_FLOAT:
    default:
      break;
  }
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetFormat(int svtktype, int numComps, bool shaderSupportsTextureInt)
{
  if (!this->Format)
  {
    this->Format = this->GetDefaultFormat(svtktype, numComps, shaderSupportsTextureInt);
  }
  return this->Format;
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetDefaultFormat(
  int svtktype, int numComps, bool shaderSupportsTextureInt)
{
  if (svtktype == SVTK_VOID)
  {
    return GL_DEPTH_COMPONENT;
  }

#ifndef GL_ES_VERSION_3_0
  if (this->SupportsTextureInteger && shaderSupportsTextureInt &&
    (svtktype == SVTK_SIGNED_CHAR || svtktype == SVTK_UNSIGNED_CHAR || svtktype == SVTK_SHORT ||
      svtktype == SVTK_UNSIGNED_SHORT || svtktype == SVTK_INT || svtktype == SVTK_UNSIGNED_INT))
  {
    switch (numComps)
    {
      case 1:
        return GL_RED_INTEGER;
      case 2:
        return GL_RG_INTEGER;
      case 3:
        return GL_RGB_INTEGER_EXT;
      case 4:
        return GL_RGBA_INTEGER_EXT;
    }
  }
  else
  {
    switch (numComps)
    {
      case 1:
        return GL_RED;
      case 2:
        return GL_RG;
      case 3:
        return GL_RGB;
      case 4:
        return GL_RGBA;
    }
#else
  {
    switch (numComps)
    {
#ifdef GL_RED
      case 1:
        return GL_RED;
      case 2:
        return GL_RG;
#else
      case 1:
        return GL_LUMINANCE;
      case 2:
        return GL_LUMINANCE_ALPHA;
#endif
      case 3:
        return GL_RGB;
      case 4:
        return GL_RGBA;
    }
#endif
  }
  return GL_RGB;
}

//----------------------------------------------------------------------------
void svtkTextureObject::SetFormat(unsigned int glFormat)
{
  if (this->Format != glFormat)
  {
    this->Format = glFormat;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkTextureObject::ResetFormatAndType()
{
  this->Format = 0;
  this->InternalFormat = 0;
  this->Type = 0;
}

//----------------------------------------------------------------------------
int svtkTextureObject::GetDefaultDataType(int svtk_scalar_type)
{
  // DON'T DEAL with SVTK_CHAR as this is platform dependent.
  switch (svtk_scalar_type)
  {
    case SVTK_SIGNED_CHAR:
      return GL_BYTE;

    case SVTK_UNSIGNED_CHAR:
      return GL_UNSIGNED_BYTE;

    case SVTK_SHORT:
      return GL_SHORT;

    case SVTK_UNSIGNED_SHORT:
      return GL_UNSIGNED_SHORT;

    case SVTK_INT:
      return GL_INT;

    case SVTK_UNSIGNED_INT:
      return GL_UNSIGNED_INT;

    case SVTK_FLOAT:
    case SVTK_VOID: // used for depth component textures.
      return GL_FLOAT;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkTextureObject::GetSVTKDataType()
{
  return ::svtkGetSVTKType(this->Type);
}

//----------------------------------------------------------------------------
int svtkTextureObject::GetDataType(int svtk_scalar_type)
{
  if (!this->Type)
  {
    this->Type = this->GetDefaultDataType(svtk_scalar_type);
  }

  return this->Type;
}

//----------------------------------------------------------------------------
void svtkTextureObject::SetDataType(unsigned int glType)
{
  if (this->Type != glType)
  {
    this->Type = glType;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetMinificationFilterMode(int svtktype)
{
  switch (svtktype)
  {
    case Nearest:
      return GL_NEAREST;
    case Linear:
      return GL_LINEAR;
    case NearestMipmapNearest:
      return GL_NEAREST_MIPMAP_NEAREST;
    case NearestMipmapLinear:
      return GL_NEAREST_MIPMAP_LINEAR;
    case LinearMipmapNearest:
      return GL_LINEAR_MIPMAP_NEAREST;
    case LinearMipmapLinear:
      return GL_LINEAR_MIPMAP_LINEAR;
    default:
      return GL_NEAREST;
  }
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetMagnificationFilterMode(int svtktype)
{
  switch (svtktype)
  {
    case Nearest:
      return GL_NEAREST;
    case Linear:
      return GL_LINEAR;
    default:
      return GL_NEAREST;
  }
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetWrapSMode(int svtktype)
{
  switch (svtktype)
  {
    case ClampToEdge:
      return GL_CLAMP_TO_EDGE;
    case Repeat:
      return GL_REPEAT;
#ifdef GL_CLAMP_TO_BORDER
    case ClampToBorder:
      return GL_CLAMP_TO_BORDER;
#endif
    case MirroredRepeat:
      return GL_MIRRORED_REPEAT;
    default:
      return GL_CLAMP_TO_EDGE;
  }
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetWrapTMode(int svtktype)
{
  return this->GetWrapSMode(svtktype);
}

//----------------------------------------------------------------------------
unsigned int svtkTextureObject::GetWrapRMode(int svtktype)
{
  return this->GetWrapSMode(svtktype);
}

// 1D  textures are not supported in ES 2.0 or 3.0
#ifndef GL_ES_VERSION_3_0

//----------------------------------------------------------------------------
bool svtkTextureObject::Create1D(
  int numComps, svtkPixelBufferObject* pbo, bool shaderSupportsTextureInt)
{
  assert(this->Context);
  assert(pbo->GetContext() == this->Context.GetPointer());

  GLenum target = GL_TEXTURE_1D;

  // Now, determine texture parameters using the information from the pbo.

  // * internalFormat depends on number of components and the data type.
  GLenum internalFormat =
    this->GetInternalFormat(pbo->GetType(), numComps, shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLenum format = this->GetFormat(pbo->GetType(), numComps, shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLenum type = this->GetDefaultDataType(pbo->GetType());

  if (!internalFormat || !format || !type)
  {
    svtkErrorMacro("Failed to determine texture parameters.");
    return false;
  }

  this->Target = target;
  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  pbo->Bind(svtkPixelBufferObject::UNPACKED_BUFFER);

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage1D(target, 0, static_cast<GLint>(internalFormat),
    static_cast<GLsizei>(pbo->GetSize() / static_cast<unsigned int>(numComps)), 0, format, type,
    BUFFER_OFFSET(0));
  svtkOpenGLCheckErrorMacro("failed at glTexImage1D");
  pbo->UnBind();
  this->Deactivate();

  this->Target = target;
  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = pbo->GetSize();
  this->Height = 1;
  this->Depth = 1;
  this->NumberOfDimensions = 1;
  return true;
}

//----------------------------------------------------------------------------
bool svtkTextureObject::Create1DFromRaw(unsigned int width, int numComps, int dataType, void* data)
{
  assert(this->Context);

  // Now determine the texture parameters using the arguments.
  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
  {
    svtkErrorMacro("Failed to determine texture parameters.");
    return false;
  }

  GLenum target = GL_TEXTURE_1D;
  this->Target = target;
  this->Components = numComps;
  this->Width = width;
  this->Height = 1;
  this->Depth = 1;
  this->NumberOfDimensions = 1;
  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  glTexImage1D(this->Target, 0, this->InternalFormat, static_cast<GLsizei>(this->Width), 0,
    this->Format, this->Type, static_cast<const GLvoid*>(data));

  svtkOpenGLCheckErrorMacro("failed at glTexImage1D");

  this->Deactivate();
  return true;
}

// Description:
// Create a texture buffer basically a 1D texture that can be
// very large for passing data into the fragment shader
bool svtkTextureObject::CreateTextureBuffer(
  unsigned int numValues, int numComps, int dataType, svtkOpenGLBufferObject* bo)
{
  assert(this->Context);

  // Now, determine texture parameters using the arguments.
  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
  {
    svtkErrorMacro("Failed to determine texture parameters.");
    return false;
  }

  this->Target = GL_TEXTURE_BUFFER;
  this->Components = numComps;
  this->Width = numValues;
  this->Height = 1;
  this->Depth = 1;
  this->NumberOfDimensions = 1;
  this->BufferObject = bo;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  // Source texture data from the PBO.
  glTexBuffer(this->Target, this->InternalFormat, this->BufferObject->GetHandle());

  svtkOpenGLCheckErrorMacro("failed at glTexBuffer");

  this->Deactivate();

  return true;
}

#else

// Description:
// Create a texture buffer basically a 1D texture that can be
// very large for passing data into the fragment shader
bool svtkTextureObject::CreateTextureBuffer(
  unsigned int numValues, int numComps, int dataType, svtkOpenGLBufferObject* bo)
{
  assert(this->Context);
  svtkErrorMacro("TextureBuffers not supported in OPenGL ES");
  // TODO: implement 1D and Texture buffers using 2D textures
  return false;
}

#endif // not ES 2.0 or 3.0

//----------------------------------------------------------------------------
bool svtkTextureObject::Create2D(unsigned int width, unsigned int height, int numComps,
  svtkPixelBufferObject* pbo, bool shaderSupportsTextureInt)
{
  assert(this->Context);
  assert(pbo->GetContext() == this->Context.GetPointer());

  if (pbo->GetSize() < width * height * static_cast<unsigned int>(numComps))
  {
    svtkErrorMacro("PBO size must match texture size.");
    return false;
  }

  // Now, determine texture parameters using the information from the pbo.
  // * internalFormat depends on number of components and the data type.
  // * format depends on the number of components.
  // * type if the data type in the pbo

  int svtktype = pbo->GetType();
  GLenum type = this->GetDefaultDataType(svtktype);

  GLenum internalFormat = this->GetInternalFormat(svtktype, numComps, shaderSupportsTextureInt);

  GLenum format = this->GetFormat(svtktype, numComps, shaderSupportsTextureInt);

  if (!internalFormat || !format || !type)
  {
    svtkErrorMacro("Failed to determine texture parameters.");
    return false;
  }

  GLenum target = GL_TEXTURE_2D;
  this->Target = target;
  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  // Source texture data from the PBO.
  pbo->Bind(svtkPixelBufferObject::UNPACKED_BUFFER);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(target, 0, internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height),
    0, format, type, BUFFER_OFFSET(0));

  svtkOpenGLCheckErrorMacro("failed at glTexImage2D");

  pbo->UnBind();
  this->Deactivate();

  this->Target = target;
  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;

  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Create a 2D depth texture using a PBO.
bool svtkTextureObject::CreateDepth(
  unsigned int width, unsigned int height, int internalFormat, svtkPixelBufferObject* pbo)
{
  assert("pre: context_exists" && this->GetContext() != nullptr);
  assert("pre: pbo_context_exists" && pbo->GetContext() != nullptr);
  assert("pre: context_match" && this->GetContext() == pbo->GetContext());
  assert("pre: sizes_match" && pbo->GetSize() == width * height);
  assert(
    "pre: valid_internalFormat" && internalFormat >= 0 && internalFormat < NumberOfDepthFormats);

  GLenum inFormat = OpenGLDepthInternalFormat[internalFormat];
  GLenum type = this->GetDefaultDataType(pbo->GetType());

  this->Target = GL_TEXTURE_2D;
  this->Format = GL_DEPTH_COMPONENT;
  this->Type = type;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->Components = 1;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  pbo->Bind(svtkPixelBufferObject::UNPACKED_BUFFER);

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(this->Target, 0, static_cast<GLint>(inFormat), static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, this->Format, this->Type, BUFFER_OFFSET(0));
  svtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  pbo->UnBind();
  this->Deactivate();
  return true;
}

//----------------------------------------------------------------------------
bool svtkTextureObject::Create3D(unsigned int width, unsigned int height, unsigned int depth,
  int numComps, svtkPixelBufferObject* pbo, bool shaderSupportsTextureInt)
{
#ifdef GL_TEXTURE_3D
  assert(this->Context);
  assert(this->Context.GetPointer() == pbo->GetContext());

  if (pbo->GetSize() != width * height * depth * static_cast<unsigned int>(numComps))
  {
    svtkErrorMacro("PBO size must match texture size.");
    return false;
  }

  GLenum target = GL_TEXTURE_3D;

  // Now, determine texture parameters using the information from the pbo.

  // * internalFormat depends on number of components and the data type.
  GLenum internalFormat =
    this->GetInternalFormat(pbo->GetType(), numComps, shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLenum format = this->GetFormat(pbo->GetType(), numComps, shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLenum type = this->GetDefaultDataType(pbo->GetType());

  if (!internalFormat || !format || !type)
  {
    svtkErrorMacro("Failed to determine texture parameters.");
    return false;
  }

  this->Target = target;
  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  pbo->Bind(svtkPixelBufferObject::UNPACKED_BUFFER);

  // Source texture data from the PBO.
  glTexImage3D(target, 0, static_cast<GLint>(internalFormat), static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, format, type, BUFFER_OFFSET(0));

  svtkOpenGLCheckErrorMacro("failed at glTexImage3D");

  pbo->UnBind();
  this->Deactivate();

  this->Target = target;
  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = depth;
  this->NumberOfDimensions = 3;
  return true;

#else
  return false;
#endif
}

//----------------------------------------------------------------------------
svtkPixelBufferObject* svtkTextureObject::Download(unsigned int target, unsigned int level)
{
  assert(this->Context);
  assert(this->Handle);

  svtkPixelBufferObject* pbo = svtkPixelBufferObject::New();
  pbo->SetContext(this->Context);

  int svtktype = ::svtkGetSVTKType(this->Type);
  if (svtktype == 0)
  {
    svtkErrorMacro("Failed to determine type.");
    return nullptr;
  }

  unsigned int size = this->Width * this->Height * this->Depth;

  // doesn't matter which Upload*D method we use since we are not really
  // uploading any data, simply allocating GPU space.
  if (!pbo->Upload1D(svtktype, nullptr, size, this->Components, 0))
  {
    svtkErrorMacro("Could not allocate memory for PBO.");
    pbo->Delete();
    return nullptr;
  }

  pbo->Bind(svtkPixelBufferObject::PACKED_BUFFER);
  this->Bind();

#ifndef GL_ES_VERSION_3_0
  glGetTexImage(target, level, this->Format, this->Type, BUFFER_OFFSET(0));
#else
  // you can do something with glReadPixels and binding a texture as a FBO
  // I believe for ES 2.0
#endif

  svtkOpenGLCheckErrorMacro("failed at glGetTexImage");
  this->Deactivate();
  pbo->UnBind();

  pbo->SetComponents(this->Components);

  return pbo;
}

//----------------------------------------------------------------------------
svtkPixelBufferObject* svtkTextureObject::Download()
{
  return this->Download(this->Target, 0);
}

//----------------------------------------------------------------------------
bool svtkTextureObject::Create3DFromRaw(unsigned int width, unsigned int height, unsigned int depth,
  int numComps, int dataType, void* data)
{
  assert(this->Context);
  svtkOpenGLClearErrorMacro();

  // Now, determine texture parameters using the arguments.
  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
  {
    svtkErrorMacro("Failed to determine texture parameters.");
    return false;
  }

  this->Target = GL_TEXTURE_3D;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = depth;
  this->NumberOfDimensions = 3;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage3D(this->Target, 0, this->InternalFormat, static_cast<GLsizei>(this->Width),
    static_cast<GLsizei>(this->Height), static_cast<GLsizei>(this->Depth), 0, this->Format,
    this->Type, static_cast<const GLvoid*>(data));

  this->Deactivate();

  return svtkOpenGLCheckErrors("Failed to allocate 3D texture.");
}

//----------------------------------------------------------------------------
bool svtkTextureObject::AllocateProxyTexture3D(unsigned int const width, unsigned int const height,
  unsigned int depth, int const numComps, int const dataType)
{
#ifndef GL_ES_VERSION_3_0
  assert(this->Context);

  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
  {
    svtkErrorMacro("Failed to determine texture parameters.");
    return false;
  }

  this->Target = GL_TEXTURE_3D;
  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  glTexImage3D(GL_PROXY_TEXTURE_3D, 0, this->InternalFormat, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, this->Format, this->Type,
    nullptr);

  GLsizei testWidth;
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &testWidth);

  svtkOpenGLCheckErrorMacro("Failed after glTexImage3D with PROXY target");
  this->Deactivate();

  if (testWidth == 0)
  {
    return false;
  }
#endif
  return true;
}

//----------------------------------------------------------------------------
bool svtkTextureObject::Create2DFromRaw(
  unsigned int width, unsigned int height, int numComps, int dataType, void* data)
{
  assert(this->Context);

  // Now determine the texture parameters using the arguments.
  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
  {
    svtkErrorMacro("Failed to determine texture parameters. IF="
      << this->InternalFormat << " F=" << this->Format << " T=" << this->Type);
    return false;
  }

  GLenum target = GL_TEXTURE_2D;
  this->Target = target;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(this->Target, 0, this->InternalFormat, static_cast<GLsizei>(this->Width),
    static_cast<GLsizei>(this->Height), 0, this->Format, this->Type,
    static_cast<const GLvoid*>(data));

  svtkOpenGLCheckErrorMacro("failed at glTexImage2D");

  this->Deactivate();
  return true;
}

//----------------------------------------------------------------------------
bool svtkTextureObject::CreateCubeFromRaw(
  unsigned int width, unsigned int height, int numComps, int dataType, void* data[6])
{
  assert(this->Context);

  // Now determine the texture parameters using the arguments.
  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
  {
    svtkErrorMacro("Failed to determine texture parameters. IF="
      << this->InternalFormat << " F=" << this->Format << " T=" << this->Type);
    return false;
  }

  GLenum target = GL_TEXTURE_CUBE_MAP;
  this->Target = target;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  for (int i = 0; i < 6; i++)
  {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, this->InternalFormat,
      static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), 0, this->Format,
      this->Type, data ? static_cast<const GLvoid*>(data[i]) : nullptr);
    svtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  }

  if (this->GenerateMipmap)
  {
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  }

  this->Deactivate();
  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Create a 2D depth texture using a raw pointer.
// This is a blocking call. If you can, use PBO instead.
bool svtkTextureObject::CreateDepthFromRaw(
  unsigned int width, unsigned int height, int internalFormat, int rawType, void* raw)
{
  assert("pre: context_exists" && this->GetContext() != nullptr);

  assert(
    "pre: valid_internalFormat" && internalFormat >= 0 && internalFormat < NumberOfDepthFormats);

  // Now, determine texture parameters using the arguments.
  this->GetDataType(rawType);

  if (!this->InternalFormat)
  {
    this->InternalFormat = OpenGLDepthInternalFormat[internalFormat];
  }

  if (!this->InternalFormat || !this->Type)
  {
    svtkErrorMacro("Failed to determine texture parameters.");
    return false;
  }

  this->Target = GL_TEXTURE_2D;
  this->Format = GL_DEPTH_COMPONENT;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->Components = 1;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
    static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), 0, this->Format,
    this->Type, raw);
  svtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  this->Deactivate();
  return true;
}

// ----------------------------------------------------------------------------
bool svtkTextureObject::AllocateDepth(unsigned int width, unsigned int height, int internalFormat)
{
  assert("pre: context_exists" && this->GetContext() != nullptr);
  assert(
    "pre: valid_internalFormat" && internalFormat >= 0 && internalFormat < NumberOfDepthFormats);

#ifdef GL_TEXTURE_2D_MULTISAMPLE
  this->Target = (this->Samples ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D);
#else
  this->Target = GL_TEXTURE_2D;
#endif

  this->Format = GL_DEPTH_COMPONENT;

  // Try to match svtk type to internal fmt
  if (!this->Type)
  {
    this->Type = OpenGLDepthInternalFormatType[internalFormat];
  }

  if (!this->InternalFormat)
  {
    this->InternalFormat = OpenGLDepthInternalFormat[internalFormat];
  }

  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->Components = 1;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

#ifdef GL_TEXTURE_2D_MULTISAMPLE
  if (this->Samples)
  {
    glTexImage2DMultisample(this->Target, this->Samples, static_cast<GLint>(this->InternalFormat),
      static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), GL_TRUE);
  }
  else
#endif
  {
    glTexImage2D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
      static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), 0, this->Format,
      this->Type, nullptr);
  }

  svtkOpenGLCheckErrorMacro("failed at glTexImage2D");

  this->Deactivate();
  return true;
}

bool svtkTextureObject::AllocateDepthStencil(unsigned int width, unsigned int height)
{
  assert("pre: context_exists" && this->GetContext() != nullptr);

#ifdef GL_TEXTURE_2D_MULTISAMPLE
  this->Target = (this->Samples ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D);
#else
  this->Target = GL_TEXTURE_2D;
#endif

  this->Format = GL_DEPTH_STENCIL;
  this->Type = GL_UNSIGNED_INT_24_8;
  this->InternalFormat = GL_DEPTH24_STENCIL8;

  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->Components = 1;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

#ifdef GL_TEXTURE_2D_MULTISAMPLE
  if (this->Samples)
  {
    glTexImage2DMultisample(this->Target, this->Samples, static_cast<GLint>(this->InternalFormat),
      static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), GL_TRUE);
  }
  else
#endif
  {
    glTexImage2D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
      static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), 0, this->Format,
      this->Type, nullptr);
  }

  svtkOpenGLCheckErrorMacro("failed at glTexImage2D");

  this->Deactivate();
  return true;
}

// ----------------------------------------------------------------------------
bool svtkTextureObject::Allocate1D(unsigned int width, int numComps, int svtkType)
{
#ifdef GL_TEXTURE_1D
  assert(this->Context);

  this->Target = GL_TEXTURE_1D;

  this->GetDataType(svtkType);
  this->GetInternalFormat(svtkType, numComps, false);
  this->GetFormat(svtkType, numComps, false);

  this->Components = numComps;
  this->Width = width;
  this->Height = 1;
  this->Depth = 1;
  this->NumberOfDimensions = 1;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();
  glTexImage1D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
    static_cast<GLsizei>(this->Width), 0, this->Format, this->Type, nullptr);
  svtkOpenGLCheckErrorMacro("failed at glTexImage1D");
  this->Deactivate();
  return true;
#else
  return false;
#endif
}

// ----------------------------------------------------------------------------
// Description:
// Create a 2D color texture but does not initialize its values.
// Internal format is deduced from numComps and svtkType.
bool svtkTextureObject::Allocate2D(
  unsigned int width, unsigned int height, int numComps, int svtkType, int level)
{
  assert(this->Context);

#ifdef GL_TEXTURE_2D_MULTISAMPLE
  this->Target = (this->Samples ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D);
#else
  this->Target = GL_TEXTURE_2D;
#endif

  this->GetDataType(svtkType);
  this->GetInternalFormat(svtkType, numComps, false);
  this->GetFormat(svtkType, numComps, false);

  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();

#ifdef GL_TEXTURE_2D_MULTISAMPLE
  if (this->Samples)
  {
    glTexImage2DMultisample(this->Target, this->Samples, static_cast<GLint>(this->InternalFormat),
      static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), GL_TRUE);
  }
  else
#endif
  {
    glTexImage2D(this->Target, level, static_cast<GLint>(this->InternalFormat),
      static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), 0, this->Format,
      this->Type, nullptr);
  }
  svtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  this->Deactivate();
  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Create a 3D color texture but does not initialize its values.
// Internal format is deduced from numComps and svtkType.
bool svtkTextureObject::Allocate3D(
  unsigned int width, unsigned int height, unsigned int depth, int numComps, int svtkType)
{
#ifdef GL_TEXTURE_3D
  this->Target = GL_TEXTURE_3D;

  if (this->Context == nullptr)
  {
    svtkErrorMacro("No context specified. Cannot create texture.");
    return false;
  }

  this->GetInternalFormat(svtkType, numComps, false);
  this->GetFormat(svtkType, numComps, false);
  this->GetDataType(svtkType);

  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = depth;
  this->NumberOfDimensions = 3;

  this->Context->ActivateTexture(this);
  this->CreateTexture();
  this->Bind();
  glTexImage3D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
    static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height),
    static_cast<GLsizei>(this->Depth), 0, this->Format, this->Type, nullptr);
  svtkOpenGLCheckErrorMacro("failed at glTexImage3D");
  this->Deactivate();
  return true;
#else
  return false;
#endif
}

// ----------------------------------------------------------------------------
void svtkTextureObject::CopyToFrameBuffer(svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao)
{
  // the following math really only works when texture
  // and viewport are of the same dimensions
  float minXTexCoord = static_cast<float>(static_cast<double>(0.5) / this->Width);
  float minYTexCoord = static_cast<float>(static_cast<double>(0.5) / this->Height);

  float maxXTexCoord = static_cast<float>(static_cast<double>(this->Width - 0.5) / this->Width);
  float maxYTexCoord = static_cast<float>(static_cast<double>(this->Height - 0.5) / this->Height);

  float tcoords[] = { minXTexCoord, minYTexCoord, maxXTexCoord, minYTexCoord, maxXTexCoord,
    maxYTexCoord, minXTexCoord, maxYTexCoord };

  float verts[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f };

  this->CopyToFrameBuffer(tcoords, verts, program, vao);
}

// ----------------------------------------------------------------------------
void svtkTextureObject::CopyToFrameBuffer(int srcXmin, int srcYmin, int srcXmax, int srcYmax,
  int dstXmin, int dstYmin, int dstSizeX, int dstSizeY, svtkShaderProgram* program,
  svtkOpenGLVertexArrayObject* vao)
{
  float dstXmax = static_cast<float>(dstXmin + srcXmax - srcXmin);
  float dstYmax = static_cast<float>(dstYmin + srcYmax - srcYmin);

  this->CopyToFrameBuffer(srcXmin, srcYmin, srcXmax, srcYmax, dstXmin, dstYmin, dstXmax, dstYmax,
    dstSizeX, dstSizeY, program, vao);
}

// ----------------------------------------------------------------------------
void svtkTextureObject::CopyToFrameBuffer(int srcXmin, int srcYmin, int srcXmax, int srcYmax,
  int dstXmin, int dstYmin, int dstXmax, int dstYmax, int svtkNotUsed(dstSizeX),
  int svtkNotUsed(dstSizeY), svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao)
{
  assert("pre: positive_srcXmin" && srcXmin >= 0);
  assert("pre: max_srcXmax" && static_cast<unsigned int>(srcXmax) < this->GetWidth());
  assert("pre: increasing_x" && srcXmin <= srcXmax);
  assert("pre: positive_srcYmin" && srcYmin >= 0);
  assert("pre: max_srcYmax" && static_cast<unsigned int>(srcYmax) < this->GetHeight());
  assert("pre: increasing_y" && srcYmin <= srcYmax);
  assert("pre: positive_dstXmin" && dstXmin >= 0);
  assert("pre: positive_dstYmin" && dstYmin >= 0);

  float minXTexCoord = static_cast<float>(static_cast<double>(srcXmin + 0.5) / this->Width);
  float minYTexCoord = static_cast<float>(static_cast<double>(srcYmin + 0.5) / this->Height);

  float maxXTexCoord = static_cast<float>(static_cast<double>(srcXmax + 0.5) / this->Width);
  float maxYTexCoord = static_cast<float>(static_cast<double>(srcYmax + 0.5) / this->Height);

  svtkOpenGLState::ScopedglViewport vsaver(this->Context->GetState());
  this->Context->GetState()->svtkglViewport(
    dstXmin, dstYmin, dstXmax - dstXmin + 1, dstYmax - dstYmin + 1);

  float tcoords[] = { minXTexCoord, minYTexCoord, maxXTexCoord, minYTexCoord, maxXTexCoord,
    maxYTexCoord, minXTexCoord, maxYTexCoord };

  float verts[] = { -1.f, -1.f, 0.0f, 1.0f, -1.f, 0.0f, 1.0f, 1.0f, 0.0f, -1.f, 1.0f, 0.0f };

  this->CopyToFrameBuffer(tcoords, verts, program, vao);

  svtkOpenGLCheckErrorMacro("failed after CopyToFrameBuffer");
}

void svtkTextureObject::CopyToFrameBuffer(
  float* tcoords, float* verts, svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao)
{
  svtkOpenGLClearErrorMacro();

  // if no program or VAO was provided, then use
  // a simple pass through program and bind this
  // texture to it
  if (!program || !vao)
  {
    if (!this->ShaderProgram)
    {
      this->ShaderProgram = new svtkOpenGLHelper;

      // build the shader source code
      std::string VSSource = svtkTextureObjectVS;
      std::string FSSource = svtkTextureObjectFS;
      std::string GSSource;

      // compile and bind it if needed
      svtkShaderProgram* newShader = this->Context->GetShaderCache()->ReadyShaderProgram(
        VSSource.c_str(), FSSource.c_str(), GSSource.c_str());

      // if the shader changed reinitialize the VAO
      if (newShader != this->ShaderProgram->Program)
      {
        this->ShaderProgram->Program = newShader;
        this->ShaderProgram->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
      }

      this->ShaderProgram->ShaderSourceTime.Modified();
    }
    else
    {
      this->Context->GetShaderCache()->ReadyShaderProgram(this->ShaderProgram->Program);
    }

    if (this->ShaderProgram->Program)
    {
      // bind and activate this texture
      this->Activate();
      int sourceId = this->GetTextureUnit();
      this->ShaderProgram->Program->SetUniformi("source", sourceId);
      svtkOpenGLRenderUtilities::RenderQuad(
        verts, tcoords, this->ShaderProgram->Program, this->ShaderProgram->VAO);
      this->Deactivate();
    }
  }
  else
  {
    svtkOpenGLRenderUtilities::RenderQuad(verts, tcoords, program, vao);
  }

  svtkOpenGLCheckErrorMacro("failed after CopyToFrameBuffer");
}

//----------------------------------------------------------------------------
// Description:
// Copy a sub-part of a logical buffer of the framebuffer (color or depth)
// to the texture object. src is the framebuffer, dst is the texture.
// (srcXmin,srcYmin) is the location of the lower left corner of the
// rectangle in the framebuffer. (dstXmin,dstYmin) is the location of the
// lower left corner of the rectangle in the texture. width and height
// specifies the size of the rectangle in pixels.
// If the logical buffer is a color buffer, it has to be selected first with
// glReadBuffer().
// \pre is2D: GetNumberOfDimensions()==2
void svtkTextureObject::CopyFromFrameBuffer(
  int srcXmin, int srcYmin, int svtkNotUsed(dstXmin), int svtkNotUsed(dstYmin), int width, int height)
{
  assert("pre: is2D" && this->GetNumberOfDimensions() == 2);

  // make assumption on the need to resolve
  // based on MultiSample setting
  if (this->Context->GetMultiSamples())
  {
    svtkNew<svtkOpenGLFramebufferObject> resolvedFBO;
    resolvedFBO->SetContext(this->Context);
    this->Context->GetState()->PushFramebufferBindings();
    resolvedFBO->PopulateFramebuffer(width, height,
      /* useTextures = */ true,
      /* numberOfColorAttachments = */ 1,
      /* colorDataType = */ SVTK_UNSIGNED_CHAR,
      /* wantDepthAttachment = */ true,
      /* depthBitplanes = */ 24,
      /* multisamples = */ 0);

    // PopulateFramebuffer changes active read/write buffer bindings,
    // hence we restore the read buffer bindings to read from the original
    // frame buffer.
    this->Context->GetState()->PopReadFramebufferBinding();

    svtkOpenGLState::ScopedglViewport vsaver(this->Context->GetState());
    this->Context->GetState()->svtkglViewport(0, 0, width, height);
    svtkOpenGLState::ScopedglScissor ssaver(this->Context->GetState());
    this->Context->GetState()->svtkglScissor(0, 0, width, height);

    // Now blit to resolve the MSAA and get an anti-aliased rendering in
    // resolvedFBO.
    // Note: extents are (x-min, x-max, y-min, y-max).
    const int srcExtents[4] = { srcXmin, srcXmin + width, srcYmin, srcYmin + height };
    const int destExtents[4] = { 0, width, 0, height };
    svtkOpenGLFramebufferObject::Blit(
      srcExtents, destExtents, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // Now make the resolvedFBO the read buffer and read from it.
    this->Context->GetState()->PushReadFramebufferBinding();
    resolvedFBO->Bind(GL_READ_FRAMEBUFFER);
    resolvedFBO->ActivateReadBuffer(0);

    this->Activate();

    glCopyTexImage2D(this->Target, 0, this->InternalFormat, 0, 0, width, height, 0);

    // restore bindings and release the resolvedFBO.
    this->Context->GetState()->PopFramebufferBindings();
  }
  else
  {
    this->Activate();
    glCopyTexImage2D(this->Target, 0, this->InternalFormat, srcXmin, srcYmin, width, height, 0);
  }

  svtkOpenGLCheckErrorMacro("failed at glCopyTexImage2D " << this->InternalFormat);
}

//----------------------------------------------------------------------------
int svtkTextureObject::GetMaximumTextureSize(svtkOpenGLRenderWindow* context)
{
  int maxSize = -1;
  if (context)
  {
    context->GetState()->svtkglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
  }

  return maxSize;
}

//----------------------------------------------------------------------------
int svtkTextureObject::GetMaximumTextureSize3D(svtkOpenGLRenderWindow* context)
{
  GLint maxSize = -1;
  if (context && context->IsCurrent())
  {
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxSize);
  }

  return static_cast<int>(maxSize);
}

//----------------------------------------------------------------------------
int svtkTextureObject::GetMaximumTextureSize3D()
{
  assert("Context == nullptr" && this->Context);
  return svtkTextureObject::GetMaximumTextureSize3D(this->Context);
}

//----------------------------------------------------------------------------
void svtkTextureObject::Resize(unsigned int width, unsigned int height)
{
  if (this->Width == width && this->Height == height)
  {
    return;
  }

  this->Width = width;
  this->Height = height;

  this->Context->ActivateTexture(this);
  this->Bind();

  if (this->NumberOfDimensions == 2)
  {
#ifdef GL_TEXTURE_2D_MULTISAMPLE
    if (this->Samples)
    {
      glTexImage2DMultisample(this->Target, this->Samples, static_cast<GLint>(this->InternalFormat),
        static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), GL_TRUE);
    }
    else
#endif
    {
      glTexImage2D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
        static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height), 0, this->Format,
        this->Type, nullptr);
    }
  }
  else if (this->NumberOfDimensions == 3)
  {
    glTexImage3D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
      static_cast<GLsizei>(this->Width), static_cast<GLsizei>(this->Height),
      static_cast<GLsizei>(this->Depth), 0, this->Format, this->Type, nullptr);
  }
#ifdef GL_TEXTURE_1D
  else if (this->NumberOfDimensions == 1)
  {
    glTexImage1D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
      static_cast<GLsizei>(this->Width), 0, this->Format, this->Type, nullptr);
  }
#endif

  svtkOpenGLCheckErrorMacro("failed at texture resize");
  this->Deactivate();
}

//----------------------------------------------------------------------------
void svtkTextureObject::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Width: " << this->Width << endl;
  os << indent << "Height: " << this->Height << endl;
  os << indent << "Depth: " << this->Depth << endl;
  os << indent << "Components: " << this->Components << endl;
  os << indent << "Handle: " << this->Handle << endl;
  os << indent << "Target: ";

  switch (this->Target)
  {
#ifdef GL_TEXTURE_1D
    case GL_TEXTURE_1D:
      os << "GL_TEXTURE_1D" << endl;
      break;
#endif
    case GL_TEXTURE_2D:
      os << "GL_TEXTURE_2D" << endl;
      break;
#ifdef GL_TEXTURE_3D
    case GL_TEXTURE_3D:
      os << "GL_TEXTURE_3D" << endl;
      break;
#endif
    default:
      os << "unknown value: 0x" << hex << this->Target << dec << endl;
      break;
  }

  os << indent << "NumberOfDimensions: " << this->NumberOfDimensions << endl;

  os << indent << "Format: " << this->Format << endl;
  os << indent << "InternalFormat: " << this->InternalFormat << endl;
  os << indent << "Type: " << this->Type << endl;

  os << indent << "WrapS: " << WrapAsString[this->WrapS] << endl;
  os << indent << "WrapT: " << WrapAsString[this->WrapT] << endl;
  os << indent << "WrapR: " << WrapAsString[this->WrapR] << endl;

  os << indent << "MinificationFilter: " << MinMagFilterAsString[this->MinificationFilter] << endl;

  os << indent << "MagnificationFilter: " << MinMagFilterAsString[this->MagnificationFilter]
     << endl;

  os << indent << "MinLOD: " << this->MinLOD << endl;
  os << indent << "MaxLOD: " << this->MaxLOD << endl;
  os << indent << "BaseLevel: " << this->BaseLevel << endl;
  os << indent << "MaxLevel: " << this->MaxLevel << endl;
  os << indent << "DepthTextureCompare: " << this->DepthTextureCompare << endl;
  os << indent << "DepthTextureCompareFunction: "
     << DepthTextureCompareFunctionAsString[this->DepthTextureCompareFunction] << endl;
  os << indent << "GenerateMipmap: " << this->GenerateMipmap << endl;
}
