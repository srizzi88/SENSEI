/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMapToWindowLevelColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMapToWindowLevelColors
 * @brief   Map an image through a lookup table and/or a window/level.
 *
 * The svtkImageMapToWindowLevelColors filter can be used to perform
 * the following operations depending on its settings:
 * -# If no lookup table is provided, and if the input data has a single
 *    component (any numerical scalar type is allowed), then the data is
 *    mapped through the specified Window/Level.  The type of the output
 *    scalars will be "unsigned char" with a range of (0,255).
 * -# If no lookup table is provided, and if the input data is already
 *    unsigned char, and if the Window/Level is set to 255.0/127.5, then
 *    the input data will be passed directly to the output.
 * -# If a lookup table is provided, then the first component of the
 *    input data is mapped through the lookup table (using the Range of
 *    the lookup table), and the resulting color is modulated according
 *    to the Window/Level.  For example, if the input value is 500 and
 *    the Window/Level are 2000/1000, the output value will be RGB*0.25
 *    where RGB is the color assigned by the lookup table and 0.25 is
 *    the modulation factor.
 * See SetWindow() and SetLevel() for the equations used for modulation.
 * To map scalars through a lookup table without modulating the resulting
 * color, use svtkImageMapToColors instead of this filter.
 * @sa
 * svtkLookupTable svtkScalarsToColors
 */

#ifndef svtkImageMapToWindowLevelColors_h
#define svtkImageMapToWindowLevelColors_h

#include "svtkImageMapToColors.h"
#include "svtkImagingColorModule.h" // For export macro

class SVTKIMAGINGCOLOR_EXPORT svtkImageMapToWindowLevelColors : public svtkImageMapToColors
{
public:
  static svtkImageMapToWindowLevelColors* New();
  svtkTypeMacro(svtkImageMapToWindowLevelColors, svtkImageMapToColors);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set / Get the Window to use -> modulation will be performed on the
   * color based on (S - (L - W/2))/W where S is the scalar value, L is
   * the level and W is the window.
   */
  svtkSetMacro(Window, double);
  svtkGetMacro(Window, double);
  //@}

  //@{
  /**
   * Set / Get the Level to use -> modulation will be performed on the
   * color based on (S - (L - W/2))/W where S is the scalar value, L is
   * the level and W is the window.
   */
  svtkSetMacro(Level, double);
  svtkGetMacro(Level, double);
  //@}

protected:
  svtkImageMapToWindowLevelColors();
  ~svtkImageMapToWindowLevelColors() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  double Window;
  double Level;

private:
  svtkImageMapToWindowLevelColors(const svtkImageMapToWindowLevelColors&) = delete;
  void operator=(const svtkImageMapToWindowLevelColors&) = delete;
};

#endif
