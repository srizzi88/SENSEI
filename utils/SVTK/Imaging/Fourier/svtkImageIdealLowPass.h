/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageIdealLowPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageIdealLowPass
 * @brief   Simple frequency domain band pass.
 *
 * This filter only works on an image after it has been converted to
 * frequency domain by a svtkImageFFT filter.  A svtkImageRFFT filter
 * can be used to convert the output back into the spatial domain.
 * svtkImageIdealLowPass just sets a portion of the image to zero.  The result
 * is an image with a lot of ringing.  Input and Output must be doubles.
 * Dimensionality is set when the axes are set.  Defaults to 2D on X and Y
 * axes.
 *
 * @sa
 * svtkImageButterworthLowPass svtkImageIdealHighPass svtkImageFFT svtkImageRFFT
 */

#ifndef svtkImageIdealLowPass_h
#define svtkImageIdealLowPass_h

#include "svtkImagingFourierModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGFOURIER_EXPORT svtkImageIdealLowPass : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageIdealLowPass* New();
  svtkTypeMacro(svtkImageIdealLowPass, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the cutoff frequency for each axis.
   * The values are specified in the order X, Y, Z, Time.
   * Units: Cycles per world unit (as defined by the data spacing).
   */
  svtkSetVector3Macro(CutOff, double);
  void SetCutOff(double v) { this->SetCutOff(v, v, v); }
  void SetXCutOff(double v);
  void SetYCutOff(double v);
  void SetZCutOff(double v);
  svtkGetVector3Macro(CutOff, double);
  double GetXCutOff() { return this->CutOff[0]; }
  double GetYCutOff() { return this->CutOff[1]; }
  double GetZCutOff() { return this->CutOff[2]; }
  //@}

protected:
  svtkImageIdealLowPass();
  ~svtkImageIdealLowPass() override {}

  double CutOff[3];

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int outExt[6], int id) override;

private:
  svtkImageIdealLowPass(const svtkImageIdealLowPass&) = delete;
  void operator=(const svtkImageIdealLowPass&) = delete;
};

#endif
