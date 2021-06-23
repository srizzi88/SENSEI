/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMask.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMask
 * @brief   Combines a mask and an image.
 *
 * svtkImageMask combines a mask with an image.  Non zero mask
 * implies the output pixel will be the same as the image.
 * If a mask pixel is zero,  then the output pixel
 * is set to "MaskedValue".  The filter also has the option to pass
 * the mask through a boolean not operation before processing the image.
 * This reverses the passed and replaced pixels.
 * The two inputs should have the same "WholeExtent".
 * The mask input should be unsigned char, and the image scalar type
 * is the same as the output scalar type.
 */

#ifndef svtkImageMask_h
#define svtkImageMask_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageMask : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageMask* New();
  svtkTypeMacro(svtkImageMask, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * SetGet the value of the output pixel replaced by mask.
   */
  void SetMaskedOutputValue(int num, double* v);
  void SetMaskedOutputValue(double v) { this->SetMaskedOutputValue(1, &v); }
  void SetMaskedOutputValue(double v1, double v2)
  {
    double v[2];
    v[0] = v1;
    v[1] = v2;
    this->SetMaskedOutputValue(2, v);
  }
  void SetMaskedOutputValue(double v1, double v2, double v3)
  {
    double v[3];
    v[0] = v1;
    v[1] = v2;
    v[2] = v3;
    this->SetMaskedOutputValue(3, v);
  }
  double* GetMaskedOutputValue() { return this->MaskedOutputValue; }
  int GetMaskedOutputValueLength() { return this->MaskedOutputValueLength; }

  //@{
  /**
   * Set/Get the alpha blending value for the mask
   * The input image is assumed to be at alpha = 1.0
   * and the mask image uses this alpha to blend using
   * an over operator.
   */
  svtkSetClampMacro(MaskAlpha, double, 0.0, 1.0);
  svtkGetMacro(MaskAlpha, double);
  //@}

  /**
   * Set the input to be masked.
   */
  void SetImageInputData(svtkImageData* in);

  /**
   * Set the mask to be used.
   */
  void SetMaskInputData(svtkImageData* in);

  //@{
  /**
   * When Not Mask is on, the mask is passed through a boolean not
   * before it is used to mask the image.  The effect is to pass the
   * pixels where the input mask is zero, and replace the pixels
   * where the input value is non zero.
   */
  svtkSetMacro(NotMask, svtkTypeBool);
  svtkGetMacro(NotMask, svtkTypeBool);
  svtkBooleanMacro(NotMask, svtkTypeBool);
  //@}

  /**
   * Set the two inputs to this filter
   */
  virtual void SetInput1Data(svtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(svtkDataObject* in) { this->SetInputData(1, in); }

protected:
  svtkImageMask();
  ~svtkImageMask() override;

  double* MaskedOutputValue;
  int MaskedOutputValueLength;
  svtkTypeBool NotMask;
  double MaskAlpha;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int threadId) override;

private:
  svtkImageMask(const svtkImageMask&) = delete;
  void operator=(const svtkImageMask&) = delete;
};

#endif
