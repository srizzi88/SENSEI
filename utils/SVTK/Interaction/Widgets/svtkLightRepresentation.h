/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLightRepresentation
 * @brief   represent a svtkLight
 *
 * The svtkLightRepresentation is a representation for the svtkLight.
 * This representation consists of a LightPosition sphere with an automatically resized
 * radius so it is always visible, a line between the LightPosition and the FocalPoint and
 * a cone of angle ConeAngle when using Positional.
 *
 * @sa
 * svtkLightWidget svtkSphereWidget svtkSphereRepresentation
 */

#ifndef svtkLightRepresentation_h
#define svtkLightRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkNew.h"                      // Needed for svtkNew
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkBox;
class svtkCellPicker;
class svtkConeSource;
class svtkLineSource;
class svtkPointHandleRepresentation3D;
class svtkPolyDataMapper;
class svtkProperty;
class svtkSphereSource;

class SVTKINTERACTIONWIDGETS_EXPORT svtkLightRepresentation : public svtkWidgetRepresentation
{
public:
  static svtkLightRepresentation* New();
  svtkTypeMacro(svtkLightRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the positional flag. When set to on, a cone will be visible.
   */
  svtkSetMacro(Positional, bool);
  svtkGetMacro(Positional, bool);
  svtkBooleanMacro(Positional, bool);
  //@}

  //@{
  /**
   * Set/Get the coordinates of the position of the light representation.
   */
  void SetLightPosition(double pos[3]);
  svtkGetVector3Macro(LightPosition, double);
  //@}

  //@{
  /**
   * Set/Get the coordinates of the focal point of the light representation.
   */
  void SetFocalPoint(double pos[3]);
  svtkGetVector3Macro(FocalPoint, double);
  //@}

  //@{
  /**
   * Set/Get the cone angle, in degrees, for the light.
   * Used only when positional.
   */
  void SetConeAngle(double angle);
  svtkGetMacro(ConeAngle, double);
  //@}

  //@{
  /**
   * Set/Get the light color.
   */
  void SetLightColor(double* color);
  double* GetLightColor() SVTK_SIZEHINT(3);
  //@}

  /**
   * Enum used to communicate interaction state.
   */
  enum
  {
    Outside = 0,
    MovingLight,
    MovingFocalPoint,
    MovingPositionalFocalPoint,
    ScalingConeAngle
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g., svtkLightWidget) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  svtkSetClampMacro(InteractionState, int, Outside, ScalingConeAngle);
  //@}

  //@{
  /**
   * Get the property used for all the actors
   */
  svtkGetObjectMacro(Property, svtkProperty);
  //@}

  //@{
  /**
   * Method to satisfy superclasses' API.
   */
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double eventPosition[2]) override;
  void WidgetInteraction(double eventPosition[2]) override;
  double* GetBounds() override;
  //@}

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  //@}

protected:
  svtkLightRepresentation();
  ~svtkLightRepresentation() override;

  virtual void SizeHandles();
  virtual void UpdateSources();
  virtual void ScaleConeAngle(double* pickPoint, double* lastPickPoint);

  svtkNew<svtkProperty> Property;
  svtkNew<svtkBox> BoundingBox;
  svtkCellPicker* LastPicker;
  double LastScalingDistance2 = -1;
  double LastEventPosition[3] = { 0, 0, 0 };

  // the Sphere
  svtkNew<svtkSphereSource> Sphere;
  svtkNew<svtkActor> SphereActor;
  svtkNew<svtkPolyDataMapper> SphereMapper;
  svtkNew<svtkCellPicker> SpherePicker;

  // the Cone
  svtkNew<svtkConeSource> Cone;
  svtkNew<svtkActor> ConeActor;
  svtkNew<svtkPolyDataMapper> ConeMapper;
  svtkNew<svtkCellPicker> ConePicker;

  // the Line
  svtkNew<svtkLineSource> Line;
  svtkNew<svtkActor> LineActor;
  svtkNew<svtkPolyDataMapper> LineMapper;
  svtkNew<svtkCellPicker> LinePicker;

  double LightPosition[3] = { 0, 0, 1 };
  double FocalPoint[3] = { 0, 0, 0 };
  double ConeAngle = 30;
  bool Positional = false;

private:
  svtkLightRepresentation(const svtkLightRepresentation&) = delete;
  void operator=(const svtkLightRepresentation&) = delete;
};

#endif
