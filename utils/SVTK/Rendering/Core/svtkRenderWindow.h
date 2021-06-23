/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderWindow
 * @brief   create a window for renderers to draw into
 *
 * svtkRenderWindow is an abstract object to specify the behavior of a
 * rendering window. A rendering window is a window in a graphical user
 * interface where renderers draw their images. Methods are provided to
 * synchronize the rendering process, set window size, and control double
 * buffering.  The window also allows rendering in stereo.  The interlaced
 * render stereo type is for output to a VRex stereo projector.  All of the
 * odd horizontal lines are from the left eye, and the even lines are from
 * the right eye.  The user has to make the render window aligned with the
 * VRex projector, or the eye will be swapped.
 *
 * @warning
 * In SVTK versions 4 and later, the svtkWindowToImageFilter class is
 * part of the canonical way to output an image of a window to a file
 * (replacing the obsolete SaveImageAsPPM method for svtkRenderWindows
 * that existed in 3.2 and earlier).  Connect one of these filters to
 * the output of the window, and filter's output to a writer such as
 * svtkPNGWriter.
 *
 * @sa
 * svtkRenderer svtkRenderWindowInteractor svtkWindowToImageFilter
 */

#ifndef svtkRenderWindow_h
#define svtkRenderWindow_h

#include "svtkNew.h"                 // For svtkNew
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkSmartPointer.h"        // For svtkSmartPointer
#include "svtkWindow.h"

class svtkFloatArray;
class svtkProp;
class svtkCollection;
class svtkRenderTimerLog;
class svtkRenderWindowInteractor;
class svtkRenderer;
class svtkRendererCollection;
class svtkStereoCompositor;
class svtkUnsignedCharArray;

// lets define the different types of stereo
#define SVTK_STEREO_CRYSTAL_EYES 1
#define SVTK_STEREO_RED_BLUE 2
#define SVTK_STEREO_INTERLACED 3
#define SVTK_STEREO_LEFT 4
#define SVTK_STEREO_RIGHT 5
#define SVTK_STEREO_DRESDEN 6
#define SVTK_STEREO_ANAGLYPH 7
#define SVTK_STEREO_CHECKERBOARD 8
#define SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL 9
#define SVTK_STEREO_FAKE 10
#define SVTK_STEREO_EMULATE 11

#define SVTK_CURSOR_DEFAULT 0
#define SVTK_CURSOR_ARROW 1
#define SVTK_CURSOR_SIZENE 2
#define SVTK_CURSOR_SIZENW 3
#define SVTK_CURSOR_SIZESW 4
#define SVTK_CURSOR_SIZESE 5
#define SVTK_CURSOR_SIZENS 6
#define SVTK_CURSOR_SIZEWE 7
#define SVTK_CURSOR_SIZEALL 8
#define SVTK_CURSOR_HAND 9
#define SVTK_CURSOR_CROSSHAIR 10

class SVTKRENDERINGCORE_EXPORT svtkRenderWindow : public svtkWindow
{
public:
  svtkTypeMacro(svtkRenderWindow, svtkWindow);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct an instance of  svtkRenderWindow with its screen size
   * set to 300x300, borders turned on, positioned at (0,0), double
   * buffering turned on.
   */
  static svtkRenderWindow* New();

  /**
   * Add a renderer to the list of renderers.
   */
  virtual void AddRenderer(svtkRenderer*);

  /**
   * Remove a renderer from the list of renderers.
   */
  void RemoveRenderer(svtkRenderer*);

  /**
   * Query if a renderer is in the list of renderers.
   */
  int HasRenderer(svtkRenderer*);

  /**
   * What rendering library has the user requested
   */
  static const char* GetRenderLibrary();

  /**
   * What rendering backend has the user requested
   */
  virtual const char* GetRenderingBackend();

  /**
   * Get the render timer log for this window.
   */
  svtkGetNewMacro(RenderTimer, svtkRenderTimerLog);

  /**
   * Return the collection of renderers in the render window.
   */
  svtkRendererCollection* GetRenderers() { return this->Renderers; }

  /**
   * The GL2PS exporter must handle certain props in a special way (e.g. text).
   * This method performs a render and captures all "GL2PS-special" props in
   * the specified collection. The collection will contain a
   * svtkPropCollection for each svtkRenderer in this->GetRenderers(), each
   * containing the special props rendered by the corresponding renderer.
   */
  void CaptureGL2PSSpecialProps(svtkCollection* specialProps);

  //@{
  /**
   * Returns true if the render process is capturing text actors.
   */
  svtkGetMacro(CapturingGL2PSSpecialProps, int);
  //@}

  /**
   * Ask each renderer owned by this RenderWindow to render its image and
   * synchronize this process.
   */
  void Render() override;

  /**
   * Start the rendering process for a frame
   */
  virtual void Start() {}

  /**
   * Update the system, if needed, at end of render process
   */
  virtual void End(){};

  /**
   * Finalize the rendering process.
   */
  virtual void Finalize() {}

  /**
   * A termination method performed at the end of the rendering process
   * to do things like swapping buffers (if necessary) or similar actions.
   */
  virtual void Frame() {}

  /**
   * Block the thread until the actual rendering is finished().
   * Useful for measurement only.
   */
  virtual void WaitForCompletion() {}

  /**
   * Performed at the end of the rendering process to generate image.
   * This is typically done right before swapping buffers.
   */
  virtual void CopyResultFrame();

  /**
   * Create an interactor to control renderers in this window. We need
   * to know what type of interactor to create, because we might be in
   * X Windows or MS Windows.
   */
  virtual svtkRenderWindowInteractor* MakeRenderWindowInteractor();

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want SVTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  virtual void HideCursor() {}
  virtual void ShowCursor() {}
  virtual void SetCursorPosition(int, int) {}
  //@}

  //@{
  /**
   * Change the shape of the cursor.
   */
  svtkSetMacro(CurrentCursor, int);
  svtkGetMacro(CurrentCursor, int);
  //@}

  //@{
  /**
   * Turn on/off rendering full screen window size.
   */
  virtual void SetFullScreen(svtkTypeBool) {}
  svtkGetMacro(FullScreen, svtkTypeBool);
  svtkBooleanMacro(FullScreen, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off window manager borders. Typically, you shouldn't turn the
   * borders off, because that bypasses the window manager and can cause
   * undesirable behavior.
   */
  svtkSetMacro(Borders, svtkTypeBool);
  svtkGetMacro(Borders, svtkTypeBool);
  svtkBooleanMacro(Borders, svtkTypeBool);
  //@}

  //@{
  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. Default is off.
   */
  svtkGetMacro(StereoCapableWindow, svtkTypeBool);
  svtkBooleanMacro(StereoCapableWindow, svtkTypeBool);
  virtual void SetStereoCapableWindow(svtkTypeBool capable);
  //@}

  //@{
  /**
   * Turn on/off stereo rendering.
   */
  svtkGetMacro(StereoRender, svtkTypeBool);
  void SetStereoRender(svtkTypeBool stereo);
  svtkBooleanMacro(StereoRender, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the use of alpha bitplanes.
   */
  svtkSetMacro(AlphaBitPlanes, svtkTypeBool);
  svtkGetMacro(AlphaBitPlanes, svtkTypeBool);
  svtkBooleanMacro(AlphaBitPlanes, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off point smoothing. Default is off.
   * This must be applied before the first Render.
   */
  svtkSetMacro(PointSmoothing, svtkTypeBool);
  svtkGetMacro(PointSmoothing, svtkTypeBool);
  svtkBooleanMacro(PointSmoothing, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off line smoothing. Default is off.
   * This must be applied before the first Render.
   */
  svtkSetMacro(LineSmoothing, svtkTypeBool);
  svtkGetMacro(LineSmoothing, svtkTypeBool);
  svtkBooleanMacro(LineSmoothing, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off polygon smoothing. Default is off.
   * This must be applied before the first Render.
   */
  svtkSetMacro(PolygonSmoothing, svtkTypeBool);
  svtkGetMacro(PolygonSmoothing, svtkTypeBool);
  svtkBooleanMacro(PolygonSmoothing, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get what type of stereo rendering to use.  CrystalEyes
   * mode uses frame-sequential capabilities available in OpenGL
   * to drive LCD shutter glasses and stereo projectors.  RedBlue
   * mode is a simple type of stereo for use with red-blue glasses.
   * Anaglyph mode is a superset of RedBlue mode, but the color
   * output channels can be configured using the AnaglyphColorMask
   * and the color of the original image can be (somewhat) maintained
   * using AnaglyphColorSaturation;  the default colors for Anaglyph
   * mode is red-cyan.  Interlaced stereo mode produces a composite
   * image where horizontal lines alternate between left and right
   * views.  StereoLeft and StereoRight modes choose one or the other
   * stereo view.  Dresden mode is yet another stereoscopic
   * interleaving. Fake simply causes the window to render twice without
   * actually swapping the camera from left eye to right eye. This is useful in
   * certain applications that want to emulate the rendering passes without
   * actually rendering in stereo mode. Emulate is similar to Fake, except that
   * it does render left and right eye. There is no compositing of the resulting
   * images from the two eyes at the end of each render in this mode, hence the
   * result onscreen will be the right eye.
   */
  svtkGetMacro(StereoType, int);
  void SetStereoType(int);
  void SetStereoTypeToCrystalEyes() { this->SetStereoType(SVTK_STEREO_CRYSTAL_EYES); }
  void SetStereoTypeToRedBlue() { this->SetStereoType(SVTK_STEREO_RED_BLUE); }
  void SetStereoTypeToInterlaced() { this->SetStereoType(SVTK_STEREO_INTERLACED); }
  void SetStereoTypeToLeft() { this->SetStereoType(SVTK_STEREO_LEFT); }
  void SetStereoTypeToRight() { this->SetStereoType(SVTK_STEREO_RIGHT); }
  void SetStereoTypeToDresden() { this->SetStereoType(SVTK_STEREO_DRESDEN); }
  void SetStereoTypeToAnaglyph() { this->SetStereoType(SVTK_STEREO_ANAGLYPH); }
  void SetStereoTypeToCheckerboard() { this->SetStereoType(SVTK_STEREO_CHECKERBOARD); }
  void SetStereoTypeToSplitViewportHorizontal()
  {
    this->SetStereoType(SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL);
  }
  void SetStereoTypeToFake() { this->SetStereoType(SVTK_STEREO_FAKE); }
  void SetStereoTypeToEmulate() { this->SetStereoType(SVTK_STEREO_EMULATE); }
  //@}

  //@{
  /**
   * Returns the stereo type as a string.
   */
  const char* GetStereoTypeAsString();
  static const char* GetStereoTypeAsString(int type);
  //@}

  /**
   * Update the system, if needed, due to stereo rendering. For some stereo
   * methods, subclasses might need to switch some hardware settings here.
   */
  virtual void StereoUpdate();

  /**
   * Intermediate method performs operations required between the rendering
   * of the left and right eye.
   */
  virtual void StereoMidpoint();

  /**
   * Handles work required once both views have been rendered when using
   * stereo rendering.
   */
  virtual void StereoRenderComplete();

  //@{
  /**
   * Set/get the anaglyph color saturation factor.  This number ranges from
   * 0.0 to 1.0:  0.0 means that no color from the original object is
   * maintained, 1.0 means all of the color is maintained.  The default
   * value is 0.65.  Too much saturation can produce uncomfortable 3D
   * viewing because anaglyphs also use color to encode 3D.
   */
  svtkSetClampMacro(AnaglyphColorSaturation, float, 0.0f, 1.0f);
  svtkGetMacro(AnaglyphColorSaturation, float);
  //@}

  //@{
  /**
   * Set/get the anaglyph color mask values.  These two numbers are bits
   * mask that control which color channels of the original stereo
   * images are used to produce the final anaglyph image.  The first
   * value is the color mask for the left view, the second the mask
   * for the right view.  If a bit in the mask is on for a particular
   * color for a view, that color is passed on to the final view; if
   * it is not set, that channel for that view is ignored.
   * The bits are arranged as r, g, and b, so r = 4, g = 2, and b = 1.
   * By default, the first value (the left view) is set to 4, and the
   * second value is set to 3.  That means that the red output channel
   * comes from the left view, and the green and blue values come from
   * the right view.
   */
  svtkSetVector2Macro(AnaglyphColorMask, int);
  svtkGetVectorMacro(AnaglyphColorMask, int, 2);
  //@}

  /**
   * Remap the rendering window. This probably only works on UNIX right now.
   * It is useful for changing properties that can't normally be changed
   * once the window is up.
   */
  virtual void WindowRemap() {}

  //@{
  /**
   * Turn on/off buffer swapping between images.
   */
  svtkSetMacro(SwapBuffers, svtkTypeBool);
  svtkGetMacro(SwapBuffers, svtkTypeBool);
  svtkBooleanMacro(SwapBuffers, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBRGBRGB. The
   * front argument indicates if the front buffer should be used or the back
   * buffer. It is the caller's responsibility to delete the resulting
   * array. It is very important to realize that the memory in this array
   * is organized from the bottom of the window to the top. The origin
   * of the screen is in the lower left corner. The y axis increases as
   * you go up the screen. So the storage of pixels is from left to right
   * and from bottom to top.
   * (x,y) is any corner of the rectangle. (x2,y2) is its opposite corner on
   * the diagonal.
   */
  virtual int SetPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, unsigned char* /*data*/,
    int /*front*/, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/,
    svtkUnsignedCharArray* /*data*/, int /*front*/, int /*right*/ = 0)
  {
    return 0;
  }
  //@}

  //@{
  /**
   * Same as Get/SetPixelData except that the image also contains an alpha
   * component. The image is transmitted as RGBARGBARGBA... each of which is a
   * float value. The "blend" parameter controls whether the SetRGBAPixelData
   * method blends the data with the previous contents of the frame buffer
   * or completely replaces the frame buffer data.
   */
  virtual float* GetRGBAPixelData(
    int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/, int /*right*/ = 0)
  {
    return nullptr;
  }
  virtual int GetRGBAPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/,
    svtkFloatArray* /*data*/, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetRGBAPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, float*, int /*front*/,
    int /*blend*/ = 0, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetRGBAPixelData(
    int, int, int, int, svtkFloatArray*, int, int /*blend*/ = 0, int /*right*/ = 0)
  {
    return 0;
  }
  virtual void ReleaseRGBAPixelData(float* /*data*/) {}
  virtual unsigned char* GetRGBACharPixelData(
    int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/, int /*right*/ = 0)
  {
    return nullptr;
  }
  virtual int GetRGBACharPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, int /*front*/,
    svtkUnsignedCharArray* /*data*/, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetRGBACharPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/,
    unsigned char* /*data*/, int /*front*/, int /*blend*/ = 0, int /*right*/ = 0)
  {
    return 0;
  }
  virtual int SetRGBACharPixelData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/,
    svtkUnsignedCharArray* /*data*/, int /*front*/, int /*blend*/ = 0, int /*right*/ = 0)
  {
    return 0;
  }
  //@}

  //@{
  /**
   * Set/Get the zbuffer data from the frame buffer.
   * (x,y) is any corner of the rectangle. (x2,y2) is its opposite corner on
   * the diagonal.
   */
  virtual float* GetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/) { return nullptr; }
  virtual int GetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, float* /*z*/)
  {
    return 0;
  }
  virtual int GetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, svtkFloatArray* /*z*/)
  {
    return 0;
  }
  virtual int SetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, float* /*z*/)
  {
    return 0;
  }
  virtual int SetZbufferData(int /*x*/, int /*y*/, int /*x2*/, int /*y2*/, svtkFloatArray* /*z*/)
  {
    return 0;
  }
  float GetZbufferDataAtPoint(int x, int y)
  {
    float value;
    this->GetZbufferData(x, y, x, y, &value);
    return value;
  }
  //@}

  //@{
  /**
   * This flag is set if the window hasn't rendered since it was created
   */
  svtkGetMacro(NeverRendered, int);
  //@}

  //@{
  /**
   * This is a flag that can be set to interrupt a rendering that is in
   * progress.
   */
  svtkGetMacro(AbortRender, int);
  svtkSetMacro(AbortRender, int);
  svtkGetMacro(InAbortCheck, int);
  svtkSetMacro(InAbortCheck, int);
  virtual int CheckAbortStatus();
  //@}

  //@{
  /**
   * @deprecated in SVTK 9.0
   */
  SVTK_LEGACY(svtkTypeBool GetIsPicking());
  SVTK_LEGACY(void SetIsPicking(svtkTypeBool));
  SVTK_LEGACY(void IsPickingOn());
  SVTK_LEGACY(void IsPickingOff());
  //@}

  /**
   * Check to see if a mouse button has been pressed.  All other events
   * are ignored by this method.  Ideally, you want to abort the render
   * on any event which causes the DesiredUpdateRate to switch from
   * a high-quality rate to a more interactive rate.
   */
  virtual svtkTypeBool GetEventPending() { return 0; }

  /**
   * Are we rendering at the moment
   */
  virtual int CheckInRenderStatus() { return this->InRender; }

  /**
   * Clear status (after an exception was thrown for example)
   */
  virtual void ClearInRenderStatus() { this->InRender = 0; }

  //@{
  /**
   * Set/Get the desired update rate. This is used with
   * the svtkLODActor class. When using level of detail actors you
   * need to specify what update rate you require. The LODActors then
   * will pick the correct resolution to meet your desired update rate
   * in frames per second. A value of zero indicates that they can use
   * all the time they want to.
   */
  virtual void SetDesiredUpdateRate(double);
  svtkGetMacro(DesiredUpdateRate, double);
  //@}

  //@{
  /**
   * Get the number of layers for renderers.  Each renderer should have
   * its layer set individually.  Some algorithms iterate through all layers,
   * so it is not wise to set the number of layers to be exorbitantly large
   * (say bigger than 100).
   */
  svtkGetMacro(NumberOfLayers, int);
  svtkSetClampMacro(NumberOfLayers, int, 1, SVTK_INT_MAX);
  //@}

  //@{
  /**
   * Get the interactor associated with this render window
   */
  svtkGetObjectMacro(Interactor, svtkRenderWindowInteractor);
  //@}

  /**
   * Set the interactor to the render window
   */
  void SetInteractor(svtkRenderWindowInteractor*);

  /**
   * This Method detects loops of RenderWindow<->Interactor,
   * so objects are freed properly.
   */
  void UnRegister(svtkObjectBase* o) override;

  //@{
  /**
   * Dummy stubs for svtkWindow API.
   */
  void SetDisplayId(void*) override {}
  void SetWindowId(void*) override {}
  virtual void SetNextWindowId(void*) {}
  void SetParentId(void*) override {}
  void* GetGenericDisplayId() override { return nullptr; }
  void* GetGenericWindowId() override { return nullptr; }
  void* GetGenericParentId() override { return nullptr; }
  void* GetGenericContext() override { return nullptr; }
  void* GetGenericDrawable() override { return nullptr; }
  void SetWindowInfo(const char*) override {}
  virtual void SetNextWindowInfo(const char*) {}
  void SetParentInfo(const char*) override {}
  //@}

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  virtual bool InitializeFromCurrentContext() { return false; }

  //@{
  /**
   * Set/Get an already existing window that this window should
   * share data with if possible. This must be set
   * after the shared render window has been created and initialized
   * but before this window has been initialized. Not all platforms
   * support data sharing.
   */
  virtual void SetSharedRenderWindow(svtkRenderWindow*);
  svtkGetObjectMacro(SharedRenderWindow, svtkRenderWindow);
  virtual bool GetPlatformSupportsRenderWindowSharing() { return false; }
  //@}

  /**
   * Attempt to make this window the current graphics context for the calling
   * thread.
   */
  void MakeCurrent() override {}

  /**
   * Tells if this window is the current graphics context for the calling
   * thread.
   */
  virtual bool IsCurrent() { return false; }

  /**
   * Test if the window has a valid drawable. This is
   * currently only an issue on Mac OS X Cocoa where rendering
   * to an invalid drawable results in all OpenGL calls to fail
   * with "invalid framebuffer operation".
   */
  SVTK_LEGACY(virtual bool IsDrawable());

  /**
   * If called, allow MakeCurrent() to skip cache-check when called.
   * MakeCurrent() reverts to original behavior of cache-checking
   * on the next render.
   */
  virtual void SetForceMakeCurrent() {}

  /**
   * Get report of capabilities for the render window
   */
  virtual const char* ReportCapabilities() { return "Not Implemented"; }

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  virtual int SupportsOpenGL() { return 0; }

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  virtual svtkTypeBool IsDirect() { return 0; }

  /**
   * This method should be defined by the subclass. How many bits of
   * precision are there in the zbuffer?
   */
  virtual int GetDepthBufferSize() { return 0; }

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  virtual int GetColorBufferSizes(int* /*rgba*/) { return 0; }

  //@{
  /**
   * Set / Get the number of multisamples to use for hardware antialiasing.
   * A value of 1 will be set to 0.
   */
  virtual void SetMultiSamples(int);
  svtkGetMacro(MultiSamples, int);
  //@}

  //@{
  /**
   * Set / Get the availability of the stencil buffer.
   */
  svtkSetMacro(StencilCapable, svtkTypeBool);
  svtkGetMacro(StencilCapable, svtkTypeBool);
  svtkBooleanMacro(StencilCapable, svtkTypeBool);
  //@}

  //@{
  /**
   * If there are several graphics card installed on a system,
   * this index can be used to specify which card you want to render to.
   * the default is 0. This may not work on all derived render window and
   * it may need to be set before the first render.
   */
  svtkSetMacro(DeviceIndex, int);
  svtkGetMacro(DeviceIndex, int);
  //@}
  /**
   * Returns the number of devices (graphics cards) on a system.
   * This may not work on all derived render windows.
   */
  virtual int GetNumberOfDevices() { return 0; }

  //@{
  /**
   * Set/Get if we want this window to use the sRGB color space.
   * Some hardware/drivers do not fully support this.
   */
  svtkGetMacro(UseSRGBColorSpace, bool);
  svtkSetMacro(UseSRGBColorSpace, bool);
  svtkBooleanMacro(UseSRGBColorSpace, bool);
  //@}

protected:
  svtkRenderWindow();
  ~svtkRenderWindow() override;

  virtual void DoStereoRender();

  svtkRendererCollection* Renderers;
  svtkNew<svtkRenderTimerLog> RenderTimer;
  svtkTypeBool Borders;
  svtkTypeBool FullScreen;
  int OldScreen[5];
  svtkTypeBool PointSmoothing;
  svtkTypeBool LineSmoothing;
  svtkTypeBool PolygonSmoothing;
  svtkTypeBool StereoRender;
  int StereoType;
  svtkTypeBool StereoCapableWindow;
  svtkTypeBool AlphaBitPlanes;
  svtkRenderWindowInteractor* Interactor;
  svtkSmartPointer<svtkUnsignedCharArray> StereoBuffer; // used for red blue stereo
  svtkSmartPointer<svtkUnsignedCharArray> ResultFrame;
  svtkTypeBool SwapBuffers;
  double DesiredUpdateRate;
  int AbortRender;
  int InAbortCheck;
  int InRender;
  int NeverRendered;
  int NumberOfLayers;
  int CurrentCursor;
  float AnaglyphColorSaturation;
  int AnaglyphColorMask[2];
  int MultiSamples;
  svtkTypeBool StencilCapable;
  int CapturingGL2PSSpecialProps;
  int DeviceIndex;

  bool UseSRGBColorSpace;

  /**
   * The universal time since the last abort check occurred.
   */
  double AbortCheckTime;

  svtkRenderWindow* SharedRenderWindow;

private:
  svtkRenderWindow(const svtkRenderWindow&) = delete;
  void operator=(const svtkRenderWindow&) = delete;

  svtkNew<svtkStereoCompositor> StereoCompositor;
};

#endif
