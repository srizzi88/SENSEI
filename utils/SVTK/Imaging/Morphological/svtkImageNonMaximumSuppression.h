/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageNonMaximumSuppression.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageNonMaximumSuppression
 * @brief   Performs non-maximum suppression
 *
 * svtkImageNonMaximumSuppression Sets to zero any pixel that is not a peak.
 * If a pixel has a neighbor along the vector that has larger magnitude, the
 * smaller pixel is set to zero.  The filter takes two inputs: a magnitude
 * and a vector.  Output is magnitude information and is always in doubles.
 * Typically this filter is used with svtkImageGradient and
 * svtkImageGradientMagnitude as inputs.
 */

#ifndef svtkImageNonMaximumSuppression_h
#define svtkImageNonMaximumSuppression_h

#define SVTK_IMAGE_NON_MAXIMUM_SUPPRESSION_MAGNITUDE_INPUT 0
#define SVTK_IMAGE_NON_MAXIMUM_SUPPRESSION_VECTOR_INPUT 1

#include "svtkImageData.h"                  // makes things a bit easier
#include "svtkImagingMorphologicalModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageNonMaximumSuppression
  : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageNonMaximumSuppression* New();
  svtkTypeMacro(svtkImageNonMaximumSuppression, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the magnitude and vector inputs.
   */
  void SetMagnitudeInputData(svtkImageData* input) { this->SetInputData(0, input); }
  void SetVectorInputData(svtkImageData* input) { this->SetInputData(1, input); }
  //@}

  //@{
  /**
   * If "HandleBoundariesOn" then boundary pixels are duplicated
   * So central differences can get values.
   */
  svtkSetMacro(HandleBoundaries, svtkTypeBool);
  svtkGetMacro(HandleBoundaries, svtkTypeBool);
  svtkBooleanMacro(HandleBoundaries, svtkTypeBool);
  //@}

  //@{
  /**
   * Determines how the input is interpreted (set of 2d slices or a 3D volume)
   */
  svtkSetClampMacro(Dimensionality, int, 2, 3);
  svtkGetMacro(Dimensionality, int);
  //@}

protected:
  svtkImageNonMaximumSuppression();
  ~svtkImageNonMaximumSuppression() override {}

  svtkTypeBool HandleBoundaries;
  int Dimensionality;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int threadId) override;

private:
  svtkImageNonMaximumSuppression(const svtkImageNonMaximumSuppression&) = delete;
  void operator=(const svtkImageNonMaximumSuppression&) = delete;
};

#endif
