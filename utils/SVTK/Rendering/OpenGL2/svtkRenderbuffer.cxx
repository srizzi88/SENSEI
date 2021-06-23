/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderbuffer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRenderbuffer.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtk_glew.h"

#include <cassert>

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkRenderbuffer);

//----------------------------------------------------------------------------
svtkRenderbuffer::svtkRenderbuffer()
{
  this->Context = nullptr;
  this->Handle = 0U;
  this->DepthBufferFloat = 0;
  this->Samples = 0;
  this->Format = GL_RGBA;
}

//----------------------------------------------------------------------------
svtkRenderbuffer::~svtkRenderbuffer()
{
  this->Free();
}

//----------------------------------------------------------------------------
bool svtkRenderbuffer::IsSupported(svtkRenderWindow*)
{
  return true;
}

//----------------------------------------------------------------------------
bool svtkRenderbuffer::LoadRequiredExtensions(svtkRenderWindow*)
{
  // both texture float and depth float are part of OpenGL 3.0 and later
  this->DepthBufferFloat = true;
  return true;
}

//----------------------------------------------------------------------------
void svtkRenderbuffer::Alloc()
{
  glGenRenderbuffers(1, &this->Handle);
  svtkOpenGLCheckErrorMacro("failed at glGenRenderbuffers");
}

void svtkRenderbuffer::ReleaseGraphicsResources(svtkWindow*)
{
  if (this->Context && this->Handle)
  {
    glDeleteRenderbuffers(1, &this->Handle);
    svtkOpenGLCheckErrorMacro("failed at glDeleteRenderBuffers");
  }
}

//----------------------------------------------------------------------------
void svtkRenderbuffer::Free()
{
  this->ReleaseGraphicsResources(nullptr);
}

//----------------------------------------------------------------------------
svtkRenderWindow* svtkRenderbuffer::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void svtkRenderbuffer::SetContext(svtkRenderWindow* renWin)
{
  // avoid pointless re-assignment
  if (this->Context == renWin)
  {
    return;
  }

  // free previous resources
  this->Free();
  this->Context = nullptr;
  this->DepthBufferFloat = 0;
  this->Modified();

  // check for supported context
  svtkOpenGLRenderWindow* context = dynamic_cast<svtkOpenGLRenderWindow*>(renWin);
  if (!context || !this->LoadRequiredExtensions(renWin))
  {
    svtkErrorMacro("Unsupported render context");
    return;
  }

  // allocate new fbo
  this->Context = renWin;
  this->Context->MakeCurrent();
  this->Alloc();
}
//----------------------------------------------------------------------------
int svtkRenderbuffer::CreateColorAttachment(unsigned int width, unsigned int height)
{
  assert(this->Context);
  return this->Create(GL_RGBA32F, width, height);
}

//----------------------------------------------------------------------------
int svtkRenderbuffer::CreateDepthAttachment(unsigned int width, unsigned int height)
{
  assert(this->Context);

  // typically DEPTH_COMPONENT will end up being a 32 bit floating
  // point format however it's not a guarantee and does not seem
  // to be the case with mesa hence the need to explicitly specify
  // it as such if at all possible.
  if (this->DepthBufferFloat)
  {
    return this->Create(GL_DEPTH_COMPONENT32F, width, height);
  }

  return this->Create(GL_DEPTH_COMPONENT, width, height);
}

//----------------------------------------------------------------------------
int svtkRenderbuffer::Create(unsigned int format, unsigned int width, unsigned int height)
{
  return this->Create(format, width, height, 0);
}

int svtkRenderbuffer::Create(
  unsigned int format, unsigned int width, unsigned int height, unsigned int samples)
{
  assert(this->Context);

  glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)this->Handle);
  svtkOpenGLCheckErrorMacro("failed at glBindRenderBuffer");

  if (samples)
  {
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, (GLenum)format, width, height);
  }
  else
  {
    glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)format, width, height);
  }
  svtkOpenGLCheckErrorMacro("failed at glRenderbufferStorage with format: "
    << format << " and size " << width << " by " << height);

  this->Width = width;
  this->Height = height;
  this->Format = format;
  this->Samples = samples;

  return 1;
}

void svtkRenderbuffer::Resize(unsigned int width, unsigned int height)
{
  if (this->Width == width && this->Height == height)
  {
    return;
  }

  if (this->Context && this->Handle)
  {
    glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)this->Handle);
    if (this->Samples)
    {
      glRenderbufferStorageMultisample(
        GL_RENDERBUFFER, this->Samples, (GLenum)this->Format, width, height);
    }
    else
    {
      glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)this->Format, width, height);
    }
  }
  this->Width = width;
  this->Height = height;
}

// ----------------------------------------------------------------------------
void svtkRenderbuffer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Handle=" << this->Handle << endl
     << indent << "Context=" << this->Context << endl;
}
