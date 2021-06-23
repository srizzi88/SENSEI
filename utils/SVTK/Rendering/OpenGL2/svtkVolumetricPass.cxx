/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumetricPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkVolumetricPass.h"
#include "svtkObjectFactory.h"
#include <cassert>

svtkStandardNewMacro(svtkVolumetricPass);

// ----------------------------------------------------------------------------
svtkVolumetricPass::svtkVolumetricPass() = default;

// ----------------------------------------------------------------------------
svtkVolumetricPass::~svtkVolumetricPass() = default;

// ----------------------------------------------------------------------------
void svtkVolumetricPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkVolumetricPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  this->RenderFilteredVolumetricGeometry(s);
}
