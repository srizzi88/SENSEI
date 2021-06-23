/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageShrink3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageShrink3D
 * @brief   Subsamples an image.
 *
 * svtkImageShrink3D shrinks an image by sub sampling on a
 * uniform grid (integer multiples).
 */

#ifndef svtkImageShrink3D_h
#define svtkImageShrink3D_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageShrink3D : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageShrink3D* New();
  svtkTypeMacro(svtkImageShrink3D, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the shrink factors
   */
  svtkSetVector3Macro(ShrinkFactors, int);
  svtkGetVector3Macro(ShrinkFactors, int);
  //@}

  //@{
  /**
   * Set/Get the pixel to use as origin.
   */
  svtkSetVector3Macro(Shift, int);
  svtkGetVector3Macro(Shift, int);
  //@}

  //@{
  /**
   * Choose Mean, Minimum, Maximum, Median or sub sampling.
   * The neighborhood operations are not centered on the sampled pixel.
   * This may cause a half pixel shift in your output image.
   * You can changed "Shift" to get around this.
   * svtkImageGaussianSmooth or svtkImageMean with strides.
   */
  void SetAveraging(svtkTypeBool);
  svtkTypeBool GetAveraging() { return this->GetMean(); }
  svtkBooleanMacro(Averaging, svtkTypeBool);
  //@}

  void SetMean(svtkTypeBool);
  svtkGetMacro(Mean, svtkTypeBool);
  svtkBooleanMacro(Mean, svtkTypeBool);

  void SetMinimum(svtkTypeBool);
  svtkGetMacro(Minimum, svtkTypeBool);
  svtkBooleanMacro(Minimum, svtkTypeBool);

  void SetMaximum(svtkTypeBool);
  svtkGetMacro(Maximum, svtkTypeBool);
  svtkBooleanMacro(Maximum, svtkTypeBool);

  void SetMedian(svtkTypeBool);
  svtkGetMacro(Median, svtkTypeBool);
  svtkBooleanMacro(Median, svtkTypeBool);

protected:
  svtkImageShrink3D();
  ~svtkImageShrink3D() override {}

  int ShrinkFactors[3];
  int Shift[3];
  int Mean;
  svtkTypeBool Minimum;
  svtkTypeBool Maximum;
  svtkTypeBool Median;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

  void InternalRequestUpdateExtent(int* inExt, int* outExt);

private:
  svtkImageShrink3D(const svtkImageShrink3D&) = delete;
  void operator=(const svtkImageShrink3D&) = delete;
};

#endif
