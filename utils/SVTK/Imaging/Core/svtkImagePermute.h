/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImagePermute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImagePermute
 * @brief    Permutes axes of input.
 *
 * svtkImagePermute reorders the axes of the input. Filtered axes specify
 * the input axes which become X, Y, Z.  The input has to have the
 * same scalar type of the output. The filter does copy the
 * data when it executes. This filter is actually a very thin wrapper
 * around svtkImageReslice.
 */

#ifndef svtkImagePermute_h
#define svtkImagePermute_h

#include "svtkImageReslice.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImagePermute : public svtkImageReslice
{
public:
  static svtkImagePermute* New();
  svtkTypeMacro(svtkImagePermute, svtkImageReslice);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The filtered axes are the input axes that get relabeled to X,Y,Z.
   */
  void SetFilteredAxes(int x, int y, int z);
  void SetFilteredAxes(const int xyz[3]) { this->SetFilteredAxes(xyz[0], xyz[1], xyz[2]); }
  svtkGetVector3Macro(FilteredAxes, int);
  //@}

protected:
  svtkImagePermute();
  ~svtkImagePermute() override {}

  int FilteredAxes[3];

private:
  svtkImagePermute(const svtkImagePermute&) = delete;
  void operator=(const svtkImagePermute&) = delete;
};

#endif
