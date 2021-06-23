/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSlabReslice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSlabReslice
 * @brief   Thick slab reformat through data.
 *
 * This class derives from svtkImageResliceBase. Much like svtkImageReslice, it
 * reslices the data. It is multi-threaded. It takes a three dimensional image
 * as input and produces a two dimensional thick MPR along some direction.
 * <p> The class reslices the thick slab using a blending function. Supported
 * blending functions are Minimum Intensity blend through the slab, maximum
 * intensity blend and a Mean (average) intensity of values across the slab.
 * <p> The user can adjust the thickness of the slab by using the method
 * SetSlabThickness. The distance between sample points used for blending
 * across the thickness of the slab is controlled by the method
 * SetSlabResolution. These two methods determine the number of slices used
 * across the slab for blending, which is computed as
 * {(2 x (int)(0.5 x SlabThickness/SlabResolution)) + 1}. This value may
 * be queried via GetNumBlendSamplePoints() and is always >= 1.
 * <p> Much like svtkImageReslice, the reslice axes direction cosines may be
 * set via the methods SetResliceAxes or SetResliceAxesDirectionCosines. The
 * output spacing is controlled by SetOutputSpacing and the output origin is
 * controlled by SetOutputOrigin. The default value to be set on pixels that
 * lie outside the volume when reformatting is controlled by
 * SetBackgroundColor or SetBackgroundLevel. The SetResliceAxesOrigin()
 * method can also be used to provide an (x,y,z) point that the slice will
 * pass through.
 * @sa
 * svtkImageReslice
 */

#ifndef svtkImageSlabReslice_h
#define svtkImageSlabReslice_h

#include "svtkImageReslice.h"
#include "svtkImagingGeneralModule.h" // For export macro

class SVTKIMAGINGGENERAL_EXPORT svtkImageSlabReslice : public svtkImageReslice
{
public:
  static svtkImageSlabReslice* New();
  svtkTypeMacro(svtkImageSlabReslice, svtkImageReslice);

  /**
   * Printself method.
   */
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the blend mode. Default is MIP (ie Max)
   */
  svtkSetMacro(BlendMode, int);
  svtkGetMacro(BlendMode, int);
  void SetBlendModeToMin() { this->SetBlendMode(SVTK_IMAGE_SLAB_MIN); }
  void SetBlendModeToMax() { this->SetBlendMode(SVTK_IMAGE_SLAB_MAX); }
  void SetBlendModeToMean() { this->SetBlendMode(SVTK_IMAGE_SLAB_MEAN); }
  //@}

  //@{
  /**
   * Number of sample points used across the slab cross-section. If equal to
   * 1, this ends up being a thin reslice through the data a.k.a.
   * svtkImageReslice
   */
  svtkGetMacro(NumBlendSamplePoints, int);
  //@}

  //@{
  /**
   * SlabThickness of slab in world coords. SlabThickness must be non-zero and
   * positive.
   */
  svtkSetMacro(SlabThickness, double);
  svtkGetMacro(SlabThickness, double);
  //@}

  //@{
  /**
   * Spacing between slabs in world units. (Number of Slices, ie samples to
   * blend is computed from SlabThickness and SlabResolution).
   */
  svtkSetMacro(SlabResolution, double);
  svtkGetMacro(SlabResolution, double);
  //@}

protected:
  svtkImageSlabReslice();
  ~svtkImageSlabReslice() override;

  /**
   * This method simply calls the superclass method. In addition, it also
   * precomputes the NumBlendSamplePoints based on the SlabThickness and
   * SlabResolution.
   */
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int BlendMode; // can be MIN, MIP, MAX
  double SlabThickness;
  double SlabResolution;
  int NumBlendSamplePoints;

private:
  svtkImageSlabReslice(const svtkImageSlabReslice&) = delete;
  void operator=(const svtkImageSlabReslice&) = delete;
};

#endif
