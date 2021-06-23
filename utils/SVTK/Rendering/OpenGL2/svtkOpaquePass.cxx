/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpaquePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpaquePass.h"
#include "svtkObjectFactory.h"
#include <cassert>

svtkStandardNewMacro(svtkOpaquePass);

// ----------------------------------------------------------------------------
svtkOpaquePass::svtkOpaquePass() = default;

// ----------------------------------------------------------------------------
svtkOpaquePass::~svtkOpaquePass() = default;

// ----------------------------------------------------------------------------
void svtkOpaquePass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkOpaquePass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  this->RenderFilteredOpaqueGeometry(s);
}
