/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedAreaPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderedAreaPicker
 * @brief   Uses graphics hardware to picks props behind
 * a selection rectangle on a viewport.
 *
 *
 * Like svtkAreaPicker, this class picks all props within a selection area
 * on the screen. The difference is in implementation. This class uses
 * graphics hardware to perform the test where the other uses software
 * bounding box/frustum intersection testing.
 *
 * This picker is more conservative than svtkAreaPicker. It will reject
 * some objects that pass the bounding box test of svtkAreaPicker. This
 * will happen, for instance, when picking through a corner of the bounding
 * box when the data set does not have any visible geometry in that corner.
 */

#ifndef svtkRenderedAreaPicker_h
#define svtkRenderedAreaPicker_h

#include "svtkAreaPicker.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;

class SVTKRENDERINGCORE_EXPORT svtkRenderedAreaPicker : public svtkAreaPicker
{
public:
  static svtkRenderedAreaPicker* New();
  svtkTypeMacro(svtkRenderedAreaPicker, svtkAreaPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform pick operation in volume behind the given screen coordinates.
   * Props intersecting the selection frustum will be accessible via GetProp3D.
   * GetPlanes returns a svtkImplicitFunction suitable for svtkExtractGeometry.
   */
  int AreaPick(double x0, double y0, double x1, double y1, svtkRenderer*) override;

protected:
  svtkRenderedAreaPicker();
  ~svtkRenderedAreaPicker() override;

private:
  svtkRenderedAreaPicker(const svtkRenderedAreaPicker&) = delete;
  void operator=(const svtkRenderedAreaPicker&) = delete;
};

#endif
