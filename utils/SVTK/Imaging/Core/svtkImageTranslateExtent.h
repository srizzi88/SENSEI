/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageTranslateExtent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageTranslateExtent
 * @brief   Changes extent, nothing else.
 *
 * svtkImageTranslateExtent shift the whole extent, but does not
 * change the data.
 */

#ifndef svtkImageTranslateExtent_h
#define svtkImageTranslateExtent_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImageTranslateExtent : public svtkImageAlgorithm
{
public:
  static svtkImageTranslateExtent* New();
  svtkTypeMacro(svtkImageTranslateExtent, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Delta to change "WholeExtent". -1 changes 0->10 to -1->9.
   */
  svtkSetVector3Macro(Translation, int);
  svtkGetVector3Macro(Translation, int);
  //@}

protected:
  svtkImageTranslateExtent();
  ~svtkImageTranslateExtent() override {}

  int Translation[3];

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageTranslateExtent(const svtkImageTranslateExtent&) = delete;
  void operator=(const svtkImageTranslateExtent&) = delete;
};

#endif
