/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxesTransformRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAxesTransformRepresentation
 * @brief   represent the svtkAxesTransformWidget
 *
 * The svtkAxesTransformRepresentation is a representation for the
 * svtkAxesTransformWidget. This representation consists of a origin sphere
 * with three tubed axes with cones at the end of the axes. In addition an
 * optional label provides delta values of motion. Note that this particular
 * widget draws its representation in 3D space, so the widget can be
 * occluded.
 * @sa
 * svtkDistanceWidget svtkDistanceRepresentation svtkDistanceRepresentation2D
 */

#ifndef svtkAxesTransformRepresentation_h
#define svtkAxesTransformRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkHandleRepresentation;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkActor;
class svtkVectorText;
class svtkFollower;
class svtkBox;
class svtkCylinderSource;
class svtkGlyph3D;
class svtkDoubleArray;
class svtkTransformPolyDataFilter;
class svtkProperty;

class SVTKINTERACTIONWIDGETS_EXPORT svtkAxesTransformRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static svtkAxesTransformRepresentation* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkAxesTransformRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the two handle representations used for the
   * svtkAxesTransformWidget. (Note: properties can be set by grabbing these
   * representations and setting the properties appropriately.)
   */
  svtkGetObjectMacro(OriginRepresentation, svtkHandleRepresentation);
  svtkGetObjectMacro(SelectionRepresentation, svtkHandleRepresentation);
  //@}

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  double* GetOriginWorldPosition();
  void GetOriginWorldPosition(double pos[3]);
  void SetOriginWorldPosition(double pos[3]);
  void SetOriginDisplayPosition(double pos[3]);
  void GetOriginDisplayPosition(double pos[3]);
  //@}

  /**
   * Specify a scale to control the size of the widget. Large values make the
   * the widget larger.
   */

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
   * Specify the format to use for labelling information during
   * transformation. Note that an empty string results in no label, or a
   * format string without a "%" character will not print numeric values.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  /**
   * Enum used to communicate interaction state.
   */
  enum
  {
    Outside = 0,
    OnOrigin,
    OnX,
    OnY,
    OnZ,
    OnXEnd,
    OnYEnd,
    OnZEnd
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g., svtkLineWidget2) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  svtkSetClampMacro(InteractionState, int, Outside, OnZEnd);
  //@}

  //@{
  /**
   * Method to satisfy superclasses' API.
   */
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
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

  //@{
  /**
   * Scale text (font size along each dimension). This helps control
   * the appearance of the 3D text.
   */
  void SetLabelScale(double x, double y, double z)
  {
    double scale[3];
    scale[0] = x;
    scale[1] = y;
    scale[2] = z;
    this->SetLabelScale(scale);
  }
  virtual void SetLabelScale(double scale[3]);
  virtual double* GetLabelScale();
  //@}

  /**
   * Get the distance annotation property
   */
  virtual svtkProperty* GetLabelProperty();

protected:
  svtkAxesTransformRepresentation();
  ~svtkAxesTransformRepresentation() override;

  // The handle and the rep used to close the handles
  svtkHandleRepresentation* OriginRepresentation;
  svtkHandleRepresentation* SelectionRepresentation;

  // Selection tolerance for the handles
  int Tolerance;

  // Format for printing the distance
  char* LabelFormat;

  // The line
  svtkPoints* LinePoints;
  svtkPolyData* LinePolyData;
  svtkPolyDataMapper* LineMapper;
  svtkActor* LineActor;

  // The distance label
  svtkVectorText* LabelText;
  svtkPolyDataMapper* LabelMapper;
  svtkFollower* LabelActor;

  // The 3D disk tick marks
  svtkPoints* GlyphPoints;
  svtkDoubleArray* GlyphVectors;
  svtkPolyData* GlyphPolyData;
  svtkCylinderSource* GlyphCylinder;
  svtkTransformPolyDataFilter* GlyphXForm;
  svtkGlyph3D* Glyph3D;
  svtkPolyDataMapper* GlyphMapper;
  svtkActor* GlyphActor;

  // Support GetBounds() method
  svtkBox* BoundingBox;

  double LastEventPosition[3];

private:
  svtkAxesTransformRepresentation(const svtkAxesTransformRepresentation&) = delete;
  void operator=(const svtkAxesTransformRepresentation&) = delete;
};

#endif
