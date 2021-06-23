/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMirrorPad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMirrorPad
 * @brief   Extra pixels are filled by mirror images.
 *
 * svtkImageMirrorPad makes an image larger by filling extra pixels with
 * a mirror image of the original image (mirror at image boundaries).
 */

#ifndef svtkImageMirrorPad_h
#define svtkImageMirrorPad_h

#include "svtkImagePadFilter.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImageMirrorPad : public svtkImagePadFilter
{
public:
  static svtkImageMirrorPad* New();
  svtkTypeMacro(svtkImageMirrorPad, svtkImagePadFilter);

protected:
  svtkImageMirrorPad() {}
  ~svtkImageMirrorPad() override {}

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int wExt[6]) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

private:
  svtkImageMirrorPad(const svtkImageMirrorPad&) = delete;
  void operator=(const svtkImageMirrorPad&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageMirrorPad.h
