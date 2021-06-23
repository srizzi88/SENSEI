/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageFourierCenter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageFourierCenter
 * @brief   Shifts constant frequency to center for
 * display.
 *
 * Is used for dispaying images in frequency space.  FFT converts spatial
 * images into frequency space, but puts the zero frequency at the origin.
 * This filter shifts the zero frequency to the center of the image.
 * Input and output are assumed to be doubles.
 */

#ifndef svtkImageFourierCenter_h
#define svtkImageFourierCenter_h

#include "svtkImageDecomposeFilter.h"
#include "svtkImagingFourierModule.h" // For export macro

class SVTKIMAGINGFOURIER_EXPORT svtkImageFourierCenter : public svtkImageDecomposeFilter
{
public:
  static svtkImageFourierCenter* New();
  svtkTypeMacro(svtkImageFourierCenter, svtkImageDecomposeFilter);

protected:
  svtkImageFourierCenter();
  ~svtkImageFourierCenter() override {}

  int IterativeRequestUpdateExtent(svtkInformation* in, svtkInformation* out) override;

  void ThreadedRequestData(svtkInformation* svtkNotUsed(request),
    svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector,
    svtkImageData*** inDataVec, svtkImageData** outDataVec, int outExt[6], int threadId) override;

private:
  svtkImageFourierCenter(const svtkImageFourierCenter&) = delete;
  void operator=(const svtkImageFourierCenter&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageFourierCenter.h
