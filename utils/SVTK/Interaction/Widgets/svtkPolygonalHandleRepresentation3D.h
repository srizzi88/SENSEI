/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolygonalHandleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolygonalHandleRepresentation3D
 * @brief   represent a user defined handle geometry in 3D space
 *
 * This class serves as the geometrical representation of a svtkHandleWidget.
 * The handle can be represented by an arbitrary polygonal data (svtkPolyData),
 * set via SetHandle(svtkPolyData *). The actual position of the handle
 * will be initially assumed to be (0,0,0). You can specify an offset from
 * this position if desired.
 * @sa
 * svtkPointHandleRepresentation3D svtkHandleRepresentation svtkHandleWidget
 */

#ifndef svtkPolygonalHandleRepresentation3D_h
#define svtkPolygonalHandleRepresentation3D_h

#include "svtkAbstractPolygonalHandleRepresentation3D.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class SVTKINTERACTIONWIDGETS_EXPORT svtkPolygonalHandleRepresentation3D
  : public svtkAbstractPolygonalHandleRepresentation3D
{
public:
  /**
   * Instantiate this class.
   */
  static svtkPolygonalHandleRepresentation3D* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkPolygonalHandleRepresentation3D, svtkAbstractPolygonalHandleRepresentation3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Set the position of the point in world and display coordinates.
   */
  void SetWorldPosition(double p[3]) override;

  //@{
  /**
   * Set/get the offset of the handle position with respect to the handle
   * center, assumed to be the origin.
   */
  svtkSetVector3Macro(Offset, double);
  svtkGetVector3Macro(Offset, double);
  //@}

protected:
  svtkPolygonalHandleRepresentation3D();
  ~svtkPolygonalHandleRepresentation3D() override {}

  double Offset[3];

private:
  svtkPolygonalHandleRepresentation3D(const svtkPolygonalHandleRepresentation3D&) = delete;
  void operator=(const svtkPolygonalHandleRepresentation3D&) = delete;
};

#endif
