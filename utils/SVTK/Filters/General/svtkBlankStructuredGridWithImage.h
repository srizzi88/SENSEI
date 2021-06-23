/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlankStructuredGridWithImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBlankStructuredGridWithImage
 * @brief   blank a structured grid with an image
 *
 * This filter can be used to set the blanking in a structured grid with
 * an image. The filter takes two inputs: the structured grid to blank,
 * and the image used to set the blanking. Make sure that the dimensions of
 * both the image and the structured grid are identical.
 *
 * Note that the image is interpreted as follows: zero values indicate that
 * the structured grid point is blanked; non-zero values indicate that the
 * structured grid point is visible. The blanking data must be unsigned char.
 *
 * @sa
 * svtkStructuredGrid
 */

#ifndef svtkBlankStructuredGridWithImage_h
#define svtkBlankStructuredGridWithImage_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkStructuredGridAlgorithm.h"

class svtkImageData;

class SVTKFILTERSGENERAL_EXPORT svtkBlankStructuredGridWithImage : public svtkStructuredGridAlgorithm
{
public:
  static svtkBlankStructuredGridWithImage* New();
  svtkTypeMacro(svtkBlankStructuredGridWithImage, svtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set / get the input image used to perform the blanking.
   */
  void SetBlankingInputData(svtkImageData* input);
  svtkImageData* GetBlankingInput();
  //@}

protected:
  svtkBlankStructuredGridWithImage();
  ~svtkBlankStructuredGridWithImage() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkBlankStructuredGridWithImage(const svtkBlankStructuredGridWithImage&) = delete;
  void operator=(const svtkBlankStructuredGridWithImage&) = delete;
};

#endif
