/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLRenderWindow
 * @brief   OpenGL rendering window
 *
 * svtkOpenGLRenderWindow is a concrete implementation of the abstract class
 * svtkRenderWindow. svtkOpenGLRenderer interfaces to the OpenGL graphics
 * library. Application programmers should normally use svtkRenderWindow
 * instead of the OpenGL specific version.
 */

#ifndef svtkOpenGLRenderWindow_h
#define svtkOpenGLRenderWindow_h

#include "svtkRect.h" // for svtkRecti
#include "svtkRenderWindow.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkType.h"                   // for ivar
#include <map>                         // for ivar
#include <set>                         // for ivar
#include <string>                      // for ivar

class svtkIdList;
class svtkOpenGLBufferObject;
class svtkOpenGLFramebufferObject;
class svtkOpenGLHardwareSupport;
class svtkOpenGLShaderCache;
class svtkOpenGLVertexBufferObjectCache;
class svtkOpenGLVertexArrayObject;
class svtkShaderProgram;
class svtkStdString;
class svtkTexture;
class svtkTextureObject;
class svtkTextureUnitManager;
class svtkGenericOpenGLResourceFreeCallback;
class svtkOpenGLState;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLRenderWindow : public svtkRenderWindow
{
public:
  svtkTypeMacro(svtkOpenGLRenderWindow, svtkRenderWindow);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Begin the rendering process.
   */
  void Start(void) override;

  /**
   * A termination method performed at the end of the rendering process
   * to do things like swapping buffers (if necessary) or similar actions.
   */
  void Frame() override;

  /**
   * What rendering backend has the user requested
   */
  const char* GetRenderingBackend() override;

  //@{
  /**
   * Set/Get the maximum number of multisamples
   */
  static void SetGlobalMaximumNumberOfMultiSamples(int val);
  static int GetGlobalMaximumNumberOfMultiSamples();
  //@}

  //@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBRGB...
   */
  unsigned char* GetPixelData(int x, int y, int x2, int y2, int front, int right) override;
  int GetPixelData(
    int x, int y, int x2, int y2, int front, svtkUnsignedCharArray* data, int right) override;
  int SetPixelData(
    int x, int y, int x2, int y2, unsigned char* data, int front, int right) override;
  int SetPixelData(
    int x, int y, int x2, int y2, svtkUnsignedCharArray* data, int front, int right) override;
  //@}

  //@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBARGBA...
   */
  float* GetRGBAPixelData(int x, int y, int x2, int y2, int front, int right = 0) override;
  int GetRGBAPixelData(
    int x, int y, int x2, int y2, int front, svtkFloatArray* data, int right = 0) override;
  int SetRGBAPixelData(
    int x, int y, int x2, int y2, float* data, int front, int blend = 0, int right = 0) override;
  int SetRGBAPixelData(int x, int y, int x2, int y2, svtkFloatArray* data, int front, int blend = 0,
    int right = 0) override;
  void ReleaseRGBAPixelData(float* data) override;
  unsigned char* GetRGBACharPixelData(
    int x, int y, int x2, int y2, int front, int right = 0) override;
  int GetRGBACharPixelData(
    int x, int y, int x2, int y2, int front, svtkUnsignedCharArray* data, int right = 0) override;
  int SetRGBACharPixelData(int x, int y, int x2, int y2, unsigned char* data, int front,
    int blend = 0, int right = 0) override;
  int SetRGBACharPixelData(int x, int y, int x2, int y2, svtkUnsignedCharArray* data, int front,
    int blend = 0, int right = 0) override;
  //@}

  //@{
  /**
   * Set/Get the zbuffer data from an image
   */
  float* GetZbufferData(int x1, int y1, int x2, int y2) override;
  int GetZbufferData(int x1, int y1, int x2, int y2, float* z) override;
  int GetZbufferData(int x1, int y1, int x2, int y2, svtkFloatArray* z) override;
  int SetZbufferData(int x1, int y1, int x2, int y2, float* buffer) override;
  int SetZbufferData(int x1, int y1, int x2, int y2, svtkFloatArray* buffer) override;
  //@}

  /**
   * Activate a texture unit for this texture
   */
  void ActivateTexture(svtkTextureObject*);

  /**
   * Deactivate a previously activated texture
   */
  void DeactivateTexture(svtkTextureObject*);

  /**
   * Get the texture unit for a given texture object
   */
  int GetTextureUnitForTexture(svtkTextureObject*);

  /**
   * Get the size of the depth buffer.
   */
  int GetDepthBufferSize() override;

  /**
   * Is this window/fo in sRGB colorspace
   */
  bool GetUsingSRGBColorSpace();

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  int GetColorBufferSizes(int* rgba) override;

  /**
   * Get the internal format of current attached texture or render buffer.
   * attachmentPoint is the index of attachment.
   * Returns 0 if not able to determine.
   */
  int GetColorBufferInternalFormat(int attachmentPoint);

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
   * Initialize OpenGL for this window.
   */
  virtual void OpenGLInit();

  // Initialize the state of OpenGL that SVTK wants for this window
  virtual void OpenGLInitState();

  // Initialize SVTK for rendering in a new OpenGL context
  virtual void OpenGLInitContext();

  /**
   * Get the major and minor version numbers of the OpenGL context we are using
   * ala 3.2, 3.3, 4.0, etc... returns 0,0 if opengl has not been initialized
   * yet
   */
  void GetOpenGLVersion(int& major, int& minor);

  /**
   * Return the OpenGL name of the back left buffer.
   * It is GL_BACK_LEFT if GL is bound to the window-system-provided
   * framebuffer. It is svtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by svtkOpenGLCamera.
   */
  unsigned int GetBackLeftBuffer();

  /**
   * Return the OpenGL name of the back right buffer.
   * It is GL_BACK_RIGHT if GL is bound to the window-system-provided
   * framebuffer. It is svtkgl::COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by svtkOpenGLCamera.
   */
  unsigned int GetBackRightBuffer();

  /**
   * Return the OpenGL name of the front left buffer.
   * It is GL_FRONT_LEFT if GL is bound to the window-system-provided
   * framebuffer. It is svtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by svtkOpenGLCamera.
   */
  unsigned int GetFrontLeftBuffer();

  /**
   * Return the OpenGL name of the front right buffer.
   * It is GL_FRONT_RIGHT if GL is bound to the window-system-provided
   * framebuffer. It is svtkgl::COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
   * application-created framebuffer object (GPU-based offscreen rendering)
   * It is used by svtkOpenGLCamera.
   */
  unsigned int GetFrontRightBuffer();

  /**
   * Return the OpenGL name of the back left buffer.
   * Identical to GetBackLeftBuffer.
   */
  unsigned int GetBackBuffer();

  /**
   * Return the OpenGL name of the front left buffer.
   * Identical to GetFrontLeftBuffer.
   */
  unsigned int GetFrontBuffer();

  /**
   * Get the time when the OpenGL context was created.
   */
  virtual svtkMTimeType GetContextCreationTime();

  /**
   * Returns an Shader Cache object
   */
  svtkOpenGLShaderCache* GetShaderCache();

  /**
   * Returns the VBO Cache
   */
  svtkOpenGLVertexBufferObjectCache* GetVBOCache();

  //@{
  /**
   * Returns the offscreen framebuffer object if any
   */
  svtkGetObjectMacro(OffScreenFramebuffer, svtkOpenGLFramebufferObject);
  //@}

  /**
   * Returns its texture unit manager object. A new one will be created if one
   * hasn't already been set up.
   */
  svtkTextureUnitManager* GetTextureUnitManager();

  /**
   * Block the thread until the actual rendering is finished().
   * Useful for measurement only.
   */
  void WaitForCompletion() override;

  /**
   * Replacement for the old glDrawPixels function
   */
  virtual void DrawPixels(
    int x1, int y1, int x2, int y2, int numComponents, int dataType, void* data);

  /**
   * Replacement for the old glDrawPixels function, but it allows
   * for scaling the data and using only part of the texture
   */
  virtual void DrawPixels(int dstXmin, int dstYmin, int dstXmax, int dstYmax, int srcXmin,
    int srcYmin, int srcXmax, int srcYmax, int srcWidth, int srcHeight, int numComponents,
    int dataType, void* data);

  /**
   * Replacement for the old glDrawPixels function.  This simple version draws all
   * the data to the entire current viewport scaling as needed.
   */
  virtual void DrawPixels(int srcWidth, int srcHeight, int numComponents, int dataType, void* data);

  /**
   * Return the largest line width supported by the hardware
   */
  virtual float GetMaximumHardwareLineWidth() { return this->MaximumHardwareLineWidth; }

  /**
   * Returns true if driver has an
   * EGL/OpenGL bug that makes svtkChartsCoreCxx-TestChartDoubleColors and other tests to fail
   * because point sprites don't work correctly (gl_PointCoord is undefined) unless
   * glEnable(GL_POINT_SPRITE)
   */
  virtual bool IsPointSpriteBugPresent() { return 0; }

  /**
   * Get a mapping of svtk data types to native texture formats for this window
   * we put this on the RenderWindow so that every texture does not have to
   * build these structures themselves
   */
  int GetDefaultTextureInternalFormat(
    int svtktype, int numComponents, bool needInteger, bool needFloat, bool needSRGB);

  /**
   * Return a message profiding additional details about the
   * results of calling SupportsOpenGL()  This can be used
   * to retrieve more specifics about what failed
   */
  std::string GetOpenGLSupportMessage() { return this->OpenGLSupportMessage; }

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override;

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  virtual void Initialize(void) {}

  std::set<svtkGenericOpenGLResourceFreeCallback*> Resources;

  void RegisterGraphicsResources(svtkGenericOpenGLResourceFreeCallback* cb)
  {
    std::set<svtkGenericOpenGLResourceFreeCallback*>::iterator it = this->Resources.find(cb);
    if (it == this->Resources.end())
    {
      this->Resources.insert(cb);
    }
  }

  void UnregisterGraphicsResources(svtkGenericOpenGLResourceFreeCallback* cb)
  {
    std::set<svtkGenericOpenGLResourceFreeCallback*>::iterator it = this->Resources.find(cb);
    if (it != this->Resources.end())
    {
      this->Resources.erase(it);
    }
  }

  /**
   * Ability to push and pop this window's context
   * as the current context. The idea being to
   * if needed make this window's context current
   * and when done releasing resources restore
   * the prior context.  The default implementation
   * here is only meant as a backup for subclasses
   * that lack a proper implementation.
   */
  virtual void PushContext() { this->MakeCurrent(); }
  virtual void PopContext() {}

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() override;

  /**
   * Returns the id for the frame buffer object, if any, used by the render window
   * in which the window does all its rendering. This may be 0, in which case
   * the render window is rendering to the default OpenGL render buffers.
   *
   * @returns the name (or id) of the frame buffer object to render to.
   */
  svtkGetMacro(DefaultFrameBufferId, unsigned int);

  /**
   * Set the number of vertical syncs required between frames.
   * A value of 0 means swap buffers as quickly as possible
   * regardless of the vertical refresh. A value of 1 means swap
   * buffers in sync with the vertical refresh to eliminate tearing.
   * A value of -1 means use a value of 1 unless we missed a frame
   * in which case swap immediately. Returns true if the call
   * succeeded.
   */
  virtual bool SetSwapControl(int) { return false; }

  // Get the state object used to keep track of
  // OpenGL state
  virtual svtkOpenGLState* GetState() { return this->State; }

  // Get a VBO that can be shared by many
  // It consists of normalized display
  // coordinates for a quad and tcoords
  svtkOpenGLBufferObject* GetTQuad2DVBO();

  // Activate and return thje texture unit for a generic 2d 64x64
  // float greyscale noise texture ranging from 0 to 1. The texture is
  // generated using PerlinNoise.  This textur eunit will automatically
  // be deactivated at the end of the render process.
  int GetNoiseTextureUnit();

  /**
   * Update the system, if needed, at end of render process
   */
  void End() override;

  /**
   * Handle opengl specific code and calls superclass
   */
  void Render() override;

  /**
   * Intermediate method performs operations required between the rendering
   * of the left and right eye.
   */
  void StereoMidpoint() override;

  // does SVTKs framebuffer require resolving for reading pixels
  bool GetBufferNeedsResolving();

  /**
   * Free up any graphics resources associated with this window
   * a value of NULL means the context may already be destroyed
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkOpenGLRenderWindow();
  ~svtkOpenGLRenderWindow() override;

  // used in testing for opengl support
  // in the SupportsOpenGL() method
  bool OpenGLSupportTested;
  int OpenGLSupportResult;
  std::string OpenGLSupportMessage;

  virtual int ReadPixels(
    const svtkRecti& rect, int front, int glFormat, int glType, void* data, int right = 0);

  /**
   * Create the offScreen framebuffer
   * Return if the creation was successful or not.
   * \pre positive_width: width>0
   * \pre positive_height: height>0
   * \pre not_initialized: !OffScreenUseFrameBuffer
   * \post valid_result: (result==0 || result==1)
   */
  int CreateOffScreenFramebuffer(int width, int height);
  svtkOpenGLFramebufferObject* OffScreenFramebuffer;

  /**
   * Create a not-off-screen window.
   */
  virtual void CreateAWindow() = 0;

  /**
   * Destroy a not-off-screen window.
   */
  virtual void DestroyWindow() = 0;

  /**
   * Query and save OpenGL state
   */
  void SaveGLState();

  /**
   * Restore OpenGL state at end of the rendering
   */
  void RestoreGLState();

  std::map<std::string, int> GLStateIntegers;

  unsigned int BackLeftBuffer;
  unsigned int BackRightBuffer;
  unsigned int FrontLeftBuffer;
  unsigned int FrontRightBuffer;
  unsigned int DefaultFrameBufferId;

  /**
   * Flag telling if the context has been created here or was inherited.
   */
  int OwnContext;

  svtkTimeStamp ContextCreationTime;

  svtkTextureObject* DrawPixelsTextureObject;

  bool Initialized;   // ensure glewinit has been called
  bool GlewInitValid; // Did glewInit initialize with a valid state?

  float MaximumHardwareLineWidth;

  char* Capabilities;

  // used for fast quad rendering
  svtkOpenGLBufferObject* TQuad2DVBO;

  // noise texture
  svtkTextureObject* NoiseTextureObject;

  double FirstRenderTime;

  // keep track of in case we need to recreate the framebuffer
  int LastMultiSamples;

  int ScreenSize[2];

private:
  svtkOpenGLRenderWindow(const svtkOpenGLRenderWindow&) = delete;
  void operator=(const svtkOpenGLRenderWindow&) = delete;

  // Keeping `State` private so the only way to access it is through
  // `this->GetState()`.
  svtkOpenGLState* State;
};

#endif
