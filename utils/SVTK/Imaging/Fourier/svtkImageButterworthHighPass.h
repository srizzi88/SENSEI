/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageButterworthHighPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageButterworthHighPass
 * @brief   Frequency domain high pass.
 *
 * This filter only works on an image after it has been converted to
 * frequency domain by a svtkImageFFT filter.  A svtkImageRFFT filter
 * can be used to convert the output back into the spatial domain.
 * svtkImageButterworthHighPass the frequency components around 0 are
 * attenuated.  Input and output are in doubles, with two components
 * (complex numbers).
 * out(i, j) = 1 / (1 + pow(CutOff/Freq(i,j), 2*Order));
 *
 * @sa
 * svtkImageButterworthLowPass
 */

#ifndef svtkImageButterworthHighPass_h
#define svtkImageButterworthHighPass_h

#include "svtkImagingFourierModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGFOURIER_EXPORT svtkImageButterworthHighPass : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageButterworthHighPass* New();
  svtkTypeMacro(svtkImageButterworthHighPass, svtkThreadedImageAlgorithm);
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

  //@{
  /**
   * The order determines sharpness of the cutoff curve.
   */
  svtkSetMacro(Order, int);
  svtkGetMacro(Order, int);
  //@}

protected:
  svtkImageButterworthHighPass();
  ~svtkImageButterworthHighPass() override {}

  int Order;
  double CutOff[3];

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int outExt[6], int id) override;

private:
  svtkImageButterworthHighPass(const svtkImageButterworthHighPass&) = delete;
  void operator=(const svtkImageButterworthHighPass&) = delete;
};

#endif
