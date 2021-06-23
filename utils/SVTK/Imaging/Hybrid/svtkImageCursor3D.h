/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageCursor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageCursor3D
 * @brief   Paints a cursor on top of an image or volume.
 *
 * svtkImageCursor3D will draw a cursor on a 2d image or 3d volume.
 */

#ifndef svtkImageCursor3D_h
#define svtkImageCursor3D_h

#include "svtkImageInPlaceFilter.h"
#include "svtkImagingHybridModule.h" // For export macro

class SVTKIMAGINGHYBRID_EXPORT svtkImageCursor3D : public svtkImageInPlaceFilter
{
public:
  static svtkImageCursor3D* New();
  svtkTypeMacro(svtkImageCursor3D, svtkImageInPlaceFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Sets/Gets the center point of the 3d cursor.
   */
  svtkSetVector3Macro(CursorPosition, double);
  svtkGetVector3Macro(CursorPosition, double);
  //@}

  //@{
  /**
   * Sets/Gets what pixel value to draw the cursor in.
   */
  svtkSetMacro(CursorValue, double);
  svtkGetMacro(CursorValue, double);
  //@}

  //@{
  /**
   * Sets/Gets the radius of the cursor. The radius determines
   * how far the axis lines project out from the cursors center.
   */
  svtkSetMacro(CursorRadius, int);
  svtkGetMacro(CursorRadius, int);
  //@}

protected:
  svtkImageCursor3D();
  ~svtkImageCursor3D() override {}

  double CursorPosition[3];
  double CursorValue;
  int CursorRadius;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageCursor3D(const svtkImageCursor3D&) = delete;
  void operator=(const svtkImageCursor3D&) = delete;
};

#endif
