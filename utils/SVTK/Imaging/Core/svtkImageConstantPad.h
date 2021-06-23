/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageConstantPad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageConstantPad
 * @brief   Makes image larger by padding with constant.
 *
 * svtkImageConstantPad changes the image extent of its input.
 * Any pixels outside of the original image extent are filled with
 * a constant value (default is 0.0).
 *
 * @sa
 * svtkImageWrapPad svtkImageMirrorPad
 */

#ifndef svtkImageConstantPad_h
#define svtkImageConstantPad_h

#include "svtkImagePadFilter.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImageConstantPad : public svtkImagePadFilter
{
public:
  static svtkImageConstantPad* New();
  svtkTypeMacro(svtkImageConstantPad, svtkImagePadFilter);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the pad value.
   */
  svtkSetMacro(Constant, double);
  svtkGetMacro(Constant, double);
  //@}

protected:
  svtkImageConstantPad();
  ~svtkImageConstantPad() override {}

  double Constant;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

private:
  svtkImageConstantPad(const svtkImageConstantPad&) = delete;
  void operator=(const svtkImageConstantPad&) = delete;
};

#endif
