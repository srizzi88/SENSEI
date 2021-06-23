/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDistanceRepresentation2D
 * @brief   represent the svtkDistanceWidget
 *
 * The svtkDistanceRepresentation2D is a representation for the
 * svtkDistanceWidget. This representation consists of a measuring line (axis)
 * and two svtkHandleWidgets to place the end points of the line. Note that
 * this particular widget draws its representation in the overlay plane, and
 * the handles also operate in the 2D overlay plane. (If you desire to use
 * the distance widget for 3D measurements, use the
 * svtkDistanceRepresentation3D.)
 *
 * @sa
 * svtkDistanceWidget svtkDistanceRepresentation svtkDistanceRepresentation3D
 */

#ifndef svtkDistanceRepresentation2D_h
#define svtkDistanceRepresentation2D_h

#include "svtkDistanceRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkAxisActor2D;
class svtkProperty2D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkDistanceRepresentation2D : public svtkDistanceRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static svtkDistanceRepresentation2D* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkDistanceRepresentation2D, svtkDistanceRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Satisfy the superclasses API.
   */
  double GetDistance() override { return this->Distance; }

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  double* GetPoint1WorldPosition() override;
  double* GetPoint2WorldPosition() override;
  void GetPoint1WorldPosition(double pos[3]) override;
  void GetPoint2WorldPosition(double pos[3]) override;
  void SetPoint1WorldPosition(double pos[3]) override;
  void SetPoint2WorldPosition(double pos[3]) override;
  //@}

  void SetPoint1DisplayPosition(double pos[3]) override;
  void SetPoint2DisplayPosition(double pos[3]) override;
  void GetPoint1DisplayPosition(double pos[3]) override;
  void GetPoint2DisplayPosition(double pos[3]) override;

  //@{
  /**
   * Retrieve the svtkAxisActor2D used to draw the measurement axis. With this
   * properties can be set and so on. There is also a convenience method to
   * get the axis property.
   */
  svtkAxisActor2D* GetAxis();
  svtkProperty2D* GetAxisProperty();
  //@}

  /**
   * Method to satisfy superclasses' API.
   */
  void BuildRepresentation() override;

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  //@}

protected:
  svtkDistanceRepresentation2D();
  ~svtkDistanceRepresentation2D() override;

  // Add a line to the mix
  svtkAxisActor2D* AxisActor;
  svtkProperty2D* AxisProperty;

  // The distance between the two points
  double Distance;

private:
  svtkDistanceRepresentation2D(const svtkDistanceRepresentation2D&) = delete;
  void operator=(const svtkDistanceRepresentation2D&) = delete;
};

#endif
