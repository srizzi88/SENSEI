/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelRenderManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  Copyright 2003 Sandia Corporation. Under the terms of Contract
  DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
  or on behalf of the U.S. Government. Redistribution and use in source and
  binary forms, with or without modification, are permitted provided that this
  Notice and any statement of authorship are reproduced on all copies.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParallelRenderManager
 * @brief   An object to control parallel rendering.
 *
 *
 * svtkParallelRenderManager operates in multiple processes.  It provides
 * proper renderers and render windows for performing the parallel
 * rendering correctly.  It can also attach itself to render windows and
 * propagate rendering events and camera views.
 *
 * @attention
 * Many parallel rendering schemes do not correctly handle transparency.
 * Unless otherwise documented, assume a sub class does not.
 *
 * @todo
 * Synchronization/barrier primitives.
 *
 * @todo
 * Query ranges of scalar values of objects in addition to the boundary in
 * three-space
 *
 */

#ifndef svtkParallelRenderManager_h
#define svtkParallelRenderManager_h

#include "svtkObject.h"
#include "svtkRenderingParallelModule.h" // For export macro

class svtkDoubleArray;
class svtkMultiProcessController;
class svtkMultiProcessStream;
class svtkRenderer;
class svtkRendererCollection;
class svtkRenderWindow;
class svtkTimerLog;
class svtkUnsignedCharArray;

class SVTKRENDERINGPARALLEL_EXPORT svtkParallelRenderManager : public svtkObject
{
public:
  svtkTypeMacro(svtkParallelRenderManager, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Builds a svtkRenderWindow compatible with this render manager.  The
   * user program is responsible for registering the render window with the
   * SetRenderWindow method and calling Delete.  It is not advisable to use
   * a parallel render manager with a render window that was not built with
   * this method.
   */
  virtual svtkRenderWindow* MakeRenderWindow();

  /**
   * Builds a svtkRenderer compatible with this render manager.  (Should we
   * also register it?)  The user program is responsible for calling
   * Delete.  It is not advisable to use a parallel render manager with a
   * renderer that was not built with this method.
   */
  virtual svtkRenderer* MakeRenderer();

  //@{
  /**
   * Set/Get the RenderWindow to use for compositing.
   * We add a start and end observer to the window.
   */
  svtkGetObjectMacro(RenderWindow, svtkRenderWindow);
  virtual void SetRenderWindow(svtkRenderWindow* renWin);
  //@}

  //@{
  /**
   * Set/Get the svtkMultiProcessController which will handle communications
   * for the parallel rendering.
   */
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  virtual void SetController(svtkMultiProcessController* controller);
  //@}

  /**
   * This method sets the piece and number of pieces for each
   * actor with a polydata mapper.
   */
  virtual void InitializePieces();

  /**
   * Make all rendering windows not viewable set as off screen rendering.
   * To make all renderwindows on screen rendering again, call
   * OffScreenRenderingOff on all the render windows.  This class assumes
   * the window on root node is the only one viewable.  Subclasses should
   * change this as necessary.
   */
  virtual void InitializeOffScreen();

  /**
   * Initializes the RMIs and then, if on root node, starts the interactor
   * on the attached render window.  Otherwise, starts processing RMIs.
   * When the interactor returns, it breaks the RMI listening on all other
   * processors.
   */
  virtual void StartInteractor();

  /**
   * If on node other than root, starts serving RMI requests for parallel
   * renders.
   */
  virtual void StartServices();

  /**
   * If on root node, stops the RMI processing on all service nodes.
   */
  virtual void StopServices();

  //@{
  /**
   * Callbacks that initialize and finish rendering and other tasks.
   */
  virtual void StartRender();
  virtual void EndRender();
  virtual void SatelliteStartRender();
  virtual void SatelliteEndRender();
  virtual void RenderRMI();
  virtual void ResetCamera(svtkRenderer* ren);
  virtual void ResetCameraClippingRange(svtkRenderer* ren);
  virtual void ComputeVisiblePropBoundsRMI(int renderId);
  //@}

  virtual void InitializeRMIs();

  /**
   * Resets the camera of each renderer contained in the RenderWindow.
   * Should only be called in the "root" process, and all remote processes
   * must be processing RMIs for this method to complete.
   */
  virtual void ResetAllCameras();

  /**
   * Calculates the bounds by gathering information from all processes.
   */
  virtual void ComputeVisiblePropBounds(svtkRenderer* ren, double bounds[6]);

  //@{
  /**
   * Turns on/off parallel rendering.  When on (the default) the object
   * responds to render events of the attached window, propagates the
   * render event to other processors, and otherwise enables the parallel
   * rendering process.
   */
  svtkSetMacro(ParallelRendering, int);
  svtkGetMacro(ParallelRendering, int);
  svtkBooleanMacro(ParallelRendering, int);
  //@}

  //@{
  /**
   * Turns on/off render event propagation.  When on (the default) and
   * ParallelRendering is on, process 0 will send an RMI call to all remote
   * processes to perform a synchronized render.  When off, render must be
   * manually called on each process.
   */
  svtkSetMacro(RenderEventPropagation, int);
  svtkGetMacro(RenderEventPropagation, int);
  svtkBooleanMacro(RenderEventPropagation, int);
  //@}

  /**
   * Get/Set the default value used for RenderEventPropagation when a new
   * instance of svtkParallelRenderManager is created.
   * Set to true by default.
   */
  static void SetDefaultRenderEventPropagation(bool val)
  {
    svtkParallelRenderManager::DefaultRenderEventPropagation = val;
  }
  static bool GetDefaultRenderEventPropagation()
  {
    return svtkParallelRenderManager::DefaultRenderEventPropagation;
  }

  //@{
  /**
   * This is used for tiled display rendering.  When data has been
   * duplicated on all processes, then we do not need to compositing.
   * Cameras and renders are still propagated though.
   */
  svtkSetMacro(UseCompositing, int);
  svtkGetMacro(UseCompositing, int);
  svtkBooleanMacro(UseCompositing, int);
  //@}

  //@{
  /**
   * Set/Get the reduction factor (for sort-last based parallel renderers).
   * The size of rendered image is divided by the reduction factor and then
   * is blown up to the size of the current svtkRenderWindow.  Setting
   * higher reduction factors enables shorter image transfer times (which
   * is often the bottleneck) but will greatly reduce image quality.  A
   * reduction factor of 2 or greater should only be used for intermediate
   * images in interactive applications.  A reduction factor of 1 (or less)
   * will result in no change in image quality.  A parallel render manager
   * may ignore the image reduction factor if it will result in little or
   * no performance enhancements (eg. it does not do image space
   * manipulations).
   */
  virtual void SetImageReductionFactor(double factor);
  svtkGetMacro(ImageReductionFactor, double);
  //@}

  svtkSetMacro(MaxImageReductionFactor, double);
  svtkGetMacro(MaxImageReductionFactor, double);

  /**
   * Sets the ReductionFactor based on the given desired update rate and
   * the rendering metrics taken from the last time UpdateServerInfo was
   * called.  Note that if AutoReductionFactor is on, this function is called
   * with the desired update rate of the render window automatically.
   */
  virtual void SetImageReductionFactorForUpdateRate(double DesiredUpdateRate);

  //@{
  /**
   * If on, the ReductionFactor is automatically adjusted to best meet the
   * the DesiredUpdateRate in the current RenderWindow based on metrics
   * from the last render.
   */
  svtkSetMacro(AutoImageReductionFactor, int);
  svtkGetMacro(AutoImageReductionFactor, int);
  svtkBooleanMacro(AutoImageReductionFactor, int);
  //@}

  //@{
  /**
   * Get rendering metrics.
   */
  svtkGetMacro(RenderTime, double);
  svtkGetMacro(ImageProcessingTime, double);
  //@}

  //@{
  /**
   * By default, the state of all renderers in the root's render window is
   * propagated to the rest of the processes.  In order for this to work, all
   * render windows must have the same renderers in the same order.  If this is
   * not the case, you can turn off the SyncRenderWindowRenderers.  When this
   * flag is off, the list of renderers held by this parallel render manager
   * (initially empty) is synced.  You can modify the list of renderers with the
   * AddRenderer, RemoveRenderer, and RemoveAllRenderers methods.
   */
  svtkGetMacro(SyncRenderWindowRenderers, int);
  svtkSetMacro(SyncRenderWindowRenderers, int);
  svtkBooleanMacro(SyncRenderWindowRenderers, int);
  virtual void AddRenderer(svtkRenderer*);
  virtual void RemoveRenderer(svtkRenderer*);
  virtual void RemoveAllRenderers();
  //@}

  //@{
  /**
   * If on (the default), the result of any image space manipulations are
   * written back to the render window frame buffer.  If off, the image
   * stored in the frame buffer may not be correct.  Either way, the
   * correct frame buffer images may be read with
   * svtkParallelRenderManager::GetPixelData.  Turning WriteBackImages off
   * may result in a speedup if the render window is not visible to the user
   * and images are read back for further processing or transit.
   */
  svtkSetMacro(WriteBackImages, int);
  svtkGetMacro(WriteBackImages, int);
  svtkBooleanMacro(WriteBackImages, int);
  //@}

  //@{
  /**
   * If on (the default), when the ImageReductionFactor is greater than 1
   * and WriteBackImages is on, the image will be magnified to fill the
   * entire render window.
   */
  svtkSetMacro(MagnifyImages, int);
  svtkGetMacro(MagnifyImages, int);
  svtkBooleanMacro(MagnifyImages, int);
  //@}

  enum
  {
    NEAREST,
    LINEAR
  };

  //@{
  /**
   * Sets the method used to magnify images.  Nearest simply replicates
   * each pixel enough times to fill the image.  Linear performs linear
   * interpolation between the pixels.
   */
  virtual void SetMagnifyImageMethod(int method);
  svtkGetMacro(MagnifyImageMethod, int);
  void SetMagnifyImageMethodToNearest() { this->SetMagnifyImageMethod(NEAREST); }
  void SetMagnifyImageMethodToLinear() { this->SetMagnifyImageMethod(LINEAR); }
  //@}

  //@{
  /**
   * Convenience functions for magnifying images.
   */
  virtual void MagnifyImage(svtkUnsignedCharArray* fullImage, const int fullImageSize[2],
    svtkUnsignedCharArray* reducedImage, const int reducedImageSize[2],
    const int fullImageViewport[4] = nullptr, const int reducedImageViewport[4] = nullptr);
  static void MagnifyImageNearest(svtkUnsignedCharArray* fullImage, const int fullImageSize[2],
    svtkUnsignedCharArray* reducedImage, const int reducedImageSize[2],
    const int fullImageViewport[4] = nullptr, const int reducedImageViewport[4] = nullptr);
  static void MagnifyImageLinear(svtkUnsignedCharArray* fullImage, const int fullImageSize[2],
    svtkUnsignedCharArray* reducedImage, const int reducedImageSize[2],
    const int fullImageViewport[4] = nullptr, const int reducedImageViewport[4] = nullptr);
  //@}

  //@{
  /**
   * The most appropriate way to retrieve full size image data after a
   * render.  Will work regardless of whether WriteBackImages or
   * MagnifyImage is on or off.  The data returned may be a shallow copy of
   * an internal array.  Therefore, the data may be invalid after the next
   * render or if the ParallelRenderManager is destroyed.
   */
  virtual void GetPixelData(svtkUnsignedCharArray* data);
  virtual void GetPixelData(int x1, int y1, int x2, int y2, svtkUnsignedCharArray* data);
  //@}

  //@{
  /**
   * The most appropriate way to retrieve reduced size image data after a
   * render.  Will work regardless of whether WriteBackImages or
   * MagnifyImage is on or off.  The data returned may be a shallow copy of
   * an internal array.  Therefore, the data may be invalid after the next
   * render or if the ParallelRenderManager is destroyed.
   */
  virtual void GetReducedPixelData(svtkUnsignedCharArray* data);
  virtual void GetReducedPixelData(int x1, int y1, int x2, int y2, svtkUnsignedCharArray* data);
  //@}

  //@{
  /**
   * Returns the full image size calculated at the last render.
   */
  svtkGetVector2Macro(FullImageSize, int);
  //@}
  //@{
  /**
   * Returns the reduced image size calculated at the last render.
   */
  svtkGetVector2Macro(ReducedImageSize, int);
  //@}

  /**
   * Given the x and y size of the render windows, reposition them
   * in a tile of n columns.
   */
  void TileWindows(int xsize, int ysize, int nColumns);

  //@{
  /**
   * Get/Set if all Images must use RGBA instead of RGB. By default,
   * this flag is on.
   */
  svtkSetMacro(UseRGBA, int);
  svtkGetMacro(UseRGBA, int);
  //@}

  //@{
  /**
   * If ForceRenderWindowSize is set to true, the render manager will use
   * the RenderWindowSize ivar instead of getting the size from the render window.
   */
  svtkSetMacro(ForceRenderWindowSize, int);
  svtkGetMacro(ForceRenderWindowSize, int);
  //@}

  //@{
  /**
   * If ForceRenderWindowSize is set to true, the render manager will use
   * the Size ivar instead of getting the size from the render window.
   */
  svtkSetVector2Macro(ForcedRenderWindowSize, int);
  svtkGetVector2Macro(ForcedRenderWindowSize, int);
  //@}

  enum Tags
  {
    RENDER_RMI_TAG = 34532,
    COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG = 54636,
    WIN_INFO_TAG = 87834,
    REN_INFO_TAG = 87836,
    LIGHT_INFO_TAG = 87838,
    REN_ID_TAG = 58794,
    BOUNDS_TAG = 23543
  };

  virtual void CheckForAbortRender() {}
  virtual int CheckForAbortComposite() { return 0; }

  //@{
  /**
   * The default is to allow the use of the back buffer for compositing.
   * If set to false, this will prevent to manager from swapping buffers.
   * This allows something else (for instance VisibleCellSelection) to
   * control front/back buffer swapping.
   */
  svtkSetMacro(UseBackBuffer, int);
  svtkGetMacro(UseBackBuffer, int);
  svtkBooleanMacro(UseBackBuffer, int);
  //@}

  //@{
  /**
   * When set the render manager will synchronize the TileViewport and TileScale
   * properties. This may not be desirable in cases where there's some other
   * mechanism to set the tile dimensions eg. Tile displays.
   */
  svtkSetMacro(SynchronizeTileProperties, int);
  svtkGetMacro(SynchronizeTileProperties, int);
  svtkBooleanMacro(SynchronizeTileProperties, int);
  //@}

  //@{
  /**
   * INTERNAL METHODS (DON NOT USE).
   * There are internal methods made public so that they can be called from
   * callback functions.
   */
  virtual void GenericStartRenderCallback();
  virtual void GenericEndRenderCallback();
  //@}

protected:
  svtkParallelRenderManager();
  ~svtkParallelRenderManager() override;

  //@{
  /**
   * Add/Remove event handlers for the render window.
   */
  void AddRenderWindowEventHandlers();
  void RemoveRenderWindowEventHandlers();
  //@}

  svtkRenderWindow* RenderWindow;
  svtkMultiProcessController* Controller;
  svtkRendererCollection* Renderers;

  virtual svtkRendererCollection* GetRenderers();

  int ForceRenderWindowSize;
  int ForcedRenderWindowSize[2];

  /**
   * The "root" node's process id.  This is the node which is listening for
   * and propagating new render events from the RenderWindow.  All
   * processes on the controller must have the same value.  This value must
   * be set before SetRenderWindow method is called.  In the constructor or
   * the SetController methods are good places.  By default this is set to
   * 0.
   */
  int RootProcessId;

  int ObservingRenderWindow;
  int ObservingAbort;

  unsigned long StartRenderTag;
  unsigned long EndRenderTag;
  unsigned long ResetCameraTag;
  unsigned long ResetCameraClippingRangeTag;
  unsigned long AbortRenderCheckTag;

  double ImageReductionFactor;
  double MaxImageReductionFactor;
  int AutoImageReductionFactor;

  int WriteBackImages;
  int MagnifyImages;
  int MagnifyImageMethod;

  int UseRGBA;
  int SynchronizeTileProperties;
  int FullImageSize[2];
  int ReducedImageSize[2];

  svtkUnsignedCharArray* FullImage;
  svtkUnsignedCharArray* ReducedImage;

  int FullImageUpToDate;
  int ReducedImageUpToDate;
  int RenderWindowImageUpToDate;

  svtkDoubleArray* Viewports;

  int Lock;
  int ParallelRendering;
  int RenderEventPropagation;
  int UseCompositing;
  int SyncRenderWindowRenderers;

  svtkTimerLog* Timer;

  double RenderTime;
  double ImageProcessingTime;

  /**
   * Used by SetImageReductionFactorForUpdateRate to smooth transitions
   * transitions between image reduction factors.
   */
  double AverageTimePerPixel;

  /**
   * Used to synchronize rendering information per frame.
   * These are old methods provided for backward compatibility. One should look
   * at using CollectWindowInformation(), ProcessWindowInformation() etc. for
   * bufferred sending of information over.
   */
  virtual void SendWindowInformation() {}
  virtual void ReceiveWindowInformation() {}
  virtual void SendRendererInformation(svtkRenderer*) {}
  virtual void ReceiveRendererInformation(svtkRenderer*) {}

  /**
   * Subclass should override these methods (instead of
   * SendWindowInformation/ReceiveWindowInformation or
   * SendRendererInformation/ReceiveRendererInformation) to collect or process
   * meta-data to synchronize rendering information per frame.
   * Subclass should not use the Controller directly to send receive messages
   * in any of these methods otherwise deadlocks may ensue.
   */
  virtual void CollectWindowInformation(svtkMultiProcessStream&) {}
  virtual bool ProcessWindowInformation(svtkMultiProcessStream&) { return true; }
  virtual void CollectRendererInformation(svtkRenderer*, svtkMultiProcessStream&) {}
  virtual bool ProcessRendererInformation(svtkRenderer*, svtkMultiProcessStream&) { return true; }

  //@{
  /**
   * Here is a good place to handle processing of data before and after
   * render.
   */
  virtual void PreRenderProcessing() = 0;
  virtual void PostRenderProcessing() = 0;
  //@}

  /**
   * Called in satellites to set the render window size to the current
   * FullImageSize and ReducedImageSize (or vice versa).
   */
  virtual void SetRenderWindowSize();

  /**
   * Called by ComputeVisiblePropBoundsRMI to get the bounds of a local
   * renderer.  Override this method if the true bounds are different than
   * those reported by the renderer.
   */
  virtual void LocalComputeVisiblePropBounds(svtkRenderer* ren, double bounds[6]);

  /**
   * When called, fills FullImage.
   */
  virtual void MagnifyReducedImage();

  /**
   * Write the full image back to the RenderWindow.
   */
  virtual void WriteFullImage();

  /**
   * Reads in the reduced image from the RenderWindow.
   */
  virtual void ReadReducedImage();

  /**
   * Returns 1 if the RenderWindow's last image is in the front buffer, 0
   * if it is in the back.
   */
  virtual int LastRenderInFrontBuffer();

  /**
   * Select buffer to read from / render into.
   */
  virtual int ChooseBuffer();

  /**
   * Sets the current render window's pixel data.
   */
  virtual void SetRenderWindowPixelData(svtkUnsignedCharArray* pixels, const int pixelDimensions[2]);

  /**
   * Returns true if the image for the given renderer should be rendered at a
   * reduced size to be magnified later.  This method always returns true, but
   * subclasses may render some renderers at a reduced size, magnify them, and
   * then render the other renderers at full resolution.
   */
  virtual int ImageReduceRenderer(svtkRenderer*) { return 1; }

  struct RenderWindowInfo
  {
    int FullSize[2];
    int ReducedSize[2];
    int NumberOfRenderers;
    int UseCompositing;
    int TileScale[2];
    double ImageReductionFactor;
    double DesiredUpdateRate;
    double TileViewport[4];

    // Initialize members
    RenderWindowInfo() {}

    // Save/restore the struct to/from a stream.
    void Save(svtkMultiProcessStream& stream);
    bool Restore(svtkMultiProcessStream& stream);
  };

  struct RendererInfo
  {
    int Draw;
    int NumberOfLights;
    double Viewport[4];
    double CameraPosition[3];
    double CameraFocalPoint[3];
    double CameraViewUp[3];
    double WindowCenter[2];
    double CameraClippingRange[2];
    double CameraViewAngle;
    double Background[3];
    double Background2[3];
    bool GradientBackground;

    double ParallelScale;

    // Initialize members
    RendererInfo() {}

    // Save/restore the struct to/from a stream.
    void Save(svtkMultiProcessStream& stream);
    bool Restore(svtkMultiProcessStream& stream);
  };

  struct LightInfo
  {
    double Position[3];
    double FocalPoint[3];
    double Type;

    // Initialize members
    LightInfo() {}

    // Save/restore the struct to/from a stream.
    void Save(svtkMultiProcessStream& stream);
    bool Restore(svtkMultiProcessStream& stream);
  };

  int AddedRMIs;
  unsigned long RenderRMIId;
  unsigned long BoundsRMIId;
  int UseBackBuffer;

  static bool DefaultRenderEventPropagation;

private:
  svtkParallelRenderManager(const svtkParallelRenderManager&) = delete;
  void operator=(const svtkParallelRenderManager&) = delete;
};

#endif // svtkParalleRenderManager_h
