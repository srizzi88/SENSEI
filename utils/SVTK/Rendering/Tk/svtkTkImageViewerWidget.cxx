/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTkImageViewerWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTkImageViewerWidget.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTkInternals.h"
#include "svtkVersionMacros.h"

#ifdef _MSC_VER
#pragma warning(disable : 4273)
#else
#if defined(SVTK_USE_COCOA)
#include "svtkCocoaRenderWindow.h"
#include "svtkCocoaTkUtilities.h"
#ifndef MAC_OSX_TK
#define MAC_OSX_TK 1
#endif
#include "tkInt.h"
#else
#include "svtkXOpenGLRenderWindow.h"
#endif
#endif

#include <cstdlib>

#define SVTK_ALL_EVENTS_MASK                                                                        \
  KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask |          \
    LeaveWindowMask | PointerMotionMask | ExposureMask | VisibilityChangeMask | FocusChangeMask |  \
    PropertyChangeMask | ColormapChangeMask

#define SVTK_MAX(a, b) (((a) > (b)) ? (a) : (b))

// These are the options that can be set when the widget is created
// or with the command configure.  The only new one is "-rw" which allows
// the uses to set their own ImageViewer window.
static Tk_ConfigSpec svtkTkImageViewerWidgetConfigSpecs[] = {
  { TK_CONFIG_PIXELS, (char*)"-height", (char*)"height", (char*)"Height", (char*)"400",
    Tk_Offset(struct svtkTkImageViewerWidget, Height), 0, nullptr },

  { TK_CONFIG_PIXELS, (char*)"-width", (char*)"width", (char*)"Width", (char*)"400",
    Tk_Offset(struct svtkTkImageViewerWidget, Width), 0, nullptr },

  { TK_CONFIG_STRING, (char*)"-iv", (char*)"iv", (char*)"IV", (char*)"",
    Tk_Offset(struct svtkTkImageViewerWidget, IV), 0, nullptr },

  { TK_CONFIG_END, (char*)nullptr, (char*)nullptr, (char*)nullptr, (char*)nullptr, 0, 0, nullptr }
};

// Forward prototypes
extern "C"
{
  void svtkTkImageViewerWidget_EventProc(ClientData clientData, XEvent* eventPtr);
}

static int svtkTkImageViewerWidget_MakeImageViewer(struct svtkTkImageViewerWidget* self);
extern int svtkImageViewerCommand(ClientData cd, Tcl_Interp* interp, int argc, char* argv[]);

//----------------------------------------------------------------------------
// It's possible to change with this function or in a script some
// options like width, height or the ImageViewer widget.
int svtkTkImageViewerWidget_Configure(
  Tcl_Interp* interp, struct svtkTkImageViewerWidget* self, int argc, char* argv[], int flags)
{
  // Let Tk handle generic configure options.
  if (Tk_ConfigureWidget(interp, self->TkWin, svtkTkImageViewerWidgetConfigSpecs, argc,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
        const_cast<CONST84 char**>(argv),
#else
        argv,
#endif
        (char*)self, flags) == TCL_ERROR)
  {
    return (TCL_ERROR);
  }

  // Get the new width and height of the widget
  Tk_GeometryRequest(self->TkWin, self->Width, self->Height);

  // Make sure the ImageViewer window has been set.  If not, create one.
  if (svtkTkImageViewerWidget_MakeImageViewer(self) == TCL_ERROR)
  {
    return TCL_ERROR;
  }

  return TCL_OK;
}

//----------------------------------------------------------------------------
// This function is called when the ImageViewer widget name is
// evaluated in a Tcl script.  It will compare string parameters
// to choose the appropriate method to invoke.
extern "C"
{
  int svtkTkImageViewerWidget_Widget(ClientData clientData, Tcl_Interp* interp, int argc,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
    CONST84
#endif
    char* argv[])
  {
    struct svtkTkImageViewerWidget* self = (struct svtkTkImageViewerWidget*)clientData;
    int result = TCL_OK;

    // Check to see if the command has enough arguments.
    if (argc < 2)
    {
      Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], " ?options?\"", nullptr);
      return TCL_ERROR;
    }

    // Make sure the widget is not deleted during this function
    Tk_Preserve((ClientData)self);

    // Handle render call to the widget
    if (strncmp(argv[1], "render", SVTK_MAX(1, strlen(argv[1]))) == 0 ||
      strncmp(argv[1], "Render", SVTK_MAX(1, strlen(argv[1]))) == 0)
    {
      // make sure we have a window
      if (self->ImageViewer == nullptr)
      {
        svtkTkImageViewerWidget_MakeImageViewer(self);
      }
      self->ImageViewer->Render();
    }
    // Handle configure method
    else if (!strncmp(argv[1], "configure", SVTK_MAX(1, strlen(argv[1]))))
    {
      if (argc == 2)
      {
        /* Return list of all configuration parameters */
        result = Tk_ConfigureInfo(
          interp, self->TkWin, svtkTkImageViewerWidgetConfigSpecs, (char*)self, (char*)nullptr, 0);
      }
      else if (argc == 3)
      {
        /* Return a specific configuration parameter */
        result = Tk_ConfigureInfo(
          interp, self->TkWin, svtkTkImageViewerWidgetConfigSpecs, (char*)self, argv[2], 0);
      }
      else
      {
        /* Execute a configuration change */
        result = svtkTkImageViewerWidget_Configure(interp, self, argc - 2,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
          const_cast<char**>(argv + 2),
#else
          argv + 2,
#endif
          TK_CONFIG_ARGV_ONLY);
      }
    }
    else if (!strcmp(argv[1], "GetImageViewer"))
    { // Get ImageViewerWindow is my own method
      // Create a ImageViewerWidget if one has not been set yet.
      result = svtkTkImageViewerWidget_MakeImageViewer(self);
      if (result != TCL_ERROR)
      {
        // Return the name (Make Tcl copy the string)
        Tcl_SetResult(interp, self->IV, TCL_VOLATILE);
      }
    }
    else
    {
      // Unknown method name.
      Tcl_AppendResult(interp, "svtkTkImageViewerWidget: Unknown option: ", argv[1], "\n",
        "Try: configure or GetImageViewer\n", nullptr);
      result = TCL_ERROR;
    }

    // Unlock the object so it can be deleted.
    Tk_Release((ClientData)self);
    return result;
  }
}

//----------------------------------------------------------------------------
// svtkTkImageViewerWidget_Cmd
// Called when svtkTkImageViewerWidget is executed
// - creation of a svtkTkImageViewerWidget widget.
//     * Creates a new window
//     * Creates an 'svtkTkImageViewerWidget' data structure
//     * Creates an event handler for this window
//     * Creates a command that handles this object
//     * Configures this svtkTkImageViewerWidget for the given arguments
extern "C"
{
  int svtkTkImageViewerWidget_Cmd(ClientData clientData, Tcl_Interp* interp, int argc,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
    CONST84
#endif
    char** argv)
  {
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
    CONST84
#endif
    char* name;
    Tk_Window main = (Tk_Window)clientData;
    Tk_Window tkwin;
    struct svtkTkImageViewerWidget* self;

    // Make sure we have an instance name.
    if (argc <= 1)
    {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, "wrong # args: should be \"pathName read filename\"", nullptr);
      return (TCL_ERROR);
    }

    // Create the window.
    name = argv[1];
    // Possibly X dependent
    tkwin = Tk_CreateWindowFromPath(interp, main, name, (char*)nullptr);
    if (tkwin == nullptr)
    {
      return TCL_ERROR;
    }

    // Tcl needs this for setting options and matching event bindings.
    Tk_SetClass(tkwin, (char*)"svtkTkImageViewerWidget");

    // Create svtkTkImageViewerWidget data structure
    self = (struct svtkTkImageViewerWidget*)ckalloc(sizeof(struct svtkTkImageViewerWidget));

    self->TkWin = tkwin;
    self->Interp = interp;
    self->Width = 0;
    self->Height = 0;
    self->ImageViewer = nullptr;
    self->IV = nullptr;

    // ...
    // Create command event handler
    Tcl_CreateCommand(interp, Tk_PathName(tkwin), svtkTkImageViewerWidget_Widget, (ClientData)self,
      (void (*)(ClientData))nullptr);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
      svtkTkImageViewerWidget_EventProc, (ClientData)self);

    // Configure svtkTkImageViewerWidget widget
    if (svtkTkImageViewerWidget_Configure(interp, self, argc - 2,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
          const_cast<char**>(argv + 2),
#else
          argv + 2,
#endif
          0) == TCL_ERROR)
    {
      Tk_DestroyWindow(tkwin);
      Tcl_DeleteCommand(interp, (char*)"svtkTkImageViewerWidget");
      // Don't free it, if we do a crash occurs later...
      // free(self);
      return TCL_ERROR;
    }

    Tcl_AppendResult(interp, Tk_PathName(tkwin), nullptr);
    return TCL_OK;
  }
}

//----------------------------------------------------------------------------
char* svtkTkImageViewerWidget_IV(const struct svtkTkImageViewerWidget* self)
{
  return self->IV;
}

//----------------------------------------------------------------------------
int svtkTkImageViewerWidget_Width(const struct svtkTkImageViewerWidget* self)
{
  return self->Width;
}

//----------------------------------------------------------------------------
int svtkTkImageViewerWidget_Height(const struct svtkTkImageViewerWidget* self)
{
  return self->Height;
}

extern "C"
{
  void svtkTkImageViewerWidget_Destroy(char* memPtr)
  {
    struct svtkTkImageViewerWidget* self = (struct svtkTkImageViewerWidget*)memPtr;

    if (self->ImageViewer)
    {
      if (self->ImageViewer->GetRenderWindow()->GetInteractor() &&
        self->ImageViewer->GetRenderWindow()->GetInteractor()->GetRenderWindow() ==
          self->ImageViewer->GetRenderWindow())
      {
        self->ImageViewer->GetRenderWindow()->GetInteractor()->SetRenderWindow(0);
      }
      if (self->ImageViewer->GetRenderWindow()->GetReferenceCount() > 1)
      {
        svtkGenericWarningMacro("A TkImageViewerWidget is being destroyed before it associated "
                               "svtkImageViewer is destroyed. This is very bad and usually due to "
                               "the order in which objects are being destroyed. Always destroy the "
                               "svtkImageViewer before destroying the user interface components.");
        return;
      }
      // Squash the ImageViewer's WindowID
      self->ImageViewer->SetWindowId((void*)nullptr);
      self->ImageViewer->UnRegister(nullptr);
      self->ImageViewer = nullptr;
      ckfree(self->IV);
    }
    ckfree((char*)memPtr);
  }
}

//----------------------------------------------------------------------------
// This gets called to handle svtkTkImageViewerWidget wind configuration events
// Possibly X dependent
extern "C"
{
  void svtkTkImageViewerWidget_EventProc(ClientData clientData, XEvent* eventPtr)
  {
    struct svtkTkImageViewerWidget* self = (struct svtkTkImageViewerWidget*)clientData;

    switch (eventPtr->type)
    {
      case Expose:
        if (eventPtr->xexpose.count == 0)
        /* && !self->UpdatePending)*/
        {
          // bid this in tcl now
          // self->ImageViewer->Render();
        }
        break;
      case ConfigureNotify:
        if (1 /*Tk_IsMapped(self->TkWin)*/)
        {
          self->Width = Tk_Width(self->TkWin);
          self->Height = Tk_Height(self->TkWin);
          // Tk_GeometryRequest(self->TkWin,self->Width,self->Height);
          if (self->ImageViewer)
          {
            int x = Tk_X(self->TkWin);
            int y = Tk_Y(self->TkWin);
#if defined(SVTK_USE_COCOA)
            // Do not call SetSize or SetPosition until we're mapped.
            if (Tk_IsMapped(self->TkWin))
            {
              // On Cocoa, compute coordinates relative to toplevel
              for (TkWindow* curPtr = ((TkWindow*)self->TkWin)->parentPtr;
                   (nullptr != curPtr) && !(curPtr->flags & TK_TOP_LEVEL);
                   curPtr = curPtr->parentPtr)
              {
                x += Tk_X(curPtr);
                y += Tk_Y(curPtr);
              }
              self->ImageViewer->SetPosition(x, y);
              self->ImageViewer->SetSize(self->Width, self->Height);
            }
#else
            self->ImageViewer->SetPosition(x, y);
            self->ImageViewer->SetSize(self->Width, self->Height);
#endif
          }

          // svtkTkImageViewerWidget_PostRedisplay(self);
        }
        break;
      case MapNotify:
#if defined(SVTK_USE_COCOA)
      {
        // On Cocoa, compute coordinates relative to the toplevel
        int x = Tk_X(self->TkWin);
        int y = Tk_Y(self->TkWin);
        for (TkWindow* curPtr = ((TkWindow*)self->TkWin)->parentPtr;
             (nullptr != curPtr) && !(curPtr->flags & TK_TOP_LEVEL); curPtr = curPtr->parentPtr)
        {
          x += Tk_X(curPtr);
          y += Tk_Y(curPtr);
        }
        self->ImageViewer->SetPosition(x, y);
        self->ImageViewer->SetSize(self->Width, self->Height);
      }
#endif
      break;
#if defined(SVTK_USE_COCOA)
      case UnmapNotify:
        break;
#endif
      case DestroyNotify:
#ifdef _WIN32
        if (self->ImageViewer->GetRenderWindow()->GetGenericWindowId())
        {
          svtkSetWindowLong((HWND)self->ImageViewer->GetRenderWindow()->GetGenericWindowId(),
            svtkGWL_USERDATA, (svtkLONG)((TkWindow*)self->TkWin)->window);
          svtkSetWindowLong((HWND)self->ImageViewer->GetRenderWindow()->GetGenericWindowId(),
            svtkGWL_WNDPROC, (svtkLONG)TkWinChildProc);
        }
#endif
        Tcl_EventuallyFree((ClientData)self, svtkTkImageViewerWidget_Destroy);
        break;
      default
        :
        // nothing
        ;
    }
  }
}

//----------------------------------------------------------------------------
// svtkTkImageViewerWidget_Init
// Called upon system startup to create svtkTkImageViewerWidget command.
extern "C"
{
  SVTK_EXPORT int Vtktkimageviewerwidget_Init(Tcl_Interp* interp);
}

#define SVTKTK_TO_STRING(x) SVTKTK_TO_STRING0(x)
#define SVTKTK_TO_STRING0(x) SVTKTK_TO_STRING1(x)
#define SVTKTK_TO_STRING1(x) #x
#define SVTKTK_VERSION SVTKTK_TO_STRING(SVTK_MAJOR_VERSION) "." SVTKTK_TO_STRING(SVTK_MINOR_VERSION)

int Vtktkimageviewerwidget_Init(Tcl_Interp* interp)
{
  // This widget requires Tk to function.
  Tcl_PkgRequire(interp, (char*)"Tk", (char*)TK_VERSION, 0);
  if (Tcl_PkgPresent(interp, (char*)"Tk", (char*)TK_VERSION, 0))
  {
    // Register the commands for this package.
    Tcl_CreateCommand(interp, (char*)"svtkTkImageViewerWidget", svtkTkImageViewerWidget_Cmd,
      Tk_MainWindow(interp), nullptr);

    // Report that the package is provided.
    return Tcl_PkgProvide(interp, (char*)"Vtktkimageviewerwidget", (char*)SVTKTK_VERSION);
  }
  else
  {
    // Tk is not available.
    return TCL_ERROR;
  }
}

// Here is the windows specific code for creating the window
// The Xwindows version follows after this
#ifdef _WIN32

LRESULT APIENTRY svtkTkImageViewerWidgetProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT rval;
  struct svtkTkImageViewerWidget* self =
    (struct svtkTkImageViewerWidget*)svtkGetWindowLong(hWnd, svtkGWL_USERDATA);

  if (!self)
  {
    return 0;
  }

  // forward message to Tk handler
  svtkSetWindowLong(hWnd, svtkGWL_USERDATA, (svtkLONG)((TkWindow*)self->TkWin)->window);
  if (((TkWindow*)self->TkWin)->parentPtr)
  {
    svtkSetWindowLong(hWnd, svtkGWL_WNDPROC, (svtkLONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd, message, wParam, lParam);
  }
  else
  {
//
// TkWinTopLevelProc has been deprecated in Tcl/Tk8.0.  Not sure how
// well this will actually work in 8.0.
//
#if (TK_MAJOR_VERSION < 8)
    svtkSetWindowLong(hWnd, svtkGWL_WNDPROC, (svtkLONG)TkWinTopLevelProc);
    rval = TkWinTopLevelProc(hWnd, message, wParam, lParam);
#else
    if (message == WM_WINDOWPOSCHANGED)
    {
      XEvent event;
      WINDOWPOS* pos = (WINDOWPOS*)lParam;
      TkWindow* winPtr = (TkWindow*)Tk_HWNDToWindow(pos->hwnd);

      if (winPtr == nullptr)
      {
        return 0;
      }

      /*
       * Update the shape of the contained window.
       */
      if (!(pos->flags & SWP_NOSIZE))
      {
        winPtr->changes.width = pos->cx;
        winPtr->changes.height = pos->cy;
      }
      if (!(pos->flags & SWP_NOMOVE))
      {
        winPtr->changes.x = pos->x;
        winPtr->changes.y = pos->y;
      }

      /*
       *  Generate a ConfigureNotify event.
       */
      event.type = ConfigureNotify;
      event.xconfigure.serial = winPtr->display->request;
      event.xconfigure.send_event = False;
      event.xconfigure.display = winPtr->display;
      event.xconfigure.event = winPtr->window;
      event.xconfigure.window = winPtr->window;
      event.xconfigure.border_width = winPtr->changes.border_width;
      event.xconfigure.override_redirect = winPtr->atts.override_redirect;
      event.xconfigure.x = winPtr->changes.x;
      event.xconfigure.y = winPtr->changes.y;
      event.xconfigure.width = winPtr->changes.width;
      event.xconfigure.height = winPtr->changes.height;
      event.xconfigure.above = None;
      Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

      Tcl_ServiceAll();
      return 0;
    }
    svtkSetWindowLong(hWnd, svtkGWL_WNDPROC, (svtkLONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd, message, wParam, lParam);
#endif
  }

  if (message != WM_PAINT)
  {
    if (self->ImageViewer)
    {
      svtkSetWindowLong(hWnd, svtkGWL_USERDATA, (svtkLONG)self->ImageViewer->GetRenderWindow());
      svtkSetWindowLong(hWnd, svtkGWL_WNDPROC, (svtkLONG)self->OldProc);
      CallWindowProc(self->OldProc, hWnd, message, wParam, lParam);
    }
  }

  // now reset to the original config
  svtkSetWindowLong(hWnd, svtkGWL_USERDATA, (svtkLONG)self);
  svtkSetWindowLong(hWnd, svtkGWL_WNDPROC, (svtkLONG)svtkTkImageViewerWidgetProc);
  return rval;
}

//-----------------------------------------------------------------------------
// Creates a ImageViewer window and forces Tk to use the window.
static int svtkTkImageViewerWidget_MakeImageViewer(struct svtkTkImageViewerWidget* self)
{
  Display* dpy;
  TkWindow* winPtr = (TkWindow*)self->TkWin;
  Tcl_HashEntry* hPtr;
  int new_flag;
  svtkImageViewer* imgViewer;
  TkWinDrawable* twdPtr;
  HWND parentWin;
  svtkRenderWindow* imgWindow;

  if (self->ImageViewer)
  {
    return TCL_OK;
  }

  dpy = Tk_Display(self->TkWin);

  if (winPtr->window != None)
  {
    // XDestroyWindow(dpy, winPtr->window);
  }

  if (self->IV[0] == '\0')
  {
    // Make the ImageViewer window.
    self->ImageViewer = imgViewer = svtkImageViewer::New();
#ifndef SVTK_PYTHON_BUILD
    svtkTclGetObjectFromPointer(self->Interp, self->ImageViewer, "svtkImageViewer");
#endif
    ckfree(self->IV);
    self->IV = strdup(Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is IV an address ? big ole python hack here
    if (self->IV[0] == 'A' && self->IV[1] == 'd' && self->IV[2] == 'd' && self->IV[3] == 'r')
    {
      void* tmp;
      sscanf(self->IV + 5, "%p", &tmp);
      imgViewer = (svtkImageViewer*)tmp;
    }
    else
    {
#ifndef SVTK_PYTHON_BUILD
      imgViewer = (svtkImageViewer*)svtkTclGetPointerFromObject(
        self->IV, "svtkImageViewer", self->Interp, new_flag);
#else
      imgViewer = 0;
#endif
    }
    if (imgViewer != self->ImageViewer)
    {
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->UnRegister(nullptr);
      }
      self->ImageViewer = (svtkImageViewer*)(imgViewer);
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->Register(nullptr);
      }
    }
  }

  // Set the size
  self->ImageViewer->SetSize(self->Width, self->Height);

  // Set the parent correctly
  // Possibly X dependent
  if ((winPtr->parentPtr != nullptr) && !(winPtr->flags & TK_TOP_LEVEL))
  {
    if (winPtr->parentPtr->window == None)
    {
      Tk_MakeWindowExist((Tk_Window)winPtr->parentPtr);
    }

    parentWin = ((TkWinDrawable*)winPtr->parentPtr->window)->window.handle;
    imgViewer->SetParentId(parentWin);
  }

  // Use the same display
  self->ImageViewer->SetDisplayId(dpy);

  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  // Tk_SetWindowVisual(self->TkWin, imgViewer->GetDesiredVisual(),
  // ImageViewer->GetDesiredDepth(),
  // ImageViewer->GetDesiredColormap());

  self->ImageViewer->Render();
  imgWindow = self->ImageViewer->GetRenderWindow();

#if (TK_MAJOR_VERSION >= 8)
  twdPtr = (TkWinDrawable*)Tk_AttachHWND(self->TkWin, (HWND)imgWindow->GetGenericWindowId());
#else
  twdPtr = (TkWinDrawable*)ckalloc(sizeof(TkWinDrawable));
  twdPtr->type = TWD_WINDOW;
  twdPtr->window.winPtr = winPtr;
  twdPtr->window.handle = (HWND)imgWindow->GetGenericWindowId();
#endif

  self->OldProc = (WNDPROC)svtkGetWindowLong(twdPtr->window.handle, svtkGWL_WNDPROC);
  svtkSetWindowLong(twdPtr->window.handle, svtkGWL_USERDATA, (svtkLONG)self);
  svtkSetWindowLong(twdPtr->window.handle, svtkGWL_WNDPROC, (svtkLONG)svtkTkImageViewerWidgetProc);

  winPtr->window = (Window)twdPtr;

  hPtr = Tcl_CreateHashEntry(&winPtr->dispPtr->winTable, (char*)winPtr->window, &new_flag);
  Tcl_SetHashValue(hPtr, winPtr);

  winPtr->dirtyAtts = 0;
  winPtr->dirtyChanges = 0;
#ifdef TK_USE_INPUT_METHODS
  winPtr->inputContext = nullptr;
#endif // TK_USE_INPUT_METHODS

  if (!(winPtr->flags & TK_TOP_LEVEL))
  {
    /*
     * If this window has a different colormap than its parent, add
     * the window to the WM_COLORMAP_WINDOWS property for its top-level.
     */
    if ((winPtr->parentPtr != nullptr) &&
      (winPtr->atts.colormap != winPtr->parentPtr->atts.colormap))
    {
      TkWmAddToColormapWindows(winPtr);
    }
  }

  /*
   * Issue a ConfigureNotify event if there were deferred configuration
   * changes (but skip it if the window is being deleted;  the
   * ConfigureNotify event could cause problems if we're being called
   * from Tk_DestroyWindow under some conditions).
   */
  if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY) && !(winPtr->flags & TK_ALREADY_DEAD))
  {
    XEvent event;

    winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;

    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.border_width = winPtr->changes.border_width;
    if (winPtr->changes.stack_mode == Above)
    {
      event.xconfigure.above = winPtr->changes.sibling;
    }
    else
    {
      event.xconfigure.above = None;
    }
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
  }

  return TCL_OK;
}

// now the Apple version for Cocoa APIs
#else
#if defined(SVTK_USE_COCOA)
//----------------------------------------------------------------------------
// Creates a ImageViewer window and forces Tk to use the window.
static int svtkTkImageViewerWidget_MakeImageViewer(struct svtkTkImageViewerWidget* self)
{
  Display* dpy;
  svtkImageViewer* imgViewer = nullptr;

  if (self->ImageViewer)
  {
    return TCL_OK;
  }

  dpy = Tk_Display(self->TkWin);

  if (self->IV[0] == '\0')
  {
    // Make the ImageViewer window.
    self->ImageViewer = imgViewer = svtkImageViewer::New();
#ifndef SVTK_PYTHON_BUILD
    svtkTclGetObjectFromPointer(self->Interp, self->ImageViewer, "svtkImageViewer");
#endif
    ckfree(self->IV);
    self->IV = strdup(Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is IV an address ? big ole python hack here
    if (self->IV[0] == 'A' && self->IV[1] == 'd' && self->IV[2] == 'd' && self->IV[3] == 'r')
    {
      void* tmp;
      sscanf(self->IV + 5, "%p", &tmp);
      imgViewer = reinterpret_cast<svtkImageViewer*>(tmp);
    }
    else
    {
#ifndef SVTK_PYTHON_BUILD
      int new_flag;
      imgViewer = static_cast<svtkImageViewer*>(
        svtkTclGetPointerFromObject(self->IV, "svtkImageViewer", self->Interp, new_flag));
#endif
    }
    if (imgViewer != self->ImageViewer)
    {
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->UnRegister(nullptr);
      }
      self->ImageViewer = (svtkImageViewer*)(imgViewer);
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->Register(nullptr);
      }
    }
  }

  Tk_MakeWindowExist(self->TkWin);
  // set the ParentId to the NSView
  svtkCocoaRenderWindow* imgWindow =
    static_cast<svtkCocoaRenderWindow*>(imgViewer->GetRenderWindow());
  imgWindow->SetParentId(svtkCocoaTkUtilities::GetDrawableView(self->TkWin));
  imgWindow->SetSize(self->Width, self->Height);

  // Set the size
  self->ImageViewer->SetSize(self->Width, self->Height);

  // Process all outstanding events so that Tk is fully updated
  Tcl_ServiceAll();

  self->ImageViewer->Render();
  return TCL_OK;
}

// now the Xwindows version
#else

//----------------------------------------------------------------------------
// Creates a ImageViewer window and forces Tk to use the window.
static int svtkTkImageViewerWidget_MakeImageViewer(struct svtkTkImageViewerWidget* self)
{
  Display* dpy;
  svtkImageViewer* imgViewer = 0;
  svtkXOpenGLRenderWindow* imgWindow;

  if (self->ImageViewer)
  {
    return TCL_OK;
  }

  dpy = Tk_Display(self->TkWin);

  if (Tk_WindowId(self->TkWin) != None)
  {
    XDestroyWindow(dpy, Tk_WindowId(self->TkWin));
  }

  if (self->IV[0] == '\0')
  {
    // Make the ImageViewer window.
    self->ImageViewer = imgViewer = svtkImageViewer::New();
#ifndef SVTK_PYTHON_BUILD
    svtkTclGetObjectFromPointer(self->Interp, self->ImageViewer, "svtkImageViewer");
#endif
    self->IV = strdup(Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is IV an address ? big ole python hack here
    if (self->IV[0] == 'A' && self->IV[1] == 'd' && self->IV[2] == 'd' && self->IV[3] == 'r')
    {
      void* tmp;
      sscanf(self->IV + 5, "%p", &tmp);
      imgViewer = (svtkImageViewer*)tmp;
    }
    else
    {
#ifndef SVTK_PYTHON_BUILD
      int new_flag;
      imgViewer = (svtkImageViewer*)svtkTclGetPointerFromObject(
        self->IV, "svtkImageViewer", self->Interp, new_flag);
#endif
    }
    if (imgViewer != self->ImageViewer)
    {
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->UnRegister(nullptr);
      }
      self->ImageViewer = (svtkImageViewer*)(imgViewer);
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->Register(nullptr);
      }
    }
  }

  // get the window
  imgWindow = static_cast<svtkXOpenGLRenderWindow*>(imgViewer->GetRenderWindow());
  // If the imageviewer has already created it's window, throw up our hands and quit...
  if (imgWindow->GetWindowId() != (Window)nullptr)
  {
    return TCL_ERROR;
  }

  // Use the same display
  imgWindow->SetDisplayId(dpy);
  // The visual MUST BE SET BEFORE the window is created.
  Tk_SetWindowVisual(self->TkWin, imgWindow->GetDesiredVisual(), imgWindow->GetDesiredDepth(),
    imgWindow->GetDesiredColormap());

  // Make this window exist, then use that information to make the svtkImageViewer in sync
  Tk_MakeWindowExist(self->TkWin);
  imgViewer->SetWindowId((void*)Tk_WindowId(self->TkWin));

  // Set the size
  self->ImageViewer->SetSize(self->Width, self->Height);

  // Set the parent correctly
  // Possibly X dependent
  if ((Tk_Parent(self->TkWin) == nullptr) || (Tk_IsTopLevel(self->TkWin)))
  {
    imgWindow->SetParentId(XRootWindow(Tk_Display(self->TkWin), Tk_ScreenNumber(self->TkWin)));
  }
  else
  {
    imgWindow->SetParentId(Tk_WindowId(Tk_Parent(self->TkWin)));
  }

  self->ImageViewer->Render();
  return TCL_OK;
}
#endif
#endif
