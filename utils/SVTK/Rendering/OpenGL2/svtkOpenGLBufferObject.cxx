/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLBufferObject.h"
#include "svtkObjectFactory.h"

#include "svtk_glew.h"

svtkStandardNewMacro(svtkOpenGLBufferObject);

namespace
{
inline GLenum convertType(svtkOpenGLBufferObject::ObjectType type)
{
  switch (type)
  {
    case svtkOpenGLBufferObject::ElementArrayBuffer:
      return GL_ELEMENT_ARRAY_BUFFER;
    case svtkOpenGLBufferObject::TextureBuffer:
#if defined(GL_TEXTURE_BUFFER)
      return GL_TEXTURE_BUFFER;
      // intentional fall through when not defined
#endif
    default:
    case svtkOpenGLBufferObject::ArrayBuffer:
      return GL_ARRAY_BUFFER;
  }
}
}

struct svtkOpenGLBufferObject::Private
{
  Private()
  {
    this->Handle = 0;
    this->Type = GL_ARRAY_BUFFER;
  }
  GLenum Type;
  GLuint Handle;
};

svtkOpenGLBufferObject::svtkOpenGLBufferObject()
{
  this->Dirty = true;
  this->Internal = new Private;
  this->Internal->Type = convertType(svtkOpenGLBufferObject::ArrayBuffer);
}

svtkOpenGLBufferObject::~svtkOpenGLBufferObject()
{
  if (this->Internal->Handle != 0)
  {
    glDeleteBuffers(1, &this->Internal->Handle);
  }
  delete this->Internal;
}

void svtkOpenGLBufferObject::ReleaseGraphicsResources()
{
  if (this->Internal->Handle != 0)
  {
    glBindBuffer(this->Internal->Type, 0);
    glDeleteBuffers(1, &this->Internal->Handle);
    this->Internal->Handle = 0;
  }
}

void svtkOpenGLBufferObject::SetType(svtkOpenGLBufferObject::ObjectType value)
{
  this->Internal->Type = convertType(value);
}

svtkOpenGLBufferObject::ObjectType svtkOpenGLBufferObject::GetType() const
{
  if (this->Internal->Type == GL_ARRAY_BUFFER)
  {
    return svtkOpenGLBufferObject::ArrayBuffer;
  }
  if (this->Internal->Type == GL_ELEMENT_ARRAY_BUFFER)
  {
    return svtkOpenGLBufferObject::ElementArrayBuffer;
  }
  else
  {
    return svtkOpenGLBufferObject::TextureBuffer;
  }
}

int svtkOpenGLBufferObject::GetHandle() const
{
  return static_cast<int>(this->Internal->Handle);
}

bool svtkOpenGLBufferObject::Bind()
{
  if (!this->Internal->Handle)
  {
    return false;
  }

  glBindBuffer(this->Internal->Type, this->Internal->Handle);
  return true;
}

bool svtkOpenGLBufferObject::Release()
{
  if (!this->Internal->Handle)
  {
    return false;
  }

  glBindBuffer(this->Internal->Type, 0);
  return true;
}

bool svtkOpenGLBufferObject::GenerateBuffer(svtkOpenGLBufferObject::ObjectType objectType)
{
  GLenum objectTypeGL = convertType(objectType);
  if (this->Internal->Handle == 0)
  {
    glGenBuffers(1, &this->Internal->Handle);
    this->Internal->Type = objectTypeGL;
  }
  return (this->Internal->Type == objectTypeGL);
}

bool svtkOpenGLBufferObject::UploadInternal(
  const void* buffer, size_t size, svtkOpenGLBufferObject::ObjectType objectType)
{
  const bool generated = this->GenerateBuffer(objectType);
  if (!generated)
  {
    this->Error = "Trying to upload array buffer to incompatible buffer.";
    return false;
  }

  glBindBuffer(this->Internal->Type, this->Internal->Handle);
  glBufferData(this->Internal->Type, size, static_cast<const GLvoid*>(buffer), GL_STATIC_DRAW);
  this->Dirty = false;
  return true;
}

//-----------------------------------------------------------------------------
void svtkOpenGLBufferObject::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
