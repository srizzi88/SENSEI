/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageCorrelation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageCorrelation
 * @brief   Correlation imageof the two inputs.
 *
 * svtkImageCorrelation finds the correlation between two data sets.
 * SetDimensionality determines
 * whether the Correlation will be 3D, 2D or 1D.
 * The default is a 2D Correlation.  The Output type will be double.
 * The output size will match the size of the first input.
 * The second input is considered the correlation kernel.
 */

#ifndef svtkImageCorrelation_h
#define svtkImageCorrelation_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageCorrelation : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageCorrelation* New();
  svtkTypeMacro(svtkImageCorrelation, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Determines how the input is interpreted (set of 2d slices ...).
   * The default is 2.
   */
  svtkSetClampMacro(Dimensionality, int, 2, 3);
  svtkGetMacro(Dimensionality, int);
  //@}

  /**
   * Set the input image.
   */
  virtual void SetInput1Data(svtkDataObject* in) { this->SetInputData(0, in); }

  /**
   * Set the correlation kernel.
   */
  virtual void SetInput2Data(svtkDataObject* in) { this->SetInputData(1, in); }

protected:
  svtkImageCorrelation();
  ~svtkImageCorrelation() override {}

  int Dimensionality;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int threadId) override;

private:
  svtkImageCorrelation(const svtkImageCorrelation&) = delete;
  void operator=(const svtkImageCorrelation&) = delete;
};

#endif
