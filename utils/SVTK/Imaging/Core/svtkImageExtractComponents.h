/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageExtractComponents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageExtractComponents
 * @brief   Outputs a single component
 *
 * svtkImageExtractComponents takes an input with any number of components
 * and outputs some of them.  It does involve a copy of the data.
 *
 * @sa
 * svtkImageAppendComponents
 */

#ifndef svtkImageExtractComponents_h
#define svtkImageExtractComponents_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageExtractComponents : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageExtractComponents* New();
  svtkTypeMacro(svtkImageExtractComponents, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the components to extract.
   */
  void SetComponents(int c1);
  void SetComponents(int c1, int c2);
  void SetComponents(int c1, int c2, int c3);
  svtkGetVector3Macro(Components, int);
  //@}

  //@{
  /**
   * Get the number of components to extract. This is set implicitly by the
   * SetComponents() method.
   */
  svtkGetMacro(NumberOfComponents, int);
  //@}

protected:
  svtkImageExtractComponents();
  ~svtkImageExtractComponents() override {}

  int NumberOfComponents;
  int Components[3];

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageExtractComponents(const svtkImageExtractComponents&) = delete;
  void operator=(const svtkImageExtractComponents&) = delete;
};

#endif
