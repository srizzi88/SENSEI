/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitPlaneRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitPlaneRepresentation
 * @brief   a class defining the representation for a svtkImplicitPlaneWidget2
 *
 * This class is a concrete representation for the
 * svtkImplicitPlaneWidget2. It represents an infinite plane defined by a
 * normal and point in the context of a bounding box. Through interaction
 * with the widget, the plane can be manipulated by adjusting the plane
 * normal or moving the origin point.
 *
 * To use this representation, you normally define a (plane) origin and (plane)
 * normal. The PlaceWidget() method is also used to initially position the
 * representation.
 *
 * @warning
 * This class, and svtkImplicitPlaneWidget2, are next generation SVTK
 * widgets. An earlier version of this functionality was defined in the
 * class svtkImplicitPlaneWidget.
 *
 * @sa
 * svtkImplicitPlaneWidget2 svtkImplicitPlaneWidget
 */

#ifndef svtkImplicitPlaneRepresentation_h
#define svtkImplicitPlaneRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkPolyDataMapper;
class svtkCellPicker;
class svtkConeSource;
class svtkLineSource;
class svtkSphereSource;
class svtkTubeFilter;
class svtkPlane;
class svtkPlaneSource;
class svtkCutter;
class svtkProperty;
class svtkImageData;
class svtkOutlineFilter;
class svtkFeatureEdges;
class svtkPolyData;
class svtkPolyDataAlgorithm;
class svtkTransform;
class svtkBox;
class svtkLookupTable;

class SVTKINTERACTIONWIDGETS_EXPORT svtkImplicitPlaneRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkImplicitPlaneRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkImplicitPlaneRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Get the origin of the plane.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  double* GetOrigin() SVTK_SIZEHINT(3);
  void GetOrigin(double xyz[3]);
  //@}

  //@{
  /**
   * Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double x[3]);
  void SetNormalToCamera();
  double* GetNormal() SVTK_SIZEHINT(3);
  void GetNormal(double xyz[3]);
  //@}

  //@{
  /**
   * Force the plane widget to be aligned with one of the x-y-z axes.
   * If one axis is set on, the other two will be set off.
   * Remember that when the state changes, a ModifiedEvent is invoked.
   * This can be used to snap the plane to the axes if it is originally
   * not aligned.
   */
  void SetNormalToXAxis(svtkTypeBool);
  svtkGetMacro(NormalToXAxis, svtkTypeBool);
  svtkBooleanMacro(NormalToXAxis, svtkTypeBool);
  void SetNormalToYAxis(svtkTypeBool);
  svtkGetMacro(NormalToYAxis, svtkTypeBool);
  svtkBooleanMacro(NormalToYAxis, svtkTypeBool);
  void SetNormalToZAxis(svtkTypeBool);
  svtkGetMacro(NormalToZAxis, svtkTypeBool);
  svtkBooleanMacro(NormalToZAxis, svtkTypeBool);
  //@}

  //@{
  /**
   * If enabled, and a svtkCamera is available through the renderer, then
   * LockNormalToCamera will cause the normal to follow the camera's
   * normal.
   */
  virtual void SetLockNormalToCamera(svtkTypeBool);
  svtkGetMacro(LockNormalToCamera, svtkTypeBool);
  svtkBooleanMacro(LockNormalToCamera, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off tubing of the wire outline of the plane. The tube thickens
   * the line by wrapping with a svtkTubeFilter.
   */
  svtkSetMacro(Tubing, svtkTypeBool);
  svtkGetMacro(Tubing, svtkTypeBool);
  svtkBooleanMacro(Tubing, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable the drawing of the plane. In some cases the plane
   * interferes with the object that it is operating on (i.e., the
   * plane interferes with the cut surface it produces producing
   * z-buffer artifacts.)
   */
  void SetDrawPlane(svtkTypeBool plane);
  svtkGetMacro(DrawPlane, svtkTypeBool);
  svtkBooleanMacro(DrawPlane, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable the drawing of the outline.
   */
  void SetDrawOutline(svtkTypeBool plane);
  svtkGetMacro(DrawOutline, svtkTypeBool);
  svtkBooleanMacro(DrawOutline, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to translate the bounding box by grabbing it
   * with the left mouse button.
   */
  svtkSetMacro(OutlineTranslation, svtkTypeBool);
  svtkGetMacro(OutlineTranslation, svtkTypeBool);
  svtkBooleanMacro(OutlineTranslation, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to move the widget outside of the bounds
   * specified in the initial PlaceWidget() invocation.
   */
  svtkSetMacro(OutsideBounds, svtkTypeBool);
  svtkGetMacro(OutsideBounds, svtkTypeBool);
  svtkBooleanMacro(OutsideBounds, svtkTypeBool);
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
   * Turn on/off whether the plane should be constrained to the widget bounds.
   * If on, the origin will not be allowed to move outside the set widget bounds.
   * This is the default behaviour.
   * If off, the origin can be freely moved and the widget outline will change
   * accordingly.
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
   * Grab the polydata that defines the plane. The polydata contains a single
   * polygon that is clipped by the bounding box.
   */
  void GetPolyData(svtkPolyData* pd);

  /**
   * Satisfies superclass API.  This returns a pointer to the underlying
   * PolyData (which represents the plane).
   */
  svtkPolyDataAlgorithm* GetPolyDataAlgorithm();

  /**
   * Get the implicit function for the plane by copying the origin and normal
   * of the cut plane into the provided svtkPlane. The user must provide the
   * instance of the class svtkPlane. Note that svtkPlane is a subclass of
   * svtkImplicitFunction, meaning that it can be used by a variety of filters
   * to perform clipping, cutting, and selection of data.
   */
  void GetPlane(svtkPlane* plane);

  /**
   * Alternative way to define the cutting plane. The normal and origin of
   * the plane provided is copied into the internal instance of the class
   * cutting svtkPlane.
   */
  void SetPlane(svtkPlane* plane);

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement(void);

  //@{
  /**
   * Get the properties on the normal (line and cone).
   */
  svtkGetObjectMacro(NormalProperty, svtkProperty);
  svtkGetObjectMacro(SelectedNormalProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the plane properties. The properties of the plane when selected
   * and unselected can be manipulated.
   */
  svtkGetObjectMacro(PlaneProperty, svtkProperty);
  svtkGetObjectMacro(SelectedPlaneProperty, svtkProperty);
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
   * Specify a translation distance used by the BumpPlane() method. Note that the
   * distance is normalized; it is the fraction of the length of the bounding
   * box of the wire outline.
   */
  svtkSetClampMacro(BumpDistance, double, 0.000001, 1);
  svtkGetMacro(BumpDistance, double);
  //@}

  /**
   * Translate the plane in the direction of the normal by the
   * specified BumpDistance.  The dir parameter controls which
   * direction the pushing occurs, either in the same direction
   * as the normal, or when negative, in the opposite direction.
   * The factor controls whether what percentage of the bump is
   * used.
   */
  void BumpPlane(int dir, double factor);

  /**
   * Push the plane the distance specified along the normal. Positive
   * values are in the direction of the normal; negative values are
   * in the opposite direction of the normal. The distance value is
   * expressed in world coordinates.
   */
  void PushPlane(double distance);

  //@{
  /**
   * Methods to interface with the svtkImplicitPlaneWidget2.
   */
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double newEventPos[2]) override;
  void EndWidgetInteraction(double newEventPos[2]) override;
  void StartComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  int ComputeComplexInteractionState(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata, int modify = 0) override;
  void EndComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetActors(svtkPropCollection* pc) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  // Manage the state of the widget
  enum _InteractionState
  {
    Outside = 0,
    Moving,
    MovingOutline,
    MovingOrigin,
    Rotating,
    Pushing,
    Scaling
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g.,
   * svtkImplicitPlaneWidget2) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
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

  // Get the underlying plane object used by this rep
  // this can be used as a cropping plane in svtkMapper
  svtkPlane* GetUnderlyingPlane() { return this->Plane; }

  //@{
  /**
   * Control if the plane should be drawn cropped by the bounding box
   * or without cropping. Defaults to on.
   */
  virtual void SetCropPlaneToBoundingBox(bool);
  svtkGetMacro(CropPlaneToBoundingBox, bool);
  svtkBooleanMacro(CropPlaneToBoundingBox, bool);
  //@}

  //@{
  /**
   * For complex events should we snap orientations to
   * be aligned with the x y z axes
   */
  svtkGetMacro(SnapToAxes, bool);
  svtkSetMacro(SnapToAxes, bool);
  //@}

  //@{
  /**
   * Forces the plane's normal to be aligned with x, y or z axis.
   * The alignment happens when calling SetNormal.
   * It defers with SnapToAxes from it is always applicable, and SnapToAxes
   * only snaps when the angle difference exceeds 16 degrees in complex interactions.
   */
  svtkGetMacro(AlwaysSnapToNearestAxis, bool);
  virtual void SetAlwaysSnapToNearestAxis(bool snap)
  {
    this->AlwaysSnapToNearestAxis = snap;
    this->SetNormal(this->GetNormal());
  }
  //@}

protected:
  svtkImplicitPlaneRepresentation();
  ~svtkImplicitPlaneRepresentation() override;

  int RepresentationState;

  // Keep track of event positions
  double LastEventPosition[3];
  double LastEventOrientation[4];
  double StartEventOrientation[4];

  // Controlling ivars
  svtkTypeBool NormalToXAxis;
  svtkTypeBool NormalToYAxis;
  svtkTypeBool NormalToZAxis;

  double SnappedEventOrientation[4];
  bool SnappedOrientation;
  bool SnapToAxes;

  bool AlwaysSnapToNearestAxis;

  // Locking normal to camera
  svtkTypeBool LockNormalToCamera;

  // Controlling the push operation
  double BumpDistance;

  // The actual plane which is being manipulated
  svtkPlane* Plane;

  int TranslationAxis;

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
  svtkTypeBool ConstrainToWidgetBounds;

  // The cut plane is produced with a svtkCutter
  svtkCutter* Cutter;
  svtkPlaneSource* PlaneSource;
  svtkPolyDataMapper* CutMapper;
  svtkActor* CutActor;
  svtkTypeBool DrawPlane;
  svtkTypeBool DrawOutline;
  void HighlightPlane(int highlight);

  // Optional tubes are represented by extracting boundary edges and tubing
  svtkFeatureEdges* Edges;
  svtkTubeFilter* EdgesTuber;
  svtkPolyDataMapper* EdgesMapper;
  svtkActor* EdgesActor;
  svtkTypeBool Tubing; // control whether tubing is on

  // The + normal cone
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

  // The origin positioning handle
  svtkSphereSource* Sphere;
  svtkPolyDataMapper* SphereMapper;
  svtkActor* SphereActor;

  // Do the picking
  svtkCellPicker* Picker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Transform the normal (used for rotation)
  svtkTransform* Transform;

  // Methods to manipulate the plane
  void Rotate(double X, double Y, double* p1, double* p2, double* vpn);
  void Rotate3D(double* p1, double* p2);
  void TranslatePlane(double* p1, double* p2);
  void TranslateOutline(double* p1, double* p2);
  void TranslateOrigin(double* p1, double* p2);
  void UpdatePose(double* p1, double* d1, double* p2, double* d2);
  void Push(double* p1, double* p2);
  void Scale(double* p1, double* p2, double X, double Y);
  void SizeHandles();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* NormalProperty;
  svtkProperty* SelectedNormalProperty;
  svtkProperty* PlaneProperty;
  svtkProperty* SelectedPlaneProperty;
  svtkProperty* OutlineProperty;
  svtkProperty* SelectedOutlineProperty;
  svtkProperty* EdgesProperty;
  void CreateDefaultProperties();

  void GeneratePlane();

  bool CropPlaneToBoundingBox;

  // Support GetBounds() method
  svtkBox* BoundingBox;

private:
  svtkImplicitPlaneRepresentation(const svtkImplicitPlaneRepresentation&) = delete;
  void operator=(const svtkImplicitPlaneRepresentation&) = delete;
};

#endif
