/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageWrapPad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageWrapPad
 * @brief   Makes an image larger by wrapping existing data.
 *
 * svtkImageWrapPad performs a modulo operation on the output pixel index
 * to determine the source input index.  The new image extent of the
 * output has to be specified.  Input has to be the same scalar type as
 * output.
 */

#ifndef svtkImageWrapPad_h
#define svtkImageWrapPad_h

#include "svtkImagePadFilter.h"
#include "svtkImagingCoreModule.h" // For export macro

class svtkInformation;
class svtkInformationVector;

class SVTKIMAGINGCORE_EXPORT svtkImageWrapPad : public svtkImagePadFilter
{
public:
  static svtkImageWrapPad* New();
  svtkTypeMacro(svtkImageWrapPad, svtkImagePadFilter);

protected:
  svtkImageWrapPad() {}
  ~svtkImageWrapPad() override {}

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int wExt[6]) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

private:
  svtkImageWrapPad(const svtkImageWrapPad&) = delete;
  void operator=(const svtkImageWrapPad&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageWrapPad.h
