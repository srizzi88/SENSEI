/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDifference.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDifference
 * @brief   Compares images for regression tests.
 *
 * svtkImageDifference takes two rgb unsigned char images and compares them.
 * It allows the images to be slightly different.  If AllowShift is on,
 * then each pixel can be shifted by two pixels. Threshold is the allowable
 * error for each pixel.
 *
 * This is a symmetric filter and the difference computed is symmetric.
 * The resulting value is the maximum error of the two directions
 * A->B and B->A
 */

#ifndef svtkImageDifference_h
#define svtkImageDifference_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class svtkImageDifferenceThreadData;
class svtkImageDifferenceSMPThreadLocal;

class SVTKIMAGINGCORE_EXPORT svtkImageDifference : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageDifference* New();
  svtkTypeMacro(svtkImageDifference, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the Image to compare the input to.
   */
  void SetImageConnection(svtkAlgorithmOutput* output) { this->SetInputConnection(1, output); }
  void SetImageData(svtkDataObject* image) { this->SetInputData(1, image); }
  svtkImageData* GetImage();
  //@}

  /**
   * Return the total error in comparing the two images.
   */
  double GetError() { return this->Error; }
  void GetError(double* e) { *e = this->GetError(); }

  /**
   * Return the total thresholded error in comparing the two images.
   * The thresholded error is the error for a given pixel minus the
   * threshold and clamped at a minimum of zero.
   */
  double GetThresholdedError() { return this->ThresholdedError; }
  void GetThresholdedError(double* e) { *e = this->GetThresholdedError(); }

  //@{
  /**
   * Specify a threshold tolerance for pixel differences.
   */
  svtkSetMacro(Threshold, int);
  svtkGetMacro(Threshold, int);
  //@}

  //@{
  /**
   * Specify whether the comparison will allow a shift of two
   * pixels between the images.  If set, then the minimum difference
   * between input images will be used to determine the difference.
   * Otherwise, the difference is computed directly between pixels
   * of identical row/column values.
   */
  svtkSetMacro(AllowShift, svtkTypeBool);
  svtkGetMacro(AllowShift, svtkTypeBool);
  svtkBooleanMacro(AllowShift, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether the comparison will include comparison of
   * averaged 3x3 data between the images. For graphics renderings
   * you normally would leave this on. For imaging operations it
   * should be off.
   */
  svtkSetMacro(Averaging, svtkTypeBool);
  svtkGetMacro(Averaging, svtkTypeBool);
  svtkBooleanMacro(Averaging, svtkTypeBool);
  //@}

  //@{
  /**
   * When doing Averaging, adjust the threshold for the average
   * by this factor. Defaults to 0.5 requiring a better match
   */
  svtkSetMacro(AverageThresholdFactor, double);
  svtkGetMacro(AverageThresholdFactor, double);
  //@}

protected:
  svtkImageDifference();
  ~svtkImageDifference() override {}

  // Parameters
  svtkTypeBool AllowShift;
  int Threshold;
  svtkTypeBool Averaging;

  // Outputs
  const char* ErrorMessage;
  double Error;
  double ThresholdedError;
  double AverageThresholdFactor;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int threadId) override;

  // Used for svtkMultiThreader operation.
  svtkImageDifferenceThreadData* ThreadData;

  // Used for svtkSMPTools operation.
  svtkImageDifferenceSMPThreadLocal* SMPThreadData;

private:
  svtkImageDifference(const svtkImageDifference&) = delete;
  void operator=(const svtkImageDifference&) = delete;

  friend class svtkImageDifferenceSMPFunctor;
};

#endif
