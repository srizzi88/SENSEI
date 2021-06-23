/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMarkerUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkMarkerUtilities
 * @brief   Utilities for generating marker images
 *
 *
 * This class programmatically generates markers of a specified size
 * for various marker styles.
 *
 * @sa
 * svtkPlotLine, svtkPlotPoints
 */

#ifndef svtkMarkerUtilities_h
#define svtkMarkerUtilities_h

#include "svtkRenderingContext2DModule.h" // For export macro

#include "svtkObject.h"

class svtkImageData;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkMarkerUtilities : public svtkObject
{
public:
  svtkTypeMacro(svtkMarkerUtilities, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Enum containing various marker styles that can be used in a plot.
   */
  enum
  {
    NONE = 0,
    CROSS,
    PLUS,
    SQUARE,
    CIRCLE,
    DIAMOND
  };

  /**
   * Generate the requested symbol of a particular style and size.
   */
  static void GenerateMarker(svtkImageData* data, int style, int width);

protected:
  svtkMarkerUtilities();
  ~svtkMarkerUtilities() override;

private:
  svtkMarkerUtilities(const svtkMarkerUtilities&) = delete;
  void operator=(const svtkMarkerUtilities&) = delete;
};

#endif // svtkMarkerUtilities_h
