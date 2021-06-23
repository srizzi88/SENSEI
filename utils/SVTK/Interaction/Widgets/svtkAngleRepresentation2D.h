/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAngleRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAngleRepresentation2D
 * @brief   represent the svtkAngleWidget
 *
 * The svtkAngleRepresentation2D is a representation for the
 * svtkAngleWidget. This representation consists of two rays and three
 * svtkHandleRepresentations to place and manipulate the three points defining
 * the angle representation. (Note: the three points are referred to as Point1,
 * Center, and Point2, at the two end points (Point1 and Point2) and Center
 * (around which the angle is measured). This particular implementation is a
 * 2D representation, meaning that it draws in the overlay plane.
 *
 * @sa
 * svtkAngleWidget svtkHandleRepresentation
 */

#ifndef svtkAngleRepresentation2D_h
#define svtkAngleRepresentation2D_h

#include "svtkAngleRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkLeaderActor2D;
class svtkProperty2D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkAngleRepresentation2D : public svtkAngleRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static svtkAngleRepresentation2D* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkAngleRepresentation2D, svtkAngleRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Satisfy the superclasses API.
   */
  double GetAngle() override;

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  void GetPoint1WorldPosition(double pos[3]) override;
  void GetCenterWorldPosition(double pos[3]) override;
  void GetPoint2WorldPosition(double pos[3]) override;
  void SetPoint1DisplayPosition(double pos[3]) override;
  void SetCenterDisplayPosition(double pos[3]) override;
  void SetPoint2DisplayPosition(double pos[3]) override;
  void GetPoint1DisplayPosition(double pos[3]) override;
  void GetCenterDisplayPosition(double pos[3]) override;
  void GetPoint2DisplayPosition(double pos[3]) override;
  //@}

  //@{
  /**
   * Set/Get the three leaders used to create this representation.
   * By obtaining these leaders the user can set the appropriate
   * properties, etc.
   */
  svtkGetObjectMacro(Ray1, svtkLeaderActor2D);
  svtkGetObjectMacro(Ray2, svtkLeaderActor2D);
  svtkGetObjectMacro(Arc, svtkLeaderActor2D);
  //@}

  /**
   * Method defined by svtkWidgetRepresentation superclass and
   * needed here.
   */
  void BuildRepresentation() override;

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOverlay(svtkViewport* viewport) override;
  //@}

protected:
  svtkAngleRepresentation2D();
  ~svtkAngleRepresentation2D() override;

  // The pieces that make up the angle representations
  svtkLeaderActor2D* Ray1;
  svtkLeaderActor2D* Ray2;
  svtkLeaderActor2D* Arc;

private:
  svtkAngleRepresentation2D(const svtkAngleRepresentation2D&) = delete;
  void operator=(const svtkAngleRepresentation2D&) = delete;
};

#endif
