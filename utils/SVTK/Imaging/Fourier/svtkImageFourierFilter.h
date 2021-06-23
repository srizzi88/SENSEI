/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageFourierFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageFourierFilter
 * @brief   Superclass that implements complex numbers.
 *
 * svtkImageFourierFilter is a class of filters that use complex numbers
 * this superclass is a container for methods that manipulate these structure
 * including fast Fourier transforms.  Complex numbers may become a class.
 * This should really be a helper class.
 */

#ifndef svtkImageFourierFilter_h
#define svtkImageFourierFilter_h

#include "svtkImageDecomposeFilter.h"
#include "svtkImagingFourierModule.h" // For export macro

/*******************************************************************
                        COMPLEX number stuff
*******************************************************************/

typedef struct
{
  double Real;
  double Imag;
} svtkImageComplex;

#define svtkImageComplexEuclidSet(C, R, I)                                                          \
  (C).Real = (R);                                                                                  \
  (C).Imag = (I)

#define svtkImageComplexPolarSet(C, M, P)                                                           \
  (C).Real = (M)*cos(P);                                                                           \
  (C).Imag = (M)*sin(P)

#define svtkImageComplexPrint(C) printf("(%.3f, %.3f)", (C).Real, (C).Imag)

#define svtkImageComplexScale(cOut, S, cIn)                                                         \
  (cOut).Real = (cIn).Real * (S);                                                                  \
  (cOut).Imag = (cIn).Imag * (S)

#define svtkImageComplexConjugate(cIn, cOut)                                                        \
  (cOut).Imag = (cIn).Imag * -1.0;                                                                 \
  (cOut).Real = (cIn).Real

#define svtkImageComplexAdd(C1, C2, cOut)                                                           \
  (cOut).Real = (C1).Real + (C2).Real;                                                             \
  (cOut).Imag = (C1).Imag + (C2).Imag

#define svtkImageComplexSubtract(C1, C2, cOut)                                                      \
  (cOut).Real = (C1).Real - (C2).Real;                                                             \
  (cOut).Imag = (C1).Imag - (C2).Imag

#define svtkImageComplexMultiply(C1, C2, cOut)                                                      \
  {                                                                                                \
    svtkImageComplex _svtkImageComplexMultiplyTemp;                                                  \
    _svtkImageComplexMultiplyTemp.Real = (C1).Real * (C2).Real - (C1).Imag * (C2).Imag;             \
    _svtkImageComplexMultiplyTemp.Imag = (C1).Real * (C2).Imag + (C1).Imag * (C2).Real;             \
    cOut = _svtkImageComplexMultiplyTemp;                                                           \
  }

// This macro calculates exp(cIn) and puts the result in cOut
#define svtkImageComplexExponential(cIn, cOut)                                                      \
  {                                                                                                \
    double tmp = exp(cIn.Real);                                                                    \
    cOut.Real = tmp * cos(cIn.Imag);                                                               \
    cOut.Imag = tmp * sin(cIn.Imag);                                                               \
  }

/******************* End of COMPLEX number stuff ********************/

class SVTKIMAGINGFOURIER_EXPORT svtkImageFourierFilter : public svtkImageDecomposeFilter
{
public:
  svtkTypeMacro(svtkImageFourierFilter, svtkImageDecomposeFilter);

  // public for templated functions of this object

  /**
   * This function calculates the whole fft of an array.
   * The contents of the input array are changed.
   * (It is engineered for no decimation)
   */
  void ExecuteFft(svtkImageComplex* in, svtkImageComplex* out, int N);

  /**
   * This function calculates the whole fft of an array.
   * The contents of the input array are changed.
   * (It is engineered for no decimation)
   */
  void ExecuteRfft(svtkImageComplex* in, svtkImageComplex* out, int N);

protected:
  svtkImageFourierFilter() {}
  ~svtkImageFourierFilter() override {}

  void ExecuteFftStep2(svtkImageComplex* p_in, svtkImageComplex* p_out, int N, int bsize, int fb);
  void ExecuteFftStepN(
    svtkImageComplex* p_in, svtkImageComplex* p_out, int N, int bsize, int n, int fb);
  void ExecuteFftForwardBackward(svtkImageComplex* in, svtkImageComplex* out, int N, int fb);

  /**
   * Override to change extent splitting rules.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageFourierFilter(const svtkImageFourierFilter&) = delete;
  void operator=(const svtkImageFourierFilter&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageFourierFilter.h
