/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClearZPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkClearZPass.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkRenderState.h"
#include "svtk_glew.h"
#include <cassert>

svtkStandardNewMacro(svtkClearZPass);

// ----------------------------------------------------------------------------
svtkClearZPass::svtkClearZPass()
{
  this->Depth = 1.0;
}

// ----------------------------------------------------------------------------
svtkClearZPass::~svtkClearZPass() = default;

// ----------------------------------------------------------------------------
void svtkClearZPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Depth:" << this->Depth << endl;
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkClearZPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);
  (void)s;
  this->NumberOfRenderedProps = 0;

  svtkOpenGLState* ostate = static_cast<svtkOpenGLRenderer*>(s->GetRenderer())->GetState();
  ostate->svtkglDepthMask(GL_TRUE);
  ostate->svtkglClearDepth(this->Depth);
  ostate->svtkglClear(GL_DEPTH_BUFFER_BIT);
}
