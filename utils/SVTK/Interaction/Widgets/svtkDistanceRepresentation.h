/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDistanceRepresentation
 * @brief   represent the svtkDistanceWidget
 *
 * The svtkDistanceRepresentation is a superclass for various types of
 * representations for the svtkDistanceWidget. Logically subclasses consist of
 * an axis and two handles for placing/manipulating the end points.
 *
 * @sa
 * svtkDistanceWidget svtkHandleRepresentation svtkDistanceRepresentation2D svtkDistanceRepresentation
 */

#ifndef svtkDistanceRepresentation_h
#define svtkDistanceRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkHandleRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkDistanceRepresentation : public svtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkDistanceRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This representation and all subclasses must keep a distance
   * consistent with the state of the widget.
   */
  virtual double GetDistance() = 0;

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  virtual void GetPoint1WorldPosition(double pos[3]) = 0;
  virtual void GetPoint2WorldPosition(double pos[3]) = 0;
  virtual double* GetPoint1WorldPosition() SVTK_SIZEHINT(3) = 0;
  virtual double* GetPoint2WorldPosition() SVTK_SIZEHINT(3) = 0;
  virtual void SetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void SetPoint2DisplayPosition(double pos[3]) = 0;
  virtual void GetPoint1DisplayPosition(double pos[3]) = 0;
  virtual void GetPoint2DisplayPosition(double pos[3]) = 0;
  virtual void SetPoint1WorldPosition(double pos[3]) = 0;
  virtual void SetPoint2WorldPosition(double pos[3]) = 0;
  //@}

  //@{
  /**
   * This method is used to specify the type of handle representation to
   * use for the two internal svtkHandleWidgets within svtkDistanceWidget.
   * To use this method, create a dummy svtkHandleWidget (or subclass),
   * and then invoke this method with this dummy. Then the
   * svtkDistanceRepresentation uses this dummy to clone two svtkHandleWidgets
   * of the same type. Make sure you set the handle representation before
   * the widget is enabled. (The method InstantiateHandleRepresentation()
   * is invoked by the svtkDistance widget.)
   */
  void SetHandleRepresentation(svtkHandleRepresentation* handle);
  void InstantiateHandleRepresentation();
  //@}

  //@{
  /**
   * Set/Get the two handle representations used for the svtkDistanceWidget. (Note:
   * properties can be set by grabbing these representations and setting the
   * properties appropriately.)
   */
  svtkGetObjectMacro(Point1Representation, svtkHandleRepresentation);
  svtkGetObjectMacro(Point2Representation, svtkHandleRepresentation);
  //@}

  //@{
  /**
   * The tolerance representing the distance to the widget (in pixels) in
   * which the cursor is considered near enough to the end points of
   * the widget to be active.
   */
  svtkSetClampMacro(Tolerance, int, 1, 100);
  svtkGetMacro(Tolerance, int);
  //@}

  //@{
  /**
   * Specify the format to use for labelling the distance. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the distance value.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Set the scale factor from SVTK world coordinates. The ruler marks and label
   * will be defined in terms of the scaled space. For example, if the SVTK world
   * coordinates are assumed to be in inches, but the desired distance units
   * should be defined in terms of centimeters, the scale factor should be set
   * to 2.54. The ruler marks will then be spaced in terms of centimeters, and
   * the label will show the measurement in centimeters.
   */
  svtkSetMacro(Scale, double);
  svtkGetMacro(Scale, double);
  //@}

  //@{
  /**
   * Enable or disable ruler mode. When enabled, the ticks on the distance widget
   * are separated by the amount specified by RulerDistance. Otherwise, the ivar
   * NumberOfRulerTicks is used to draw the tick marks.
   */
  svtkSetMacro(RulerMode, svtkTypeBool);
  svtkGetMacro(RulerMode, svtkTypeBool);
  svtkBooleanMacro(RulerMode, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the RulerDistance which indicates the spacing of the major ticks.
   * This ivar only has effect when the RulerMode is on.
   */
  svtkSetClampMacro(RulerDistance, double, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(RulerDistance, double);
  //@}

  //@{
  /**
   * Specify the number of major ruler ticks. This overrides any subclasses
   * (e.g., svtkDistanceRepresentation2D) that have alternative methods to
   * specify the number of major ticks. Note: the number of ticks is the
   * number between the two handle endpoints. This ivar only has effect
   * when the RulerMode is off.
   */
  svtkSetClampMacro(NumberOfRulerTicks, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfRulerTicks, int);
  //@}

  // Used to communicate about the state of the representation
  enum
  {
    Outside = 0,
    NearP1,
    NearP2
  };

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  void StartComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  int ComputeComplexInteractionState(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata, int modify = 0) override;
  //@}

protected:
  svtkDistanceRepresentation();
  ~svtkDistanceRepresentation() override;

  // The handle and the rep used to close the handles
  svtkHandleRepresentation* HandleRepresentation;
  svtkHandleRepresentation* Point1Representation;
  svtkHandleRepresentation* Point2Representation;

  // Selection tolerance for the handles
  int Tolerance;

  // Format for printing the distance
  char* LabelFormat;

  // Scale to change from the SVTK world coordinates to the desired coordinate
  // system.
  double Scale;

  // Ruler related stuff
  svtkTypeBool RulerMode;
  double RulerDistance;
  int NumberOfRulerTicks;

private:
  svtkDistanceRepresentation(const svtkDistanceRepresentation&) = delete;
  void operator=(const svtkDistanceRepresentation&) = delete;
};

#endif
