/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkWin32OpenGL2RenderWindow.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWin32OpenGL2RenderWindow
 * @brief   OpenGL rendering window
 *
 * svtkWin32OpenGL2RenderWindow is a concrete implementation of the abstract
 * class svtkRenderWindow. svtkWin32OpenGL2Renderer interfaces to the standard
 * OpenGL graphics library in the Windows/NT environment..
 */

#ifndef svtkWin32OpenGLRenderWindow_h
#define svtkWin32OpenGLRenderWindow_h

#include "svtkOpenGLRenderWindow.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include <stack>                       // for ivar

#include "svtkWindows.h" // For windows API

class svtkIdList;

class SVTKRENDERINGOPENGL2_EXPORT svtkWin32OpenGLRenderWindow : public svtkOpenGLRenderWindow
{
public:
  static svtkWin32OpenGLRenderWindow* New();
  svtkTypeMacro(svtkWin32OpenGLRenderWindow, svtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * End the rendering process and display the image.
   */
  void Frame(void) override;

  /**
   * Create the window
   */
  virtual void WindowInitialize(void);

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  void Initialize(void) override;

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize(void) override;

  /**
   * Change the window to fill the entire screen.
   */
  void SetFullScreen(svtkTypeBool) override;

  /**
   * Remap the window.
   */
  void WindowRemap(void) override;

  /**
   * Show or not Show the window
   */
  void SetShowWindow(bool val) override;

  /**
   * Set the preferred window size to full screen.
   */
  virtual void PrefFullScreen(void);

  //@{
  /**
   * Set the size (width and height) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   *
   * If the size has changed, this method will fire
   * svtkCommand::WindowResizeEvent.
   */
  void SetSize(int width, int height) override;
  void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }
  //@}

  /**
   * Get the size (width and height) of the rendering window in
   * screen coordinates (in pixels).
   */
  int* GetSize() SVTK_SIZEHINT(2) override;

  //@{
  /**
   * Set the position (x and y) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   */
  void SetPosition(int x, int y) override;
  void SetPosition(int a[2]) override { this->SetPosition(a[0], a[1]); }
  //@}

  /**
   * Get the current size of the screen in pixels.
   * An HDTV for example would be 1920 x 1080 pixels.
   */
  int* GetScreenSize() SVTK_SIZEHINT(2) override;

  /**
   * Get the position (x and y) of the rendering window in
   * screen coordinates (in pixels).
   */
  int* GetPosition() SVTK_SIZEHINT(2) override;

  /**
   * Set the name of the window. This appears at the top of the window
   * normally.
   */
  void SetWindowName(const char*) override;

  /**
   * Set this RenderWindow's window id to a pre-existing window.
   */
  void SetWindowInfo(const char*) override;

  /**
   * Sets the WindowInfo that will be used after a WindowRemap.
   */
  void SetNextWindowInfo(const char*) override;

  /**
   * Sets the HWND id of the window that WILL BE created.
   */
  void SetParentInfo(const char*) override;

  void* GetGenericDisplayId() override { return (void*)this->ContextId; }
  void* GetGenericWindowId() override { return (void*)this->WindowId; }
  void* GetGenericParentId() override { return (void*)this->ParentId; }
  void* GetGenericContext() override { return (void*)this->DeviceContext; }
  void* GetGenericDrawable() override { return (void*)this->WindowId; }
  void SetDisplayId(void*) override;

  /**
   * Get the window id.
   */
  HWND GetWindowId();

  //@{
  /**
   * Set the window id to a pre-existing window.
   */
  void SetWindowId(HWND);
  void SetWindowId(void* foo) override { this->SetWindowId((HWND)foo); }
  //@}

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() override;

  /**
   * Does this platform support render window data sharing.
   */
  bool GetPlatformSupportsRenderWindowSharing() override { return true; }

  //@{
  /**
   * Set the window's parent id to a pre-existing window.
   */
  void SetParentId(HWND);
  void SetParentId(void* foo) override { this->SetParentId((HWND)foo); }
  //@}

  void SetContextId(HGLRC);   // hsr
  void SetDeviceContext(HDC); // hsr

  /**
   * Set the window id of the new window once a WindowRemap is done.
   */
  void SetNextWindowId(HWND);

  /**
   * Set the window id of the new window once a WindowRemap is done.
   * This is the generic prototype as required by the svtkRenderWindow
   * parent.
   */
  void SetNextWindowId(void* arg) override;

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  void SetStereoCapableWindow(svtkTypeBool capable) override;

  /**
   * Make this windows OpenGL context the current context.
   */
  void MakeCurrent() override;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override;

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  svtkTypeBool IsDirect() override;

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * This is a useful check to abort a long render.
   */
  svtkTypeBool GetEventPending() override;

  //@{
  /**
   * Initialize OpenGL for this window.
   */
  virtual void SetupPalette(HDC hDC);
  virtual void SetupPixelFormatPaletteAndContext(
    HDC hDC, DWORD dwFlags, int debug, int bpp = 16, int zbpp = 16);
  //@}

  /**
   * Clean up device contexts, rendering contexts, etc.
   */
  void Clean();

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want SVTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  void HideCursor() override;
  void ShowCursor() override;
  void SetCursorPosition(int x, int y) override;
  //@}

  /**
   * Change the shape of the cursor
   */
  void SetCurrentCursor(int) override;

  bool DetectDPI() override;

  //@{
  /**
   * Ability to push and pop this window's context
   * as the current context. The idea being to
   * if needed make this window's context current
   * and when done releasing resources restore
   * the prior context
   */
  void PushContext() override;
  void PopContext() override;
  //@}

  /**
   * Set the number of vertical syncs required between frames.
   * A value of 0 means swap buffers as quickly as possible
   * regardless of the vertical refresh. A value of 1 means swap
   * buffers in sync with the vertical refresh to eliminate tearing.
   * A value of -1 means use a value of 1 unless we missed a frame
   * in which case swap immediately. Returns true if the call
   * succeeded.
   */
  bool SetSwapControl(int i) override;

protected:
  svtkWin32OpenGLRenderWindow();
  ~svtkWin32OpenGLRenderWindow() override;

  HINSTANCE ApplicationInstance;
  HPALETTE Palette;
  HPALETTE OldPalette;
  HGLRC ContextId;
  HDC DeviceContext;
  BOOL MFChandledWindow;
  HWND WindowId;
  HWND ParentId;
  HWND NextWindowId;
  svtkTypeBool OwnWindow;

  std::stack<HGLRC> ContextStack;
  std::stack<HDC> DCStack;

  // message handler
  virtual LRESULT MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  static LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  svtkTypeBool CursorHidden;
  svtkTypeBool ForceMakeCurrent;

  int WindowIdReferenceCount;
  void ResizeWhileOffscreen(int xsize, int ysize);
  void CreateAWindow() override;
  void DestroyWindow() override;
  void InitializeApplication();
  void CleanUpRenderers();
  void SVTKRegisterClass();

private:
  svtkWin32OpenGLRenderWindow(const svtkWin32OpenGLRenderWindow&) = delete;
  void operator=(const svtkWin32OpenGLRenderWindow&) = delete;
};

#endif
