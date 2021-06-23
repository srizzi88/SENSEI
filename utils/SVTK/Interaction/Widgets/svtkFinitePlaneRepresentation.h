/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFinitePlaneRepresentation.h

  Copyright (c)
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFinitePlaneRepresentation
 * @brief   represent the svtkFinitePlaneWidget.
 *
 * This class is a concrete representation for the svtkFinitePlaneWidget. It
 * represents a plane with three handles: one on two faces, plus a
 * center handle. Through interaction with the widget, the plane
 * representation can be arbitrarily positioned and modified in the 3D space.
 *
 * To use this representation, you normally use the PlaceWidget() method
 * to position the widget at a specified region in space.
 *
 * @sa
 * svtkFinitePlaneWidget svtkImplicitPlaneWidget2
 */

#ifndef svtkFinitePlaneRepresentation_h
#define svtkFinitePlaneRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkBox;
class svtkCellPicker;
class svtkConeSource;
class svtkFeatureEdges;
class svtkLineSource;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkProperty;
class svtkSphereSource;
class svtkTransform;
class svtkTubeFilter;

class SVTKINTERACTIONWIDGETS_EXPORT svtkFinitePlaneRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkFinitePlaneRepresentation* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkFinitePlaneRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Grab the polydata that defines the plane. The polydata contains a single
   * polygon.
   */
  void GetPolyData(svtkPolyData* pd);

  //@{
  /**
   * Get the handle properties (the little balls are the handles). The
   * properties of the handles, when selected or normal, can be
   * specified.
   */
  svtkGetObjectMacro(V1HandleProperty, svtkProperty);
  svtkGetObjectMacro(V2HandleProperty, svtkProperty);
  svtkGetObjectMacro(SelectedHandleProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the plane properties. The
   * properties of the plane when selected and normal can be
   * set.
   */
  svtkGetObjectMacro(PlaneProperty, svtkProperty);
  svtkGetObjectMacro(SelectedPlaneProperty, svtkProperty);
  //@}

  //@{
  /**
   * Turn on/off tubing of the wire outline of the plane. The tube thickens
   * the line by wrapping with a svtkTubeFilter.
   */
  svtkSetMacro(Tubing, bool);
  svtkGetMacro(Tubing, bool);
  svtkBooleanMacro(Tubing, bool);
  //@}

  //@{
  /**
   * Enable/disable the drawing of the plane. In some cases the plane
   * interferes with the object that it is operating on (i.e., the
   * plane interferes with the cut surface it produces producing
   * z-buffer artifacts.)
   */
  void SetDrawPlane(bool plane);
  svtkGetMacro(DrawPlane, bool);
  svtkBooleanMacro(DrawPlane, bool);
  //@}

  //@{
  /**
   * Switches handles (the spheres) on or off by manipulating the underlying
   * actor visibility.
   */
  void SetHandles(bool handles);
  virtual void HandlesOn();
  virtual void HandlesOff();
  //@}

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  double* GetBounds() override;
  //@}

  //@{
  /**
   * Methods supporting, and required by, the rendering process.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  svtkSetClampMacro(InteractionState, int, Outside, Pushing);

  //@{
  /**
   * Set/Get the origin of the plane.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  svtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * Set/Get the normal to the plane.
   */
  void SetNormal(double x, double y, double z);
  void SetNormal(double x[3]);
  svtkGetVector3Macro(Normal, double);
  //@}

  //@{
  /**
   * Set/Get the v1 vector of the plane.
   */
  void SetV1(double x, double y);
  void SetV1(double x[2]);
  svtkGetVector2Macro(V1, double);
  //@}

  //@{
  /**
   * Set/Get the v2 vector of the plane.
   */
  void SetV2(double x, double y);
  void SetV2(double x[2]);
  svtkGetVector2Macro(V2, double);
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
   * Get the properties on the normal (line and cone).
   */
  svtkGetObjectMacro(NormalProperty, svtkProperty);
  svtkGetObjectMacro(SelectedNormalProperty, svtkProperty);
  //@}

  // Methods to manipulate the plane
  void TranslateOrigin(double* p1, double* p2);
  void MovePoint1(double* p1, double* p2);
  void MovePoint2(double* p1, double* p2);
  void Push(double* p1, double* p2);
  void Rotate(int X, int Y, double* p1, double* p2, double* vpn);

  enum _InteractionState
  {
    Outside = 0,
    MoveOrigin,
    ModifyV1,
    ModifyV2,
    Moving,
    Rotating,
    Pushing
  };

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  svtkFinitePlaneRepresentation();
  ~svtkFinitePlaneRepresentation() override;

  virtual void CreateDefaultProperties();

  // Size the glyphs representing hot spots (e.g., handles)
  virtual void SizeHandles();

  void SetHighlightNormal(int highlight);
  void SetHighlightPlane(int highlight);
  void SetHighlightHandle(svtkProp* prop);

  double LastEventPosition[3];

  // the representation state
  int RepresentationState;

  // the origin
  svtkSphereSource* OriginGeometry;
  svtkPolyDataMapper* OriginMapper;
  svtkActor* OriginActor;
  double Origin[3];

  // the normal
  double Normal[3];

  // the previous normal
  double PreviousNormal[3];

  // the rotation transform
  svtkTransform* Transform;

  // the X Vector
  svtkSphereSource* V1Geometry;
  svtkPolyDataMapper* V1Mapper;
  svtkActor* V1Actor;
  double V1[3];

  // the Y Vector
  svtkSphereSource* V2Geometry;
  svtkPolyDataMapper* V2Mapper;
  svtkActor* V2Actor;
  double V2[3];

  // The + normal cone
  svtkConeSource* ConeSource;
  svtkPolyDataMapper* ConeMapper;
  svtkActor* ConeActor;

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

  // The finite plane
  svtkPolyData* PlanePolyData;
  svtkPolyDataMapper* PlaneMapper;
  svtkActor* PlaneActor;

  // Optional tubes are represented by extracting boundary edges
  svtkFeatureEdges* Edges;
  svtkTubeFilter* EdgesTuber;
  svtkPolyDataMapper* EdgesMapper;
  svtkActor* EdgesActor;
  bool Tubing;    // control whether tubing is on
  bool DrawPlane; // control whether plane is on

  // Picking objects
  svtkCellPicker* HandlePicker;
  svtkActor* CurrentHandle;

  // Transform the planes (used for rotations)
  svtkTransform* TransformRotation;

  // Support GetBounds() method
  svtkBox* BoundingBox;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* OriginHandleProperty;
  svtkProperty* V1HandleProperty;
  svtkProperty* V2HandleProperty;
  svtkProperty* SelectedHandleProperty;
  svtkProperty* PlaneProperty;
  svtkProperty* SelectedPlaneProperty;
  svtkProperty* NormalProperty;
  svtkProperty* SelectedNormalProperty;

private:
  svtkFinitePlaneRepresentation(const svtkFinitePlaneRepresentation&) = delete;
  void operator=(const svtkFinitePlaneRepresentation&) = delete;
};

#endif
