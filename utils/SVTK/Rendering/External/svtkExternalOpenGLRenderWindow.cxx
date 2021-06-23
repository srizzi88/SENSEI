/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExternalOpenGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtk_glew.h"

#include "svtkExternalOpenGLRenderWindow.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLState.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"

svtkStandardNewMacro(svtkExternalOpenGLRenderWindow);

//----------------------------------------------------------------------------
svtkExternalOpenGLRenderWindow::svtkExternalOpenGLRenderWindow()
{
  this->AutomaticWindowPositionAndResize = 1;
  this->UseExternalContent = true;
}

//----------------------------------------------------------------------------
svtkExternalOpenGLRenderWindow::~svtkExternalOpenGLRenderWindow() {}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderWindow::Start(void)
{
  // Make sure all important OpenGL options are set for SVTK
  this->OpenGLInit();

  // Use hardware acceleration
  this->SetIsDirect(1);

  auto ostate = this->GetState();

  if (this->AutomaticWindowPositionAndResize)
  {
    int info[4];
    ostate->svtkglGetIntegerv(GL_VIEWPORT, info);
    this->SetPosition(info[0], info[1]);
    this->SetSize(info[2], info[3]);
  }

  // creates or resizes the framebuffer
  this->Size[0] = (this->Size[0] > 0 ? this->Size[0] : 300);
  this->Size[1] = (this->Size[1] > 0 ? this->Size[1] : 300);
  this->CreateOffScreenFramebuffer(this->Size[0], this->Size[1]);

  // For stereo, render the correct eye based on the OpenGL buffer mode
  GLint bufferType;
  ostate->svtkglGetIntegerv(GL_DRAW_BUFFER, &bufferType);
  svtkCollectionSimpleIterator sit;
  svtkRenderer* renderer;
  for (this->GetRenderers()->InitTraversal(sit);
       (renderer = this->GetRenderers()->GetNextRenderer(sit));)
  {
    if (bufferType == GL_BACK_RIGHT || bufferType == GL_RIGHT || bufferType == GL_FRONT_RIGHT)
    {
      this->StereoRenderOn();
      this->SetStereoTypeToRight();
    }
    else
    {
      this->SetStereoTypeToLeft();
    }
  }

  ostate->PushFramebufferBindings();

  if (this->UseExternalContent)
  {
    const int destExtents[4] = { 0, this->Size[0], 0, this->Size[1] };
    this->OffScreenFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->GetState()->svtkglViewport(0, 0, this->Size[0], this->Size[1]);
    this->GetState()->svtkglScissor(0, 0, this->Size[0], this->Size[1]);
    svtkOpenGLFramebufferObject::Blit(
      destExtents, destExtents, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  }

  this->OffScreenFramebuffer->Bind();
}

//----------------------------------------------------------------------------
bool svtkExternalOpenGLRenderWindow::IsCurrent(void)
{
  return true;
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLRenderWindow::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "UseExternalContent: " << this->UseExternalContent << endl;
  this->Superclass::PrintSelf(os, indent);
}
