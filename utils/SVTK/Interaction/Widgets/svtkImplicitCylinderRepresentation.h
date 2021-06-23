/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitCylinderRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitCylinderRepresentation
 * @brief   defining the representation for a svtkImplicitCylinderWidget
 *
 * This class is a concrete representation for the
 * svtkImplicitCylinderWidget. It represents an infinite cylinder
 * defined by a radius, a center, and an axis. The cylinder is placed
 * within its associated bounding box and the intersection of the
 * cylinder with the bounding box is shown to visually indicate the
 * orientation and position of the representation. This cylinder
 * representation can be manipulated by using the
 * svtkImplicitCylinderWidget to adjust the cylinder radius, axis,
 * and/or center point. (Note that the bounding box is defined during
 * invocation of the superclass' PlaceWidget() method.)
 *
 * To use this representation, you normally specify a radius, center,
 * and axis. Optionally you can specify a minimum and maximum radius,
 * and a resolution for the cylinder. Finally, place the widget and
 * its representation in the scene using PlaceWidget().
 *
 * @sa
 * svtkImplicitCylinderWidget svtkImplicitPlaneWidget svtkImplicitPlaneWidget
 */

#ifndef svtkImplicitCylinderRepresentation_h
#define svtkImplicitCylinderRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkPolyDataMapper;
class svtkCellPicker;
class svtkConeSource;
class svtkLineSource;
class svtkSphereSource;
class svtkTubeFilter;
class svtkCylinder;
class svtkProperty;
class svtkImageData;
class svtkOutlineFilter;
class svtkFeatureEdges;
class svtkPolyData;
class svtkPolyDataAlgorithm;
class svtkTransform;
class svtkBox;
class svtkLookupTable;

#define SVTK_MAX_CYL_RESOLUTION 2048

class SVTKINTERACTIONWIDGETS_EXPORT svtkImplicitCylinderRepresentation
  : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkImplicitCylinderRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkImplicitCylinderRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Get the center of the cylinder. The center is located along the
   * cylinder axis.
   */
  void SetCenter(double x, double y, double z);
  void SetCenter(double x[3]);
  double* GetCenter() SVTK_SIZEHINT(3);
  void GetCenter(double xyz[3]);
  //@}

  //@{
  /**
   * Set/Get the axis of rotation for the cylinder. If the axis is not
   * specified as a unit vector, it will be normalized.
   */
  void SetAxis(double x, double y, double z);
  void SetAxis(double a[3]);
  double* GetAxis() SVTK_SIZEHINT(3);
  void GetAxis(double a[3]);
  //@}

  //@{
  /**
   * Set/Get the radius of the cylinder. Note that if the radius is
   * too big the cylinder will be outside of the bounding box.
   */
  void SetRadius(double r);
  double GetRadius();
  //@}

  //@{
  /**
   * Set/Get the minimum and maximum radius of the cylinder. This
   * helps prevent the cylinder from "disappearing" during
   * interaction.  Note that the minimum and maximum radius is
   * specified as a fraction of the diagonal length of the widget
   * bounding box.
   */
  svtkSetClampMacro(MinRadius, double, 0.001, 0.25);
  svtkGetMacro(MinRadius, double);
  svtkSetClampMacro(MaxRadius, double, 0.25, SVTK_FLOAT_MAX);
  svtkGetMacro(MaxRadius, double);
  //@}

  //@{
  /**
   * Force the cylinder widget to be aligned with one of the x-y-z axes.
   * If one axis is set on, the other two will be set off.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the cylinder to the axes if it is originally
   * not aligned.
   */
  void SetAlongXAxis(svtkTypeBool);
  svtkGetMacro(AlongXAxis, svtkTypeBool);
  svtkBooleanMacro(AlongXAxis, svtkTypeBool);
  void SetAlongYAxis(svtkTypeBool);
  svtkGetMacro(AlongYAxis, svtkTypeBool);
  svtkBooleanMacro(AlongYAxis, svtkTypeBool);
  void SetAlongZAxis(svtkTypeBool);
  svtkGetMacro(AlongZAxis, svtkTypeBool);
  svtkBooleanMacro(AlongZAxis, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable the drawing of the cylinder. In some cases the cylinder
   * interferes with the object that it is operating on (e.g., the
   * cylinder interferes with the cut surface it produces resulting in
   * z-buffer artifacts.) By default it is off.
   */
  void SetDrawCylinder(svtkTypeBool drawCyl);
  svtkGetMacro(DrawCylinder, svtkTypeBool);
  svtkBooleanMacro(DrawCylinder, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the resolution of the cylinder. This is the number of
   * polygonal facets used to approximate the curved cylindrical
   * surface (for rendering purposes). An svtkCylinder is used under
   * the hood to provide an exact surface representation.
   */
  svtkSetClampMacro(Resolution, int, 8, SVTK_MAX_CYL_RESOLUTION);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Turn on/off tubing of the wire outline of the cylinder
   * intersection (against the bounding box). The tube thickens the
   * line by wrapping with a svtkTubeFilter.
   */
  svtkSetMacro(Tubing, svtkTypeBool);
  svtkGetMacro(Tubing, svtkTypeBool);
  svtkBooleanMacro(Tubing, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to translate the bounding box by moving it
   * with the mouse.
   */
  svtkSetMacro(OutlineTranslation, svtkTypeBool);
  svtkGetMacro(OutlineTranslation, svtkTypeBool);
  svtkBooleanMacro(OutlineTranslation, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to move the widget outside of the bounds
   * specified in the PlaceWidget() invocation.
   */
  svtkSetMacro(OutsideBounds, svtkTypeBool);
  svtkGetMacro(OutsideBounds, svtkTypeBool);
  svtkBooleanMacro(OutsideBounds, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the bounds of the widget representation. PlaceWidget can also be
   * used to set the bounds of the widget but it may also have other effects
   * on the internal state of the representation. Use this function when only
   * the widget bounds are needs to be modified.
   */
  svtkSetVector6Macro(WidgetBounds, double);
  svtkGetVector6Macro(WidgetBounds, double);
  //@}

  //@{
  /**
   * Turn on/off whether the cylinder should be constrained to the widget bounds.
   * If on, the center will not be allowed to move outside the set widget bounds
   * and the radius will be limited by MinRadius and MaxRadius. This is the
   * default behaviour.
   * If off, the center can be freely moved and the radius can be set to
   * arbitrary values. The widget outline will change accordingly.
   */
  svtkSetMacro(ConstrainToWidgetBounds, svtkTypeBool);
  svtkGetMacro(ConstrainToWidgetBounds, svtkTypeBool);
  svtkBooleanMacro(ConstrainToWidgetBounds, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to scale the widget with the mouse.
   */
  svtkSetMacro(ScaleEnabled, svtkTypeBool);
  svtkGetMacro(ScaleEnabled, svtkTypeBool);
  svtkBooleanMacro(ScaleEnabled, svtkTypeBool);
  //@}

  /**
   * Get the implicit function for the cylinder. The user must provide the
   * instance of the class svtkCylinder. Note that svtkCylinder is a subclass of
   * svtkImplicitFunction, meaning that it can be used by a variety of filters
   * to perform clipping, cutting, and selection of data.
   */
  void GetCylinder(svtkCylinder* cyl);

  /**
   * Grab the polydata that defines the cylinder. The polydata contains
   * polygons that are clipped by the bounding box.
   */
  void GetPolyData(svtkPolyData* pd);

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource.
   */
  void UpdatePlacement(void);

  //@{
  /**
   * Get the properties on the axis (line and cone).
   */
  svtkGetObjectMacro(AxisProperty, svtkProperty);
  svtkGetObjectMacro(SelectedAxisProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the cylinder properties. The properties of the cylinder when selected
   * and unselected can be manipulated.
   */
  svtkGetObjectMacro(CylinderProperty, svtkProperty);
  svtkGetObjectMacro(SelectedCylinderProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the property of the outline.
   */
  svtkGetObjectMacro(OutlineProperty, svtkProperty);
  svtkGetObjectMacro(SelectedOutlineProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the property of the intersection edges. (This property also
   * applies to the edges when tubed.)
   */
  svtkGetObjectMacro(EdgesProperty, svtkProperty);
  //@}
  //@{
  /**
   * Set color to the edge
   */
  void SetEdgeColor(svtkLookupTable*);
  void SetEdgeColor(double, double, double);
  void SetEdgeColor(double x[3]);
  //@}

  //@{
  /**
   * Methods to interface with the svtkImplicitCylinderWidget.
   */
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void EndWidgetInteraction(double newEventPos[2]) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  double* GetBounds() override;
  void GetActors(svtkPropCollection* pc) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Specify a translation distance used by the BumpCylinder() method. Note that the
   * distance is normalized; it is the fraction of the length of the bounding
   * box of the wire outline.
   */
  svtkSetClampMacro(BumpDistance, double, 0.000001, 1);
  svtkGetMacro(BumpDistance, double);
  //@}

  /**
   * Translate the cylinder in the direction of the view vector by the
   * specified BumpDistance. The dir parameter controls which
   * direction the pushing occurs, either in the same direction as the
   * view vector, or when negative, in the opposite direction.  The factor
   * controls what percentage of the bump is used.
   */
  void BumpCylinder(int dir, double factor);

  /**
   * Push the cylinder the distance specified along the view
   * vector. Positive values are in the direction of the view vector;
   * negative values are in the opposite direction. The distance value
   * is expressed in world coordinates.
   */
  void PushCylinder(double distance);

  // Manage the state of the widget
  enum _InteractionState
  {
    Outside = 0,
    Moving,
    MovingOutline,
    MovingCenter,
    RotatingAxis,
    AdjustingRadius,
    Scaling,
    TranslatingCenter
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g.,
   * svtkImplicitCylinderWidget) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
   */
  svtkSetClampMacro(InteractionState, int, Outside, TranslatingCenter);
  //@}

  //@{
  /**
   * Sets the visual appearance of the representation based on the
   * state it is in. This state is usually the same as InteractionState.
   */
  virtual void SetRepresentationState(int);
  svtkGetMacro(RepresentationState, int);
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  //@{
  /**
   * Gets/Sets the constraint axis for translations. Returns Axis::NONE
   * if none.
   **/
  svtkGetMacro(TranslationAxis, int);
  svtkSetClampMacro(TranslationAxis, int, -1, 2);
  //@}

  //@{
  /**
   * Toggles constraint translation axis on/off.
   */
  void SetXTranslationAxisOn() { this->TranslationAxis = Axis::XAxis; }
  void SetYTranslationAxisOn() { this->TranslationAxis = Axis::YAxis; }
  void SetZTranslationAxisOn() { this->TranslationAxis = Axis::ZAxis; }
  void SetTranslationAxisOff() { this->TranslationAxis = Axis::NONE; }
  //@}

  //@{
  /**
   * Returns true if ContrainedAxis
   **/
  bool IsTranslationConstrained() { return this->TranslationAxis != Axis::NONE; }
  //@}

protected:
  svtkImplicitCylinderRepresentation();
  ~svtkImplicitCylinderRepresentation() override;

  int RepresentationState;

  int TranslationAxis;

  // Keep track of event positions
  double LastEventPosition[3];

  // Control the radius
  double MinRadius;
  double MaxRadius;

  // Controlling the push operation
  double BumpDistance;

  // Controlling ivars
  svtkTypeBool AlongXAxis;
  svtkTypeBool AlongYAxis;
  svtkTypeBool AlongZAxis;

  // The actual cylinder which is being manipulated
  svtkCylinder* Cylinder;

  // The facet resolution for rendering purposes.
  int Resolution;

  // The bounding box is represented by a single voxel image data
  svtkImageData* Box;
  svtkOutlineFilter* Outline;
  svtkPolyDataMapper* OutlineMapper;
  svtkActor* OutlineActor;
  void HighlightOutline(int highlight);
  svtkTypeBool OutlineTranslation; // whether the outline can be moved
  svtkTypeBool ScaleEnabled;       // whether the widget can be scaled
  svtkTypeBool OutsideBounds;      // whether the widget can be moved outside input's bounds
  double WidgetBounds[6];
  int ConstrainToWidgetBounds;

  // The cut cylinder is produced with a svtkCutter
  svtkPolyData* Cyl;
  svtkPolyDataMapper* CylMapper;
  svtkActor* CylActor;
  svtkTypeBool DrawCylinder;
  void HighlightCylinder(int highlight);

  // Optional tubes are represented by extracting boundary edges and tubing
  svtkFeatureEdges* Edges;
  svtkTubeFilter* EdgesTuber;
  svtkPolyDataMapper* EdgesMapper;
  svtkActor* EdgesActor;
  svtkTypeBool Tubing; // control whether tubing is on

  // The + normal cone (i.e., in positive direction along normal)
  svtkConeSource* ConeSource;
  svtkPolyDataMapper* ConeMapper;
  svtkActor* ConeActor;
  void HighlightNormal(int highlight);

  // The + normal line
  svtkLineSource* LineSource;
  svtkPolyDataMapper* LineMapper;
  svtkActor* LineActor;

  // The - normal cone
  svtkConeSource* ConeSource2;
  svtkPolyDataMapper* ConeMapper2;
  svtkActor* ConeActor2;

  // The - normal line
  svtkLineSource* LineSource2;
  svtkPolyDataMapper* LineMapper2;
  svtkActor* LineActor2;

  // The center positioning handle
  svtkSphereSource* Sphere;
  svtkPolyDataMapper* SphereMapper;
  svtkActor* SphereActor;

  // Do the picking
  svtkCellPicker* Picker;
  svtkCellPicker* CylPicker;

  // Transform the normal (used for rotation)
  svtkTransform* Transform;

  // Methods to manipulate the cylinder
  void Rotate(double X, double Y, double* p1, double* p2, double* vpn);
  void TranslateCylinder(double* p1, double* p2);
  void TranslateOutline(double* p1, double* p2);
  void TranslateCenter(double* p1, double* p2);
  void TranslateCenterOnAxis(double* p1, double* p2);
  void ScaleRadius(double* p1, double* p2);
  void AdjustRadius(double X, double Y, double* p1, double* p2);
  void Scale(double* p1, double* p2, double X, double Y);
  void SizeHandles();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* AxisProperty;
  svtkProperty* SelectedAxisProperty;
  svtkProperty* CylinderProperty;
  svtkProperty* SelectedCylinderProperty;
  svtkProperty* OutlineProperty;
  svtkProperty* SelectedOutlineProperty;
  svtkProperty* EdgesProperty;
  void CreateDefaultProperties();

  // Intersect oriented infinite cylinder against bounding box
  void BuildCylinder();

  // Support GetBounds() method
  svtkBox* BoundingBox;

private:
  svtkImplicitCylinderRepresentation(const svtkImplicitCylinderRepresentation&) = delete;
  void operator=(const svtkImplicitCylinderRepresentation&) = delete;
};

#endif
