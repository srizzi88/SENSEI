/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageToImageStencil
 * @brief   clip an image with a mask image
 *
 * svtkImageToImageStencil will convert a svtkImageData into an stencil
 * that can be used with svtkImageStecil or other svtk classes that apply
 * a stencil to an image.
 * @sa
 * svtkImageStencil svtkImplicitFunctionToImageStencil svtkPolyDataToImageStencil
 */

#ifndef svtkImageToImageStencil_h
#define svtkImageToImageStencil_h

#include "svtkImageStencilAlgorithm.h"
#include "svtkImagingStencilModule.h" // For export macro

class svtkImageData;

class SVTKIMAGINGSTENCIL_EXPORT svtkImageToImageStencil : public svtkImageStencilAlgorithm
{
public:
  static svtkImageToImageStencil* New();
  svtkTypeMacro(svtkImageToImageStencil, svtkImageStencilAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the image data to convert into a stencil.
   */
  void SetInputData(svtkImageData* input);
  svtkImageData* GetInput();
  //@}

  /**
   * The values greater than or equal to the value match.
   */
  void ThresholdByUpper(double thresh);

  /**
   * The values less than or equal to the value match.
   */
  void ThresholdByLower(double thresh);

  /**
   * The values in a range (inclusive) match
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  svtkSetMacro(UpperThreshold, double);
  svtkGetMacro(UpperThreshold, double);
  svtkSetMacro(LowerThreshold, double);
  svtkGetMacro(LowerThreshold, double);
  //@}

protected:
  svtkImageToImageStencil();
  ~svtkImageToImageStencil() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  double UpperThreshold;
  double LowerThreshold;
  double Threshold;

private:
  svtkImageToImageStencil(const svtkImageToImageStencil&) = delete;
  void operator=(const svtkImageToImageStencil&) = delete;
};

#endif
