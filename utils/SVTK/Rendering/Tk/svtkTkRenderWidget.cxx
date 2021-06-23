/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTkRenderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTkRenderWidget.h"
#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkImageData.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTclUtil.h"
#include "svtkTkInternals.h"
#include "svtkToolkits.h"
#include "svtkVersionMacros.h"

#ifdef _WIN32
#include "svtkWin32OpenGLRenderWindow.h"
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

#include <cstdint>
#include <cstdlib>
#include <vector>

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#define SVTK_ALL_EVENTS_MASK                                                                        \
  KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask |          \
    LeaveWindowMask | PointerMotionMask | ExposureMask | VisibilityChangeMask | FocusChangeMask |  \
    PropertyChangeMask | ColormapChangeMask

#define SVTK_MAX(a, b) (((a) > (b)) ? (a) : (b))

// These are the options that can be set when the widget is created
// or with the command configure.  The only new one is "-rw" which allows
// the uses to set their own render window.
static Tk_ConfigSpec svtkTkRenderWidgetConfigSpecs[] = {
  { TK_CONFIG_PIXELS, (char*)"-height", (char*)"height", (char*)"Height", (char*)"400",
    Tk_Offset(struct svtkTkRenderWidget, Height), 0, nullptr },

  { TK_CONFIG_PIXELS, (char*)"-width", (char*)"width", (char*)"Width", (char*)"400",
    Tk_Offset(struct svtkTkRenderWidget, Width), 0, nullptr },

  { TK_CONFIG_STRING, (char*)"-rw", (char*)"rw", (char*)"RW", (char*)"",
    Tk_Offset(struct svtkTkRenderWidget, RW), 0, nullptr },

  { TK_CONFIG_END, (char*)nullptr, (char*)nullptr, (char*)nullptr, (char*)nullptr, 0, 0, nullptr }
};

// Forward prototypes
extern "C"
{
  void svtkTkRenderWidget_EventProc(ClientData clientData, XEvent* eventPtr);
}

static int svtkTkRenderWidget_MakeRenderWindow(struct svtkTkRenderWidget* self);
extern int svtkRenderWindowCommand(ClientData cd, Tcl_Interp* interp, int argc, char* argv[]);

// Start of svtkImageDataToTkPhoto
template <class T>
void svtkExtractImageData(unsigned char* buffer, T* inPtr, double shift, double scale, int width,
  int height, int pitch, int pixelSize, int components)
{
  T* ImagePtr;
  unsigned char* BufferPtr;
  int i, j, c;
  float pixel;

  BufferPtr = buffer;
  // ImagePtr = inPtr;

  for (j = 0; j < height; j++)
  {
    ImagePtr = j * pitch + inPtr;
    for (i = 0; i < width; i++)
    {
      for (c = 0; c < components; c++)
      {
        // Clamp
        pixel = (*ImagePtr + shift) * scale;
        if (pixel < 0)
        {
          pixel = 0;
        }
        else
        {
          if (pixel > 255)
          {
            pixel = 255;
          }
        }
        *BufferPtr = (unsigned char)pixel;
        ImagePtr++;
        BufferPtr++;
      }
      ImagePtr += pixelSize - components;
    }
  }
  return;
}

extern "C"
{
#define SVTKIMAGEDATATOTKPHOTO_CORONAL 0
#define SVTKIMAGEDATATOTKPHOTO_SAGITTAL 1
#define SVTKIMAGEDATATOTKPHOTO_TRANSVERSE 2
  int svtkImageDataToTkPhoto_Cmd(ClientData svtkNotUsed(clientData), Tcl_Interp* interp, int argc,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
    CONST84
#endif
    char** argv)
  {
    int status = 0;
    svtkImageData* image;
    Tk_PhotoHandle photo;
    int slice = 0;
    double window = 256.0;
    double level = window / 2.0;
    int orientation = SVTKIMAGEDATATOTKPHOTO_TRANSVERSE;

    // Usage: svtkImageDataToTkPhoto svtkImageData photo slice
    if (argc < 4 || argc > 7)
    {
      const char m[] = "wrong # args: should be \"svtkImageDataToTkPhoto svtkImageData photo slice "
                       "[orientation] [window] [level]\"";

      Tcl_SetResult(interp, const_cast<char*>(m), TCL_VOLATILE);
      return TCL_ERROR;
    }

    // Start with slice, it's fast, etc...
    status = Tcl_GetInt(interp, argv[3], &slice);
    if (status != TCL_OK)
    {
      return status;
    }

    // Find the image
#ifdef SVTK_PYTHON_BUILD
    char typeCheck[256];
    unsigned long long l;
    sscanf(argv[1], "_%llx_%s", &l, typeCheck);
    union {
      void* p;
      uintptr_t l;
    } u;
    u.l = static_cast<uintptr_t>(l);
    // Various historical pointer manglings
    if ((strcmp("svtkAlgorithmOutput", typeCheck) == 0 ||
          strcmp("svtkAlgorithmOutput_p", typeCheck) == 0 ||
          strcmp("p_svtkAlgorithmOutput", typeCheck) == 0))
    {
      svtkAlgorithmOutput* algOutput = static_cast<svtkAlgorithmOutput*>(u.p);
      if (algOutput)
      {
        svtkAlgorithm* alg = algOutput->GetProducer();
        alg->Update();
        u.p = svtkImageData::SafeDownCast(alg->GetOutputDataObject(algOutput->GetIndex()));
      }
    }
    else if (strcmp("svtkImageData", typeCheck) != 0 && strcmp("svtkImageData_p", typeCheck) != 0 &&
      strcmp("p_svtkImageData", typeCheck) != 0 && strcmp("svtkStructuredPoints", typeCheck) != 0 &&
      strcmp("svtkStructuredPoints_p", typeCheck) != 0 &&
      strcmp("p_svtkStructuredPoints", typeCheck) != 0)
    {
      // bad type
      u.p = nullptr;
    }
    image = static_cast<svtkImageData*>(u.p);
#else
    image = static_cast<svtkImageData*>(
      svtkTclGetPointerFromObject(argv[1], "svtkImageData", interp, status));
    if (!image)
    {
      svtkAlgorithmOutput* algOutput = static_cast<svtkAlgorithmOutput*>(
        svtkTclGetPointerFromObject(argv[1], "svtkAlgorithmOutput", interp, status));
      if (algOutput)
      {
        svtkAlgorithm* alg = algOutput->GetProducer();
        alg->Update();
        image = svtkImageData::SafeDownCast(alg->GetOutputDataObject(algOutput->GetIndex()));
      }
    }
#endif
    if (!image)
    {
      Tcl_AppendResult(interp, "could not find svtkImageData: ", argv[1], nullptr);
      return TCL_ERROR;
    }

    // Find the photo widget
    photo = Tk_FindPhoto(interp, argv[2]);
    if (!photo)
    {
      Tcl_AppendResult(interp, "could not find photo: ", argv[2], nullptr);
      return TCL_ERROR;
    }

    int components = image->GetNumberOfScalarComponents();
    if (components != 1 && components != 3)
    {
      const char* m = "number of scalar components must be 1, 3, 4";
      Tcl_SetResult(interp, const_cast<char*>(m), TCL_VOLATILE);
      return TCL_ERROR;
    }

    // Determine the orientation
    if (argc >= 5)
    {
      if (strcmp(argv[4], "transverse") == 0)
      {
        orientation = SVTKIMAGEDATATOTKPHOTO_TRANSVERSE;
      }
      if (strcmp(argv[4], "coronal") == 0)
      {
        orientation = SVTKIMAGEDATATOTKPHOTO_CORONAL;
      }
      if (strcmp(argv[4], "sagittal") == 0)
      {
        orientation = SVTKIMAGEDATATOTKPHOTO_SAGITTAL;
      }
    }

    // Get Window/Level
    if (argc >= 6)
    {
      if ((status = Tcl_GetDouble(interp, argv[5], &window)) != TCL_OK)
      {
        return status;
      }
    }
    if (argc >= 7)
    {
      if ((status = Tcl_GetDouble(interp, argv[6], &level)) != TCL_OK)
      {
        return status;
      }
    }

    int extent[6];
    // Pass the check?
    int valid = 1;
    image->GetExtent(extent);
    // Setup the photo data block, this info will be used later to
    // handle the svtk data types and window/level
    // For reference:
    // pitch - address difference between two vertically adjacent pixels
    // pixelSize - address difference between two
    //             horizontally adjacent pixels
    Tk_PhotoImageBlock block;
    block.width = 0;
    block.height = 0;
    block.pixelSize = 0;
    block.pitch = 0;
    void* TempPointer = 0;
    switch (orientation)
    {
      case SVTKIMAGEDATATOTKPHOTO_TRANSVERSE:
      {
        valid = (slice >= extent[4] && slice <= extent[5]);
        if (valid)
        {
          TempPointer = image->GetScalarPointer(0, extent[3], slice);
          block.width = extent[1] - extent[0] + 1;
          block.height = extent[3] - extent[2] + 1;
          block.pixelSize = components;
          block.pitch = -components * (block.width);
        }
        break;
      }
      case SVTKIMAGEDATATOTKPHOTO_SAGITTAL:
      {
        valid = (slice >= extent[0] && slice <= extent[1]);
        if (valid)
        {
          TempPointer = image->GetScalarPointer(slice, extent[3], 0);
          block.width = extent[3] - extent[2] + 1;
          block.height = extent[5] - extent[4] + 1;
          block.pixelSize = -components * (extent[1] - extent[0] + 1);
          block.pitch = components * (extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1);
        }
        break;
      }
      case SVTKIMAGEDATATOTKPHOTO_CORONAL:
      {
        valid = (slice >= extent[2] && slice <= extent[3]);
        if (valid)
        {
          TempPointer = image->GetScalarPointer(0, slice, 0);
          block.width = extent[1] - extent[0] + 1;
          block.height = extent[5] - extent[4] + 1;
          block.pixelSize = components;
          block.pitch = components * (extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1);
        }
        break;
      }
    }

    if (!valid)
    {
      const char* m = "slice is outside the image extent";
      Tcl_SetResult(interp, const_cast<char*>(m), TCL_VOLATILE);
      return TCL_ERROR;
    }

    // Extract the data, and reset the block
    std::vector<unsigned char> photobuffer(block.width * block.height * components);
    double shift, scale;
    shift = window / 2.0 - level;
    scale = 255.0 / window;
    switch (image->GetScalarType())
    {
      svtkTemplateMacro(svtkExtractImageData(photobuffer.data(), static_cast<SVTK_TT*>(TempPointer),
        shift, scale, block.width, block.height, block.pitch, block.pixelSize, components));
    }
    block.pitch = block.width * components;
    block.pixelSize = components;
    block.pixelPtr = photobuffer.data();

    block.offset[0] = 0;
    block.offset[1] = 1;
    block.offset[2] = 2;
    block.offset[3] = 0;
    switch (components)
    {
      case 1:
        block.offset[0] = 0;
        block.offset[1] = 0;
        block.offset[2] = 0;
        block.offset[3] = 0;
        break;
      case 3:
        block.offset[3] = 0;
        break;
      case 4:
        block.offset[3] = 3;
        break;
    }
    Tk_PhotoSetSize(photo, block.width, block.height);
    Tk_PhotoPutBlock(photo, &block, 0, 0, block.width, block.height);
    return TCL_OK;
  }
}

//----------------------------------------------------------------------------
// It's possible to change with this function or in a script some
// options like width, height or the render widget.
int svtkTkRenderWidget_Configure(
  Tcl_Interp* interp, struct svtkTkRenderWidget* self, int argc, char* argv[], int flags)
{
  // Let Tk handle generic configure options.
  if (Tk_ConfigureWidget(interp, self->TkWin, svtkTkRenderWidgetConfigSpecs, argc,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
        const_cast<CONST84 char**>(argv),
#else
        argv,
#endif
        (char*)self, flags) == TCL_ERROR)
  {
    return (TCL_ERROR);
  }

  // Get the new  width and height of the widget
  Tk_GeometryRequest(self->TkWin, self->Width, self->Height);

  // Make sure the render window has been set.  If not, create one.
  if (svtkTkRenderWidget_MakeRenderWindow(self) == TCL_ERROR)
  {
    return TCL_ERROR;
  }

  return TCL_OK;
}

//----------------------------------------------------------------------------
// This function is called when the render widget name is
// evaluated in a Tcl script.  It will compare string parameters
// to choose the appropriate method to invoke.
extern "C"
{
  int svtkTkRenderWidget_Widget(ClientData clientData, Tcl_Interp* interp, int argc,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
    CONST84
#endif
    char* argv[])
  {
    struct svtkTkRenderWidget* self = (struct svtkTkRenderWidget*)clientData;
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
      if (self->RenderWindow == nullptr)
      {
        svtkTkRenderWidget_MakeRenderWindow(self);
      }
      self->RenderWindow->Render();
    }
    // Handle configure method
    else if (!strncmp(argv[1], "configure", SVTK_MAX(1, strlen(argv[1]))))
    {
      if (argc == 2)
      {
        /* Return list of all configuration parameters */
        result = Tk_ConfigureInfo(
          interp, self->TkWin, svtkTkRenderWidgetConfigSpecs, (char*)self, (char*)nullptr, 0);
      }
      else if (argc == 3)
      {
        /* Return a specific configuration parameter */
        result = Tk_ConfigureInfo(
          interp, self->TkWin, svtkTkRenderWidgetConfigSpecs, (char*)self, argv[2], 0);
      }
      else
      {
        /* Execute a configuration change */
        result = svtkTkRenderWidget_Configure(interp, self, argc - 2,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
          const_cast<char**>(argv + 2),
#else
          argv + 2,
#endif
          TK_CONFIG_ARGV_ONLY);
      }
    }
    else if (!strcmp(argv[1], "GetRenderWindow"))
    { // Get RenderWindow is my own method
      // Create a RenderWidget if one has not been set yet.
      result = svtkTkRenderWidget_MakeRenderWindow(self);
      if (result != TCL_ERROR)
      {
        // Return the name (Make Tcl copy the string)
        Tcl_SetResult(interp, self->RW, TCL_VOLATILE);
      }
    }
    else
    {
      // Unknown method name.
      Tcl_AppendResult(interp, "svtkTkRenderWidget: Unknown option: ", argv[1], "\n",
        "Try: configure or GetRenderWindow\n", nullptr);
      result = TCL_ERROR;
    }

    // Unlock the object so it can be deleted.
    Tk_Release((ClientData)self);
    return result;
  }
}

//----------------------------------------------------------------------------
// svtkTkRenderWidget_Cmd
// Called when svtkTkRenderWidget is executed
// - creation of a svtkTkRenderWidget widget.
//     * Creates a new window
//     * Creates an 'svtkTkRenderWidget' data structure
//     * Creates an event handler for this window
//     * Creates a command that handles this object
//     * Configures this svtkTkRenderWidget for the given arguments
extern "C"
{
  int svtkTkRenderWidget_Cmd(ClientData clientData, Tcl_Interp* interp, int argc,
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
    struct svtkTkRenderWidget* self;

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
    Tk_SetClass(tkwin, (char*)"svtkTkRenderWidget");

    // Create svtkTkRenderWidget data structure
    self = (struct svtkTkRenderWidget*)ckalloc(sizeof(struct svtkTkRenderWidget));
    self->TkWin = tkwin;
    self->Interp = interp;
    self->Width = 0;
    self->Height = 0;
    self->RenderWindow = nullptr;
    self->RW = nullptr;

    // ...
    // Create command event handler
    Tcl_CreateCommand(interp, Tk_PathName(tkwin), svtkTkRenderWidget_Widget, (ClientData)self,
      (void (*)(ClientData))nullptr);
    Tk_CreateEventHandler(
      tkwin, ExposureMask | StructureNotifyMask, svtkTkRenderWidget_EventProc, (ClientData)self);

    // Configure svtkTkRenderWidget widget
    if (svtkTkRenderWidget_Configure(interp, self, argc - 2,
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)
          const_cast<char**>(argv + 2),
#else
          argv + 2,
#endif
          0) == TCL_ERROR)
    {
      Tk_DestroyWindow(tkwin);
      Tcl_DeleteCommand(interp, (char*)"svtkTkRenderWidget");
      // Don't free it, if we do a crash occurs later...
      // free(self);
      return TCL_ERROR;
    }

    Tcl_AppendResult(interp, Tk_PathName(tkwin), nullptr);
    return TCL_OK;
  }
}

//----------------------------------------------------------------------------
const char* svtkTkRenderWidget_RW(const struct svtkTkRenderWidget* self)
{
  return self->RW;
}

//----------------------------------------------------------------------------
int svtkTkRenderWidget_Width(const struct svtkTkRenderWidget* self)
{
  return self->Width;
}

//----------------------------------------------------------------------------
int svtkTkRenderWidget_Height(const struct svtkTkRenderWidget* self)
{
  return self->Height;
}

/*
 *----------------------------------------------------------------------
 *
 * svtkTkRenderWidget_Destroy ---
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *      to clean up the internal structure of a canvas at a safe time
 *      (when no-one is using it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the canvas is freed up.
 *
 *----------------------------------------------------------------------
 */

extern "C"
{
  void svtkTkRenderWidget_Destroy(char* memPtr)
  {
    struct svtkTkRenderWidget* self = (struct svtkTkRenderWidget*)memPtr;

    if (self->RenderWindow)
    {
      if (self->RenderWindow->GetInteractor() &&
        self->RenderWindow->GetInteractor()->GetRenderWindow() == self->RenderWindow)
      {
        self->RenderWindow->GetInteractor()->SetRenderWindow(0);
      }
      if (self->RenderWindow->GetReferenceCount() > 1)
      {
        svtkGenericWarningMacro(
          "A TkRenderWidget is being destroyed before it associated svtkRenderWindow is destroyed."
          "This is very bad and usually due to the order in which objects are being destroyed."
          "Always destroy the svtkRenderWindow before destroying the user interface components.");
      }
      self->RenderWindow->UnRegister(nullptr);
      self->RenderWindow = nullptr;
    }
    ckfree(self->RW);
    ckfree(memPtr);
  }
}

//----------------------------------------------------------------------------
// This gets called to handle svtkTkRenderWidget window configuration events
// Possibly X dependent
extern "C"
{
  void svtkTkRenderWidget_EventProc(ClientData clientData, XEvent* eventPtr)
  {
    struct svtkTkRenderWidget* self = (struct svtkTkRenderWidget*)clientData;

    switch (eventPtr->type)
    {
      case Expose:
        if (eventPtr->xexpose.count == 0)
        /* && !self->UpdatePending)*/
        {
          // let the user bind expose events
          // self->RenderWindow->Render();
        }
        break;
      case ConfigureNotify:
        // if ( Tk_IsMapped(self->TkWin) )
        {
          self->Width = Tk_Width(self->TkWin);
          self->Height = Tk_Height(self->TkWin);
          // Tk_GeometryRequest(self->TkWin,self->Width,self->Height);
          if (self->RenderWindow)
          {
#if defined(SVTK_USE_COCOA)
            // Do not call SetSize or SetPosition until we're mapped.
            if (Tk_IsMapped(self->TkWin))
            {
              // On Cocoa, compute coordinates relative to toplevel
              int x = Tk_X(self->TkWin);
              int y = Tk_Y(self->TkWin);
              for (TkWindow* curPtr = ((TkWindow*)self->TkWin)->parentPtr;
                   (nullptr != curPtr) && !(curPtr->flags & TK_TOP_LEVEL);
                   curPtr = curPtr->parentPtr)
              {
                x += Tk_X(curPtr);
                y += Tk_Y(curPtr);
              }
              self->RenderWindow->SetPosition(x, y);
              self->RenderWindow->SetSize(self->Width, self->Height);
            }
#else
            self->RenderWindow->SetPosition(Tk_X(self->TkWin), Tk_Y(self->TkWin));
            self->RenderWindow->SetSize(self->Width, self->Height);
#endif
          }
          // svtkTkRenderWidget_PostRedisplay(self);
        }
        break;
      case MapNotify:
      {
#if defined(SVTK_USE_COCOA)
        // On Cocoa, compute coordinates relative to the toplevel
        int x = Tk_X(self->TkWin);
        int y = Tk_Y(self->TkWin);
        for (TkWindow* curPtr = ((TkWindow*)self->TkWin)->parentPtr;
             (nullptr != curPtr) && !(curPtr->flags & TK_TOP_LEVEL); curPtr = curPtr->parentPtr)
        {
          x += Tk_X(curPtr);
          y += Tk_Y(curPtr);
        }
        self->RenderWindow->SetPosition(x, y);
        self->RenderWindow->SetSize(self->Width, self->Height);
#endif
        break;
      }
#if defined(SVTK_USE_COCOA)
      case UnmapNotify:
      {
        break;
      }
#endif
      case DestroyNotify:
        Tcl_EventuallyFree((ClientData)self, svtkTkRenderWidget_Destroy);
        break;
      default
        :
        // nothing
        ;
    }
  }
}

//----------------------------------------------------------------------------
// svtkTkRenderWidget_Init
// Called upon system startup to create svtkTkRenderWidget command.
extern "C"
{
  SVTK_EXPORT int Vtktkrenderwidget_Init(Tcl_Interp* interp);
}

#define SVTKTK_TO_STRING(x) SVTKTK_TO_STRING0(x)
#define SVTKTK_TO_STRING0(x) SVTKTK_TO_STRING1(x)
#define SVTKTK_TO_STRING1(x) #x
#define SVTKTK_VERSION SVTKTK_TO_STRING(SVTK_MAJOR_VERSION) "." SVTKTK_TO_STRING(SVTK_MINOR_VERSION)

int SVTK_EXPORT Vtktkrenderwidget_Init(Tcl_Interp* interp)
{
  // This widget requires Tk to function.
  Tcl_PkgRequire(interp, (char*)"Tk", (char*)TK_VERSION, 0);
  if (Tcl_PkgPresent(interp, (char*)"Tk", (char*)TK_VERSION, 0))
  {
    // Register the commands for this package.
    Tcl_CreateCommand(
      interp, (char*)"svtkTkRenderWidget", svtkTkRenderWidget_Cmd, Tk_MainWindow(interp), nullptr);
    Tcl_CreateCommand(
      interp, (char*)"svtkImageDataToTkPhoto", svtkImageDataToTkPhoto_Cmd, nullptr, nullptr);

    // Report that the package is provided.
    return Tcl_PkgProvide(interp, (char*)"Vtktkrenderwidget", (char*)SVTKTK_VERSION);
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

LRESULT APIENTRY svtkTkRenderWidgetProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT rval;
  struct svtkTkRenderWidget* self =
    (struct svtkTkRenderWidget*)svtkGetWindowLong(hWnd, sizeof(svtkLONG));

  if (!self)
  {
    return 1;
  }

  // watch for WM_USER + 12  this is a special message
  // from the svtkRenderWindowInteractor letting us
  // know it wants to get events also
  if ((message == WM_USER + 12) && (wParam == 24))
  {
    WNDPROC tmp = (WNDPROC)lParam;
    // we need to tell it what the original svtk event handler was
    svtkSetWindowLong(hWnd, sizeof(svtkLONG), (svtkLONG)self->RenderWindow);
    tmp(hWnd, WM_USER + 13, 26, (svtkLONG)self->OldProc);
    svtkSetWindowLong(hWnd, sizeof(svtkLONG), (svtkLONG)self);
    self->OldProc = tmp;
    return 1;
  }
  if ((message == WM_USER + 14) && (wParam == 28))
  {
    WNDPROC tmp = (WNDPROC)lParam;
    self->OldProc = tmp;
    return 1;
  }

  if (!self->TkWin)
  {
    return 1;
  }

  // forward message to Tk handler
  svtkSetWindowLong(hWnd, sizeof(svtkLONG), (svtkLONG)((TkWindow*)self->TkWin)->window);
  if (((TkWindow*)self->TkWin)->parentPtr)
  {
    svtkSetWindowLong(hWnd, svtkGWL_WNDPROC, (svtkLONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd, message, wParam, lParam);
  }
  else
  {
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
    if (self->RenderWindow)
    {
      svtkSetWindowLong(hWnd, sizeof(svtkLONG), (svtkLONG)self->RenderWindow);
      svtkSetWindowLong(hWnd, svtkGWL_WNDPROC, (svtkLONG)self->OldProc);
      CallWindowProc(self->OldProc, hWnd, message, wParam, lParam);
    }
  }

  // now reset to the original config
  svtkSetWindowLong(hWnd, sizeof(svtkLONG), (svtkLONG)self);
  svtkSetWindowLong(hWnd, svtkGWL_WNDPROC, (svtkLONG)svtkTkRenderWidgetProc);
  return rval;
}

//-----------------------------------------------------------------------------
// Creates a render window and forces Tk to use the window.
static int svtkTkRenderWidget_MakeRenderWindow(struct svtkTkRenderWidget* self)
{
  Display* dpy;
  TkWindow* winPtr = (TkWindow*)self->TkWin;
  Tcl_HashEntry* hPtr;
  int new_flag;
  svtkWin32OpenGLRenderWindow* renderWindow;
  TkWinDrawable* twdPtr;
  HWND parentWin;

  if (self->RenderWindow)
  {
    return TCL_OK;
  }

  dpy = Tk_Display(self->TkWin);

  if (winPtr->window != None)
  {
    // XDestroyWindow(dpy, winPtr->window);
  }

  if (self->RW[0] == '\0')
  {
    // Make the Render window.
    self->RenderWindow = svtkRenderWindow::New();
    self->RenderWindow->Register(nullptr);
    self->RenderWindow->Delete();
    renderWindow = (svtkWin32OpenGLRenderWindow*)(self->RenderWindow);
#ifndef SVTK_PYTHON_BUILD
    svtkTclGetObjectFromPointer(self->Interp, self->RenderWindow, "svtkRenderWindow");
#endif
    // in Tcl 8.6.x, ckalloc was changed to return "void *".
    self->RW = static_cast<char*>(
      ckalloc(static_cast<unsigned int>(strlen(Tcl_GetStringResult(self->Interp)) + 1)));
    strcpy(self->RW, Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is RW an address ? big ole python hack here
    if (self->RW[0] == 'A' && self->RW[1] == 'd' && self->RW[2] == 'd' && self->RW[3] == 'r')
    {
      void* tmp;
      sscanf(self->RW + 5, "%p", &tmp);
      renderWindow = (svtkWin32OpenGLRenderWindow*)tmp;
    }
    else
    {
#ifndef SVTK_PYTHON_BUILD
      renderWindow = (svtkWin32OpenGLRenderWindow*)svtkTclGetPointerFromObject(
        self->RW, "svtkRenderWindow", self->Interp, new_flag);
#else
      renderWindow = 0;
#endif
    }
    if (renderWindow != self->RenderWindow)
    {
      if (self->RenderWindow != nullptr)
      {
        self->RenderWindow->UnRegister(nullptr);
      }
      self->RenderWindow = (svtkRenderWindow*)(renderWindow);
      if (self->RenderWindow != nullptr)
      {
        self->RenderWindow->Register(nullptr);
      }
    }
  }

  // Set the size
  self->RenderWindow->SetSize(self->Width, self->Height);

  // Set the parent correctly
  // Possibly X dependent
  if ((winPtr->parentPtr != nullptr) && !(winPtr->flags & TK_TOP_LEVEL))
  {
    if (winPtr->parentPtr->window == None)
    {
      Tk_MakeWindowExist((Tk_Window)winPtr->parentPtr);
    }

    parentWin = ((TkWinDrawable*)winPtr->parentPtr->window)->window.handle;
    renderWindow->SetParentId(parentWin);
  }

  // Use the same display
  self->RenderWindow->SetDisplayId(dpy);

  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  // Tk_SetWindowVisual(self->TkWin, renderWindow->GetDesiredVisual(),
  // renderWindow->GetDesiredDepth(),
  // renderWindow->GetDesiredColormap());

  self->RenderWindow->Render();

#if (TK_MAJOR_VERSION >= 8)
  twdPtr = (TkWinDrawable*)Tk_AttachHWND(self->TkWin, renderWindow->GetWindowId());
#else
  twdPtr = (TkWinDrawable*)ckalloc(sizeof(TkWinDrawable));
  twdPtr->type = TWD_WINDOW;
  twdPtr->window.winPtr = winPtr;
  twdPtr->window.handle = renderWindow->GetWindowId();
#endif

  self->OldProc = (WNDPROC)svtkGetWindowLong(twdPtr->window.handle, svtkGWL_WNDPROC);
  svtkSetWindowLong(twdPtr->window.handle, sizeof(svtkLONG), (svtkLONG)self);
  svtkSetWindowLong(twdPtr->window.handle, svtkGWL_WNDPROC, (svtkLONG)svtkTkRenderWidgetProc);

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
#else

// the cocoa version
#if defined(SVTK_USE_COCOA)
//----------------------------------------------------------------------------
// Creates a render window and forces Tk to use the window.
static int svtkTkRenderWidget_MakeRenderWindow(struct svtkTkRenderWidget* self)
{
  svtkRenderWindow* renderWindow = nullptr;

  if (self->RenderWindow)
  {
    return TCL_OK;
  }

  if (self->RW[0] == '\0')
  {
    // Make the Render window.
    self->RenderWindow = svtkRenderWindow::New();
    self->RenderWindow->Register(nullptr);
    self->RenderWindow->Delete();
    renderWindow = self->RenderWindow;
#ifndef SVTK_PYTHON_BUILD
    svtkTclGetObjectFromPointer(self->Interp, self->RenderWindow, "svtkRenderWindow");
#endif
    // in Tcl 8.6.x, ckalloc was changed to return "void *".
    self->RW = static_cast<char*>(
      ckalloc(static_cast<unsigned int>(strlen(Tcl_GetStringResult(self->Interp)) + 1)));
    strcpy(self->RW, Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is RW an address ? big ole python hack here
    if (self->RW[0] == 'A' && self->RW[1] == 'd' && self->RW[2] == 'd' && self->RW[3] == 'r')
    {
      void* tmp;
      sscanf(self->RW + 5, "%p", &tmp);
      renderWindow = reinterpret_cast<svtkRenderWindow*>(tmp);
    }
    else
    {
#ifndef SVTK_PYTHON_BUILD
      int new_flag;
      renderWindow = static_cast<svtkRenderWindow*>(
        svtkTclGetPointerFromObject(self->RW, "svtkRenderWindow", self->Interp, new_flag));
#endif
    }

    if (renderWindow != self->RenderWindow)
    {
      if (self->RenderWindow != nullptr)
      {
        self->RenderWindow->UnRegister(nullptr);
      }
      self->RenderWindow = renderWindow;
      if (self->RenderWindow != nullptr)
      {
        self->RenderWindow->Register(nullptr);
      }
    }
  }

  TkWindow* winPtr = reinterpret_cast<TkWindow*>(self->TkWin);

  Tk_MakeWindowExist(self->TkWin);
  // set the ParentId to the NSView of the Tk toplevel
  renderWindow->SetParentId(svtkCocoaTkUtilities::GetDrawableView(self->TkWin));
  renderWindow->SetSize(self->Width, self->Height);

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

  // Process all outstanding events so that Tk is fully updated
  Tcl_ServiceAll();

  self->RenderWindow->Render();

  return TCL_OK;
}

// now the Xwindows version
#else

//----------------------------------------------------------------------------
// Creates a render window and forces Tk to use the window.
static int svtkTkRenderWidget_MakeRenderWindow(struct svtkTkRenderWidget* self)
{
  Display* dpy;
  svtkXOpenGLRenderWindow* renderWindow = 0;

  if (self->RenderWindow)
  {
    return TCL_OK;
  }

  dpy = Tk_Display(self->TkWin);

  if (Tk_WindowId(self->TkWin) != None)
  {
    XDestroyWindow(dpy, Tk_WindowId(self->TkWin));
  }

  if (self->RW[0] == '\0')
  {
    // Make the Render window.
    self->RenderWindow = svtkRenderWindow::New();
    self->RenderWindow->Register(nullptr);
    self->RenderWindow->Delete();
    renderWindow = (svtkXOpenGLRenderWindow*)(self->RenderWindow);
#ifndef SVTK_PYTHON_BUILD
    svtkTclGetObjectFromPointer(self->Interp, self->RenderWindow, "svtkRenderWindow");
#endif
    // in Tcl 8.6.x, ckalloc was changed to return "void *".
    self->RW = static_cast<char*>(
      ckalloc(static_cast<unsigned int>(strlen(Tcl_GetStringResult(self->Interp)) + 1)));
    strcpy(self->RW, Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is RW an address ? big ole python hack here
    if (self->RW[0] == 'A' && self->RW[1] == 'd' && self->RW[2] == 'd' && self->RW[3] == 'r')
    {
      void* tmp;
      sscanf(self->RW + 5, "%p", &tmp);
      renderWindow = (svtkXOpenGLRenderWindow*)tmp;
    }
    else
    {
#ifndef SVTK_PYTHON_BUILD
      int new_flag;
      renderWindow = (svtkXOpenGLRenderWindow*)svtkTclGetPointerFromObject(
        self->RW, "svtkRenderWindow", self->Interp, new_flag);
#endif
    }
    if (renderWindow != self->RenderWindow)
    {
      if (self->RenderWindow != nullptr)
      {
        self->RenderWindow->UnRegister(nullptr);
      }
      self->RenderWindow = (svtkRenderWindow*)(renderWindow);
      if (self->RenderWindow != nullptr)
      {
        self->RenderWindow->Register(nullptr);
      }
    }
  }

  // If window already exists, return an error
  if (renderWindow->GetWindowId() != (Window)nullptr)
  {
    return TCL_ERROR;
  }

  // Use the same display
  renderWindow->SetDisplayId(dpy);

  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  // The visual MUST BE SET BEFORE the window is created.
  Tk_SetWindowVisual(self->TkWin, renderWindow->GetDesiredVisual(), renderWindow->GetDesiredDepth(),
    renderWindow->GetDesiredColormap());

  // Make this window exist, then use that information to make the svtkImageViewer in sync
  Tk_MakeWindowExist(self->TkWin);
  renderWindow->SetWindowId((void*)Tk_WindowId(self->TkWin));

  // Set the size
  self->RenderWindow->SetSize(self->Width, self->Height);

  // Set the parent correctly
  // Possibly X dependent
  if ((Tk_Parent(self->TkWin) == nullptr) || (Tk_IsTopLevel(self->TkWin)))
  {
    renderWindow->SetParentId(XRootWindow(Tk_Display(self->TkWin), Tk_ScreenNumber(self->TkWin)));
  }
  else
  {
    renderWindow->SetParentId(Tk_WindowId(Tk_Parent(self->TkWin)));
  }

  self->RenderWindow->Render();
  XSelectInput(dpy, Tk_WindowId(self->TkWin), SVTK_ALL_EVENTS_MASK);

  return TCL_OK;
}
#endif

#endif
