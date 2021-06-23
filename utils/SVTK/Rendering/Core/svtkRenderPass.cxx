/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRenderPass.h"
#include "svtkRenderer.h"
#include <cassert>

// ----------------------------------------------------------------------------
// Description:
// Default constructor. Do nothing.
svtkRenderPass::svtkRenderPass()
{
  this->NumberOfRenderedProps = 0;
}

// ----------------------------------------------------------------------------
// Description:
// Destructor. Do nothing.
svtkRenderPass::~svtkRenderPass() = default;

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources. Default implementation is empty.
// \pre w_exists: w!=0
void svtkRenderPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  // empty implementation;
  static_cast<void>(w); // avoid warning in release mode.
}

// ----------------------------------------------------------------------------
// Description:
// Call UpdateCamera() on Renderer. This ugly mechanism gives access to
// a protected method of Renderer to subclasses of svtkRenderPass.
// \pre renderer_exists: renderer!=0
void svtkRenderPass::UpdateCamera(svtkRenderer* renderer)
{
  assert("pre: renderer_exists" && renderer != nullptr);
  renderer->UpdateCamera();
}

// ----------------------------------------------------------------------------
// Description:
// Call ClearLights() on Renderer. See note about UpdateCamera().
// \pre renderer_exists: renderer!=0
void svtkRenderPass::ClearLights(svtkRenderer* renderer)
{
  //  assert("pre: renderer_exists" && renderer != 0);
  renderer->ClearLights();
}

// ----------------------------------------------------------------------------
// Description:
// Call UpdateLightGeometry() on Renderer. See note about UpdateCamera().
// \pre renderer_exists: renderer!=0
void svtkRenderPass::UpdateLightGeometry(svtkRenderer* renderer)
{
  assert("pre: renderer_exists" && renderer != nullptr);
  renderer->UpdateLightGeometry();
}

// ----------------------------------------------------------------------------
// Description:
// Call UpdateLights() on Renderer. See note about UpdateCamera().
// \pre renderer_exists: renderer!=0
void svtkRenderPass::UpdateLights(svtkRenderer* renderer)
{
  assert("pre: renderer_exists" && renderer != nullptr);
  renderer->UpdateLights();
}

// ----------------------------------------------------------------------------
// Description:
// Call UpdateGeometry() on Renderer. See note about UpdateCamera().
// \pre renderer_exists: renderer!=0
void svtkRenderPass::UpdateGeometry(svtkRenderer* renderer, svtkFrameBufferObjectBase* fbo)
{
  assert("pre: renderer_exists" && renderer != nullptr);
  renderer->UpdateGeometry(fbo);
}

// ----------------------------------------------------------------------------
void svtkRenderPass::SetLastRenderingUsedDepthPeeling(svtkRenderer* renderer, bool value)
{
  assert("pre: renderer_exists" && renderer != nullptr);

  renderer->LastRenderingUsedDepthPeeling = value;
}

// ----------------------------------------------------------------------------
void svtkRenderPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfRenderedProps:" << this->NumberOfRenderedProps << endl;
}
