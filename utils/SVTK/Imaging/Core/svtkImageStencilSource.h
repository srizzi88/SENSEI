/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStencilSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageStencilSource
 * @brief   generate an image stencil
 *
 * svtkImageStencilSource is a superclass for filters that generate image
 * stencils.  Given a clipping object such as a svtkImplicitFunction, it
 * will set up a list of clipping extents for each x-row through the
 * image data.  The extents for each x-row can be retrieved via the
 * GetNextExtent() method after the extent lists have been built
 * with the BuildExtents() method.  For large images, using clipping
 * extents is much more memory efficient (and slightly more time-efficient)
 * than building a mask.  This class can be subclassed to allow clipping
 * with objects other than svtkImplicitFunction.
 * @sa
 * svtkImplicitFunction svtkImageStencil svtkPolyDataToImageStencil
 */

#ifndef svtkImageStencilSource_h
#define svtkImageStencilSource_h

#include "svtkImageStencilAlgorithm.h"
#include "svtkImagingCoreModule.h" // For export macro

class svtkImageStencilData;
class svtkImageData;

class SVTKIMAGINGCORE_EXPORT svtkImageStencilSource : public svtkImageStencilAlgorithm
{
public:
  static svtkImageStencilSource* New();
  svtkTypeMacro(svtkImageStencilSource, svtkImageStencilAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set a svtkImageData that has the Spacing, Origin, and
   * WholeExtent that will be used for the stencil.  This
   * input should be set to the image that you wish to
   * apply the stencil to.  If you use this method, then
   * any values set with the SetOutputSpacing, SetOutputOrigin,
   * and SetOutputWholeExtent methods will be ignored.
   */
  virtual void SetInformationInput(svtkImageData*);
  svtkGetObjectMacro(InformationInput, svtkImageData);
  //@}

  //@{
  /**
   * Set the Origin to be used for the stencil.  It should be
   * set to the Origin of the image you intend to apply the
   * stencil to. The default value is (0,0,0).
   */
  svtkSetVector3Macro(OutputOrigin, double);
  svtkGetVector3Macro(OutputOrigin, double);
  //@}

  //@{
  /**
   * Set the Spacing to be used for the stencil. It should be
   * set to the Spacing of the image you intend to apply the
   * stencil to. The default value is (1,1,1)
   */
  svtkSetVector3Macro(OutputSpacing, double);
  svtkGetVector3Macro(OutputSpacing, double);
  //@}

  //@{
  /**
   * Set the whole extent for the stencil (anything outside
   * this extent will be considered to be "outside" the stencil).
   */
  svtkSetVector6Macro(OutputWholeExtent, int);
  svtkGetVector6Macro(OutputWholeExtent, int);
  //@}

  /**
   * Report object referenced by instances of this class.
   */
  void ReportReferences(svtkGarbageCollector*) override;

protected:
  svtkImageStencilSource();
  ~svtkImageStencilSource() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkImageData* InformationInput;

  int OutputWholeExtent[6];
  double OutputOrigin[3];
  double OutputSpacing[3];

private:
  svtkImageStencilSource(const svtkImageStencilSource&) = delete;
  void operator=(const svtkImageStencilSource&) = delete;
};

#endif
