/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageCheckerboard.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageCheckerboard
 * @brief   show two images at once using a checkboard pattern
 *
 *  svtkImageCheckerboard displays two images as one using a checkerboard
 *  pattern. This filter can be used to compare two images. The
 *  checkerboard pattern is controlled by the NumberOfDivisions
 *  ivar. This controls the number of checkerboard divisions in the whole
 *  extent of the image.
 */

#ifndef svtkImageCheckerboard_h
#define svtkImageCheckerboard_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageCheckerboard : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageCheckerboard* New();
  svtkTypeMacro(svtkImageCheckerboard, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the number of divisions along each axis.
   */
  svtkSetVector3Macro(NumberOfDivisions, int);
  svtkGetVectorMacro(NumberOfDivisions, int, 3);
  //@}

  /**
   * Set the two inputs to this filter
   */
  virtual void SetInput1Data(svtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(svtkDataObject* in) { this->SetInputData(1, in); }

protected:
  svtkImageCheckerboard();
  ~svtkImageCheckerboard() override {}

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int threadId) override;
  int NumberOfDivisions[3];

private:
  svtkImageCheckerboard(const svtkImageCheckerboard&) = delete;
  void operator=(const svtkImageCheckerboard&) = delete;
};

#endif
