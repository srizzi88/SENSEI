/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractPolygonalHandleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractPolygonalHandleRepresentation3D
 * @brief   represent a user defined handle geometry in 3D while maintaining a fixed orientation
 * w.r.t the camera.
 *
 * This class serves as the geometrical representation of a svtkHandleWidget.
 * The handle can be represented by an arbitrary polygonal data (svtkPolyData),
 * set via SetHandle(svtkPolyData *). The actual position of the handle
 * will be initially assumed to be (0,0,0). You can specify an offset from
 * this position if desired. This class differs from
 * svtkPolygonalHandleRepresentation3D in that the handle will always remain
 * front facing, ie it maintains a fixed orientation with respect to the
 * camera. This is done by using svtkFollowers internally to render the actors.
 * @sa
 * svtkPolygonalHandleRepresentation3D svtkHandleRepresentation svtkHandleWidget
 */

#ifndef svtkAbstractPolygonalHandleRepresentation3D_h
#define svtkAbstractPolygonalHandleRepresentation3D_h

#include "svtkHandleRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkProperty;
class svtkPolyDataMapper;
class svtkCellPicker;
class svtkTransformPolyDataFilter;
class svtkMatrixToLinearTransform;
class svtkMatrix4x4;
class svtkPolyData;
class svtkAbstractTransform;
class svtkActor;
class svtkFollower;
class svtkVectorText;

class SVTKINTERACTIONWIDGETS_EXPORT svtkAbstractPolygonalHandleRepresentation3D
  : public svtkHandleRepresentation
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkAbstractPolygonalHandleRepresentation3D, svtkHandleRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  using svtkHandleRepresentation::Translate;

  //@{
  /**
   * Set the position of the point in world and display coordinates.
   */
  void SetWorldPosition(double p[3]) override;
  void SetDisplayPosition(double p[3]) override;
  //@}

  //@{
  /**
   * Set/get the handle polydata.
   */
  void SetHandle(svtkPolyData*);
  svtkPolyData* GetHandle();
  //@}

  //@{
  /**
   * Set/Get the handle properties when unselected and selected.
   */
  void SetProperty(svtkProperty*);
  void SetSelectedProperty(svtkProperty*);
  svtkGetObjectMacro(Property, svtkProperty);
  svtkGetObjectMacro(SelectedProperty, svtkProperty);
  //@}

  /**
   * Get the transform used to transform the generic handle polydata before
   * placing it in the render window
   */
  virtual svtkAbstractTransform* GetTransform();

  //@{
  /**
   * Methods to make this class properly act like a svtkWidgetRepresentation.
   */
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  //@}

  //@{
  /**
   * Methods to make this class behave as a svtkProp.
   */
  void ShallowCopy(svtkProp* prop) override;
  void DeepCopy(svtkProp* prop) override;
  void GetActors(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  double* GetBounds() override;
  //@}

  //@{
  /**
   * A label may be associated with the seed. The string can be set via
   * SetLabelText. The visibility of the label can be turned on / off.
   */
  svtkSetMacro(LabelVisibility, svtkTypeBool);
  svtkGetMacro(LabelVisibility, svtkTypeBool);
  svtkBooleanMacro(LabelVisibility, svtkTypeBool);
  virtual void SetLabelText(const char* label);
  virtual char* GetLabelText();
  //@}

  //@{
  /**
   * Scale text (font size along each dimension).
   */
  virtual void SetLabelTextScale(double scale[3]);
  void SetLabelTextScale(double x, double y, double z)
  {
    double scale[3] = { x, y, z };
    this->SetLabelTextScale(scale);
  }
  virtual double* GetLabelTextScale();
  //@}

  //@{
  /**
   * Get the label text actor
   */
  svtkGetObjectMacro(LabelTextActor, svtkFollower);
  //@}

  /**
   * The handle may be scaled uniformly in all three dimensions using this
   * API. The handle can also be scaled interactively using the right
   * mouse button.
   */
  virtual void SetUniformScale(double scale);

  //@{
  /**
   * Toggle the visibility of the handle on and off
   */
  svtkSetMacro(HandleVisibility, svtkTypeBool);
  svtkGetMacro(HandleVisibility, svtkTypeBool);
  svtkBooleanMacro(HandleVisibility, svtkTypeBool);
  //@}

  void Highlight(int highlight) override;

  //@{
  /**
   * Turn on/off smooth motion of the handle. See the documentation of
   * MoveFocusRequest for details. By default, SmoothMotion is ON. However,
   * in certain applications the user may want to turn it off. For instance
   * when using certain specific PointPlacer's with the representation such
   * as the svtkCellCentersPointPlacer, which causes the representation to
   * snap to the center of cells, or using a svtkPolygonalSurfacePointPlacer
   * which constrains the widget to the surface of a mesh. In such cases,
   * inherent restrictions on handle placement might conflict with a request
   * for smooth motion of the handles.
   */
  svtkSetMacro(SmoothMotion, svtkTypeBool);
  svtkGetMacro(SmoothMotion, svtkTypeBool);
  svtkBooleanMacro(SmoothMotion, svtkTypeBool);
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  svtkAbstractPolygonalHandleRepresentation3D();
  ~svtkAbstractPolygonalHandleRepresentation3D() override;

  svtkActor* Actor;
  svtkPolyDataMapper* Mapper;
  svtkTransformPolyDataFilter* HandleTransformFilter;
  svtkMatrixToLinearTransform* HandleTransform;
  svtkMatrix4x4* HandleTransformMatrix;
  svtkCellPicker* HandlePicker;
  double LastPickPosition[3];
  double LastEventPosition[2];
  int ConstraintAxis;
  svtkProperty* Property;
  svtkProperty* SelectedProperty;
  int WaitingForMotion;
  int WaitCount;
  svtkTypeBool HandleVisibility;

  // Methods to manipulate the cursor
  virtual void Translate(const double* p1, const double* p2) override;
  virtual void Scale(const double* p1, const double* p2, const double eventPos[2]);
  virtual void MoveFocus(const double* p1, const double* p2);

  void CreateDefaultProperties();

  // Given a motion vector defined by p1 --> p2 (p1 and p2 are in
  // world coordinates), the new display position of the handle center is
  // populated into requestedDisplayPos. This is again only a request for the
  // new display position. It is up to the point placer to deduce the
  // appropriate world co-ordinates that this display position will map into.
  // The placer may even disallow such a movement.
  // If "SmoothMotion" is OFF, the returned requestedDisplayPos is the same
  // as the event position, ie the location of the mouse cursor. If its OFF,
  // incremental offsets as described above are used to compute it.
  void MoveFocusRequest(
    const double* p1, const double* p2, const double eventPos[2], double requestedDisplayPos[3]);

  int DetermineConstraintAxis(int constraint, double* x, double* startPickPos);

  /**
   * Update the actor position. Different subclasses handle this differently.
   * For instance svtkPolygonalHandleRepresentation3D updates the handle
   * transformation and sets this on the handle.
   * svtkOrientedPolygonalHandleRepresentation3D, which uses a svtkFollower to
   * keep the handle geometry facinig the camera handles this differently. This
   * is an opportunity for subclasses to update the actor's position etc each
   * time the handle is rendered.
   */
  virtual void UpdateHandle();

  /**
   * Opportunity to update the label position and text during each render.
   */
  virtual void UpdateLabel();

  // Handle the label.
  svtkTypeBool LabelVisibility;
  svtkFollower* LabelTextActor;
  svtkPolyDataMapper* LabelTextMapper;
  svtkVectorText* LabelTextInput;
  bool LabelAnnotationTextScaleInitialized;
  svtkTypeBool SmoothMotion;

private:
  svtkAbstractPolygonalHandleRepresentation3D(
    const svtkAbstractPolygonalHandleRepresentation3D&) = delete;
  void operator=(const svtkAbstractPolygonalHandleRepresentation3D&) = delete;
};

#endif
