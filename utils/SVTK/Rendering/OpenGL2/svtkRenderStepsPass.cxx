/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderStepsPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRenderStepsPass.h"
#include "svtkObjectFactory.h"
#include <cassert>

#include "svtkCameraPass.h"
#include "svtkLightsPass.h"
#include "svtkOpaquePass.h"
#include "svtkOverlayPass.h"
#include "svtkRenderPassCollection.h"
#include "svtkSequencePass.h"
#include "svtkTranslucentPass.h"
#include "svtkVolumetricPass.h"

svtkStandardNewMacro(svtkRenderStepsPass);

svtkCxxSetObjectMacro(svtkRenderStepsPass, CameraPass, svtkCameraPass);
svtkCxxSetObjectMacro(svtkRenderStepsPass, LightsPass, svtkRenderPass);
svtkCxxSetObjectMacro(svtkRenderStepsPass, OpaquePass, svtkRenderPass);
svtkCxxSetObjectMacro(svtkRenderStepsPass, TranslucentPass, svtkRenderPass);
svtkCxxSetObjectMacro(svtkRenderStepsPass, VolumetricPass, svtkRenderPass);
svtkCxxSetObjectMacro(svtkRenderStepsPass, OverlayPass, svtkRenderPass);
svtkCxxSetObjectMacro(svtkRenderStepsPass, PostProcessPass, svtkRenderPass);

// ----------------------------------------------------------------------------
svtkRenderStepsPass::svtkRenderStepsPass()
{
  this->CameraPass = svtkCameraPass::New();
  this->LightsPass = svtkLightsPass::New();
  this->OpaquePass = svtkOpaquePass::New();
  this->TranslucentPass = svtkTranslucentPass::New();
  this->VolumetricPass = svtkVolumetricPass::New();
  this->OverlayPass = svtkOverlayPass::New();
  this->SequencePass = svtkSequencePass::New();
  svtkRenderPassCollection* rpc = svtkRenderPassCollection::New();
  this->SequencePass->SetPasses(rpc);
  rpc->Delete();
  this->CameraPass->SetDelegatePass(this->SequencePass);
  this->PostProcessPass = nullptr;
}

// ----------------------------------------------------------------------------
svtkRenderStepsPass::~svtkRenderStepsPass()
{
  if (this->CameraPass)
  {
    this->CameraPass->Delete();
    this->CameraPass = nullptr;
  }
  if (this->LightsPass)
  {
    this->LightsPass->Delete();
    this->LightsPass = nullptr;
  }
  if (this->OpaquePass)
  {
    this->OpaquePass->Delete();
    this->OpaquePass = nullptr;
  }
  if (this->TranslucentPass)
  {
    this->TranslucentPass->Delete();
    this->TranslucentPass = nullptr;
  }
  if (this->VolumetricPass)
  {
    this->VolumetricPass->Delete();
    this->VolumetricPass = nullptr;
  }
  if (this->OverlayPass)
  {
    this->OverlayPass->Delete();
    this->OverlayPass = nullptr;
  }
  if (this->PostProcessPass)
  {
    this->PostProcessPass->Delete();
    this->PostProcessPass = nullptr;
  }
  if (this->SequencePass)
  {
    this->SequencePass->Delete();
    this->SequencePass = nullptr;
  }
}

// ----------------------------------------------------------------------------
void svtkRenderStepsPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CameraPass:";
  if (this->CameraPass != nullptr)
  {
    this->CameraPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "LightsPass:";
  if (this->LightsPass != nullptr)
  {
    this->LightsPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "opaquePass:";
  if (this->OpaquePass != nullptr)
  {
    this->OpaquePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "TranslucentPass:";
  if (this->TranslucentPass != nullptr)
  {
    this->TranslucentPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "VolumetricPass:";
  if (this->VolumetricPass != nullptr)
  {
    this->VolumetricPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "OverlayPass:";
  if (this->OverlayPass != nullptr)
  {
    this->OverlayPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "PostProcessPass:";
  if (this->PostProcessPass != nullptr)
  {
    this->PostProcessPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkRenderStepsPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  svtkRenderPassCollection* passes = this->SequencePass->GetPasses();
  passes->RemoveAllItems();

  if (this->LightsPass)
  {
    passes->AddItem(this->LightsPass);
  }
  if (this->OpaquePass)
  {
    passes->AddItem(this->OpaquePass);
  }
  if (this->TranslucentPass)
  {
    passes->AddItem(this->TranslucentPass);
  }
  if (this->VolumetricPass)
  {
    passes->AddItem(this->VolumetricPass);
  }
  if (this->OverlayPass)
  {
    passes->AddItem(this->OverlayPass);
  }

  this->NumberOfRenderedProps = 0;
  if (this->CameraPass)
  {
    this->CameraPass->Render(s);
    this->NumberOfRenderedProps += this->CameraPass->GetNumberOfRenderedProps();
  }
  if (this->PostProcessPass)
  {
    this->PostProcessPass->Render(s);
    this->NumberOfRenderedProps += this->PostProcessPass->GetNumberOfRenderedProps();
  }
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkRenderStepsPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  if (this->CameraPass)
  {
    this->CameraPass->ReleaseGraphicsResources(w);
  }
  if (this->LightsPass)
  {
    this->LightsPass->ReleaseGraphicsResources(w);
  }
  if (this->OpaquePass)
  {
    this->OpaquePass->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentPass)
  {
    this->TranslucentPass->ReleaseGraphicsResources(w);
  }
  if (this->VolumetricPass)
  {
    this->VolumetricPass->ReleaseGraphicsResources(w);
  }
  if (this->OverlayPass)
  {
    this->OverlayPass->ReleaseGraphicsResources(w);
  }
  if (this->PostProcessPass)
  {
    this->PostProcessPass->ReleaseGraphicsResources(w);
  }
}
