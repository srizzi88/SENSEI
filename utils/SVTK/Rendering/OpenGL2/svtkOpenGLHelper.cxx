/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLHelper.h"

#include "svtkOpenGLIndexBufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLVertexArrayObject.h"
#include "svtkShaderProgram.h"

svtkOpenGLHelper::svtkOpenGLHelper()
{
  this->Program = nullptr;
  this->IBO = svtkOpenGLIndexBufferObject::New();
  this->VAO = svtkOpenGLVertexArrayObject::New();
  this->ShaderChangeValue = 0;
}

svtkOpenGLHelper::~svtkOpenGLHelper()
{
  this->IBO->Delete();
  this->VAO->Delete();
}

void svtkOpenGLHelper::ReleaseGraphicsResources(svtkWindow* win)
{
  svtkOpenGLRenderWindow* rwin = svtkOpenGLRenderWindow::SafeDownCast(win);
  if (rwin)
  {
    // Ensure that the context is current before releasing any
    // graphics resources tied to it.
    rwin->MakeCurrent();
  }

  if (this->Program)
  {
    // Let ShaderCache release the graphics resources as it is
    // responsible for creation and deletion.
    this->Program = nullptr;
  }
  this->IBO->ReleaseGraphicsResources();
  this->VAO->ReleaseGraphicsResources();
}
