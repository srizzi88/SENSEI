/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleDrawPolygon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleDrawPolygon
 * @brief   draw polygon during mouse move
 *
 * This interactor style allows the user to draw a polygon in the render
 * window using the left mouse button while mouse is moving.
 * When the mouse button is released, a SelectionChangedEvent will be fired.
 */

#ifndef svtkInteractorStyleDrawPolygon_h
#define svtkInteractorStyleDrawPolygon_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyle.h"

#include "svtkVector.h" // For Polygon Points
#include <vector>      // For returning Polygon Points

class svtkUnsignedCharArray;

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleDrawPolygon : public svtkInteractorStyle
{
public:
  static svtkInteractorStyleDrawPolygon* New();
  svtkTypeMacro(svtkInteractorStyleDrawPolygon, svtkInteractorStyle);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Event bindings
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  //@}

  //@{
  /**
   * Whether to draw polygon in screen pixels. Default is ON
   */
  svtkSetMacro(DrawPolygonPixels, bool);
  svtkGetMacro(DrawPolygonPixels, bool);
  svtkBooleanMacro(DrawPolygonPixels, bool);
  //@}

  /**
   * Get the current polygon points in display units
   */
  std::vector<svtkVector2i> GetPolygonPoints();

protected:
  svtkInteractorStyleDrawPolygon();
  ~svtkInteractorStyleDrawPolygon() override;

  virtual void DrawPolygon();

  int StartPosition[2];
  int EndPosition[2];
  int Moving;

  bool DrawPolygonPixels;

  svtkUnsignedCharArray* PixelArray;

private:
  svtkInteractorStyleDrawPolygon(const svtkInteractorStyleDrawPolygon&) = delete;
  void operator=(const svtkInteractorStyleDrawPolygon&) = delete;

  class svtkInternal;
  svtkInternal* Internal;
};

#endif
