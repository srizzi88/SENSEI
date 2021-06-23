/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRenderManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageRenderManager.h"

#include "svtkFloatArray.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkImageRenderManager);

//----------------------------------------------------------------------------
svtkImageRenderManager::svtkImageRenderManager() {}

//----------------------------------------------------------------------------
svtkImageRenderManager::~svtkImageRenderManager() {}

//----------------------------------------------------------------------------
void svtkImageRenderManager::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkImageRenderManager::PreRenderProcessing()
{
  // Turn swap buffers off before the render so the end render method has a
  // chance to add to the back buffer.
  if (this->UseBackBuffer)
  {
    this->RenderWindow->SwapBuffersOff();
  }
}

//----------------------------------------------------------------------------
void svtkImageRenderManager::PostRenderProcessing()
{
  if (!this->UseCompositing || this->CheckForAbortComposite())
  {
    return;
  }

  // Swap buffers here
  if (this->UseBackBuffer)
  {
    this->RenderWindow->SwapBuffersOn();
  }
  this->RenderWindow->Frame();
}
