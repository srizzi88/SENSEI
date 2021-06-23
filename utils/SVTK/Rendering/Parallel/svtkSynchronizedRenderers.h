/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSynchronizedRenderers.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSynchronizedRenderers
 * @brief   synchronizes renderers across processes.
 *
 * svtkSynchronizedRenderers is used to synchronize renderers (svtkRenderer and
 * subclasses) across processes for parallel rendering. It's designed to be used
 * in conjunction with svtkSynchronizedRenderWindows to synchronize the render
 * windows among those processes.
 * This class handles synchronization of certain render parameters among the
 * renderers such as viewport, camera parameters. It doesn't support compositing
 * of rendered images across processes on its own. You typically either subclass
 * to implement a compositing algorithm or use a renderer capable of compositing
 * eg. IceT based renderer.
 */

#ifndef svtkSynchronizedRenderers_h
#define svtkSynchronizedRenderers_h

#include "svtkObject.h"
#include "svtkRenderingParallelModule.h" // For export macro
#include "svtkSmartPointer.h"            // needed for svtkSmartPointer.
#include "svtkUnsignedCharArray.h"       // needed for svtkUnsignedCharArray.

class svtkFXAAOptions;
class svtkRenderer;
class svtkMultiProcessController;
class svtkMultiProcessStream;
class svtkOpenGLFXAAFilter;
class svtkOpenGLRenderer;

class SVTKRENDERINGPARALLEL_EXPORT svtkSynchronizedRenderers : public svtkObject
{
public:
  static svtkSynchronizedRenderers* New();
  svtkTypeMacro(svtkSynchronizedRenderers, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the renderer to be synchronized by this instance. A
   * svtkSynchronizedRenderers instance can be used to synchronize exactly 1
   * renderer on each processes. You can create multiple instances on
   * svtkSynchronizedRenderers to synchronize multiple renderers.
   */
  virtual void SetRenderer(svtkRenderer*);
  virtual svtkRenderer* GetRenderer();
  //@}

  //@{
  /**
   * Set the parallel message communicator. This is used to communicate among
   * processes.
   */
  virtual void SetParallelController(svtkMultiProcessController*);
  svtkGetObjectMacro(ParallelController, svtkMultiProcessController);
  //@}

  //@{
  /**
   * Enable/Disable parallel rendering. Unless Parallel rendering is on, the
   * cameras won't be synchronized across processes.
   */
  svtkSetMacro(ParallelRendering, bool);
  svtkGetMacro(ParallelRendering, bool);
  svtkBooleanMacro(ParallelRendering, bool);
  //@}

  //@{
  /**
   * Get/Set the image reduction factor.
   */
  svtkSetClampMacro(ImageReductionFactor, int, 1, 50);
  svtkGetMacro(ImageReductionFactor, int);
  //@}

  //@{
  /**
   * If on (default), the rendered images are pasted back on to the screen. You
   * should turn this flag off on processes that are not meant to be visible to
   * the user.
   */
  svtkSetMacro(WriteBackImages, bool);
  svtkGetMacro(WriteBackImages, bool);
  svtkBooleanMacro(WriteBackImages, bool);
  //@}

  //@{
  /**
   * Get/Set the root-process id. This is required when the ParallelController
   * is a svtkSocketController. Set to 0 by default (which will not work when
   * using a svtkSocketController but will work for svtkMPIController).
   */
  svtkSetMacro(RootProcessId, int);
  svtkGetMacro(RootProcessId, int);
  //@}

  /**
   * Computes visible prob bounds. This must be called on all processes at the
   * same time. The collective result is made available on all processes once
   * this method returns.
   * Note that this method requires that bounds is initialized to some value.
   * This expands the bounds to include the prop bounds.
   */
  void CollectiveExpandForVisiblePropBounds(double bounds[6]);

  //@{
  /**
   * When set, this->CaptureRenderedImage() does not capture image from the
   * screen instead passes the call to the delegate.
   */
  virtual void SetCaptureDelegate(svtkSynchronizedRenderers*);
  svtkGetObjectMacro(CaptureDelegate, svtkSynchronizedRenderers);
  //@}

  //@{
  /**
   * When multiple groups of processes are synchronized together using different
   * controllers, one needs to specify the order in which the various
   * synchronizers execute. In such cases one starts with the outer most
   * svtkSynchronizedRenderers, sets the dependent one as a CaptureDelegate on it
   * and the turn off AutomaticEventHandling on the delegate.
   */
  svtkSetMacro(AutomaticEventHandling, bool);
  svtkGetMacro(AutomaticEventHandling, bool);
  svtkBooleanMacro(AutomaticEventHandling, bool);
  //@}

  //@{
  /**
   * When doing rendering between multiple processes, it is often easier to have
   * all ranks do the rendering on a black background. This helps avoid issues
   * where the background gets over blended as the images are composted
   * together. If  set to true (default is false), before the rendering begins,
   * svtkSynchronizedRenderers will change the renderer's background color and
   * other flags to make it render on a black background and then restore then
   * on end render. If WriteBackImages is true, then the background will indeed
   * be restored before the write-back happens, thus ensuring the result
   * displayed to the user is on correct background.
   */
  svtkSetMacro(FixBackground, bool);
  svtkGetMacro(FixBackground, bool);
  svtkBooleanMacro(FixBackground, bool);
  //@}

  enum
  {
    SYNC_RENDERER_TAG = 15101,
    RESET_CAMERA_TAG = 15102,
    COMPUTE_BOUNDS_TAG = 15103
  };

  /// svtkRawImage can be used to make it easier to deal with images for
  /// compositing/communicating over client-server etc.
  struct SVTKRENDERINGPARALLEL_EXPORT svtkRawImage
  {
  public:
    svtkRawImage()
    {
      this->Valid = false;
      this->Size[0] = this->Size[1] = 0;
      this->Data = svtkSmartPointer<svtkUnsignedCharArray>::New();
    }

    void Resize(int dx, int dy, int numcomps)
    {
      this->Valid = false;
      this->Allocate(dx, dy, numcomps);
    }

    /**
     * Create the buffer from an image data.
     */
    void Initialize(int dx, int dy, svtkUnsignedCharArray* data);

    void MarkValid() { this->Valid = true; }
    void MarkInValid() { this->Valid = false; }

    bool IsValid() { return this->Valid; }
    int GetWidth() { return this->Size[0]; }
    int GetHeight() { return this->Size[1]; }
    svtkUnsignedCharArray* GetRawPtr() { return this->Data; }

    /**
     * Pushes the image to the viewport. The OpenGL viewport  and scissor region
     * is setup using the viewport defined by the renderer.
     */
    bool PushToViewport(svtkRenderer* renderer);

    /**
     * This is a raw version of PushToViewport() that assumes that the
     * glViewport() has already been setup externally.
     */
    bool PushToFrameBuffer(svtkRenderer* ren);

    // Captures the image from the viewport.
    // This doesn't trigger a render, just captures what's currently there in
    // the active buffer.
    bool Capture(svtkRenderer*);

    // Save the image as a png. Useful for debugging.
    void SaveAsPNG(const char* filename);

  private:
    bool Valid;
    int Size[2];
    svtkSmartPointer<svtkUnsignedCharArray> Data;

    void Allocate(int dx, int dy, int numcomps);
  };

protected:
  svtkSynchronizedRenderers();
  ~svtkSynchronizedRenderers() override;

  struct RendererInfo
  {
    int ImageReductionFactor;
    int Draw;
    int CameraParallelProjection;
    double Viewport[4];
    double CameraPosition[3];
    double CameraFocalPoint[3];
    double CameraViewUp[3];
    double CameraWindowCenter[2];
    double CameraClippingRange[2];
    double CameraViewAngle;
    double CameraParallelScale;
    double EyeTransformMatrix[16];
    double ModelTransformMatrix[16];

    // Save/restore the struct to/from a stream.
    void Save(svtkMultiProcessStream& stream);
    bool Restore(svtkMultiProcessStream& stream);

    void CopyFrom(svtkRenderer*);
    void CopyTo(svtkRenderer*);
  };

  // These methods are called on all processes as a consequence of corresponding
  // events being called on the renderer.
  virtual void HandleStartRender();
  virtual void HandleEndRender();
  virtual void HandleAbortRender() {}

  virtual void MasterStartRender();
  virtual void SlaveStartRender();

  virtual void MasterEndRender();
  virtual void SlaveEndRender();

  svtkMultiProcessController* ParallelController;
  svtkOpenGLRenderer* Renderer;

  /**
   * Can be used in HandleEndRender(), MasterEndRender() or SlaveEndRender()
   * calls to capture the rendered image. The captured image is stored in
   * `this->Image`.
   */
  virtual svtkRawImage& CaptureRenderedImage();

  /**
   * Can be used in HandleEndRender(), MasterEndRender() or SlaveEndRender()
   * calls to paste back the image from this->Image to the viewport.
   */
  virtual void PushImageToScreen();

  svtkSynchronizedRenderers* CaptureDelegate;
  svtkRawImage Image;

  bool ParallelRendering;
  int ImageReductionFactor;
  bool WriteBackImages;
  int RootProcessId;
  bool AutomaticEventHandling;

private:
  svtkSynchronizedRenderers(const svtkSynchronizedRenderers&) = delete;
  void operator=(const svtkSynchronizedRenderers&) = delete;

  class svtkObserver;
  svtkObserver* Observer;
  friend class svtkObserver;

  bool UseFXAA;
  svtkOpenGLFXAAFilter* FXAAFilter;

  double LastViewport[4];

  double LastBackground[3];
  double LastBackgroundAlpha;
  bool LastTexturedBackground;
  bool LastGradientBackground;
  bool FixBackground;
};

#endif
