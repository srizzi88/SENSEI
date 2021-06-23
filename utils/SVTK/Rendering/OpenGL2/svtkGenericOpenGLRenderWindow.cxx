/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericOpenGLRenderWindow.h"

#include "svtkCommand.h"
#include "svtkLogger.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkRendererCollection.h"

svtkStandardNewMacro(svtkGenericOpenGLRenderWindow);

svtkGenericOpenGLRenderWindow::svtkGenericOpenGLRenderWindow()
{
  this->ReadyForRendering = true;
  this->DirectStatus = 0;
  this->CurrentStatus = false;
  this->SupportsOpenGLStatus = 0;
  this->ForceMaximumHardwareLineWidth = 0;
}

svtkGenericOpenGLRenderWindow::~svtkGenericOpenGLRenderWindow()
{
  this->Finalize();

  svtkRenderer* ren;
  svtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(nullptr);
  }
}

void svtkGenericOpenGLRenderWindow::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

float svtkGenericOpenGLRenderWindow::GetMaximumHardwareLineWidth()
{
  return this->ForceMaximumHardwareLineWidth > 0 ? this->ForceMaximumHardwareLineWidth
                                                 : this->Superclass::GetMaximumHardwareLineWidth();
}

void svtkGenericOpenGLRenderWindow::SetFrontLeftBuffer(unsigned int b)
{
  this->FrontLeftBuffer = b;
}

void svtkGenericOpenGLRenderWindow::SetFrontRightBuffer(unsigned int b)
{
  this->FrontRightBuffer = b;
}

void svtkGenericOpenGLRenderWindow::SetBackLeftBuffer(unsigned int b)
{
  this->BackLeftBuffer = b;
}

void svtkGenericOpenGLRenderWindow::SetBackRightBuffer(unsigned int b)
{
  this->BackRightBuffer = b;
}

void svtkGenericOpenGLRenderWindow::SetDefaultFrameBufferId(unsigned int id)
{
  this->DefaultFrameBufferId = id;
}

void svtkGenericOpenGLRenderWindow::SetOwnContext(int val)
{
  this->OwnContext = val;
}

void svtkGenericOpenGLRenderWindow::Finalize()
{
  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by svtkRenderWindow's
  // destructor)
  this->ReleaseGraphicsResources(this);
}

void svtkGenericOpenGLRenderWindow::Frame()
{
  this->Superclass::Frame();
  this->InvokeEvent(svtkCommand::WindowFrameEvent, nullptr);
  this->GetState()->ResetFramebufferBindings();
}

void svtkGenericOpenGLRenderWindow::MakeCurrent()
{
  this->InvokeEvent(svtkCommand::WindowMakeCurrentEvent, nullptr);
}

bool svtkGenericOpenGLRenderWindow::IsCurrent()
{
  this->InvokeEvent(svtkCommand::WindowIsCurrentEvent, &this->CurrentStatus);
  return this->CurrentStatus;
}

int svtkGenericOpenGLRenderWindow::SupportsOpenGL()
{
  this->InvokeEvent(svtkCommand::WindowSupportsOpenGLEvent, &this->SupportsOpenGLStatus);
  return this->SupportsOpenGLStatus;
}

svtkTypeBool svtkGenericOpenGLRenderWindow::IsDirect()
{
  this->InvokeEvent(svtkCommand::WindowIsDirectEvent, &this->DirectStatus);
  return this->DirectStatus;
}

void svtkGenericOpenGLRenderWindow::SetWindowId(void*) {}

void* svtkGenericOpenGLRenderWindow::GetGenericWindowId()
{
  return nullptr;
}

void svtkGenericOpenGLRenderWindow::SetDisplayId(void*) {}

void svtkGenericOpenGLRenderWindow::SetParentId(void*) {}

void* svtkGenericOpenGLRenderWindow::GetGenericDisplayId()
{
  return nullptr;
}

void* svtkGenericOpenGLRenderWindow::GetGenericParentId()
{
  return nullptr;
}

void* svtkGenericOpenGLRenderWindow::GetGenericContext()
{
  return nullptr;
}

void* svtkGenericOpenGLRenderWindow::GetGenericDrawable()
{
  return nullptr;
}

void svtkGenericOpenGLRenderWindow::SetWindowInfo(const char*) {}

void svtkGenericOpenGLRenderWindow::SetParentInfo(const char*) {}

int* svtkGenericOpenGLRenderWindow::GetScreenSize()
{
  return this->ScreenSize;
}

void svtkGenericOpenGLRenderWindow::HideCursor() {}

void svtkGenericOpenGLRenderWindow::ShowCursor() {}

void svtkGenericOpenGLRenderWindow::SetFullScreen(svtkTypeBool) {}

void svtkGenericOpenGLRenderWindow::WindowRemap() {}

svtkTypeBool svtkGenericOpenGLRenderWindow::GetEventPending()
{
  return 0;
}

void svtkGenericOpenGLRenderWindow::SetNextWindowId(void*) {}

void svtkGenericOpenGLRenderWindow::SetNextWindowInfo(const char*) {}

void svtkGenericOpenGLRenderWindow::CreateAWindow() {}

void svtkGenericOpenGLRenderWindow::DestroyWindow() {}

void svtkGenericOpenGLRenderWindow::SetIsDirect(svtkTypeBool newValue)
{
  this->DirectStatus = newValue;
}

void svtkGenericOpenGLRenderWindow::SetSupportsOpenGL(int newValue)
{
  this->SupportsOpenGLStatus = newValue;
}

void svtkGenericOpenGLRenderWindow::SetIsCurrent(bool newValue)
{
  this->CurrentStatus = newValue;
}

void svtkGenericOpenGLRenderWindow::Render()
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    if (!this->IsCurrent())
    {
      svtkLogF(TRACE, "rendering skipped since `MakeCurrent` was not successful.");
    }
    else
    {
      // Query current GL state and store them
      this->SaveGLState();

      this->Superclass::Render();

      // Restore state to previous known value
      this->RestoreGLState();
    }
  }
}

void svtkGenericOpenGLRenderWindow::SetCurrentCursor(int cShape)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting current Cursor to "
                << cShape);
  if (this->GetCurrentCursor() != cShape)
  {
    this->CurrentCursor = cShape;
    this->Modified();
    this->InvokeEvent(svtkCommand::CursorChangedEvent, &cShape);
  }
}

int svtkGenericOpenGLRenderWindow::ReadPixels(
  const svtkRecti& rect, int front, int glFormat, int glType, void* data, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::ReadPixels(rect, front, glFormat, glType, data, right);
  }

  svtkWarningMacro("`ReadPixels` called before window is ready for rendering; ignoring.");
  return SVTK_ERROR;
}

int svtkGenericOpenGLRenderWindow::SetPixelData(
  int x1, int y1, int x2, int y2, unsigned char* data, int front, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::SetPixelData(x1, y1, x2, y2, data, front, right);
  }

  svtkWarningMacro("`SetPixelData` called before window is ready for rendering; ignoring.");
  return SVTK_ERROR;
}

int svtkGenericOpenGLRenderWindow::SetPixelData(
  int x1, int y1, int x2, int y2, svtkUnsignedCharArray* data, int front, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::SetPixelData(x1, y1, x2, y2, data, front, right);
  }

  svtkWarningMacro("`SetPixelData` called before window is ready for rendering; ignoring.");
  return SVTK_ERROR;
}

int svtkGenericOpenGLRenderWindow::SetRGBACharPixelData(
  int x1, int y1, int x2, int y2, unsigned char* data, int front, int blend, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::SetRGBACharPixelData(x1, y1, x2, y2, data, front, blend, right);
  }

  svtkWarningMacro("`SetRGBACharPixelData` called before window is ready for rendering; ignoring.");
  return SVTK_ERROR;
}

int svtkGenericOpenGLRenderWindow::SetRGBACharPixelData(
  int x1, int y1, int x2, int y2, svtkUnsignedCharArray* data, int front, int blend, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::SetRGBACharPixelData(x1, y1, x2, y2, data, front, blend, right);
  }

  svtkWarningMacro("`SetRGBACharPixelData` called before window is ready for rendering; ignoring.");
  return SVTK_ERROR;
}

//----------------------------------------------------------------------------
#ifndef SVTK_LEGACY_REMOVE
bool svtkGenericOpenGLRenderWindow::IsDrawable()
{
  return this->ReadyForRendering;
}
#endif
