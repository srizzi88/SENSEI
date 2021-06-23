/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWindowToImageFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWindowToImageFilter
 * @brief   Use a svtkWindow as input to image pipeline
 *
 * svtkWindowToImageFilter provides methods needed to read the data in
 * a svtkWindow and use it as input to the imaging pipeline. This is
 * useful for saving an image to a file for example. The window can
 * be read as either RGB or RGBA pixels;  in addition, the depth buffer
 * can also be read.   RGB and RGBA pixels are of type unsigned char,
 * while Z-Buffer data is returned as floats.  Use this filter
 * to convert RenderWindows or ImageWindows to an image format.
 *
 * @warning
 * A svtkWindow doesn't behave like other parts of the SVTK pipeline: its
 * modification time doesn't get updated when an image is rendered.  As a
 * result, naive use of svtkWindowToImageFilter will produce an image of
 * the first image that the window rendered, but which is never updated
 * on subsequent window updates.  This behavior is unexpected and in
 * general undesirable.
 *
 * @warning
 * To force an update of the output image, call svtkWindowToImageFilter's
 * Modified method after rendering to the window.
 *
 * @warning
 * In SVTK versions 4 and later, this filter is part of the canonical
 * way to output an image of a window to a file (replacing the
 * obsolete SaveImageAsPPM method for svtkRenderWindows that existed in
 * 3.2 and earlier).  Connect this filter to the output of the window,
 * and filter's output to a writer such as svtkPNGWriter.
 *
 * @warning
 * Reading back alpha planes is dependent on the correct operation of
 * the render window's GetRGBACharPixelData method, which in turn is
 * dependent on the configuration of the window's alpha planes.  As of
 * SVTK 4.4+, machine-independent behavior is not automatically
 * assured because of these dependencies.
 *
 * @sa
 * svtkRendererSource svtkRendererPointCloudSource svtkWindow
 * svtkRenderLargeImage
 */

#ifndef svtkWindowToImageFilter_h
#define svtkWindowToImageFilter_h

#include "svtkAlgorithm.h"
#include "svtkImageData.h"           // makes things a bit easier
#include "svtkRenderingCoreModule.h" // For export macro

// SVTK_RGB and SVTK_RGBA are defined in system includes
#define SVTK_ZBUFFER 5

class svtkWindow;

class svtkWTI2DHelperClass;
class SVTKRENDERINGCORE_EXPORT svtkWindowToImageFilter : public svtkAlgorithm
{
public:
  static svtkWindowToImageFilter* New();

  svtkTypeMacro(svtkWindowToImageFilter, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Indicates what renderer to get the pixel data from. Initial value is 0.
   */
  void SetInput(svtkWindow* input);

  //@{
  /**
   * Returns which renderer is being used as the source for the pixel data.
   * Initial value is 0.
   */
  svtkGetObjectMacro(Input, svtkWindow);
  //@}

  //@{
  /**
   * Get/Set the scale (or magnification) factors in X and Y.
   */
  svtkSetVector2Macro(Scale, int);
  svtkGetVector2Macro(Scale, int);
  //@}

  /**
   * Convenience method to set same scale factors for x and y.
   * i.e. same as calling this->SetScale(scale, scale).
   */
  void SetScale(int scale) { this->SetScale(scale, scale); }

  //@{
  /**
   * When scale factor > 1, this class render the full image in tiles.
   * Sometimes that results in artificial artifacts at internal tile seams.
   * To overcome this issue, set this flag to true.
   */
  svtkSetMacro(FixBoundary, bool);
  svtkGetMacro(FixBoundary, bool);
  svtkBooleanMacro(FixBoundary, bool);
  //@}

  //@{
  /**
   * Set/Get the flag that determines which buffer to read from.
   * The default is to read from the front buffer.
   */
  svtkBooleanMacro(ReadFrontBuffer, svtkTypeBool);
  svtkGetMacro(ReadFrontBuffer, svtkTypeBool);
  svtkSetMacro(ReadFrontBuffer, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get whether to re-render the input window. Initial value is true.
   * (This option makes no difference if scale factor > 1.)
   */
  svtkBooleanMacro(ShouldRerender, svtkTypeBool);
  svtkSetMacro(ShouldRerender, svtkTypeBool);
  svtkGetMacro(ShouldRerender, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the extents to be used to generate the image. Initial value is
   * {0,0,1,1} (This option does not work if scale factor > 1.)
   */
  void SetViewport(double, double, double, double);
  void SetViewport(double*);
  svtkGetVectorMacro(Viewport, double, 4);
  //@}

  //@{
  /**
   * Set/get the window buffer from which data will be read.  Choices
   * include SVTK_RGB (read the color image from the window), SVTK_RGBA
   * (same, but include the alpha channel), and SVTK_ZBUFFER (depth
   * buffer, returned as a float array). Initial value is SVTK_RGB.
   */
  svtkSetMacro(InputBufferType, int);
  svtkGetMacro(InputBufferType, int);
  void SetInputBufferTypeToRGB() { this->SetInputBufferType(SVTK_RGB); }
  void SetInputBufferTypeToRGBA() { this->SetInputBufferType(SVTK_RGBA); }
  void SetInputBufferTypeToZBuffer() { this->SetInputBufferType(SVTK_ZBUFFER); }
  //@}

  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkImageData* GetOutput();

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkWindowToImageFilter();
  ~svtkWindowToImageFilter() override;

  // svtkWindow is not a svtkDataObject, so we need our own ivar.
  svtkWindow* Input;
  int Scale[2];
  svtkTypeBool ReadFrontBuffer;
  svtkTypeBool ShouldRerender;
  double Viewport[4];
  int InputBufferType;
  bool FixBoundary;

  void RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  virtual void RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Allows subclasses to customize how a request for render is handled.
   * Default implementation checks if the render window has an interactor, if
   * so, call interactor->Render(). If not, then renderWindow->Render() is
   * called. Note, this may be called even when this->ShouldRerender is false,
   * e.g. when saving images Scale > 1.
   */
  virtual void Render();

  // The following was extracted from svtkRenderLargeImage, and patch to handle viewports
  void Rescale2DActors();
  void Shift2DActors(int x, int y);
  void Restore2DActors();
  svtkWTI2DHelperClass* StoredData;

private:
  svtkWindowToImageFilter(const svtkWindowToImageFilter&) = delete;
  void operator=(const svtkWindowToImageFilter&) = delete;
};

#endif
