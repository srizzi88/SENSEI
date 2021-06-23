/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeCutter
 * @brief   Cut composite data sets with user-specified implicit function
 *
 * Loop over each data set in the composite input and apply svtkCutter
 * @sa
 * svtkCutter
 */

#ifndef svtkCompositeCutter_h
#define svtkCompositeCutter_h

#include "svtkCutter.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkCompositeCutter : public svtkCutter
{
public:
  svtkTypeMacro(svtkCompositeCutter, svtkCutter);

  static svtkCompositeCutter* New();

  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkCompositeCutter(svtkImplicitFunction* cf = nullptr);
  ~svtkCompositeCutter() override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkCompositeCutter(const svtkCompositeCutter&) = delete;
  void operator=(const svtkCompositeCutter&) = delete;
};

#endif
