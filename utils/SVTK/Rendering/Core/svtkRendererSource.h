/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRendererSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRendererSource
 * @brief   take a renderer's image and/or depth map into the pipeline
 *
 *
 * svtkRendererSource is a source object whose input is a renderer's image
 * and/or depth map, which is then used to produce an output image. This
 * output can then be used in the visualization pipeline. You must explicitly
 * send a Modify() to this object to get it to reload its data from the
 * renderer. Consider also using svtkWindowToImageFilter instead of this
 * class.
 *
 * By default, the data placed into the output is the renderer's image RGB
 * values (these color scalars are represented by unsigned chars, one per
 * color channel). Optionally, you can also grab the image depth (e.g.,
 * z-buffer) values, and include it in the output in one of three ways. 1)
 * First, when the data member DepthValues is enabled, a separate float array
 * of these depth values is included in the output point data with array name
 * "ZBuffer". 2) If DepthValuesInScalars is enabled, then the z-buffer values
 * are shifted and scaled to fit into an unsigned char and included in the
 * output image (so the output image pixels are four components RGBZ). Note
 * that DepthValues and and DepthValuesInScalars can be enabled
 * simultaneously if desired. Finally 3) if DepthValuesOnly is enabled, then
 * the output image consists only of the z-buffer values represented by a
 * single component float array; and the data members DepthValues and
 * DepthValuesInScalars are ignored.
 *
 * @sa
 * svtkWindowToImageFilter svtkRendererPointCloudSource svtkRenderer
 * svtkImageData svtkDepthImageToPointCloud
 */

#ifndef svtkRendererSource_h
#define svtkRendererSource_h

#include "svtkAlgorithm.h"
#include "svtkImageData.h"           // makes things a bit easier
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;

class SVTKRENDERINGCORE_EXPORT svtkRendererSource : public svtkAlgorithm
{
public:
  static svtkRendererSource* New();
  svtkTypeMacro(svtkRendererSource, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the MTime also considering the Renderer.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Indicates what renderer to get the pixel data from.
   */
  void SetInput(svtkRenderer*);

  //@{
  /**
   * Returns which renderer is being used as the source for the pixel data.
   */
  svtkGetObjectMacro(Input, svtkRenderer);
  //@}

  //@{
  /**
   * Use the entire RenderWindow as a data source or just the Renderer.
   * The default is zero, just the Renderer.
   */
  svtkSetMacro(WholeWindow, svtkTypeBool);
  svtkGetMacro(WholeWindow, svtkTypeBool);
  svtkBooleanMacro(WholeWindow, svtkTypeBool);
  //@}

  //@{
  /**
   * If this flag is on, then filter execution causes a render first.
   */
  svtkSetMacro(RenderFlag, svtkTypeBool);
  svtkGetMacro(RenderFlag, svtkTypeBool);
  svtkBooleanMacro(RenderFlag, svtkTypeBool);
  //@}

  //@{
  /**
   * A boolean value to control whether to grab z-buffer
   * (i.e., depth values) along with the image data. The z-buffer data
   * is placed into a field data attributes named "ZBuffer" .
   */
  svtkSetMacro(DepthValues, svtkTypeBool);
  svtkGetMacro(DepthValues, svtkTypeBool);
  svtkBooleanMacro(DepthValues, svtkTypeBool);
  //@}

  //@{
  /**
   * A boolean value to control whether to grab z-buffer
   * (i.e., depth values) along with the image data. The z-buffer data
   * is placed in the scalars as a fourth Z component (shift and scaled
   * to map the full 0..255 range).
   */
  svtkSetMacro(DepthValuesInScalars, svtkTypeBool);
  svtkGetMacro(DepthValuesInScalars, svtkTypeBool);
  svtkBooleanMacro(DepthValuesInScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * A boolean value to control whether to grab only the z-buffer (i.e.,
   * depth values) without the associated image (color scalars) data. If
   * enabled, the output data contains only a depth image which is the
   * z-buffer values represented by float values. By default, this is
   * disabled. Note that if enabled, then the DepthValues and
   * DepthValuesInScalars are ignored.
   */
  svtkSetMacro(DepthValuesOnly, svtkTypeBool);
  svtkGetMacro(DepthValuesOnly, svtkTypeBool);
  svtkBooleanMacro(DepthValuesOnly, svtkTypeBool);
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
  svtkRendererSource();
  ~svtkRendererSource() override;

  void RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);
  virtual void RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  svtkRenderer* Input;
  svtkTypeBool WholeWindow;
  svtkTypeBool RenderFlag;
  svtkTypeBool DepthValues;
  svtkTypeBool DepthValuesInScalars;
  svtkTypeBool DepthValuesOnly;

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkRendererSource(const svtkRendererSource&) = delete;
  void operator=(const svtkRendererSource&) = delete;
};

#endif
