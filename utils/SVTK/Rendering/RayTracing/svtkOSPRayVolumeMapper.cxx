/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOSPRayVolumeMapper.h"

#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkObjectFactory.h"
#include "svtkRenderer.h"

//============================================================================
svtkStandardNewMacro(svtkOSPRayVolumeMapper);

// ----------------------------------------------------------------------------
svtkOSPRayVolumeMapper::svtkOSPRayVolumeMapper()
{
  this->InternalRenderer = nullptr;
  this->InternalOSPRayPass = nullptr;
  this->Initialized = false;
}

// ----------------------------------------------------------------------------
svtkOSPRayVolumeMapper::~svtkOSPRayVolumeMapper()
{
  if (this->InternalRenderer)
  {
    this->InternalRenderer->SetPass(nullptr);
    this->InternalRenderer->Delete();
  }
  if (this->InternalOSPRayPass)
  {
    this->InternalOSPRayPass->Delete();
  }
}

// ----------------------------------------------------------------------------
void svtkOSPRayVolumeMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void svtkOSPRayVolumeMapper::Init()
{
  this->InternalOSPRayPass = svtkOSPRayPass::New();
  this->InternalRenderer = svtkRenderer::New();
  svtkOSPRayRendererNode::SetCompositeOnGL(1, this->InternalRenderer);
  this->InternalRenderer->SetLayer(0); // TODO: hacked in for now
  this->Initialized = true;
}

// ----------------------------------------------------------------------------
// Render the volume
void svtkOSPRayVolumeMapper::Render(svtkRenderer* ren, svtkVolume* vol)
{
  if (!ren)
  {
    return;
  }
  if (!this->Initialized)
  {
    this->Init();
  }
  this->InternalRenderer->SetRenderWindow(ren->GetRenderWindow());
  this->InternalRenderer->SetActiveCamera(ren->GetActiveCamera());
  this->InternalRenderer->SetBackground(ren->GetBackground());
  if (!this->InternalRenderer->HasViewProp(vol))
  {
    this->InternalRenderer->RemoveAllViewProps();
    this->InternalRenderer->AddVolume(vol);
  }
  this->InternalRenderer->SetPass(this->InternalOSPRayPass);
  this->InternalRenderer->Render();
  this->InternalRenderer->SetPass(0);
  svtkOSPRayRendererNode::SetCompositeOnGL(
    ren->GetNumberOfPropsRendered() > 0, this->InternalRenderer);
  this->InternalRenderer->SetErase(ren->GetNumberOfPropsRendered() < 1);
  this->InternalRenderer->RemoveVolume(vol); // prevent a mem leak
}

// ----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this mapper.
// The parameter window could be used to determine which graphic
// resources to release.
void svtkOSPRayVolumeMapper::ReleaseGraphicsResources(svtkWindow*) {}
