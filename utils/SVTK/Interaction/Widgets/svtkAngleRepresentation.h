/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAngleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAngleRepresentation
 * @brief   represent the svtkAngleWidget
 *
 * The svtkAngleRepresentation is a superclass for classes representing the
 * svtkAngleWidget. This representation consists of two rays and three
 * svtkHandleRepresentations to place and manipulate the three points defining
 * the angle representation. (Note: the three points are referred to as Point1,
 * Center, and Point2, at the two end points (Point1 and Point2) and Center
 * (around which the angle is measured).
 *
 * @sa
 * svtkAngleWidget svtkHandleRepresentation svtkAngleRepresentation2D
 */

#ifndef svtkAngleRepresentation_h
#define svtkAngleRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkHandleRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkAngleRepresentation : public svtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkAngleRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This representation and all subclasses must keep an angle (in degrees)
   * consistent with the state of the widget.
   */
  virtual double GetAngle() = 0;

  //@{
  /**
   * Methods to Set/Get the coordinates of the three points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  virtual void GetPoint1WorldPosition(double pos[3]) = 0;
  virtual void GetCenterWorldPosition(double pos[3]) = 0;
  virtual void GetPoint2WorldPosition(double pos[3]) = 0;
  virtual void SetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void SetCenterDisplayPosition(double pos[3]) = 0;
  virtual void SetPoint2DisplayPosition(double pos[3]) = 0;
  virtual void GetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void GetCenterDisplayPosition(double pos[3]) = 0;
  virtual void GetPoint2DisplayPosition(double pos[3]) = 0;
  //@}

  //@{
  /**
   * This method is used to specify the type of handle representation to use
   * for the three internal svtkHandleWidgets within svtkAngleRepresentation.
   * To use this method, create a dummy svtkHandleRepresentation (or
   * subclass), and then invoke this method with this dummy. Then the
   * svtkAngleRepresentation uses this dummy to clone three
   * svtkHandleRepresentations of the same type. Make sure you set the handle
   * representation before the widget is enabled. (The method
   * InstantiateHandleRepresentation() is invoked by the svtkAngle widget.)
   */
  void SetHandleRepresentation(svtkHandleRepresentation* handle);
  void InstantiateHandleRepresentation();
  //@}

  //@{
  /**
   * Set/Get the handle representations used for the svtkAngleRepresentation.
   */
  svtkGetObjectMacro(Point1Representation, svtkHandleRepresentation);
  svtkGetObjectMacro(CenterRepresentation, svtkHandleRepresentation);
  svtkGetObjectMacro(Point2Representation, svtkHandleRepresentation);
  //@}

  //@{
  /**
   * The tolerance representing the distance to the representation (in
   * pixels) in which the cursor is considered near enough to the end points
   * of the representation to be active.
   */
  svtkSetClampMacro(Tolerance, int, 1, 100);
  svtkGetMacro(Tolerance, int);
  //@}

  //@{
  /**
   * Specify the format to use for labeling the angle. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the angle value.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Special methods for turning off the rays and arc that define the cone
   * and arc of the angle.
   */
  svtkSetMacro(Ray1Visibility, svtkTypeBool);
  svtkGetMacro(Ray1Visibility, svtkTypeBool);
  svtkBooleanMacro(Ray1Visibility, svtkTypeBool);
  svtkSetMacro(Ray2Visibility, svtkTypeBool);
  svtkGetMacro(Ray2Visibility, svtkTypeBool);
  svtkBooleanMacro(Ray2Visibility, svtkTypeBool);
  svtkSetMacro(ArcVisibility, svtkTypeBool);
  svtkGetMacro(ArcVisibility, svtkTypeBool);
  svtkBooleanMacro(ArcVisibility, svtkTypeBool);
  //@}

  // Used to communicate about the state of the representation
  enum
  {
    Outside = 0,
    NearP1,
    NearCenter,
    NearP2
  };

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  virtual void CenterWidgetInteraction(double e[2]);
  void WidgetInteraction(double e[2]) override;
  //@}

protected:
  svtkAngleRepresentation();
  ~svtkAngleRepresentation() override;

  // The handle and the rep used to close the handles
  svtkHandleRepresentation* HandleRepresentation;
  svtkHandleRepresentation* Point1Representation;
  svtkHandleRepresentation* CenterRepresentation;
  svtkHandleRepresentation* Point2Representation;

  // Selection tolerance for the handles
  int Tolerance;

  // Visibility of the various pieces of the representation
  svtkTypeBool Ray1Visibility;
  svtkTypeBool Ray2Visibility;
  svtkTypeBool ArcVisibility;

  // Format for the label
  char* LabelFormat;

private:
  svtkAngleRepresentation(const svtkAngleRepresentation&) = delete;
  void operator=(const svtkAngleRepresentation&) = delete;
};

#endif
