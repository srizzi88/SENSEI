/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClearRGBPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkClearRGBPass.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkRenderState.h"
#include "svtk_glew.h"

svtkStandardNewMacro(svtkClearRGBPass);

// ----------------------------------------------------------------------------
svtkClearRGBPass::svtkClearRGBPass()
{
  this->Background[0] = 0;
  this->Background[1] = 0;
  this->Background[2] = 0;
}

// ----------------------------------------------------------------------------
svtkClearRGBPass::~svtkClearRGBPass() = default;

// ----------------------------------------------------------------------------
void svtkClearRGBPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Background:" << this->Background[0] << "," << this->Background[1] << ","
     << this->Background[2] << endl;
}

// ----------------------------------------------------------------------------
void svtkClearRGBPass::Render(const svtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;

  svtkOpenGLState* ostate = static_cast<svtkOpenGLRenderer*>(s->GetRenderer())->GetState();
  ostate->svtkglClearColor(static_cast<GLclampf>(this->Background[0]),
    static_cast<GLclampf>(this->Background[1]), static_cast<GLclampf>(this->Background[2]),
    static_cast<GLclampf>(0.0));
  ostate->svtkglClear(GL_COLOR_BUFFER_BIT);
}
