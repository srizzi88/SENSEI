/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliderRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSliderRepresentation3D
 * @brief   provide the representation for a svtkSliderWidget with a 3D skin
 *
 * This class is used to represent and render a svtkSliderWidget. To use this
 * class, you must at a minimum specify the end points of the
 * slider. Optional instance variable can be used to modify the appearance of
 * the widget.
 *
 *
 * @sa
 * svtkSliderWidget
 */

#ifndef svtkSliderRepresentation3D_h
#define svtkSliderRepresentation3D_h

#include "svtkCoordinate.h"               // For svtkViewportCoordinateMacro
#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkSliderRepresentation.h"

class svtkActor;
class svtkPolyDataMapper;
class svtkSphereSource;
class svtkCellPicker;
class svtkProperty;
class svtkCylinderSource;
class svtkVectorText;
class svtkAssembly;
class svtkTransform;
class svtkTransformPolyDataFilter;
class svtkMatrix4x4;

class SVTKINTERACTIONWIDGETS_EXPORT svtkSliderRepresentation3D : public svtkSliderRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkSliderRepresentation3D* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkSliderRepresentation3D, svtkSliderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Position the first end point of the slider. Note that this point is an
   * instance of svtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point1Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  svtkCoordinate* GetPoint1Coordinate();
  void SetPoint1InWorldCoordinates(double x, double y, double z);
  //@}

  //@{
  /**
   * Position the second end point of the slider. Note that this point is an
   * instance of svtkCoordinate, meaning that Point 1 can be specified in a
   * variety of coordinate systems, and can even be relative to another
   * point. To set the point, you'll want to get the Point2Coordinate and
   * then invoke the necessary methods to put it into the correct coordinate
   * system and set the correct initial value.
   */
  svtkCoordinate* GetPoint2Coordinate();
  void SetPoint2InWorldCoordinates(double x, double y, double z);
  //@}

  //@{
  /**
   * Specify the title text for this widget. If the value is not set, or set
   * to the empty string "", then the title text is not displayed.
   */
  void SetTitleText(const char*) override;
  const char* GetTitleText() override;
  //@}

  //@{
  /**
   * Specify whether to use a sphere or cylinder slider shape. By default, a
   * sphere shape is used.
   */
  svtkSetClampMacro(SliderShape, int, SphereShape, CylinderShape);
  svtkGetMacro(SliderShape, int);
  void SetSliderShapeToSphere() { this->SetSliderShape(SphereShape); }
  void SetSliderShapeToCylinder() { this->SetSliderShape(CylinderShape); }
  //@}

  //@{
  /**
   * Set the rotation of the slider widget around the axis of the widget. This is
   * used to control which way the widget is initially oriented. (This is especially
   * important for the label and title.)
   */
  svtkSetMacro(Rotation, double);
  svtkGetMacro(Rotation, double);
  //@}

  //@{
  /**
   * Get the slider properties. The properties of the slider when selected
   * and unselected can be manipulated.
   */
  svtkGetObjectMacro(SliderProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the properties for the tube and end caps.
   */
  svtkGetObjectMacro(TubeProperty, svtkProperty);
  svtkGetObjectMacro(CapProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the selection property. This property is used to modify the appearance of
   * selected objects (e.g., the slider).
   */
  svtkGetObjectMacro(SelectedProperty, svtkProperty);
  //@}

  //@{
  /**
   * Methods to interface with the svtkSliderWidget.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void Highlight(int) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetActors(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /**
   * Override GetMTime to include point coordinates
   */
  svtkMTimeType GetMTime() override;

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  svtkSliderRepresentation3D();
  ~svtkSliderRepresentation3D() override;

  // Positioning the widget
  svtkCoordinate* Point1Coordinate;
  svtkCoordinate* Point2Coordinate;
  double Length;

  // These are the slider end points taking into account the thickness
  // of the slider
  double SP1[3];
  double SP2[3];

  // More ivars controlling the appearance of the widget
  double Rotation;
  int SliderShape;

  // Do the picking
  svtkCellPicker* Picker;

  // Determine the parameter t along the slider
  virtual double ComputePickPosition(double eventPos[2]);

  // The widget consists of several actors, all grouped
  // together using an assembly. This makes it easier to
  // perform the final transformation into
  svtkAssembly* WidgetAssembly;

  // Cylinder used by other objects
  svtkCylinderSource* CylinderSource;
  svtkTransformPolyDataFilter* Cylinder;

  // The tube
  svtkPolyDataMapper* TubeMapper;
  svtkActor* TubeActor;
  svtkProperty* TubeProperty;

  // The slider
  svtkSphereSource* SliderSource;
  svtkPolyDataMapper* SliderMapper;
  svtkActor* SliderActor;
  svtkProperty* SliderProperty;
  svtkProperty* SelectedProperty;

  // The left cap
  svtkPolyDataMapper* LeftCapMapper;
  svtkActor* LeftCapActor;
  svtkProperty* CapProperty;

  // The right cap
  svtkPolyDataMapper* RightCapMapper;
  svtkActor* RightCapActor;

  // The text. There is an extra transform used to rotate
  // both the title and label
  svtkVectorText* LabelText;
  svtkPolyDataMapper* LabelMapper;
  svtkActor* LabelActor;

  svtkVectorText* TitleText;
  svtkPolyDataMapper* TitleMapper;
  svtkActor* TitleActor;

  // Transform used during slider motion
  svtkMatrix4x4* Matrix;
  svtkTransform* Transform;

  // Manage the state of the widget
  enum _SliderShape
  {
    SphereShape,
    CylinderShape
  };

private:
  svtkSliderRepresentation3D(const svtkSliderRepresentation3D&) = delete;
  void operator=(const svtkSliderRepresentation3D&) = delete;
};

#endif
