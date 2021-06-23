/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOSOpenGLRenderWindow.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtk_glew.h"
#include <GL/gl.h>

#ifndef GLAPI
#define GLAPI extern
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifndef APIENTRY
#define APIENTRY GLAPIENTRY
#endif
#include <GL/osmesa.h>

#include "svtkOSOpenGLRenderWindow.h"
#include "svtkOpenGLActor.h"
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLLight.h"
#include "svtkOpenGLProperty.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLTexture.h"

#include "svtkCommand.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"
#include "svtkRendererCollection.h"

#include "svtksys/SystemTools.hxx"
#include <sstream>

class svtkOSOpenGLRenderWindow;
class svtkRenderWindow;

typedef OSMesaContext GLAPIENTRY (*OSMesaCreateContextAttribs_func)(
  const int* attribList, OSMesaContext sharelist);

class svtkOSOpenGLRenderWindowInternal
{
  friend class svtkOSOpenGLRenderWindow;

private:
  svtkOSOpenGLRenderWindowInternal(svtkRenderWindow*);

  // store previous settings of on screen window
  int ScreenDoubleBuffer;
  int ScreenMapped;

  // OffScreen stuff
  OSMesaContext OffScreenContextId;
  void* OffScreenWindow;
};

svtkOSOpenGLRenderWindowInternal::svtkOSOpenGLRenderWindowInternal(svtkRenderWindow* rw)
{

  this->ScreenMapped = rw->GetMapped();
  this->ScreenDoubleBuffer = rw->GetDoubleBuffer();

  // OpenGL specific
  this->OffScreenContextId = nullptr;
  this->OffScreenWindow = nullptr;
}

svtkStandardNewMacro(svtkOSOpenGLRenderWindow);

// a couple of routines for offscreen rendering
void svtkOSMesaDestroyWindow(void* Window)
{
  free(Window);
}

void* svtkOSMesaCreateWindow(int width, int height)
{
  return malloc(width * height * 4);
}

svtkOSOpenGLRenderWindow::svtkOSOpenGLRenderWindow()
{
  //   this->ParentId = (Window)nullptr;
  this->ScreenSize[0] = 1280;
  this->ScreenSize[1] = 1024;
  this->OwnDisplay = 0;
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->OwnWindow = 0;
  this->ShowWindow = false;

  this->Internal = new svtkOSOpenGLRenderWindowInternal(this);
}

// free up memory & close the window
svtkOSOpenGLRenderWindow::~svtkOSOpenGLRenderWindow()
{
  // close-down all system-specific drawing resources
  this->Finalize();

  svtkRenderer* ren;
  svtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(nullptr);
  }

  delete this->Internal;
}

// End the rendering process and display the image.
void svtkOSOpenGLRenderWindow::Frame()
{
  this->MakeCurrent();
  this->Superclass::Frame();
}

//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void svtkOSOpenGLRenderWindow::SetStereoCapableWindow(svtkTypeBool capable)
{
  if (!this->Internal->OffScreenContextId)
  {
    svtkOpenGLRenderWindow::SetStereoCapableWindow(capable);
  }
  else
  {
    svtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
  }
}

void svtkOSOpenGLRenderWindow::CreateAWindow()
{
  this->CreateOffScreenWindow(this->Size[0], this->Size[1]);
}

void svtkOSOpenGLRenderWindow::DestroyWindow()
{
  this->MakeCurrent();
  this->ReleaseGraphicsResources(this);

  delete[] this->Capabilities;
  this->Capabilities = 0;

  this->DestroyOffScreenWindow();

  // make sure all other code knows we're not mapped anymore
  this->Mapped = 0;
}

void svtkOSOpenGLRenderWindow::CreateOffScreenWindow(int width, int height)
{
  this->DoubleBuffer = 0;

  if (!this->Internal->OffScreenWindow)
  {
    this->Internal->OffScreenWindow = svtkOSMesaCreateWindow(width, height);
    this->OwnWindow = 1;
  }
#if (OSMESA_MAJOR_VERSION * 100 + OSMESA_MINOR_VERSION >= 1102) &&                                 \
  defined(OSMESA_CONTEXT_MAJOR_VERSION)
  if (!this->Internal->OffScreenContextId)
  {
    static const int attribs[] = { OSMESA_FORMAT, OSMESA_RGBA, OSMESA_DEPTH_BITS, 32,
      OSMESA_STENCIL_BITS, 0, OSMESA_ACCUM_BITS, 0, OSMESA_PROFILE, OSMESA_CORE_PROFILE,
      OSMESA_CONTEXT_MAJOR_VERSION, 3, OSMESA_CONTEXT_MINOR_VERSION, 2, 0 };

    OSMesaCreateContextAttribs_func OSMesaCreateContextAttribs =
      (OSMesaCreateContextAttribs_func)OSMesaGetProcAddress("OSMesaCreateContextAttribs");

    if (OSMesaCreateContextAttribs != nullptr)
    {
      this->Internal->OffScreenContextId = OSMesaCreateContextAttribs(attribs, nullptr);
    }
  }
#endif
  // if we still have no context fall back to the generic signature
  if (!this->Internal->OffScreenContextId)
  {
    this->Internal->OffScreenContextId = OSMesaCreateContext(GL_RGBA, nullptr);
  }
  this->MakeCurrent();

  this->Mapped = 0;
  this->Size[0] = width;
  this->Size[1] = height;

  this->MakeCurrent();

  // tell our renderers about us
  svtkRenderer* ren;
  for (this->Renderers->InitTraversal(); (ren = this->Renderers->GetNextItem());)
  {
    ren->SetRenderWindow(0);
    ren->SetRenderWindow(this);
  }

  this->OpenGLInit();
}

void svtkOSOpenGLRenderWindow::DestroyOffScreenWindow()
{
  // Release graphic resources.

  // First release graphics resources on the window itself
  // since call to Renderer's SetRenderWindow(nullptr), just
  // calls ReleaseGraphicsResources on svtkProps. And also
  // this call invokes Renderer's ReleaseGraphicsResources
  // method which only invokes ReleaseGraphicsResources on
  // rendering passes.
  this->ReleaseGraphicsResources(this);

  if (this->Internal->OffScreenContextId)
  {
    OSMesaDestroyContext(this->Internal->OffScreenContextId);
    this->Internal->OffScreenContextId = nullptr;
    svtkOSMesaDestroyWindow(this->Internal->OffScreenWindow);
    this->Internal->OffScreenWindow = nullptr;
  }
}

void svtkOSOpenGLRenderWindow::ResizeOffScreenWindow(int width, int height)
{
  if (this->Internal->OffScreenContextId)
  {
    this->DestroyOffScreenWindow();
    this->CreateOffScreenWindow(width, height);
  }
}

// Initialize the window for rendering.
void svtkOSOpenGLRenderWindow::WindowInitialize(void)
{
  this->CreateAWindow();

  this->MakeCurrent();

  // tell our renderers about us
  svtkRenderer* ren;
  for (this->Renderers->InitTraversal(); (ren = this->Renderers->GetNextItem());)
  {
    ren->SetRenderWindow(0);
    ren->SetRenderWindow(this);
  }

  this->OpenGLInit();
}

// Initialize the rendering window.
void svtkOSOpenGLRenderWindow::Initialize(void)
{
  if (!(this->Internal->OffScreenContextId))
  {
    // initialize offscreen window
    int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
    int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
    this->CreateOffScreenWindow(width, height);
  }
}

void svtkOSOpenGLRenderWindow::Finalize(void)
{
  // clean and destroy window
  this->DestroyWindow();
}

// Change the window to fill the entire screen.
void svtkOSOpenGLRenderWindow::SetFullScreen(svtkTypeBool arg)
{
  (void)arg;
  this->Modified();
}

// Resize the window.
void svtkOSOpenGLRenderWindow::WindowRemap()
{
  // shut everything down
  this->Finalize();

  // set everything up again
  this->Initialize();
}

// Specify the size of the rendering window.
void svtkOSOpenGLRenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height))
  {
    this->Superclass::SetSize(width, height);
    if (!this->UseOffScreenBuffers)
    {
      this->ResizeOffScreenWindow(width, height);
    }
    this->Modified();
  }
}

void svtkOSOpenGLRenderWindow::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OffScreenContextId: " << this->Internal->OffScreenContextId << "\n";
}

void svtkOSOpenGLRenderWindow::MakeCurrent()
{
  // set the current window
  if (this->Internal->OffScreenContextId)
  {
    if (OSMesaMakeCurrent(this->Internal->OffScreenContextId, this->Internal->OffScreenWindow,
          GL_UNSIGNED_BYTE, this->Size[0], this->Size[1]) != GL_TRUE)
    {
      svtkWarningMacro("failed call to OSMesaMakeCurrent");
    }
  }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool svtkOSOpenGLRenderWindow::IsCurrent()
{
  bool result = false;
  if (this->Internal->OffScreenContextId)
  {
    result = this->Internal->OffScreenContextId == OSMesaGetCurrentContext();
  }
  return result;
}

void svtkOSOpenGLRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

void* svtkOSOpenGLRenderWindow::GetGenericContext()
{
  return (void*)this->Internal->OffScreenContextId;
}

svtkTypeBool svtkOSOpenGLRenderWindow::GetEventPending()
{
  return 0;
}

// Get the size of the screen in pixels
int* svtkOSOpenGLRenderWindow::GetScreenSize()
{
  this->ScreenSize[0] = 1280;
  this->ScreenSize[1] = 1024;
  return this->ScreenSize;
}

// Get the position in screen coordinates (pixels) of the window.
int* svtkOSOpenGLRenderWindow::GetPosition(void)
{
  return this->Position;
}

// Move the window to a new position on the display.
void svtkOSOpenGLRenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
  }
  this->Position[0] = x;
  this->Position[1] = y;
}

// Set this RenderWindow's X window id to a pre-existing window.
void svtkOSOpenGLRenderWindow::SetWindowInfo(const char* info)
{
  int tmp;

  this->OwnDisplay = 1;

  sscanf(info, "%i", &tmp);
}

// Set this RenderWindow's X window id to a pre-existing window.
void svtkOSOpenGLRenderWindow::SetNextWindowInfo(const char* info)
{
  int tmp;
  sscanf(info, "%i", &tmp);

  //   this->SetNextWindowId((Window)tmp);
}

// Sets the X window id of the window that WILL BE created.
void svtkOSOpenGLRenderWindow::SetParentInfo(const char* info)
{
  int tmp;

  // get the default display connection
  this->OwnDisplay = 1;

  sscanf(info, "%i", &tmp);

  //   this->SetParentId(tmp);
}

void svtkOSOpenGLRenderWindow::SetWindowId(void* arg)
{
  (void)arg;
  //   this->SetWindowId((Window)arg);
}
void svtkOSOpenGLRenderWindow::SetParentId(void* arg)
{
  (void)arg;
  //   this->SetParentId((Window)arg);
}

const char* svtkOSOpenGLRenderWindow::ReportCapabilities()
{
  MakeCurrent();

  //   int scrnum = DefaultScreen(this->DisplayId);

  const char* glVendor = (const char*)glGetString(GL_VENDOR);
  const char* glRenderer = (const char*)glGetString(GL_RENDERER);
  const char* glVersion = (const char*)glGetString(GL_VERSION);
  const char* glExtensions = (const char*)glGetString(GL_EXTENSIONS);

  std::ostringstream strm;
  strm << "OpenGL vendor string:  " << glVendor << endl;
  strm << "OpenGL renderer string:  " << glRenderer << endl;
  strm << "OpenGL version string:  " << glVersion << endl;
  strm << "OpenGL extensions:  " << glExtensions << endl;
  delete[] this->Capabilities;
  size_t len = strm.str().length();
  this->Capabilities = new char[len + 1];
  strncpy(this->Capabilities, strm.str().c_str(), len);
  this->Capabilities[len] = 0;
  return this->Capabilities;
}

int svtkOSOpenGLRenderWindow::SupportsOpenGL()
{
  MakeCurrent();
  return 1;
}

svtkTypeBool svtkOSOpenGLRenderWindow::IsDirect()
{
  MakeCurrent();
  return 0;
}

void svtkOSOpenGLRenderWindow::SetWindowName(const char* cname)
{
  char* name = new char[strlen(cname) + 1];
  strcpy(name, cname);
  svtkOpenGLRenderWindow::SetWindowName(name);
  delete[] name;
}

void svtkOSOpenGLRenderWindow::SetNextWindowId(void* arg)
{
  (void)arg;
}

// This probably has been moved to superclass.
void* svtkOSOpenGLRenderWindow::GetGenericWindowId()
{
  return (void*)this->Internal->OffScreenWindow;
}
