/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLFramebufferObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLFramebufferObject.h"

#include "svtk_glew.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLBufferObject.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLResourceFreeCallback.h"
#include "svtkOpenGLState.h"
#include "svtkPixelBufferObject.h"
#include "svtkRenderbuffer.h"
#include "svtkTextureObject.h"

#include <cassert>
#include <vector>
using std::vector;

class svtkFOInfo
{
public:
  unsigned int Attachment;
  unsigned int Target;
  unsigned int MipmapLevel;
  bool Attached;
  svtkTextureObject* Texture;
  svtkRenderbuffer* Renderbuffer;
  bool CreatedByFO;
  unsigned int ZSlice;

  svtkFOInfo()
  {
    this->Attachment = 0;
    this->Target = 0;
    this->MipmapLevel = 0;
    this->Texture = nullptr;
    this->Renderbuffer = nullptr;
    this->CreatedByFO = false;
    this->ZSlice = 0;
    this->Attached = false;
  }

  ~svtkFOInfo() { this->Clear(); }

  void Clear()
  {
    if (this->Texture)
    {
      this->Texture->Delete();
      this->Texture = nullptr;
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->Delete();
      this->Renderbuffer = nullptr;
    }
    this->Attachment = 0;
    this->Target = 0;
    this->MipmapLevel = 0;
    this->CreatedByFO = false;
    this->ZSlice = 0;
    this->Attached = false;
  }

  bool IsSet() { return this->Texture || this->Renderbuffer; }

  void ReleaseGraphicsResources(svtkWindow* win)
  {
    if (this->Texture)
    {
      this->Texture->ReleaseGraphicsResources(win);
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->ReleaseGraphicsResources(win);
    }
  }

  void Attach(int mode)
  {
    if (this->Attached)
    {
      return;
    }
    if (this->Texture)
    {
      if (this->Texture->GetNumberOfDimensions() == 3)
      {
#ifndef GL_ES_VERSION_3_0
        glFramebufferTexture3D((GLenum)mode, this->Attachment, this->Target,
          this->Texture->GetHandle(), this->MipmapLevel, this->ZSlice);
        this->Attached = true;
#else
        svtkGenericWarningMacro("Attempt to use 3D frame buffer texture in OpenGL ES 2 or 3");
#endif
      }
      else
      {
        glFramebufferTexture2D((GLenum)mode, this->Attachment, this->Target,
          this->Texture->GetHandle(), this->MipmapLevel);
        this->Attached = true;
      }
    }
    else if (this->Renderbuffer)
    {
      glFramebufferRenderbuffer(
        (GLenum)mode, this->Attachment, GL_RENDERBUFFER, this->Renderbuffer->GetHandle());
      this->Attached = true;
    }
  }

  void Detach(int mode)
  {
    if (!this->Attached)
    {
      return;
    }
    if (this->Texture)
    {
      if (this->Texture->GetNumberOfDimensions() == 3)
      {
#ifndef GL_ES_VERSION_3_0
        glFramebufferTexture3D(
          (GLenum)mode, this->Attachment, this->Target, 0, this->MipmapLevel, this->ZSlice);
        this->Attached = false;
#else
        svtkGenericWarningMacro("Attempt to use 3D frame buffer texture in OpenGL ES 2 or 3");
#endif
      }
      else
      {
        glFramebufferTexture2D((GLenum)mode, this->Attachment, this->Target, 0, this->MipmapLevel);
        this->Attached = false;
      }
    }
    else if (this->Renderbuffer)
    {
      glFramebufferRenderbuffer((GLenum)mode, this->Attachment, GL_RENDERBUFFER, 0);
      this->Attached = false;
    }
  }

  void SetTexture(svtkTextureObject* val, unsigned int attachment, unsigned int target = 0,
    unsigned int mipmapLevel = 0)
  {

    // always reset to false
    this->CreatedByFO = false;

    if (this->Texture == val && this->Attachment == attachment)
    {
      return;
    }
    this->Attached = false;
    val->Register(nullptr);
    if (this->Texture)
    {
      this->Texture->Delete();
      this->Texture = nullptr;
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->Delete();
      this->Renderbuffer = nullptr;
    }
    this->Texture = val;
    this->Attachment = attachment;
    // if target not specified, used texture target
    // a custom target is useful for cubemap
    this->Target = target ? target : val->GetTarget();
    this->MipmapLevel = mipmapLevel;
  }

  void SetRenderbuffer(svtkRenderbuffer* val, unsigned int attachment)
  {

    // always reset to false
    this->CreatedByFO = false;

    if (this->Renderbuffer == val && this->Attachment == attachment)
    {
      return;
    }
    this->Attached = false;
    val->Register(nullptr);
    if (this->Texture)
    {
      this->Texture->Delete();
      this->Texture = nullptr;
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->Delete();
      this->Renderbuffer = nullptr;
    }
    this->Renderbuffer = val;
    this->Attachment = attachment;
  }

  int GetSamples()
  {
    if (this->Texture)
    {
      return this->Texture->GetSamples();
    }
    if (this->Renderbuffer)
    {
      return this->Renderbuffer->GetSamples();
    }
    return 0;
  }

  void GetSize(int (&size)[2])
  {
    if (this->Texture)
    {
      size[0] = this->Texture->GetWidth() >> this->MipmapLevel;
      size[1] = this->Texture->GetHeight() >> this->MipmapLevel;
      return;
    }
    if (this->Renderbuffer)
    {
      size[0] = this->Renderbuffer->GetWidth();
      size[1] = this->Renderbuffer->GetHeight();
      return;
    }
  }

  void Resize(int size[2])
  {
    if (this->Texture)
    {
      this->Texture->Resize(size[0], size[1]);
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->Resize(size[0], size[1]);
    }
  }
};

typedef std::map<unsigned int, svtkFOInfo*>::iterator foIter;

// #define SVTK_FBO_DEBUG // display info on RenderQuad()

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLFramebufferObject);

//----------------------------------------------------------------------------
svtkOpenGLFramebufferObject::svtkOpenGLFramebufferObject()
{
  this->Context = nullptr;
  this->FBOIndex = 0;

  this->DrawBindingSaved = false;
  this->ReadBindingSaved = false;
  this->DrawBufferSaved = false;
  this->ReadBufferSaved = false;

  this->ActiveReadBuffer = GL_COLOR_ATTACHMENT0;

  this->LastSize[0] = this->LastSize[1] = -1;

  this->DepthBuffer = new svtkFOInfo;

  this->ActiveBuffers.push_back(0);

  this->ResourceCallback = new svtkOpenGLResourceFreeCallback<svtkOpenGLFramebufferObject>(
    this, &svtkOpenGLFramebufferObject::ReleaseGraphicsResources);
}

//----------------------------------------------------------------------------
svtkOpenGLFramebufferObject::~svtkOpenGLFramebufferObject()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = nullptr;
  }
  delete this->DepthBuffer;
  for (foIter i = this->ColorBuffers.begin(); i != this->ColorBuffers.end(); ++i)
  {
    delete i->second;
  }
  this->ColorBuffers.clear();
  this->Context = nullptr;
}

//-----------------------------------------------------------------------------
int svtkOpenGLFramebufferObject::GetOpenGLType(int svtkType)
{
  // convert svtk type to open gl type
  int oglType = 0;
  switch (svtkType)
  {
    case SVTK_FLOAT:
      oglType = GL_FLOAT;
      break;
    case SVTK_INT:
      oglType = GL_INT;
      break;
    case SVTK_UNSIGNED_INT:
      oglType = GL_UNSIGNED_INT;
      break;
    case SVTK_CHAR:
      oglType = GL_BYTE;
      break;
    case SVTK_UNSIGNED_CHAR:
      oglType = GL_UNSIGNED_BYTE;
      break;
    default:
      svtkErrorMacro("Unsupported type");
      return 0;
  }
  return oglType;
}

unsigned int svtkOpenGLFramebufferObject::GetDrawMode()
{
  return GL_DRAW_FRAMEBUFFER;
}
unsigned int svtkOpenGLFramebufferObject::GetReadMode()
{
  return GL_READ_FRAMEBUFFER;
}
unsigned int svtkOpenGLFramebufferObject::GetBothMode()
{
  return GL_FRAMEBUFFER;
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::CreateFBO()
{
  if (!this->FBOIndex)
  {
    this->ResourceCallback->RegisterGraphicsResources(this->Context);
    this->FBOIndex = 0;
    GLuint temp;
    glGenFramebuffers(1, &temp);
    svtkOpenGLCheckErrorMacro("failed at glGenFramebuffers");
    this->FBOIndex = temp;
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::DestroyFBO()
{
  if (this->FBOIndex != 0)
  {
    GLuint fbo = static_cast<GLuint>(this->FBOIndex);
    glDeleteFramebuffers(1, &fbo);
    svtkOpenGLCheckErrorMacro("failed at glDeleteFramebuffers");
    this->FBOIndex = 0;
  }
}

void svtkOpenGLFramebufferObject::ReleaseGraphicsResources(svtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  // free previous resources
  this->DestroyDepthBuffer(win);
  this->DestroyColorBuffers(win);
  this->DestroyFBO();
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::SetContext(svtkRenderWindow* rw)
{
  svtkOpenGLRenderWindow* renWin = static_cast<svtkOpenGLRenderWindow*>(rw);

  // avoid pointless re-assignment
  if (this->Context == renWin)
  {
    return;
  }

  // all done if assigned null
  if (!renWin)
  {
    return;
  }
  // check for support
  if (!this->LoadRequiredExtensions(renWin))
  {
    svtkErrorMacro("Context does not support the required extensions");
    return;
  }
  // initialize
  this->Context = renWin;
}

//----------------------------------------------------------------------------
svtkOpenGLRenderWindow* svtkOpenGLFramebufferObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::InitializeViewport(int width, int height)
{
  svtkOpenGLState* ostate = this->Context->GetState();
  ostate->svtkglDisable(GL_BLEND);
  ostate->svtkglDisable(GL_DEPTH_TEST);
  ostate->svtkglDisable(GL_SCISSOR_TEST);

  // Viewport transformation for 1:1 'pixel=texel=data' mapping.
  // Note this is not enough for 1:1 mapping, because depending on the
  // primitive displayed (point,line,polygon), the rasterization rules
  // are different.
  ostate->svtkglViewport(0, 0, width, height);

  svtkOpenGLStaticCheckErrorMacro("failed after InitializeViewport");
}

bool svtkOpenGLFramebufferObject::StartNonOrtho(int width, int height)
{
  this->Bind();

  // make sure sizes are consistent for all attachments
  // this will adjust the depth buffer size if we
  // created it.
  this->UpdateSize();

  // If width/height does not match attachments error
  if (this->LastSize[0] != width || this->LastSize[1] != height)
  {
    svtkErrorMacro("FBO size does not match the size of its attachments!.");
  }

  this->ActivateBuffers();

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    svtkErrorMacro("Frame buffer object was not initialized correctly.");
    this->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    this->DisplayFrameBufferAttachments();
    this->DisplayDrawBuffers();
    this->DisplayReadBuffer();
    return false;
  }

  return true;
}

void svtkOpenGLFramebufferObject::UpdateSize()
{
  bool foundSize = false;
  int size[2];
  int aSize[2];
  bool mismatch = false;

  size[0] = 0;
  size[1] = 0;
  aSize[0] = 0;
  aSize[1] = 0;

  // loop through all attachments and
  // verify they are of the same size.
  for (foIter i = this->ColorBuffers.begin(); i != this->ColorBuffers.end(); ++i)
  {
    if (!i->second->CreatedByFO && i->second->IsSet())
    {
      i->second->GetSize(aSize);
      if (!foundSize)
      {
        size[0] = aSize[0];
        size[1] = aSize[1];
        foundSize = true;
      }
      else
      {
        if (aSize[0] != size[0] || aSize[1] != size[1])
        {
          mismatch = true;
        }
      }
    }
  }

  if (!this->DepthBuffer->CreatedByFO && this->DepthBuffer->IsSet())
  {
    this->DepthBuffer->GetSize(aSize);
    if (!foundSize)
    {
      size[0] = aSize[0];
      size[1] = aSize[1];
      foundSize = true;
    }
    else
    {
      if (aSize[0] != size[0] || aSize[1] != size[1])
      {
        mismatch = true;
      }
    }
  }

  if (mismatch)
  {
    svtkErrorMacro("The framebuffer has mismatched attachments.");
  }

  // resize any FO created items
  this->LastSize[0] = size[0];
  this->LastSize[1] = size[1];

  // now resize any buffers we created that are the wrong size
  if (this->DepthBuffer->IsSet() && this->DepthBuffer->CreatedByFO)
  {
    this->DepthBuffer->Resize(this->LastSize);
  }
}

void svtkOpenGLFramebufferObject::Resize(int width, int height)
{
  // resize all items
  this->LastSize[0] = width;
  this->LastSize[1] = height;

  // loop through all attachments and
  // verify they are of the same size.
  for (foIter i = this->ColorBuffers.begin(); i != this->ColorBuffers.end(); ++i)
  {
    i->second->Resize(this->LastSize);
  }

  // now resize any buffers we created that are the wrong size
  if (this->DepthBuffer->IsSet())
  {
    this->DepthBuffer->Resize(this->LastSize);
  }
}

//----------------------------------------------------------------------------
bool svtkOpenGLFramebufferObject::Start(int width, int height)
{
  if (!this->StartNonOrtho(width, height))
  {
    return false;
  }

  this->InitializeViewport(width, height);
  return true;
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::ActivateBuffers()
{
  GLint maxbuffers;
  // todo move to cache
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);

  GLenum* buffers = new GLenum[maxbuffers];
  GLint count = 0;
  for (unsigned int cc = 0; cc < this->ActiveBuffers.size() && count < maxbuffers; cc++)
  {
    buffers[cc] = GL_COLOR_ATTACHMENT0 + this->ActiveBuffers[cc];
    count++;
  }

  this->Context->GetState()->svtkDrawBuffers(count, buffers, this);

  delete[] buffers;
}

void svtkOpenGLFramebufferObject::ActivateDrawBuffer(unsigned int num)
{
  this->ActivateDrawBuffers(&num, 1);
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::ActivateReadBuffer(unsigned int colorAtt)
{
  colorAtt += GL_COLOR_ATTACHMENT0;
  this->Context->GetState()->svtkReadBuffer((GLenum)colorAtt, this);
  this->ActiveReadBuffer = colorAtt;
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::ActivateDrawBuffers(unsigned int num)
{
  GLint maxbuffers;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);

  GLenum* buffers = new GLenum[maxbuffers];
  GLint count = 0;
  for (unsigned int cc = 0; cc < num && count < maxbuffers; cc++)
  {
    buffers[cc] = GL_COLOR_ATTACHMENT0 + cc;
    count++;
  }

  this->Context->GetState()->svtkDrawBuffers(count, buffers, this);
  delete[] buffers;

  this->ActiveBuffers.clear();
  for (unsigned int cc = 0; cc < num; cc++)
  {
    this->ActiveBuffers.push_back(cc);
  }
  this->Modified();
}

unsigned int svtkOpenGLFramebufferObject::GetActiveDrawBuffer(unsigned int id)
{
  if (id >= this->ActiveBuffers.size())
  {
    return GL_NONE;
  }
  return GL_COLOR_ATTACHMENT0 + this->ActiveBuffers[id];
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::ActivateDrawBuffers(unsigned int* ids, int num)
{
  GLint maxbuffers;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);

  GLenum* buffers = new GLenum[maxbuffers];
  GLint count = 0;
  for (unsigned int cc = 0; cc < static_cast<unsigned int>(num) && count < maxbuffers; cc++)
  {
    buffers[cc] = GL_COLOR_ATTACHMENT0 + ids[cc];
    count++;
  }

  this->Context->GetState()->svtkDrawBuffers(count, buffers, this);
  delete[] buffers;

  this->ActiveBuffers.clear();
  for (int cc = 0; cc < num; cc++)
  {
    this->ActiveBuffers.push_back(ids[cc]);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::DeactivateDrawBuffers()
{
  GLenum att = GL_NONE;
  this->Context->GetState()->svtkDrawBuffers(1, &att, this);
  this->ActiveBuffers.clear();
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::DeactivateReadBuffer()
{
  this->Context->GetState()->svtkReadBuffer(GL_NONE, this);
  this->ActiveReadBuffer = GL_NONE;
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::SaveCurrentBindingsAndBuffers()
{
  this->SaveCurrentBindingsAndBuffers(GL_FRAMEBUFFER);
}

void svtkOpenGLFramebufferObject::SaveCurrentBindingsAndBuffers(unsigned int mode)
{
  if (!this->Context)
  {
    svtkErrorMacro("Attempt to save bindings without a context");
    return;
  }
  if (mode == GL_FRAMEBUFFER || mode == GL_DRAW_FRAMEBUFFER)
  {
    this->Context->GetState()->PushDrawFramebufferBinding();
    this->DrawBindingSaved = true;
  }
  if (mode == GL_FRAMEBUFFER || mode == GL_READ_FRAMEBUFFER)
  {
    this->Context->GetState()->PushReadFramebufferBinding();
    this->ReadBindingSaved = true;
  }
}

void svtkOpenGLFramebufferObject::RestorePreviousBindingsAndBuffers()
{
  this->RestorePreviousBindingsAndBuffers(GL_FRAMEBUFFER);
}

void svtkOpenGLFramebufferObject::RestorePreviousBindingsAndBuffers(unsigned int mode)
{
  if (!this->Context)
  {
    svtkErrorMacro("Attempt to restore bindings without a context");
    return;
  }
  if ((mode == GL_FRAMEBUFFER || mode == GL_DRAW_FRAMEBUFFER) && this->DrawBindingSaved)
  {
    this->Context->GetState()->PopDrawFramebufferBinding();
    this->DrawBindingSaved = false;
    this->DrawBufferSaved = false;
  }
  if ((mode == GL_FRAMEBUFFER || mode == GL_READ_FRAMEBUFFER) && this->ReadBindingSaved)
  {
    this->Context->GetState()->PopReadFramebufferBinding();
    this->ReadBindingSaved = false;
    this->ReadBufferSaved = false;
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::Bind()
{
  this->Bind(GL_FRAMEBUFFER);
}

void svtkOpenGLFramebufferObject::Bind(unsigned int mode)
{
  if (!this->Context)
  {
    svtkErrorMacro("Attempt to bind framebuffer without a context");
    return;
  }
  this->Context->MakeCurrent();
  this->CreateFBO();
  if (this->FBOIndex != 0)
  {
    // note this also changes the draw/read buffers as they are
    // tied to the binding
    this->Context->GetState()->svtkBindFramebuffer(mode, this);
  }
}

void svtkOpenGLFramebufferObject::AttachColorBuffer(unsigned int index)
{
  if (this->FBOIndex != 0)
  {
    foIter i = this->ColorBuffers.find(index);
    this->Context->GetState()->PushDrawFramebufferBinding();
    this->Context->GetState()->svtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, this);
    if (i != this->ColorBuffers.end())
    {
      i->second->Attach(GL_DRAW_FRAMEBUFFER);
    }
    this->Context->GetState()->PopDrawFramebufferBinding();
  }
}

void svtkOpenGLFramebufferObject::AttachDepthBuffer()
{
  if (this->FBOIndex != 0)
  {
    this->Context->GetState()->PushDrawFramebufferBinding();
    this->Context->GetState()->svtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, this);
    this->DepthBuffer->Attach(GL_DRAW_FRAMEBUFFER);
    this->Context->GetState()->PopDrawFramebufferBinding();
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::UnBind()
{
  if (this->FBOIndex != 0)
  {
    this->Context->GetState()->svtkBindFramebuffer(GL_FRAMEBUFFER, nullptr);
  }
}

void svtkOpenGLFramebufferObject::UnBind(unsigned int mode)
{
  if (this->FBOIndex != 0)
  {
    this->Context->GetState()->svtkBindFramebuffer(mode, nullptr);
  }
}

void svtkOpenGLFramebufferObject::AddDepthAttachment()
{
  // create as needed
  if (!this->DepthBuffer->IsSet())
  {
    // create a renderbuffer
    svtkRenderbuffer* rb = svtkRenderbuffer::New();
    rb->SetContext(this->Context);
    rb->CreateDepthAttachment(this->LastSize[0], this->LastSize[1]);

    this->AddDepthAttachment(rb);
    this->DepthBuffer->CreatedByFO = true;
    rb->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::DestroyDepthBuffer(svtkWindow*)
{
  this->DepthBuffer->Clear();
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::DestroyColorBuffers(svtkWindow*)
{
  for (foIter i = this->ColorBuffers.begin(); i != this->ColorBuffers.end(); ++i)
  {
    i->second->Clear();
  }
}

//----------------------------------------------------------------------------
unsigned int svtkOpenGLFramebufferObject::GetMaximumNumberOfActiveTargets()
{
  unsigned int result = 0;
  if (this->Context)
  {
    GLint maxbuffers;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);
    result = static_cast<unsigned int>(maxbuffers);
  }
  return result;
}

//----------------------------------------------------------------------------
unsigned int svtkOpenGLFramebufferObject::GetMaximumNumberOfRenderTargets()
{
  unsigned int result = 0;
  if (this->Context)
  {
    GLint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    result = static_cast<unsigned int>(maxColorAttachments);
  }
  return result;
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::RemoveDepthAttachment()
{
  if (this->FBOIndex != 0)
  {
    this->Context->GetState()->PushDrawFramebufferBinding();
    this->Context->GetState()->svtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, this);
    this->DepthBuffer->Detach(GL_DRAW_FRAMEBUFFER);
    this->Context->GetState()->PopDrawFramebufferBinding();
  }
  delete this->DepthBuffer;
  this->DepthBuffer = new svtkFOInfo;
}

void svtkOpenGLFramebufferObject::AddDepthAttachment(svtkTextureObject* tex)
{
  this->DepthBuffer->SetTexture(tex, GL_DEPTH_ATTACHMENT);
  this->AttachDepthBuffer();
}

void svtkOpenGLFramebufferObject::AddDepthAttachment(svtkRenderbuffer* rb)
{
  this->DepthBuffer->SetRenderbuffer(rb, GL_DEPTH_ATTACHMENT);
  this->AttachDepthBuffer();
}

void svtkOpenGLFramebufferObject::AddColorAttachment(unsigned int index, svtkTextureObject* tex,
  unsigned int zslice, unsigned int format, unsigned int mipmapLevel)
{
  // is the fbo size is not set do it here
  if (this->LastSize[0] == -1)
  {
    this->LastSize[0] = tex->GetWidth();
    this->LastSize[1] = tex->GetHeight();
  }

  foIter i = this->ColorBuffers.find(index);
  if (i == this->ColorBuffers.end())
  {
    svtkFOInfo* foinfo = new svtkFOInfo;
    i = this->ColorBuffers.insert(std::make_pair(index, foinfo)).first;
  }
  i->second->SetTexture(tex, GL_COLOR_ATTACHMENT0 + index, format, mipmapLevel);
  i->second->ZSlice = zslice;
  this->AttachColorBuffer(index);
}

void svtkOpenGLFramebufferObject::AddColorAttachment(unsigned int index, svtkRenderbuffer* rb)
{
  // is the fbo size is not set do it here
  if (this->LastSize[0] == -1)
  {
    this->LastSize[0] = rb->GetWidth();
    this->LastSize[1] = rb->GetHeight();
  }

  foIter i = this->ColorBuffers.find(index);
  if (i == this->ColorBuffers.end())
  {
    svtkFOInfo* foinfo = new svtkFOInfo;
    i = this->ColorBuffers.insert(std::make_pair(index, foinfo)).first;
  }
  i->second->SetRenderbuffer(rb, GL_COLOR_ATTACHMENT0 + index);
  this->AttachColorBuffer(index);
}

//----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::RemoveColorAttachments(unsigned int num)
{
  for (unsigned int i = 0; i < num; ++i)
  {
    this->RemoveColorAttachment(i);
  }
}

void svtkOpenGLFramebufferObject::RemoveColorAttachment(unsigned int index)
{
  foIter i = this->ColorBuffers.find(index);

  if (i != this->ColorBuffers.end())
  {
    if (this->FBOIndex != 0)
    {
      this->Context->GetState()->PushDrawFramebufferBinding();
      this->Context->GetState()->svtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, this);
      i->second->Detach(GL_DRAW_FRAMEBUFFER);
      this->Context->GetState()->PopDrawFramebufferBinding();
    }
    delete i->second;
    i->second = nullptr;
    this->ColorBuffers.erase(i);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display all the attachments of the current framebuffer object.
void svtkOpenGLFramebufferObject::DisplayFrameBufferAttachments()
{
  GLint framebufferBinding;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebufferBinding);
  svtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_BINDING");
  if (framebufferBinding == 0)
  {
    cout << "Current framebuffer is bind to the system one" << endl;
  }
  else
  {
    cout << "Current framebuffer is bind to framebuffer object " << framebufferBinding << endl;

    GLint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    svtkOpenGLCheckErrorMacro("after getting MAX_COLOR_ATTACHMENTS");
    int i = 0;
    while (i < maxColorAttachments)
    {
      cout << "color attachment " << i << ":" << endl;
      this->DisplayFrameBufferAttachment(GL_COLOR_ATTACHMENT0 + i);
      ++i;
    }
    cout << "depth attachment :" << endl;
    this->DisplayFrameBufferAttachment(GL_DEPTH_ATTACHMENT);
    cout << "stencil attachment :" << endl;
    this->DisplayFrameBufferAttachment(GL_STENCIL_ATTACHMENT);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display a given attachment for the current framebuffer object.
void svtkOpenGLFramebufferObject::DisplayFrameBufferAttachment(unsigned int uattachment)
{
  GLenum attachment = static_cast<GLenum>(uattachment);

  GLint params;
  glGetFramebufferAttachmentParameteriv(
    GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &params);

  svtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE");

  switch (params)
  {
    case GL_NONE:
      cout << " this attachment is empty" << endl;
      break;
    case GL_TEXTURE:
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params);
      svtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME");
      cout << " this attachment is a texture with name: " << params << endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &params);
      svtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL");
      cout << " its mipmap level is: " << params << endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, &params);
      svtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE");
      if (params == 0)
      {
        cout << " this is not a cube map texture." << endl;
      }
      else
      {
        cout << " this is a cube map texture and the image is contained in face " << params << endl;
      }
#ifdef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET, &params);
#endif
      svtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET");
      if (params == 0)
      {
        cout << " this is not 3D texture." << endl;
      }
      else
      {
        cout << " this is a 3D texture and the zoffset of the attached image is " << params << endl;
      }
      break;
    case GL_RENDERBUFFER:
      cout << " this attachment is a renderbuffer" << endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params);
      //      this->PrintError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME");
      cout << " this attachment is a renderbuffer with name: " << params << endl;

      glBindRenderbuffer(GL_RENDERBUFFER, params);
      //      this->PrintError(
      //        "after getting binding the current RENDERBUFFER to params");

      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &params);
      //      this->PrintError("after getting RENDERBUFFER_WIDTH");
      cout << " renderbuffer width=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &params);
      //      this->PrintError("after getting RENDERBUFFER_HEIGHT");
      cout << " renderbuffer height=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &params);
      //      this->PrintError("after getting RENDERBUFFER_INTERNAL_FORMAT");

      cout << " renderbuffer internal format=0x" << std::hex << params << std::dec << endl;

      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_RED_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_RED_SIZE");
      cout << " renderbuffer actual resolution for the red component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_GREEN_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_GREEN_SIZE");
      cout << " renderbuffer actual resolution for the green component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_BLUE_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_BLUE_SIZE");
      cout << " renderbuffer actual resolution for the blue component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_ALPHA_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_ALPHA_SIZE");
      cout << " renderbuffer actual resolution for the alpha component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_DEPTH_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_DEPTH_SIZE");
      cout << " renderbuffer actual resolution for the depth component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_STENCIL_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_STENCIL_SIZE");
      cout << " renderbuffer actual resolution for the stencil component=" << params << endl;
      break;
    default:
      cout << " unexcepted value." << endl;
      break;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display the draw buffers.
void svtkOpenGLFramebufferObject::DisplayDrawBuffers()
{
  GLint ivalue = 1;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &ivalue);

  cout << "there ";
  if (ivalue == 1)
  {
    cout << "is ";
  }
  else
  {
    cout << "are ";
  }
  cout << ivalue << " draw buffer";
  if (ivalue != 1)
  {
    cout << "s";
  }
  cout << ". " << endl;

  GLint i = 0;
  int c = ivalue;
  while (i < c)
  {
    glGetIntegerv(GL_DRAW_BUFFER0 + i, &ivalue);

    cout << "draw buffer[" << i << "]=";
    this->DisplayBuffer(ivalue);
    cout << endl;
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display the read buffer.
void svtkOpenGLFramebufferObject::DisplayReadBuffer()
{
  GLint ivalue;
  glGetIntegerv(GL_READ_BUFFER, &ivalue);
  cout << "read buffer=";
  this->DisplayBuffer(ivalue);
  cout << endl;
}

// ----------------------------------------------------------------------------
// Description:
// Display any buffer (convert value into string).
void svtkOpenGLFramebufferObject::DisplayBuffer(int value)
{
  if (value >= static_cast<int>(GL_COLOR_ATTACHMENT0) &&
    value <= static_cast<int>(GL_COLOR_ATTACHMENT0 + 15))
  {
    cout << "GL_COLOR_ATTACHMENT" << (value - GL_COLOR_ATTACHMENT0);
  }
  else
  {
#ifdef GL_ES_VERSION_3_0
    svtkErrorMacro("Attempt to use bad display destintation");
#else
    if (value >= GL_AUX0)
    {
      int b = value - GL_AUX0;
      GLint ivalue;
      glGetIntegerv(GL_AUX_BUFFERS, &ivalue);
      if (b < ivalue)
      {
        cout << "GL_AUX" << b;
      }
      else
      {
        cout << "invalid aux buffer: " << b << ", upper limit is " << (ivalue - 1)
             << ", raw value is 0x" << std::hex << (GL_AUX0 + b) << std::dec;
      }
    }
    else
    {
      switch (value)
      {
        case GL_NONE:
          cout << "GL_NONE";
          break;
        case GL_FRONT_LEFT:
          cout << "GL_FRONT_LEFT";
          break;
        case GL_FRONT_RIGHT:
          cout << "GL_FRONT_RIGHT";
          break;
        case GL_BACK_LEFT:
          cout << "GL_BACK_LEFT";
          break;
        case GL_BACK_RIGHT:
          cout << "GL_BACK_RIGHT";
          break;
        case GL_FRONT:
          cout << "GL_FRONT";
          break;
        case GL_BACK:
          cout << "GL_BACK";
          break;
        case GL_LEFT:
          cout << "GL_LEFT";
          break;
        case GL_RIGHT:
          cout << "GL_RIGHT";
          break;
        case GL_FRONT_AND_BACK:
          cout << "GL_FRONT_AND_BACK";
          break;
        default:
          cout << "unknown 0x" << std::hex << value << std::dec;
          break;
      }
    }
#endif
  }
}

// ---------------------------------------------------------------------------
// a program must be bound
// a VAO must be bound
void svtkOpenGLFramebufferObject::RenderQuad(int minX, int maxX, int minY, int maxY,
  svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao)
{
  assert("pre positive_minX" && minX >= 0);
  assert("pre increasing_x" && minX <= maxX);
  assert("pre valid_maxX" && maxX < this->LastSize[0]);
  assert("pre positive_minY" && minY >= 0);
  assert("pre increasing_y" && minY <= maxY);
  assert("pre valid_maxY" && maxY < this->LastSize[1]);

#ifdef SVTK_FBO_DEBUG
  cout << "render quad: minX=" << minX << " maxX=" << maxX << " minY=" << minY << " maxY=" << maxY
       << endl;

  GLuint queryId;
  GLuint nbPixels = 0;
  glGenQueries(1, &queryId);
  glBeginQuery(GL_SAMPLES_PASSED, queryId);
#endif

  float maxYTexCoord;
  if (minY == maxY)
  {
    maxYTexCoord = 0.0;
  }
  else
  {
    maxYTexCoord = 1.0;
  }

  float fminX = 2.0 * minX / (this->LastSize[0] - 1.0) - 1.0;
  float fminY = 2.0 * minY / (this->LastSize[1] - 1.0) - 1.0;
  float fmaxX = 2.0 * maxX / (this->LastSize[0] - 1.0) - 1.0;
  float fmaxY = 2.0 * maxY / (this->LastSize[1] - 1.0) - 1.0;

  float verts[] = { fminX, fminY, 0, fmaxX, fminY, 0, fmaxX, fmaxY, 0, fminX, fmaxY, 0 };

  float tcoords[] = { 0, 0, 1.0, 0, 1.0, maxYTexCoord, 0, maxYTexCoord };
  svtkOpenGLRenderUtilities::RenderQuad(verts, tcoords, program, vao);

  svtkOpenGLCheckErrorMacro("failed after Render");

#ifdef SVTK_FBO_DEBUG
  glEndQuery(GL_SAMPLES_PASSED);
  glGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &nbPixels);
  cout << nbPixels << " have been modified." << endl;
#endif
}

// ----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LastSize : " << this->LastSize[0] << this->LastSize[1] << endl;
}

// ----------------------------------------------------------------------------
bool svtkOpenGLFramebufferObject::GetFrameBufferStatus(unsigned int mode, const char*& desc)
{
  bool ok = false;
  desc = "error";
  GLenum status = glCheckFramebufferStatus((GLenum)mode);
  switch (status)
  {
    case GL_FRAMEBUFFER_COMPLETE:
      desc = "FBO complete";
      ok = true;
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      desc = "FRAMEBUFFER_UNSUPPORTED";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      desc = "FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      desc = "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
      break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      desc = "FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
      desc = "FRAMEBUFFER_INCOMPLETE_FORMATS";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      desc = "FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      desc = "FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      desc = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
      break;
#endif

    default:
      desc = "Unknown status";
  }
  if (!ok)
  {
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------------
int svtkOpenGLFramebufferObject::CheckFrameBufferStatus(unsigned int mode)
{
  bool ok = false;
  const char* str = "error";
  GLenum status = glCheckFramebufferStatus((GLenum)mode);
  svtkOpenGLCheckErrorMacro("failed at glCheckFramebufferStatus");
  switch (status)
  {
    case GL_FRAMEBUFFER_COMPLETE:
      str = "FBO complete";
      ok = true;
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      str = "FRAMEBUFFER_UNSUPPORTED";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      str = "FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      str = "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
      break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      str = "FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
      str = "FRAMEBUFFER_INCOMPLETE_FORMATS";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      str = "FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      str = "FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      str = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
      break;
#endif
    default:
      str = "Unknown status";
  }
  if (!ok)
  {
    svtkErrorMacro("The framebuffer is incomplete : " << str);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkOpenGLFramebufferObject::Blit(
  const int srcExt[4], const int destExt[4], unsigned int bits, unsigned int mapping)
{
  glBlitFramebuffer((GLint)srcExt[0], (GLint)srcExt[2], (GLint)srcExt[1], (GLint)srcExt[3],
    (GLint)destExt[0], (GLint)destExt[2], (GLint)destExt[1], (GLint)destExt[3], (GLbitfield)bits,
    (GLenum)mapping);

  svtkOpenGLStaticCheckErrorMacro("failed at glBlitFramebuffer");

  return 1;
}

//-----------------------------------------------------------------------------
svtkPixelBufferObject* svtkOpenGLFramebufferObject::DownloadDepth(int extent[4], int svtkType)
{
  assert(this->Context);

  return this->Download(extent, svtkType, 1, this->GetOpenGLType(svtkType), GL_DEPTH_COMPONENT);
}

//-----------------------------------------------------------------------------
svtkPixelBufferObject* svtkOpenGLFramebufferObject::DownloadColor4(int extent[4], int svtkType)
{
  assert(this->Context);

  return this->Download(extent, svtkType, 4, this->GetOpenGLType(svtkType), GL_RGBA);
}

//-----------------------------------------------------------------------------
svtkPixelBufferObject* svtkOpenGLFramebufferObject::DownloadColor3(int extent[4], int svtkType)
{
  assert(this->Context);

  return this->Download(extent, svtkType, 3, this->GetOpenGLType(svtkType), GL_RGB);
}

//-----------------------------------------------------------------------------
svtkPixelBufferObject* svtkOpenGLFramebufferObject::DownloadColor1(
  int extent[4], int svtkType, int channel)
{
  assert(this->Context);
  GLenum oglChannel = 0;
  switch (channel)
  {
    case 0:
      oglChannel = GL_RED;
      break;
    case 1:
      oglChannel = GL_GREEN;
      break;
    case 2:
      oglChannel = GL_BLUE;
      break;
    default:
      svtkErrorMacro("Invalid channel");
      return nullptr;
  }

  return this->Download(extent, svtkType, 1, this->GetOpenGLType(svtkType), oglChannel);
}

//-----------------------------------------------------------------------------
svtkPixelBufferObject* svtkOpenGLFramebufferObject::Download(
  int extent[4], int svtkType, int nComps, int oglType, int oglFormat)
{
  svtkPixelBufferObject* pbo = svtkPixelBufferObject::New();
  pbo->SetContext(this->Context);

  this->Download(extent, svtkType, nComps, oglType, oglFormat, pbo);

  return pbo;
}
//-----------------------------------------------------------------------------
void svtkOpenGLFramebufferObject::Download(
  int extent[4], int svtkType, int nComps, int oglType, int oglFormat, svtkPixelBufferObject* pbo)
{
  unsigned int extentSize[2] = { static_cast<unsigned int>(extent[1] - extent[0] + 1),
    static_cast<unsigned int>(extent[3] - extent[2] + 1) };

  unsigned int nTups = extentSize[0] * extentSize[1];

  pbo->Allocate(svtkType, nTups, nComps, svtkPixelBufferObject::PACKED_BUFFER);

  pbo->Bind(svtkPixelBufferObject::PACKED_BUFFER);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(extent[0], extent[2], extentSize[0], extentSize[1], oglFormat, oglType, nullptr);

  svtkOpenGLStaticCheckErrorMacro("failed at glReadPixels");

  pbo->UnBind();
}

int svtkOpenGLFramebufferObject::GetMultiSamples()
{
  int abuff = this->ActiveBuffers[0];
  return this->ColorBuffers[abuff]->GetSamples();
}

bool svtkOpenGLFramebufferObject::PopulateFramebuffer(int width, int height)
{
  return this->PopulateFramebuffer(width, height, true, 1, SVTK_UNSIGNED_CHAR, true, 24, 0);
}

bool svtkOpenGLFramebufferObject::PopulateFramebuffer(int width, int height, bool useTextures,
  int numberOfColorAttachments, int colorDataType, bool wantDepthAttachment, int depthBitplanes,
  int multisamples, bool wantStencilAttachment)
{
  // MAX_DEPTH_TEXTURE_SAMPLES
  this->Bind();
  this->LastSize[0] = width;
  this->LastSize[1] = height;

  if (useTextures)
  {
    for (int i = 0; i < numberOfColorAttachments; i++)
    {
      svtkTextureObject* color = svtkTextureObject::New();
      color->SetContext(this->Context);
      color->SetSamples(multisamples);
      color->SetWrapS(svtkTextureObject::Repeat);
      color->SetWrapT(svtkTextureObject::Repeat);
      color->SetMinificationFilter(svtkTextureObject::Nearest);
      color->SetMagnificationFilter(svtkTextureObject::Nearest);
      color->Allocate2D(this->LastSize[0], this->LastSize[1], 4, colorDataType);
      this->AddColorAttachment(i, color);
      color->Delete();
    }

    if (wantDepthAttachment)
    {
      svtkTextureObject* depth = svtkTextureObject::New();
      depth->SetContext(this->Context);
      depth->SetSamples(multisamples);
      depth->SetWrapS(svtkTextureObject::Repeat);
      depth->SetWrapT(svtkTextureObject::Repeat);
      depth->SetMinificationFilter(svtkTextureObject::Nearest);
      depth->SetMagnificationFilter(svtkTextureObject::Nearest);
      if (wantStencilAttachment)
      {
        depth->AllocateDepthStencil(this->LastSize[0], this->LastSize[1]);
      }
      else
      {
        switch (depthBitplanes)
        {
          case 16:
            depth->AllocateDepth(this->LastSize[0], this->LastSize[1], svtkTextureObject::Fixed16);
            break;
          case 32:
            depth->AllocateDepth(this->LastSize[0], this->LastSize[1], svtkTextureObject::Fixed32);
            break;
          case 24:
          default:
            depth->AllocateDepth(this->LastSize[0], this->LastSize[1], svtkTextureObject::Fixed24);
            break;
        }
      }
      this->AddDepthAttachment(depth);
      depth->Delete();
    }
  }
  else
  {
    for (int i = 0; i < numberOfColorAttachments; i++)
    {
      svtkRenderbuffer* color = svtkRenderbuffer::New();
      color->SetContext(this->Context);
      switch (colorDataType)
      {
        case SVTK_UNSIGNED_CHAR:
          color->Create(GL_RGBA8, this->LastSize[0], this->LastSize[1], multisamples);
          break;
        case SVTK_FLOAT:
          color->Create(GL_RGBA32F, this->LastSize[0], this->LastSize[1], multisamples);
          break;
      }
      this->AddColorAttachment(i, color);
      color->Delete();
    }

    if (wantDepthAttachment)
    {
      svtkRenderbuffer* depth = svtkRenderbuffer::New();
      depth->SetContext(this->Context);
      if (wantStencilAttachment)
      {
        depth->Create(GL_DEPTH_STENCIL, this->LastSize[0], this->LastSize[1], multisamples);
      }
      else
      {
        switch (depthBitplanes)
        {
          case 16:
            depth->Create(GL_DEPTH_COMPONENT16, this->LastSize[0], this->LastSize[1], multisamples);
            break;
#ifdef GL_DEPTH_COMPONENT32
          case 32:
            depth->Create(GL_DEPTH_COMPONENT32, this->LastSize[0], this->LastSize[1], multisamples);
            break;
#endif
          case 24:
          default:
            depth->Create(GL_DEPTH_COMPONENT24, this->LastSize[0], this->LastSize[1], multisamples);
            break;
        }
      }
      this->AddDepthAttachment(depth);
      depth->Delete();
    }
  }

  const char* desc;
  if (this->GetFrameBufferStatus(this->GetBothMode(), desc))
  {
    this->ActivateDrawBuffer(0);
    this->ActivateReadBuffer(0);
    return true;
  }
  return false;
}

int svtkOpenGLFramebufferObject::GetNumberOfColorAttachments()
{
  return static_cast<int>(this->ColorBuffers.size());
}
