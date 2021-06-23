/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClientServerCompositePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkClientServerCompositePass.h"

#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkRenderState.h"
#include "svtkSynchronizedRenderers.h"

svtkStandardNewMacro(svtkClientServerCompositePass);
svtkCxxSetObjectMacro(svtkClientServerCompositePass, Controller, svtkMultiProcessController);
svtkCxxSetObjectMacro(svtkClientServerCompositePass, RenderPass, svtkRenderPass);
svtkCxxSetObjectMacro(svtkClientServerCompositePass, PostProcessingRenderPass, svtkRenderPass);
//----------------------------------------------------------------------------
svtkClientServerCompositePass::svtkClientServerCompositePass()
{
  this->Controller = nullptr;
  this->RenderPass = nullptr;
  this->PostProcessingRenderPass = nullptr;
  this->ServerSideRendering = true;
  this->ProcessIsServer = false;
}

//----------------------------------------------------------------------------
svtkClientServerCompositePass::~svtkClientServerCompositePass()
{
  this->SetController(nullptr);
  this->SetRenderPass(nullptr);
  this->SetPostProcessingRenderPass(nullptr);
}

//----------------------------------------------------------------------------
void svtkClientServerCompositePass::ReleaseGraphicsResources(svtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);
  if (this->RenderPass)
  {
    this->RenderPass->ReleaseGraphicsResources(w);
  }
  if (this->PostProcessingRenderPass)
  {
    this->PostProcessingRenderPass->ReleaseGraphicsResources(w);
  }
}

//----------------------------------------------------------------------------
void svtkClientServerCompositePass::Render(const svtkRenderState* s)
{
  if (!this->ServerSideRendering || this->ProcessIsServer)
  {
    if (this->RenderPass)
    {
      this->RenderPass->Render(s);
    }
    else
    {
      svtkWarningMacro("No render pass set.");
    }
  }

  if (this->ServerSideRendering)
  {
    if (!this->Controller)
    {
      svtkErrorMacro("Cannot do remote rendering with a controller.");
    }
    else if (this->ProcessIsServer)
    {
      // server.
      svtkSynchronizedRenderers::svtkRawImage rawImage;
      rawImage.Capture(s->GetRenderer());
      int header[4];
      header[0] = rawImage.IsValid() ? 1 : 0;
      header[1] = rawImage.GetWidth();
      header[2] = rawImage.GetHeight();
      header[3] = rawImage.IsValid() ? rawImage.GetRawPtr()->GetNumberOfComponents() : 0;
      // send the image to the client.
      this->Controller->Send(header, 4, 1, 0x023430);
      if (rawImage.IsValid())
      {
        this->Controller->Send(rawImage.GetRawPtr(), 1, 0x023430);
      }
    }
    else
    {
      // client.
      svtkSynchronizedRenderers::svtkRawImage rawImage;
      int header[4];
      this->Controller->Receive(header, 4, 1, 0x023430);
      if (header[0] > 0)
      {
        rawImage.Resize(header[1], header[2], header[3]);
        this->Controller->Receive(rawImage.GetRawPtr(), 1, 0x023430);
        rawImage.MarkValid();
      }
      rawImage.PushToViewport(s->GetRenderer());
    }
  }

  if (this->PostProcessingRenderPass)
  {
    this->PostProcessingRenderPass->Render(s);
  }
}

//----------------------------------------------------------------------------
void svtkClientServerCompositePass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: ";
  if (this->Controller == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->Controller << endl;
  }

  os << indent << "ServerSideRendering: " << this->ServerSideRendering << endl;
  os << indent << "ProcessIsServer: " << this->ProcessIsServer << endl;

  os << indent << "RenderPass: ";
  if (this->RenderPass == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->RenderPass << endl;
  }
  os << indent << "PostProcessingRenderPass: ";
  if (this->PostProcessingRenderPass == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->PostProcessingRenderPass << endl;
  }
}
