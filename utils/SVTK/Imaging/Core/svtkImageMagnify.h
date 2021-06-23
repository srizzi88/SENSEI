/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMagnify.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMagnify
 * @brief   magnify an image by an integer value
 *
 * svtkImageMagnify maps each pixel of the input onto a nxmx... region
 * of the output.  Location (0,0,...) remains in the same place. The
 * magnification occurs via pixel replication, or if Interpolate is on,
 * by bilinear interpolation. Initially, interpolation is off and magnification
 * factors are set to 1 in all directions.
 */

#ifndef svtkImageMagnify_h
#define svtkImageMagnify_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageMagnify : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageMagnify* New();
  svtkTypeMacro(svtkImageMagnify, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the integer magnification factors in the i-j-k directions.
   * Initially, factors are set to 1 in all directions.
   */
  svtkSetVector3Macro(MagnificationFactors, int);
  svtkGetVector3Macro(MagnificationFactors, int);
  //@}

  //@{
  /**
   * Turn interpolation on and off (pixel replication is used when off).
   * Initially, interpolation is off.
   */
  svtkSetMacro(Interpolate, svtkTypeBool);
  svtkGetMacro(Interpolate, svtkTypeBool);
  svtkBooleanMacro(Interpolate, svtkTypeBool);
  //@}

protected:
  svtkImageMagnify();
  ~svtkImageMagnify() override {}

  int MagnificationFactors[3];
  svtkTypeBool Interpolate;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int outExt[6], int id) override;

  void InternalRequestUpdateExtent(int* inExt, int* outExt);

private:
  svtkImageMagnify(const svtkImageMagnify&) = delete;
  void operator=(const svtkImageMagnify&) = delete;
};

#endif
