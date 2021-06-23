/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLProperty.h"
#include "svtkOpenGLRenderer.h"

#include "svtkOpenGLHelper.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLState.h"
#include "svtkOpenGLTexture.h"
#include "svtkTexture.h"

#include "svtkOpenGLError.h"

#include <cassert>

svtkStandardNewMacro(svtkOpenGLProperty);

svtkOpenGLProperty::svtkOpenGLProperty() = default;

svtkOpenGLProperty::~svtkOpenGLProperty() = default;

// ----------------------------------------------------------------------------
// Implement base class method.
void svtkOpenGLProperty::Render(svtkActor* anActor, svtkRenderer* ren)
{
  // turn on/off backface culling
  svtkOpenGLState* ostate = static_cast<svtkOpenGLRenderer*>(ren)->GetState();
  if (!this->BackfaceCulling && !this->FrontfaceCulling)
  {
    ostate->svtkglDisable(GL_CULL_FACE);
  }
  else if (this->BackfaceCulling)
  {
    ostate->svtkglCullFace(GL_BACK);
    ostate->svtkglEnable(GL_CULL_FACE);
  }
  else // if both front & back culling on, will fall into backface culling
  {    // if you really want both front and back, use the Actor's visibility flag
    ostate->svtkglCullFace(GL_FRONT);
    ostate->svtkglEnable(GL_CULL_FACE);
  }

  this->RenderTextures(anActor, ren);
  this->Superclass::Render(anActor, ren);
}

//-----------------------------------------------------------------------------
bool svtkOpenGLProperty::RenderTextures(svtkActor*, svtkRenderer* ren)
{
  // render any textures.
  auto textures = this->GetAllTextures();
  for (auto ti : textures)
  {
    ti.second->Render(ren);
  }

  svtkOpenGLCheckErrorMacro("failed after Render");

  return (!textures.empty());
}

//-----------------------------------------------------------------------------
void svtkOpenGLProperty::PostRender(svtkActor* actor, svtkRenderer* renderer)
{
  svtkOpenGLClearErrorMacro();

  // Reset the face culling now we are done, leaking into text actor etc.
  if (this->BackfaceCulling || this->FrontfaceCulling)
  {
    static_cast<svtkOpenGLRenderer*>(renderer)->GetState()->svtkglDisable(GL_CULL_FACE);
  }

  // deactivate any textures.
  auto textures = this->GetAllTextures();
  for (auto ti : textures)
  {
    ti.second->PostRender(renderer);
  }

  this->Superclass::PostRender(actor, renderer);

  svtkOpenGLCheckErrorMacro("failed after PostRender");
}

//-----------------------------------------------------------------------------
// Implement base class method.
void svtkOpenGLProperty::BackfaceRender(svtkActor* svtkNotUsed(anActor), svtkRenderer* svtkNotUsed(ren))
{
}

//-----------------------------------------------------------------------------
void svtkOpenGLProperty::ReleaseGraphicsResources(svtkWindow* win)
{
  // release any textures.
  auto textures = this->GetAllTextures();
  for (auto ti : textures)
  {
    ti.second->ReleaseGraphicsResources(win);
  }

  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void svtkOpenGLProperty::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
