/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLContextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLContextActor.h"

#include "svtkContext2D.h"
#include "svtkContext3D.h"
#include "svtkContextScene.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLContextDevice2D.h"
#include "svtkOpenGLContextDevice3D.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkOpenGLContextActor);

//----------------------------------------------------------------------------
svtkOpenGLContextActor::svtkOpenGLContextActor() = default;

//----------------------------------------------------------------------------
svtkOpenGLContextActor::~svtkOpenGLContextActor() = default;

//----------------------------------------------------------------------------
void svtkOpenGLContextActor::ReleaseGraphicsResources(svtkWindow* window)
{
  svtkOpenGLContextDevice2D* device =
    svtkOpenGLContextDevice2D::SafeDownCast(this->Context->GetDevice());
  if (device)
  {
    device->ReleaseGraphicsResources(window);
  }

  if (this->Scene)
  {
    this->Scene->ReleaseGraphicsResources();
  }
}

//----------------------------------------------------------------------------
// Renders an actor2D's property and then it's mapper.
int svtkOpenGLContextActor::RenderOverlay(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkContextActor::RenderOverlay");

  if (!this->Context)
  {
    svtkErrorMacro(<< "svtkContextActor::Render - No painter set");
    return 0;
  }

  if (!this->Initialized)
  {
    this->Initialize(viewport);
  }

  svtkOpenGLContextDevice3D::SafeDownCast(this->Context3D->GetDevice())->Begin(viewport);

  return this->Superclass::RenderOverlay(viewport);
}

//----------------------------------------------------------------------------
void svtkOpenGLContextActor::Initialize(svtkViewport* viewport)
{
  svtkContextDevice2D* dev2D = nullptr;
  svtkDebugMacro("Using OpenGL 2 for 2D rendering.");
  if (this->ForceDevice)
  {
    dev2D = this->ForceDevice;
    dev2D->Register(this);
  }
  else
  {
    dev2D = svtkOpenGLContextDevice2D::New();
  }

  if (dev2D)
  {
    this->Context->Begin(dev2D);

    svtkOpenGLContextDevice2D* oglDev2D = svtkOpenGLContextDevice2D::SafeDownCast(dev2D);
    if (oglDev2D)
    {
      svtkOpenGLContextDevice3D* dev3D = svtkOpenGLContextDevice3D::New();
      dev3D->Initialize(svtkRenderer::SafeDownCast(viewport), oglDev2D);
      this->Context3D->Begin(dev3D);
      dev3D->Delete();
    }

    dev2D->Delete();
    this->Initialized = true;
  }
  else
  {
    // Failed
    svtkErrorMacro("Error: failed to initialize the render device.");
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLContextActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
