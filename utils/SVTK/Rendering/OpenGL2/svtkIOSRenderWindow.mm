/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkIOSRenderWindow.mm

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLRenderWindow.h"

#import "svtkCommand.h"
#import "svtkIOSRenderWindow.h"
#import "svtkIdList.h"
#import "svtkObjectFactory.h"
#import "svtkRenderWindowInteractor.h"
#import "svtkRendererCollection.h"

#import <sstream>

#include "svtk_glew.h"

svtkStandardNewMacro(svtkIOSRenderWindow);

//----------------------------------------------------------------------------
svtkIOSRenderWindow::svtkIOSRenderWindow()
{
  this->WindowCreated = 0;
  this->ViewCreated = 0;
  this->SetWindowName("Visualization Toolkit - IOS");
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->OnScreenInitialized = 0;
  this->OffScreenInitialized = 0;
  // it seems that LEFT/RIGHT cause issues on IOS so we just use
  // generic BACK/FRONT
  this->BackLeftBuffer = static_cast<unsigned int>(GL_BACK);
  this->BackRightBuffer = static_cast<unsigned int>(GL_BACK);
  this->FrontLeftBuffer = static_cast<unsigned int>(GL_FRONT);
  this->FrontRightBuffer = static_cast<unsigned int>(GL_FRONT);
}

//----------------------------------------------------------------------------
svtkIOSRenderWindow::~svtkIOSRenderWindow()
{
  if (this->CursorHidden)
  {
    this->ShowCursor();
  }
  this->Finalize();

  svtkRenderer* ren;
  svtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(NULL);
  }

  this->SetContextId(NULL);
  this->SetPixelFormat(NULL);
  this->SetRootWindow(NULL);
  this->SetWindowId(NULL);
  this->SetParentId(NULL);
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::Finalize()
{
  if (this->OffScreenInitialized)
  {
    this->OffScreenInitialized = 0;
    this->DestroyOffScreenWindow();
  }
  if (this->OnScreenInitialized)
  {
    this->OnScreenInitialized = 0;
    this->DestroyWindow();
  }
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::DestroyWindow()
{
  // finish OpenGL rendering
  if (this->OwnContext && this->GetContextId())
  {
    this->MakeCurrent();
    this->ReleaseGraphicsResources(this);
  }
  this->SetContextId(NULL);
  this->SetPixelFormat(NULL);

  this->SetWindowId(NULL);
  this->SetParentId(NULL);
  this->SetRootWindow(NULL);
}

int svtkIOSRenderWindow::ReadPixels(
  const svtkRecti& rect, int front, int glFormat, int glType, void* data, int right)
{
  if (glFormat != GL_RGB || glType != GL_UNSIGNED_BYTE)
  {
    return this->Superclass::ReadPixels(rect, front, glFormat, glType, data, right);
  }

  // iOS has issues with getting RGB so we get RGBA
  unsigned char* uc4data = new unsigned char[rect.GetWidth() * rect.GetHeight() * 4];
  int retVal = this->Superclass::ReadPixels(rect, front, GL_RGBA, GL_UNSIGNED_BYTE, uc4data, right);

  unsigned char* dPtr = reinterpret_cast<unsigned char*>(data);
  const unsigned char* lPtr = uc4data;
  for (int i = 0, height = rect.GetHeight(); i < height; i++)
  {
    for (int j = 0, width = rect.GetWidth(); j < width; j++)
    {
      *(dPtr++) = *(lPtr++);
      *(dPtr++) = *(lPtr++);
      *(dPtr++) = *(lPtr++);
      lPtr++;
    }
  }
  delete[] uc4data;
  return retVal;
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetWindowName(const char* _arg)
{
  svtkWindow::SetWindowName(_arg);
}

//----------------------------------------------------------------------------
bool svtkIOSRenderWindow::InitializeFromCurrentContext()
{
  // NSOpenGLContext* currentContext = [NSOpenGLContext currentContext];
  // if (currentContext != nullptr)
  // {
  //   this->SetContextId(currentContext);
  //   this->SetPixelFormat([currentContext pixelFormat]);
  //
  //   return this->Superclass::InitializeFromCurrentContext();
  //}
  return false;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkIOSRenderWindow::GetEventPending()
{
  return 0;
}

//----------------------------------------------------------------------------
// Initialize the rendering process.
void svtkIOSRenderWindow::Start()
{
  this->Initialize();

  // set the current window
  this->MakeCurrent();
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::MakeCurrent()
{
  // if (this->GetContextId())
  //   {
  //   [(NSOpenGLContext*)this->GetContextId() makeCurrentContext];
  //   }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool svtkIOSRenderWindow::IsCurrent()
{
  return true;
}

//----------------------------------------------------------------------------
#ifndef SVTK_LEGACY_REMOVE
bool svtkIOSRenderWindow::IsDrawable()
{
  // you must initialize it first
  // else it always evaluates false
  this->Initialize();

  return true;
}
#endif

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::UpdateContext() {}

//----------------------------------------------------------------------------
const char* svtkIOSRenderWindow::ReportCapabilities()
{
  this->MakeCurrent();

  const char* glVendor = (const char*)glGetString(GL_VENDOR);
  const char* glRenderer = (const char*)glGetString(GL_RENDERER);
  const char* glVersion = (const char*)glGetString(GL_VERSION);
  const char* glExtensions = (const char*)glGetString(GL_EXTENSIONS);

  std::ostringstream strm;
  strm << "OpenGL vendor string:  " << glVendor << "\nOpenGL renderer string:  " << glRenderer
       << "\nOpenGL version string:  " << glVersion << "\nOpenGL extensions:  " << glExtensions
       << endl;

  delete[] this->Capabilities;

  size_t len = strm.str().length() + 1;
  this->Capabilities = new char[len];
  strlcpy(this->Capabilities, strm.str().c_str(), len);

  return this->Capabilities;
}

//----------------------------------------------------------------------------
int svtkIOSRenderWindow::SupportsOpenGL()
{
  this->MakeCurrent();
  if (!this->GetContextId() || !this->GetPixelFormat())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkIOSRenderWindow::IsDirect()
{
  this->MakeCurrent();
  if (!this->GetContextId() || !this->GetPixelFormat())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height) || this->GetParentId())
  {
    this->Modified();
    this->Size[0] = width;
    this->Size[1] = height;
  }
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x) || (this->Position[1] != y) || this->GetParentId())
  {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
  }
}

//----------------------------------------------------------------------------
// End the rendering process and display the image.
void svtkIOSRenderWindow::Frame()
{
  this->MakeCurrent();
  this->Superclass::Frame();

  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
  {
    //    [(NSOpenGLContext*)this->GetContextId() flushBuffer];
  }
}

//----------------------------------------------------------------------------
// Specify various window parameters.
void svtkIOSRenderWindow::WindowConfigure()
{
  // this is all handled by the desiredVisualInfo method
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetupPixelFormat(void*, void*, int, int, int)
{
  svtkErrorMacro(<< "svtkIOSRenderWindow::SetupPixelFormat - IMPLEMENT");
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetupPalette(void*)
{
  svtkErrorMacro(<< "svtkIOSRenderWindow::SetupPalette - IMPLEMENT");
}

//----------------------------------------------------------------------------
// Initialize the window for rendering.
void svtkIOSRenderWindow::CreateAWindow()
{
  this->CreateGLContext();

  this->MakeCurrent();

  // wipe out any existing display lists
  svtkRenderer* renderer = NULL;
  svtkCollectionSimpleIterator rsit;

  for (this->Renderers->InitTraversal(rsit); (renderer = this->Renderers->GetNextRenderer(rsit));)
  {
    renderer->SetRenderWindow(0);
    renderer->SetRenderWindow(this);
  }
  this->OpenGLInit();
  this->Mapped = 1;
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::CreateGLContext() {}

//----------------------------------------------------------------------------
// Initialize the rendering window.
void svtkIOSRenderWindow::Initialize()
{
  this->OpenGLInit();
  this->Mapped = 1;
}

//-----------------------------------------------------------------------------
void svtkIOSRenderWindow::DestroyOffScreenWindow() {}

//----------------------------------------------------------------------------
// Get the current size of the window.
int* svtkIOSRenderWindow::GetSize()
{
  // if we aren't mapped then just return call super
  if (!this->Mapped)
  {
    return this->Superclass::GetSize();
  }

  return this->Superclass::GetSize();
}

//----------------------------------------------------------------------------
// Get the current size of the screen in pixels.
int* svtkIOSRenderWindow::GetScreenSize()
{
  // TODO: use UISceen to actually determine screen size.

  return this->ScreenSize;
}

//----------------------------------------------------------------------------
// Get the position in screen coordinates of the window.
int* svtkIOSRenderWindow::GetPosition()
{
  return this->Position;
}

//----------------------------------------------------------------------------
// Change the window to fill the entire screen.
void svtkIOSRenderWindow::SetFullScreen(svtkTypeBool arg) {}

//----------------------------------------------------------------------------
//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void svtkIOSRenderWindow::SetStereoCapableWindow(svtkTypeBool capable)
{
  if (this->GetContextId() == 0)
  {
    svtkRenderWindow::SetStereoCapableWindow(capable);
  }
  else
  {
    svtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
  }
}

//----------------------------------------------------------------------------
// Set the preferred window size to full screen.
void svtkIOSRenderWindow::PrefFullScreen()
{
  const int* size = this->GetScreenSize();
  svtkWarningMacro(<< "Can only set FullScreen before showing window: " << size[0] << 'x' << size[1]
                  << ".");
}

//----------------------------------------------------------------------------
// Remap the window.
void svtkIOSRenderWindow::WindowRemap()
{
  svtkWarningMacro(<< "Can't remap the window.");
  // Acquire the display and capture the screen.
  // Create the full-screen window.
  // Add the context.
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RootWindow (UIWindow): " << this->GetRootWindow() << endl;
  os << indent << "WindowId (UIView): " << this->GetWindowId() << endl;
  os << indent << "ParentId: " << this->GetParentId() << endl;
  os << indent << "ContextId: " << this->GetContextId() << endl;
  os << indent << "PixelFormat: " << this->GetPixelFormat() << endl;
  os << indent << "WindowCreated: " << (this->WindowCreated ? "Yes" : "No") << endl;
  os << indent << "ViewCreated: " << (this->ViewCreated ? "Yes" : "No") << endl;
}

//----------------------------------------------------------------------------
int svtkIOSRenderWindow::GetDepthBufferSize()
{
  if (this->Mapped)
  {
    GLint size = 0;
    glGetIntegerv(GL_DEPTH_BITS, &size);
    return (int)size;
  }
  else
  {
    svtkDebugMacro(<< "Window is not mapped yet!");
    return 24;
  }
}

//----------------------------------------------------------------------------
// Returns the UIWindow* associated with this svtkRenderWindow.
void* svtkIOSRenderWindow::GetRootWindow()
{
  return NULL;
}

//----------------------------------------------------------------------------
// Sets the UIWindow* associated with this svtkRenderWindow.
void svtkIOSRenderWindow::SetRootWindow(void* svtkNotUsed(arg)) {}

//----------------------------------------------------------------------------
// Returns the UIView* associated with this svtkRenderWindow.
void* svtkIOSRenderWindow::GetWindowId()
{
  return NULL;
}

//----------------------------------------------------------------------------
// Sets the UIView* associated with this svtkRenderWindow.
void svtkIOSRenderWindow::SetWindowId(void* svtkNotUsed(arg)) {}

//----------------------------------------------------------------------------
// Returns the UIView* that is the parent of this svtkRenderWindow.
void* svtkIOSRenderWindow::GetParentId()
{
  return NULL;
}

//----------------------------------------------------------------------------
// Sets the UIView* that this svtkRenderWindow should use as a parent.
void svtkIOSRenderWindow::SetParentId(void* svtkNotUsed(arg)) {}

//----------------------------------------------------------------------------
// Sets the NSOpenGLContext* associated with this svtkRenderWindow.
void svtkIOSRenderWindow::SetContextId(void* svtkNotUsed(contextId)) {}

//----------------------------------------------------------------------------
// Returns the NSOpenGLContext* associated with this svtkRenderWindow.
void* svtkIOSRenderWindow::GetContextId()
{
  return NULL;
}

//----------------------------------------------------------------------------
// Sets the NSOpenGLPixelFormat* associated with this svtkRenderWindow.
void svtkIOSRenderWindow::SetPixelFormat(void* svtkNotUsed(pixelFormat)) {}

//----------------------------------------------------------------------------
// Returns the NSOpenGLPixelFormat* associated with this svtkRenderWindow.
void* svtkIOSRenderWindow::GetPixelFormat()
{
  return NULL;
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetWindowInfo(const char* info)
{
  // The parameter is an ASCII string of a decimal number representing
  // a pointer to the window. Convert it back to a pointer.
  ptrdiff_t tmp = 0;
  if (info)
  {
    (void)sscanf(info, "%tu", &tmp);
  }

  this->SetWindowId(reinterpret_cast<void*>(tmp));
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetParentInfo(const char* info)
{
  // The parameter is an ASCII string of a decimal number representing
  // a pointer to the window. Convert it back to a pointer.
  ptrdiff_t tmp = 0;
  if (info)
  {
    (void)sscanf(info, "%tu", &tmp);
  }

  this->SetParentId(reinterpret_cast<void*>(tmp));
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::HideCursor()
{
  if (this->CursorHidden)
  {
    return;
  }
  this->CursorHidden = 1;
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
  {
    return;
  }
  this->CursorHidden = 0;
}

// ---------------------------------------------------------------------------
int svtkIOSRenderWindow::GetWindowCreated()
{
  return this->WindowCreated;
}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetCursorPosition(int x, int y) {}

//----------------------------------------------------------------------------
void svtkIOSRenderWindow::SetCurrentCursor(int shape)
{
  if (this->InvokeEvent(svtkCommand::CursorChangedEvent, &shape))
  {
    return;
  }
  this->Superclass::SetCurrentCursor(shape);
}
