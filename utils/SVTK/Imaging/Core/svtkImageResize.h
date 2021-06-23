/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageResize.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageResize
 * @brief   High-quality image resizing filter
 *
 * svtkImageResize will magnify or shrink an image with interpolation and
 * antialiasing.  The resizing is done with a 5-lobe Lanczos-windowed sinc
 * filter that is bandlimited to the output sampling frequency in order to
 * avoid aliasing when the image size is reduced.  This filter utilizes a
 * O(n) algorithm to provide good efficiency even though the filtering kernel
 * is large.  The sinc interpolator can be turned off if nearest-neighbor
 * interpolation is required, or it can be replaced with a different
 * svtkImageInterpolator object.
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to SVTK.
 */

#ifndef svtkImageResize_h
#define svtkImageResize_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class svtkAbstractImageInterpolator;

class SVTKIMAGINGCORE_EXPORT svtkImageResize : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageResize* New();
  svtkTypeMacro(svtkImageResize, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    OUTPUT_DIMENSIONS,
    OUTPUT_SPACING,
    MAGNIFICATION_FACTORS
  };

  //@{
  /**
   * The resizing method to use.  The default is to set the output image
   * dimensions, and allow the filter to resize the image to these new
   * dimensions.  It is also possible to resize the image by setting the
   * output image spacing or by setting a magnification factor.
   */
  svtkSetClampMacro(ResizeMethod, int, OUTPUT_DIMENSIONS, MAGNIFICATION_FACTORS);
  svtkGetMacro(ResizeMethod, int);
  void SetResizeMethodToOutputDimensions() { this->SetResizeMethod(OUTPUT_DIMENSIONS); }
  void SetResizeMethodToOutputSpacing() { this->SetResizeMethod(OUTPUT_SPACING); }
  void SetResizeMethodToMagnificationFactors() { this->SetResizeMethod(MAGNIFICATION_FACTORS); }
  virtual const char* GetResizeMethodAsString();
  //@}

  //@{
  /**
   * The desired output dimensions.  This is only used if the ResizeMethod is
   * set to OutputDimensions.  If you want to keep one of the image dimensions
   * the same as the input, then set that dimension to -1.
   */
  svtkSetVector3Macro(OutputDimensions, int);
  svtkGetVector3Macro(OutputDimensions, int);
  //@}

  //@{
  /**
   * The desired output spacing.  This is only used if the ResizeMethod is
   * set to OutputSpacing.  If you want to keep one of the original spacing
   * values, then set that spacing value to zero.
   */
  svtkSetVector3Macro(OutputSpacing, double);
  svtkGetVector3Macro(OutputSpacing, double);
  //@}

  //@{
  /**
   * The desired magnification factor, meaning that the sample spacing will
   * be reduced by this factor.  This setting is only used if the ResizeMethod
   * is set to MagnificationFactors.
   */
  svtkSetVector3Macro(MagnificationFactors, double);
  svtkGetVector3Macro(MagnificationFactors, double);
  //@}

  //@{
  /**
   * If Border is Off (the default), then the centers of each of the corner
   * voxels will be considered to form the rectangular bounds of the image.
   * This is the way that SVTK normally computes image bounds.  If Border is On,
   * then the image bounds will be defined by the outer corners of the voxels.
   * This setting impacts how the resizing is done.  For example, if a
   * MagnificationFactor of two is applied to a 256x256 image, the output
   * image will be 512x512 if Border is On, or 511x511 if Border is Off.
   */
  svtkSetMacro(Border, svtkTypeBool);
  svtkBooleanMacro(Border, svtkTypeBool);
  svtkGetMacro(Border, svtkTypeBool);
  //@}

  //@{
  /**
   * Whether to crop the input image before resizing (Off by default).  If this
   * is On, then the CroppingRegion must be set.
   */
  svtkSetMacro(Cropping, svtkTypeBool);
  svtkBooleanMacro(Cropping, svtkTypeBool);
  svtkGetMacro(Cropping, svtkTypeBool);
  //@}

  //@{
  /**
   * If Cropping is On, then the CroppingRegion will be used to crop the image
   * before it is resized.  The region must be specified in data coordinates,
   * rather than voxel indices.
   */
  svtkSetVector6Macro(CroppingRegion, double);
  svtkGetVector6Macro(CroppingRegion, double);
  //@}

  //@{
  /**
   * Turn interpolation on or off (by default, interpolation is on).
   */
  svtkSetMacro(Interpolate, svtkTypeBool);
  svtkBooleanMacro(Interpolate, svtkTypeBool);
  svtkGetMacro(Interpolate, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the interpolator for resampling the data.
   */
  virtual void SetInterpolator(svtkAbstractImageInterpolator* sampler);
  virtual svtkAbstractImageInterpolator* GetInterpolator();
  //@}

  /**
   * Get the modified time of the filter.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkImageResize();
  ~svtkImageResize() override;

  virtual svtkAbstractImageInterpolator* GetInternalInterpolator();

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

  int ResizeMethod;
  int OutputDimensions[3];
  double OutputSpacing[3];
  double MagnificationFactors[3];
  svtkTypeBool Border;
  svtkTypeBool Cropping;
  double CroppingRegion[6];

  double IndexStretch[3];
  double IndexTranslate[3];

  svtkAbstractImageInterpolator* Interpolator;
  svtkAbstractImageInterpolator* NNInterpolator;
  svtkTypeBool Interpolate;

private:
  svtkImageResize(const svtkImageResize&) = delete;
  void operator=(const svtkImageResize&) = delete;
};

#endif
