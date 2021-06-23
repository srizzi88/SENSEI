/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRayCastImageDisplayHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLRayCastImageDisplayHelper.h"

#include "svtkFixedPointRayCastImage.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkTextureObject.h"
#include "svtkTransform.h"
#include "svtkVolume.h"

#include "svtkOpenGLHelper.h"

#include "svtkOpenGLError.h"

#include <cmath>

#include "svtkTextureObjectVS.h" // a pass through shader

svtkStandardNewMacro(svtkOpenGLRayCastImageDisplayHelper);

//----------------------------------------------------------------------------
// Construct a new svtkOpenGLRayCastImageDisplayHelper with default values
svtkOpenGLRayCastImageDisplayHelper::svtkOpenGLRayCastImageDisplayHelper()
{
  this->TextureObject = svtkTextureObject::New();
  this->ShaderProgram = nullptr;
}

//----------------------------------------------------------------------------
// Destruct a svtkOpenGLRayCastImageDisplayHelper - clean up any memory used
svtkOpenGLRayCastImageDisplayHelper::~svtkOpenGLRayCastImageDisplayHelper()
{
  if (this->TextureObject)
  {
    this->TextureObject->Delete();
    this->TextureObject = nullptr;
  }
  if (this->ShaderProgram)
  {
    delete this->ShaderProgram;
    this->ShaderProgram = nullptr;
  }
}

//----------------------------------------------------------------------------
// imageMemorySize   is how big the texture is - this is always a power of two
//
// imageViewportSize is how big the renderer viewport is in pixels
//
// imageInUseSize    is the rendered image - equal or smaller than imageMemorySize
//                   and imageViewportSize
//
// imageOrigin       is the starting pixel of the imageInUseSize image on the
//                   the imageViewportSize viewport
//
void svtkOpenGLRayCastImageDisplayHelper::RenderTexture(
  svtkVolume* vol, svtkRenderer* ren, svtkFixedPointRayCastImage* image, float requestedDepth)
{
  this->RenderTextureInternal(vol, ren, image->GetImageMemorySize(), image->GetImageViewportSize(),
    image->GetImageInUseSize(), image->GetImageOrigin(), requestedDepth, SVTK_UNSIGNED_SHORT,
    image->GetImage());
}

//----------------------------------------------------------------------------
void svtkOpenGLRayCastImageDisplayHelper::RenderTexture(svtkVolume* vol, svtkRenderer* ren,
  int imageMemorySize[2], int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2],
  float requestedDepth, unsigned char* image)
{
  this->RenderTextureInternal(vol, ren, imageMemorySize, imageViewportSize, imageInUseSize,
    imageOrigin, requestedDepth, SVTK_UNSIGNED_CHAR, static_cast<void*>(image));
}

//----------------------------------------------------------------------------
void svtkOpenGLRayCastImageDisplayHelper::RenderTexture(svtkVolume* vol, svtkRenderer* ren,
  int imageMemorySize[2], int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2],
  float requestedDepth, unsigned short* image)
{
  this->RenderTextureInternal(vol, ren, imageMemorySize, imageViewportSize, imageInUseSize,
    imageOrigin, requestedDepth, SVTK_UNSIGNED_SHORT, static_cast<void*>(image));
}

//----------------------------------------------------------------------------
void svtkOpenGLRayCastImageDisplayHelper::RenderTextureInternal(svtkVolume* vol, svtkRenderer* ren,
  int imageMemorySize[2], int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2],
  float requestedDepth, int imageScalarType, void* image)
{
  svtkOpenGLClearErrorMacro();

  // Set the context
  svtkOpenGLRenderWindow* ctx = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  this->TextureObject->SetContext(ctx);

  float offsetX, offsetY;

  float depth;
  if (requestedDepth > 0.0 && requestedDepth <= 1.0)
  {
    depth = requestedDepth * 2.0 - 1.0;
  }
  else
  {
    // Pass the center of the volume through the world to view function
    // of the renderer to get the z view coordinate to use for the
    // view to world transformation of the image bounds. This way we
    // will draw the image at the depth of the center of the volume
    ren->SetWorldPoint(vol->GetCenter()[0], vol->GetCenter()[1], vol->GetCenter()[2], 1.0);
    ren->WorldToDisplay();
    depth = ren->GetDisplayPoint()[2];
  }

  // Don't write into the Zbuffer - just use it for comparisons
  svtkOpenGLState* ostate = ctx->GetState();
  ostate->svtkglDepthMask(0);

  this->TextureObject->SetMinificationFilter(svtkTextureObject::Linear);
  this->TextureObject->SetMagnificationFilter(svtkTextureObject::Linear);

  if (imageScalarType == SVTK_UNSIGNED_CHAR)
  {
    this->TextureObject->Create2DFromRaw(imageMemorySize[0], imageMemorySize[1], 4,
      SVTK_UNSIGNED_CHAR, static_cast<unsigned char*>(image));
  }
  else
  {
    this->TextureObject->Create2DFromRaw(imageMemorySize[0], imageMemorySize[1], 4,
      SVTK_UNSIGNED_SHORT, static_cast<unsigned short*>(image));
  }

  offsetX = .5 / static_cast<float>(imageMemorySize[0]);
  offsetY = .5 / static_cast<float>(imageMemorySize[1]);

  float tcoords[8];
  tcoords[0] = 0.0 + offsetX;
  tcoords[1] = 0.0 + offsetY;
  tcoords[2] = (float)imageInUseSize[0] / (float)imageMemorySize[0] - offsetX;
  tcoords[3] = offsetY;
  tcoords[4] = (float)imageInUseSize[0] / (float)imageMemorySize[0] - offsetX;
  tcoords[5] = (float)imageInUseSize[1] / (float)imageMemorySize[1] - offsetY;
  tcoords[6] = offsetX;
  tcoords[7] = (float)imageInUseSize[1] / (float)imageMemorySize[1] - offsetY;

  float verts[12] = { 2.0f * imageOrigin[0] / imageViewportSize[0] - 1.0f,
    2.0f * imageOrigin[1] / imageViewportSize[1] - 1.0f, depth,
    2.0f * (imageOrigin[0] + imageInUseSize[0]) / imageViewportSize[0] - 1.0f,
    2.0f * imageOrigin[1] / imageViewportSize[1] - 1.0f, depth,
    2.0f * (imageOrigin[0] + imageInUseSize[0]) / imageViewportSize[0] - 1.0f,
    2.0f * (imageOrigin[1] + imageInUseSize[1]) / imageViewportSize[1] - 1.0f, depth,
    2.0f * imageOrigin[0] / imageViewportSize[0] - 1.0f,
    2.0f * (imageOrigin[1] + imageInUseSize[1]) / imageViewportSize[1] - 1.0f, depth };

  if (!this->ShaderProgram)
  {
    this->ShaderProgram = new svtkOpenGLHelper;

    // build the shader source code
    std::string VSSource = svtkTextureObjectVS;
    std::string FSSource = "//SVTK::System::Dec\n"
                           "//SVTK::Output::Dec\n"
                           "in vec2 tcoordVC;\n"
                           "uniform sampler2D source;\n"
                           "uniform float scale;\n"
                           "void main(void)\n"
                           "{\n"
                           "  gl_FragData[0] = texture2D(source,tcoordVC)*scale;\n"
                           "}\n";
    std::string GSSource;

    // compile and bind it if needed
    svtkShaderProgram* newShader = ctx->GetShaderCache()->ReadyShaderProgram(
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
    ctx->GetShaderCache()->ReadyShaderProgram(this->ShaderProgram->Program);
  }

  ostate->svtkglEnable(GL_BLEND);

  // backup current GL blend state
  svtkOpenGLState::ScopedglBlendFuncSeparate bfsaver(ostate);

  if (this->PreMultipliedColors)
  {
    // make the blend function correct for textures premultiplied by alpha.
    ostate->svtkglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }

  // bind and activate this texture
  this->TextureObject->Activate();
  int sourceId = this->TextureObject->GetTextureUnit();
  this->ShaderProgram->Program->SetUniformi("source", sourceId);
  this->ShaderProgram->Program->SetUniformf("scale", this->PixelScale);
  svtkOpenGLRenderUtilities::RenderQuad(
    verts, tcoords, this->ShaderProgram->Program, this->ShaderProgram->VAO);
  this->TextureObject->Deactivate();

  svtkOpenGLCheckErrorMacro("failed after RenderTextureInternal");
}

//----------------------------------------------------------------------------
void svtkOpenGLRayCastImageDisplayHelper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkOpenGLRayCastImageDisplayHelper::ReleaseGraphicsResources(svtkWindow* win)
{
  this->TextureObject->ReleaseGraphicsResources(win);
  if (this->ShaderProgram)
  {
    this->ShaderProgram->ReleaseGraphicsResources(win);
    delete this->ShaderProgram;
    this->ShaderProgram = nullptr;
  }
}
