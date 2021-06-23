/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelopipedRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParallelopipedRepresentation
 * @brief   Default representation for svtkParallelopipedWidget
 *
 * This class provides the default geometrical representation for
 * svtkParallelopipedWidget. As a result of interactions of the widget, this
 * representation can take on of the following shapes:
 * <p>1) A parallelopiped. (8 handles, 6 faces)
 * <p>2) Paralleopiped with a chair depression on any one handle. (A chair
 * is a depression on one of the handles that carves inwards so as to allow
 * the user to visualize cuts in the volume). (14 handles, 9 faces).
 *
 * @sa
 * svtkParallelopipedWidget
 */

#ifndef svtkParallelopipedRepresentation_h
#define svtkParallelopipedRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkPlane;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkProperty;
class svtkCellArray;
class svtkTransform;
class svtkHandleRepresentation;
class svtkClosedSurfacePointPlacer;
class svtkPlaneCollection;
class svtkParallelopipedTopology;

class SVTKINTERACTIONWIDGETS_EXPORT svtkParallelopipedRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkParallelopipedRepresentation* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkParallelopipedRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Methods to satisfy the superclass.
   */
  void GetActors(svtkPropCollection* pc) override;

  //@{
  /**
   * Place the widget in the scene. You can use either of the two APIs :
   * 1) PlaceWidget( double bounds[6] )
   * Creates a cuboid conforming to the said bounds.
   * 2) PlaceWidget( double corners[8][3] )
   * Creates a parallelopiped with corners specified. The order in
   * which corners are specified must obey the following rule:
   * Corner 0 - 1 - 2 - 3 - 0  forms a face
   * Corner 4 - 5 - 6 - 7 - 4  forms a face
   * Corner 0 - 4 - 5 - 1 - 0  forms a face
   * Corner 1 - 5 - 6 - 2 - 1  forms a face
   * Corner 2 - 6 - 7 - 3 - 2  forms a face
   * Corner 3 - 7 - 4 - 0 - 3  forms a face
   */
  virtual void PlaceWidget(double corners[8][3]);
  void PlaceWidget(double bounds[6]) override;
  //@}

  //@{
  /**
   * The interaction state may be set from a widget (e.g., PointWidget)
   * or other object. This controls how the interaction with the
   * widget proceeds.
   */
  svtkSetMacro(InteractionState, int);
  //@}

  /**
   * Get the bounding planes of the object. The first 6 planes will
   * be bounding planes of the parallelopiped. If in chair mode, three
   * additional planes will be present. The last three planes will be those
   * of the chair. The normals of all the planes will point into the object.
   */
  void GetBoundingPlanes(svtkPlaneCollection* pc);

  /**
   * The parallelopiped polydata.
   */
  void GetPolyData(svtkPolyData* pd);

  /**
   * The parallelopiped polydata.
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;

  //@{
  /**
   * Set/Get the handle properties.
   */
  virtual void SetHandleProperty(svtkProperty*);
  virtual void SetHoveredHandleProperty(svtkProperty*);
  virtual void SetSelectedHandleProperty(svtkProperty*);
  svtkGetObjectMacro(HandleProperty, svtkProperty);
  svtkGetObjectMacro(HoveredHandleProperty, svtkProperty);
  svtkGetObjectMacro(SelectedHandleProperty, svtkProperty);
  //@}

  void SetHandleRepresentation(svtkHandleRepresentation* handle);
  svtkHandleRepresentation* GetHandleRepresentation(int index);

  //@{
  /**
   * Turns the visibility of the handles on/off. Sometimes they may get in
   * the way of visualization.
   */
  void HandlesOn();
  void HandlesOff();
  //@}

  //@{
  /**
   * Get the face properties. When a face is being translated, the face gets
   * highlighted with the SelectedFaceProperty.
   */
  svtkGetObjectMacro(FaceProperty, svtkProperty);
  svtkGetObjectMacro(SelectedFaceProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the outline properties. These are the properties with which the
   * parallelopiped wireframe is rendered.
   */
  svtkGetObjectMacro(OutlineProperty, svtkProperty);
  svtkGetObjectMacro(SelectedOutlineProperty, svtkProperty);
  //@}

  /**
   * This actually constructs the geometry of the widget from the various
   * data parameters.
   */
  void BuildRepresentation() override;

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  //@}

  /**
   * Given and x-y display coordinate, compute the interaction state of
   * the widget.
   */
  int ComputeInteractionState(int X, int Y, int modify = 0) override;

  // Manage the state of the widget
  enum _InteractionState
  {
    Outside = 0,
    Inside,
    RequestResizeParallelopiped,
    RequestResizeParallelopipedAlongAnAxis,
    RequestChairMode,
    RequestTranslateParallelopiped,
    RequestScaleParallelopiped,
    RequestRotateParallelopiped,
    ResizingParallelopiped,
    ResizingParallelopipedAlongAnAxis,
    ChairMode,
    TranslatingParallelopiped,
    ScalingParallelopiped,
    RotatingParallelopiped
  };

  // Methods to manipulate the piped.
  virtual void Translate(double translation[3]);
  virtual void Translate(int X, int Y);
  virtual void Scale(int X, int Y);

  /**
   * Synchronize the parallelopiped handle positions with the
   * Polygonal datastructure.
   */
  virtual void PositionHandles();

  //@{
  /**
   * Minimum thickness for the parallelopiped. User interactions cannot make
   * any individual axis of the parallopiped thinner than this value.
   * Default is 0.05 expressed as a fraction of the diagonal of the bounding
   * box used in the PlaceWidget() invocation.
   */
  svtkSetMacro(MinimumThickness, double);
  svtkGetMacro(MinimumThickness, double);
  //@}

protected:
  svtkParallelopipedRepresentation();
  ~svtkParallelopipedRepresentation() override;

  /**
   * Translate the nth PtId (0 <= n <= 15) by the specified amount.
   */
  void TranslatePoint(int n, const double motionVector[3]);

  /**
   * Set the highlight state of a handle.
   * If handleIdx is -1, the property is applied to all handles.
   */
  void SetHandleHighlight(int handleIdx, svtkProperty* property);

  //@{
  /**
   * Highlight face defined by the supplied ptids with the specified property.
   */
  void SetFaceHighlight(svtkCellArray* face, svtkProperty*);
  void HighlightAllFaces();
  void UnHighlightAllFaces();
  //@}

  // Node can be a value within [0,7]. This will create a chair one one of
  // the handle corners. '0 < InitialChairDepth < 1' value dicates the starting
  // depth of the cavity.
  void UpdateChairAtNode(int node);

  // Removes any existing chairs.
  void RemoveExistingChairs();

  // Convenience method to get just the planes that define the parallelopiped.
  // If we aren't in chair mode, this will be the same as GetBoundingPlanes().
  // If we are in chair mode, this will be the first 6 planes from amongst
  // those returned by "GetBoundingPlanes".
  // All planes have their normals pointing inwards.
  void GetParallelopipedBoundingPlanes(svtkPlaneCollection* pc);

  // Convenience method to edefine a plane passing through 3 points.
  void DefinePlane(svtkPlane*, double p[3][3]);

  // Convenience method to edefine a plane passing through 3 pointIds of the
  // parallelopiped. The point Ids must like in the range [0,15], ie the
  // 15 points comprising the parallelopiped and the chair (also modelled
  // as a parallelopiped)
  void DefinePlane(svtkPlane*, svtkIdType, svtkIdType, svtkIdType);

  svtkActor* HexActor;
  svtkPolyDataMapper* HexMapper;
  svtkPolyData* HexPolyData;
  svtkPoints* Points;
  svtkActor* HexFaceActor;
  svtkPolyDataMapper* HexFaceMapper;
  svtkPolyData* HexFacePolyData;

  double LastEventPosition[2];

  // Cache the axis index used for face aligned resize.
  int LastResizeAxisIdx;

  svtkHandleRepresentation* HandleRepresentation;
  svtkHandleRepresentation** HandleRepresentations;
  int CurrentHandleIdx;
  int ChairHandleIdx;

  // When a chair is carved out for the first time, this is the initial
  // depth of the chair
  double InitialChairDepth;

  svtkProperty* HandleProperty;
  svtkProperty* HoveredHandleProperty;
  svtkProperty* FaceProperty;
  svtkProperty* OutlineProperty;
  svtkProperty* SelectedHandleProperty;
  svtkProperty* SelectedFaceProperty;
  svtkProperty* SelectedOutlineProperty;
  svtkClosedSurfacePointPlacer* ChairPointPlacer;
  svtkParallelopipedTopology* Topology;
  double MinimumThickness;
  double AbsoluteMinimumThickness;

private:
  svtkParallelopipedRepresentation(const svtkParallelopipedRepresentation&) = delete;
  void operator=(const svtkParallelopipedRepresentation&) = delete;
};

#endif
