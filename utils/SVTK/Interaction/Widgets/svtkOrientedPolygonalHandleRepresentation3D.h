/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOrientedPolygonalHandleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOrientedPolygonalHandleRepresentation3D
 * @brief   represent a user defined handle geometry in 3D while maintaining a fixed orientation
 * w.r.t the camera.
 *
 * This class serves as the geometrical representation of a svtkHandleWidget.
 * The handle can be represented by an arbitrary polygonal data (svtkPolyData),
 * set via SetHandle(svtkPolyData *). The actual position of the handle
 * will be initially assumed to be (0,0,0). You can specify an offset from
 * this position if desired. This class differs from
 * svtkPolygonalHandleRepresentation3D in that the handle will always remain
 * front facing, ie it maintains a fixed orientation with respect to the
 * camera. This is done by using svtkFollowers internally to render the actors.
 * @sa
 * svtkPolygonalHandleRepresentation3D svtkHandleRepresentation svtkHandleWidget
 */

#ifndef svtkOrientedPolygonalHandleRepresentation3D_h
#define svtkOrientedPolygonalHandleRepresentation3D_h

#include "svtkAbstractPolygonalHandleRepresentation3D.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class SVTKINTERACTIONWIDGETS_EXPORT svtkOrientedPolygonalHandleRepresentation3D
  : public svtkAbstractPolygonalHandleRepresentation3D
{
public:
  /**
   * Instantiate this class.
   */
  static svtkOrientedPolygonalHandleRepresentation3D* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(
    svtkOrientedPolygonalHandleRepresentation3D, svtkAbstractPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

protected:
  svtkOrientedPolygonalHandleRepresentation3D();
  ~svtkOrientedPolygonalHandleRepresentation3D() override;

  /**
   * Override the superclass method.
   */
  void UpdateHandle() override;

private:
  svtkOrientedPolygonalHandleRepresentation3D(
    const svtkOrientedPolygonalHandleRepresentation3D&) = delete;
  void operator=(const svtkOrientedPolygonalHandleRepresentation3D&) = delete;
};

#endif
