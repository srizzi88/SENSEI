/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLineRepresentation
 * @brief   a class defining the representation for a svtkLineWidget2
 *
 * This class is a concrete representation for the svtkLineWidget2. It
 * represents a straight line with three handles: one at the beginning and
 * ending of the line, and one used to translate the line. Through
 * interaction with the widget, the line representation can be arbitrarily
 * placed in the 3D space.
 *
 * To use this representation, you normally specify the position of the two
 * end points (either in world or display coordinates). The PlaceWidget()
 * method is also used to initially position the representation.
 *
 * @warning
 * This class, and svtkLineWidget2, are next generation SVTK
 * widgets. An earlier version of this functionality was defined in the
 * class svtkLineWidget.
 *
 * @sa
 * svtkLineWidget2 svtkLineWidget
 */

#ifndef svtkLineRepresentation_h
#define svtkLineRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkConeSource;
class svtkPolyDataMapper;
class svtkLineSource;
class svtkProperty;
class svtkPolyData;
class svtkPolyDataAlgorithm;
class svtkPointHandleRepresentation3D;
class svtkBox;
class svtkFollower;
class svtkVectorText;
class svtkPolyDataMapper;
class svtkCellPicker;

class SVTKINTERACTIONWIDGETS_EXPORT svtkLineRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkLineRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkLineRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Methods to Set/Get the coordinates of the two points defining
   * this representation. Note that methods are available for both
   * display and world coordinates.
   */
  void GetPoint1WorldPosition(double pos[3]);
  double* GetPoint1WorldPosition() SVTK_SIZEHINT(3);
  void GetPoint1DisplayPosition(double pos[3]);
  double* GetPoint1DisplayPosition() SVTK_SIZEHINT(3);
  void SetPoint1WorldPosition(double pos[3]);
  void SetPoint1DisplayPosition(double pos[3]);
  void GetPoint2DisplayPosition(double pos[3]);
  double* GetPoint2DisplayPosition() SVTK_SIZEHINT(3);
  void GetPoint2WorldPosition(double pos[3]);
  double* GetPoint2WorldPosition() SVTK_SIZEHINT(3);
  void SetPoint2WorldPosition(double pos[3]);
  void SetPoint2DisplayPosition(double pos[3]);
  //@}

  //@{
  /**
   * This method is used to specify the type of handle representation to
   * use for the three internal svtkHandleWidgets within svtkLineWidget2.
   * To use this method, create a dummy svtkHandleWidget (or subclass),
   * and then invoke this method with this dummy. Then the
   * svtkLineRepresentation uses this dummy to clone three svtkHandleWidgets
   * of the same type. Make sure you set the handle representation before
   * the widget is enabled. (The method InstantiateHandleRepresentation()
   * is invoked by the svtkLineWidget2.)
   */
  void SetHandleRepresentation(svtkPointHandleRepresentation3D* handle);
  void InstantiateHandleRepresentation();
  //@}

  //@{
  /**
   * Get the three handle representations used for the svtkLineWidget2.
   */
  svtkGetObjectMacro(Point1Representation, svtkPointHandleRepresentation3D);
  svtkGetObjectMacro(Point2Representation, svtkPointHandleRepresentation3D);
  svtkGetObjectMacro(LineHandleRepresentation, svtkPointHandleRepresentation3D);
  //@}

  //@{
  /**
   * Get the end-point (sphere) properties. The properties of the end-points
   * when selected and unselected can be manipulated.
   */
  svtkGetObjectMacro(EndPointProperty, svtkProperty);
  svtkGetObjectMacro(SelectedEndPointProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the end-point (sphere) properties. The properties of the end-points
   * when selected and unselected can be manipulated.
   */
  svtkGetObjectMacro(EndPoint2Property, svtkProperty);
  svtkGetObjectMacro(SelectedEndPoint2Property, svtkProperty);
  //@}

  //@{
  /**
   * Get the line properties. The properties of the line when selected
   * and unselected can be manipulated.
   */
  svtkGetObjectMacro(LineProperty, svtkProperty);
  svtkGetObjectMacro(SelectedLineProperty, svtkProperty);
  //@}

  //@{
  /**
   * The tolerance representing the distance to the widget (in pixels) in
   * which the cursor is considered near enough to the line or end point
   * to be active.
   */
  svtkSetClampMacro(Tolerance, int, 1, 100);
  svtkGetMacro(Tolerance, int);
  //@}

  //@{
  /**
   * Set/Get the resolution (number of subdivisions) of the line. A line with
   * resolution greater than one is useful when points along the line are
   * desired; e.g., generating a rake of streamlines.
   */
  void SetResolution(int res);
  int GetResolution();
  //@}

  /**
   * Retrieve the polydata (including points) that defines the line.  The
   * polydata consists of n+1 points, where n is the resolution of the
   * line. These point values are guaranteed to be up-to-date whenever any
   * one of the three handles are moved. To use this method, the user
   * provides the svtkPolyData as an input argument, and the points and
   * polyline are copied into it.
   */
  void GetPolyData(svtkPolyData* pd);

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  double* GetBounds() SVTK_SIZEHINT(6) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  void GetActors(svtkPropCollection* pc) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  // Manage the state of the widget
  enum
  {
    Outside = 0,
    OnP1,
    OnP2,
    TranslatingP1,
    TranslatingP2,
    OnLine,
    Scaling
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
  svtkSetClampMacro(InteractionState, int, Outside, Scaling);
  //@}

  //@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(int);
  svtkGetMacro(RepresentationState, int);
  //@}

  //@{
  /**
   * Sets the representation to be a directional line with point 1 represented
   * as a cone.
   */
  void SetDirectionalLine(bool val);
  svtkGetMacro(DirectionalLine, bool);
  svtkBooleanMacro(DirectionalLine, bool);
  //@}

  /**
   * Overload the superclasses' GetMTime() because internal classes
   * are used to keep the state of the representation.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Overridden to set the rendererer on the internal representations.
   */
  void SetRenderer(svtkRenderer* ren) override;

  //@{
  /**
   * Show the distance between the points.
   */
  svtkSetMacro(DistanceAnnotationVisibility, svtkTypeBool);
  svtkGetMacro(DistanceAnnotationVisibility, svtkTypeBool);
  svtkBooleanMacro(DistanceAnnotationVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the format to use for labelling the line. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the angle value.
   */
  svtkSetStringMacro(DistanceAnnotationFormat);
  svtkGetStringMacro(DistanceAnnotationFormat);
  //@}

  //@{
  /**
   * Scale text (font size along each dimension).
   */
  void SetDistanceAnnotationScale(double x, double y, double z)
  {
    double scale[3];
    scale[0] = x;
    scale[1] = y;
    scale[2] = z;
    this->SetDistanceAnnotationScale(scale);
  }
  virtual void SetDistanceAnnotationScale(double scale[3]);
  virtual double* GetDistanceAnnotationScale() SVTK_SIZEHINT(3);
  //@}

  /**
   * Get the distance between the points.
   */
  double GetDistance();

  /**
   * Convenience method to set the line color.
   * Ideally one should use GetLineProperty()->SetColor().
   */
  void SetLineColor(double r, double g, double b);

  /**
   * Get the distance annotation property
   */
  virtual svtkProperty* GetDistanceAnnotationProperty();

  //@{
  /**
   * Get the text actor
   */
  svtkGetObjectMacro(TextActor, svtkFollower);
  //@}

  enum
  {
    RestrictNone = 0,
    RestrictToX,
    RestrictToY,
    RestrictToZ
  };

  /**
   * Set if translations should be restricted to one of the axes (disabled if
   * RestrictNone is specified).
   */
  SVTK_LEGACY(void SetRestrictFlag(int restrict_flag));

protected:
  svtkLineRepresentation();
  ~svtkLineRepresentation() override;

  // The handle and the rep used to close the handles
  svtkPointHandleRepresentation3D* HandleRepresentation;
  svtkPointHandleRepresentation3D* Point1Representation;
  svtkPointHandleRepresentation3D* Point2Representation;
  svtkPointHandleRepresentation3D* LineHandleRepresentation;

  // Manage how the representation appears
  int RepresentationState;
  bool DirectionalLine;

  // the line
  svtkActor* LineActor;
  svtkPolyDataMapper* LineMapper;
  svtkLineSource* LineSource;

  // glyphs representing hot spots (e.g., handles)
  svtkActor** Handle;
  svtkPolyDataMapper** HandleMapper;
  svtkPolyDataAlgorithm** HandleGeometry;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* EndPointProperty;
  svtkProperty* SelectedEndPointProperty;
  svtkProperty* EndPoint2Property;
  svtkProperty* SelectedEndPoint2Property;
  svtkProperty* LineProperty;
  svtkProperty* SelectedLineProperty;
  void CreateDefaultProperties();

  // Selection tolerance for the handles and the line
  int Tolerance;

  // Helper members
  int ClampToBounds;
  void ClampPosition(double x[3]);
  void HighlightPoint(int ptId, int highlight);
  void HighlightLine(int highlight);
  int InBounds(double x[3]);
  void SizeHandles();

  // Ivars used during widget interaction to hold initial positions
  double StartP1[3];
  double StartP2[3];
  double StartLineHandle[3];
  double Length;
  double LastEventPosition[3];

  // Support GetBounds() method
  svtkBox* BoundingBox;

  // Need to keep track if we have successfully initialized the display position.
  // The widget tends to do stuff in world coordinates, put if the renderer has
  // not been assigned, then certain operations do not properly update the display
  // position.
  int InitializedDisplayPosition;

  // Format for the label
  svtkTypeBool DistanceAnnotationVisibility;
  char* DistanceAnnotationFormat;

  svtkFollower* TextActor;
  svtkPolyDataMapper* TextMapper;
  svtkVectorText* TextInput;
  double Distance;
  bool AnnotationTextScaleInitialized;

  svtkCellPicker* LinePicker;

private:
  svtkLineRepresentation(const svtkLineRepresentation&) = delete;
  void operator=(const svtkLineRepresentation&) = delete;
};

#endif
