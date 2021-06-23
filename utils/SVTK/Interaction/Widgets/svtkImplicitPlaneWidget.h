/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitPlaneWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitPlaneWidget
 * @brief   3D widget for manipulating an infinite plane
 *
 * This 3D widget defines an infinite plane that can be interactively placed
 * in a scene. The widget is represented by a plane with a normal vector; the
 * plane is contained by a bounding box, and where the plane intersects the
 * bounding box the edges are shown (possibly tubed). The normal can be
 * selected and moved to rotate the plane; the plane itself can be selected
 * and translated in various directions. As the plane is moved, the implicit
 * plane function and polygon (representing the plane cut against the bounding
 * box) is updated.
 *
 * To use this object, just invoke SetInteractor() with the argument of the
 * method a svtkRenderWindowInteractor.  You may also wish to invoke
 * "PlaceWidget()" to initially position the widget. If the "i" key (for
 * "interactor") is pressed, the svtkImplicitPlaneWidget will appear. (See
 * superclass documentation for information about changing this behavior.)
 * If you select the normal vector, the plane can be arbitrarily rotated. The
 * plane can be translated along the normal by selecting the plane and moving
 * it. The plane (the plane origin) can also be arbitrary moved by selecting
 * the plane with the middle mouse button. The right mouse button can be used
 * to uniformly scale the bounding box (moving "up" the box scales larger;
 * moving "down" the box scales smaller). Events that occur outside of the
 * widget (i.e., no part of the widget is picked) are propagated to any other
 * registered obsevers (such as the interaction style).  Turn off the widget
 * by pressing the "i" key again (or invoke the Off() method).
 *
 * The svtkImplicitPlaneWidget has several methods that can be used in
 * conjunction with other SVTK objects.  The GetPolyData() method can be used
 * to get a polygonal representation (the single polygon clipped by the
 * bounding box).  Typical usage of the widget is to make use of the
 * StartInteractionEvent, InteractionEvent, and EndInteractionEvent
 * events. The InteractionEvent is called on mouse motion; the other two
 * events are called on button down and button up (either left or right
 * button). (Note: there is also a PlaceWidgetEvent that is invoked when
 * the widget is placed with PlaceWidget().)
 *
 * Some additional features of this class include the ability to control the
 * properties of the widget. You do this by setting property values on the
 * normal vector (selected and unselected properties); the plane (selected
 * and unselected properties); the outline (selected and unselected
 * properties); and the edges. The edges may also be tubed or not.
 *
 * @sa
 * svtk3DWidget svtkBoxWidget svtkPlaneWidget svtkLineWidget svtkPointWidget
 * svtkSphereWidget svtkImagePlaneWidget
 */

#ifndef svtkImplicitPlaneWidget_h
#define svtkImplicitPlaneWidget_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPolyDataSourceWidget.h"

class svtkActor;
class svtkPolyDataMapper;
class svtkCellPicker;
class svtkConeSource;
class svtkLineSource;
class svtkSphereSource;
class svtkTubeFilter;
class svtkPlane;
class svtkCutter;
class svtkProperty;
class svtkImageData;
class svtkOutlineFilter;
class svtkFeatureEdges;
class svtkPolyData;
class svtkTransform;

class SVTKINTERACTIONWIDGETS_EXPORT svtkImplicitPlaneWidget : public svtkPolyDataSourceWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkImplicitPlaneWidget* New();

  svtkTypeMacro(svtkImplicitPlaneWidget, svtkPolyDataSourceWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Methods that satisfy the superclass' API.
   */
  void SetEnabled(int) override;
  void PlaceWidget(double bounds[6]) override;
  void PlaceWidget() override { this->Superclass::PlaceWidget(); }
  void PlaceWidget(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax) override
  {
    this->Superclass::PlaceWidget(xmin, xmax, ymin, ymax, zmin, zmax);
  }
  //@}

  //@{
  /**
   * Get the origin of the plane.
   */
  virtual void SetOrigin(double x, double y, double z);
  virtual void SetOrigin(double x[3]);
  double* GetOrigin() SVTK_SIZEHINT(3);
  void GetOrigin(double xyz[3]);
  //@}

  //@{
  /**
   * Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double x[3]);
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
   * Turn on/off the ability to translate the bounding box by grabbing it
   * with the left mouse button.
   */
  svtkSetMacro(OutlineTranslation, svtkTypeBool);
  svtkGetMacro(OutlineTranslation, svtkTypeBool);
  svtkBooleanMacro(OutlineTranslation, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to move the widget outside of the input's bound
   */
  svtkSetMacro(OutsideBounds, svtkTypeBool);
  svtkGetMacro(OutsideBounds, svtkTypeBool);
  svtkBooleanMacro(OutsideBounds, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to scale with the mouse
   */
  svtkSetMacro(ScaleEnabled, svtkTypeBool);
  svtkGetMacro(ScaleEnabled, svtkTypeBool);
  svtkBooleanMacro(ScaleEnabled, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the ability to translate the origin (sphere)
   * with the left mouse button.
   */
  svtkSetMacro(OriginTranslation, svtkTypeBool);
  svtkGetMacro(OriginTranslation, svtkTypeBool);
  svtkBooleanMacro(OriginTranslation, svtkTypeBool);
  //@}

  //@{
  /**
   * By default the arrow is 30% of the diagonal length. DiagonalRatio control
   * this ratio in the interval [0-2]
   */
  svtkSetClampMacro(DiagonalRatio, double, 0, 2);
  svtkGetMacro(DiagonalRatio, double);
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
  svtkPolyDataAlgorithm* GetPolyDataAlgorithm() override;

  /**
   * Get the implicit function for the plane. The user must provide the
   * instance of the class svtkPlane. Note that svtkPlane is a subclass of
   * svtkImplicitFunction, meaning that it can be used by a variety of filters
   * to perform clipping, cutting, and selection of data.
   */
  void GetPlane(svtkPlane* plane);

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement() override;

  /**
   * Control widget appearance
   */
  void SizeHandles() override;

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

protected:
  svtkImplicitPlaneWidget();
  ~svtkImplicitPlaneWidget() override;

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Start = 0,
    MovingPlane,
    MovingOutline,
    MovingOrigin,
    Scaling,
    Pushing,
    Rotating,
    Outside
  };

  // handles the events
  static void ProcessEvents(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnMiddleButtonDown();
  void OnMiddleButtonUp();
  void OnRightButtonDown();
  void OnRightButtonUp();
  void OnMouseMove();

  // Controlling ivars
  svtkTypeBool NormalToXAxis;
  svtkTypeBool NormalToYAxis;
  svtkTypeBool NormalToZAxis;
  void UpdateRepresentation();

  // The actual plane which is being manipulated
  svtkPlane* Plane;

  // The bounding box is represented by a single voxel image data
  svtkImageData* Box;
  svtkOutlineFilter* Outline;
  svtkPolyDataMapper* OutlineMapper;
  svtkActor* OutlineActor;
  void HighlightOutline(int highlight);
  svtkTypeBool OutlineTranslation; // whether the outline can be moved
  svtkTypeBool ScaleEnabled;       // whether the widget can be scaled
  svtkTypeBool OutsideBounds;      // whether the widget can be moved outside input's bounds

  // The cut plane is produced with a svtkCutter
  svtkCutter* Cutter;
  svtkPolyDataMapper* CutMapper;
  svtkActor* CutActor;
  svtkTypeBool DrawPlane;
  virtual void HighlightPlane(int highlight);

  // Optional tubes are represented by extracting boundary edges and tubing
  svtkFeatureEdges* Edges;
  svtkTubeFilter* EdgesTuber;
  svtkPolyDataMapper* EdgesMapper;
  svtkActor* EdgesActor;
  svtkTypeBool Tubing; // control whether tubing is on

  // Control final length of the arrow:
  double DiagonalRatio;

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
  svtkTypeBool OriginTranslation; // whether the origin (sphere) can be moved

  // Do the picking
  svtkCellPicker* Picker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Transform the normal (used for rotation)
  svtkTransform* Transform;

  // Methods to manipulate the plane
  void ConstrainOrigin(double x[3]);
  void Rotate(int X, int Y, double* p1, double* p2, double* vpn);
  void TranslatePlane(double* p1, double* p2);
  void TranslateOutline(double* p1, double* p2);
  void TranslateOrigin(double* p1, double* p2);
  void Push(double* p1, double* p2);
  void Scale(double* p1, double* p2, int X, int Y);

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

private:
  svtkImplicitPlaneWidget(const svtkImplicitPlaneWidget&) = delete;
  void operator=(const svtkImplicitPlaneWidget&) = delete;
};

#endif
