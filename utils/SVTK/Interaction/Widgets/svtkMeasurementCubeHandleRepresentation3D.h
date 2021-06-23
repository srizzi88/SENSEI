/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMeasurementCubeHandleRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMeasurementCubeHandleRepresentation3D
 * @brief   represent a unit cube for measuring/comparing to data.
 *
 * @sa
 * svtkPolygonalHandleRepresentation3D svtkHandleRepresentation svtkHandleWidget
 */

#ifndef svtkMeasurementCubeHandleRepresentation3D_h
#define svtkMeasurementCubeHandleRepresentation3D_h

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
class svtkBillboardTextActor3D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkMeasurementCubeHandleRepresentation3D
  : public svtkHandleRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkMeasurementCubeHandleRepresentation3D* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkMeasurementCubeHandleRepresentation3D, svtkHandleRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set the position of the point in world and display coordinates.
   */
  void SetWorldPosition(double p[3]) override;
  void SetDisplayPosition(double p[3]) override;
  //@}

  //@{
  /**
   * Get the handle polydata.
   */
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
   * A label may be associated with the cube. The string can be set via
   * SetLabelText. The visibility of the label can be turned on / off.
   */
  svtkSetMacro(LabelVisibility, svtkTypeBool);
  svtkGetMacro(LabelVisibility, svtkTypeBool);
  svtkBooleanMacro(LabelVisibility, svtkTypeBool);
  svtkSetMacro(SelectedLabelVisibility, svtkTypeBool);
  svtkGetMacro(SelectedLabelVisibility, svtkTypeBool);
  svtkBooleanMacro(SelectedLabelVisibility, svtkTypeBool);

  virtual void SetLabelTextInput(const char* label);
  virtual char* GetLabelTextInput();
  //@}

  //@{
  /**
   * Get the label text actor
   */
  svtkGetObjectMacro(LabelText, svtkBillboardTextActor3D);
  //@}

  //@{
  /**
   * Toggle the visibility of the handle on and off
   */
  svtkSetMacro(HandleVisibility, svtkTypeBool);
  svtkGetMacro(HandleVisibility, svtkTypeBool);
  svtkBooleanMacro(HandleVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Toggle highlighting (used when the cube is selected).
   */
  void Highlight(int highlight) override;
  //@}

  //@{
  /**
   * Turn on/off smooth motion of the handle. See the documentation of
   * MoveFocusRequest for details. By default, SmoothMotion is ON. However,
   * in certain applications the user may want to turn it off. For instance
   * when using certain specific PointPlacer's with the representation such
   * as the svtkCellCentersPointPlacer, which causes the representation to
   * snap to the center of cells. In such cases, inherent restrictions on
   * handle placement might conflict with a request for smooth motion of the
   * handles.
   */
  svtkSetMacro(SmoothMotion, svtkTypeBool);
  svtkGetMacro(SmoothMotion, svtkTypeBool);
  svtkBooleanMacro(SmoothMotion, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the length of a side of the cube (default is 1).
   */
  void SetSideLength(double);
  svtkGetMacro(SideLength, double);
  //@}

  //@{
  /**
   * Turn on/off adaptive scaling for the cube.
   */
  svtkSetMacro(AdaptiveScaling, svtkTypeBool);
  svtkGetMacro(AdaptiveScaling, svtkTypeBool);
  svtkBooleanMacro(AdaptiveScaling, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the rescaling increment for the cube. This value is applied to
   * each dimension, so volume scaling = std::pow(RescaleFactor, 3).
   */
  svtkSetClampMacro(RescaleFactor, double, 1., SVTK_DOUBLE_MAX);
  svtkGetMacro(RescaleFactor, double);
  //@}

  //@{
  /**
   * Set the min/max cube representational area relative to the render window
   * area. If adaptive scaling is on and the cube's image is outside of these
   * bounds, the cube is adaptively scaled. The max and min relative cube sizes
   * are clamped between 1. and 1.e-6, and MaxRelativeubeSize must be more than
   * <RescaleFactor> greater than MinRelativeCubeScreenArea.
   */
  void SetMinRelativeCubeScreenArea(double);
  svtkGetMacro(MinRelativeCubeScreenArea, double);
  void SetMaxRelativeCubeScreenArea(double);
  svtkGetMacro(MaxRelativeCubeScreenArea, double);
  //@}

  //@{
  /**
   * Set the label for the unit of length of a side of the cube.
   */
  svtkSetStringMacro(LengthUnit);
  svtkGetStringMacro(LengthUnit);
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  svtkMeasurementCubeHandleRepresentation3D();
  ~svtkMeasurementCubeHandleRepresentation3D() override;

  svtkActor* Actor;
  svtkPolyDataMapper* Mapper;
  svtkTransformPolyDataFilter* HandleTransformFilter;
  svtkMatrixToLinearTransform* HandleTransform;
  svtkMatrix4x4* HandleTransformMatrix;
  svtkCellPicker* HandlePicker;
  double LastPickPosition[3];
  double LastEventPosition[2];
  svtkProperty* Property;
  svtkProperty* SelectedProperty;
  int WaitingForMotion;
  int WaitCount;
  svtkTypeBool HandleVisibility;
  double Offset[3];
  svtkTypeBool AdaptiveScaling;
  double RescaleFactor;
  double MinRelativeCubeScreenArea;
  double MaxRelativeCubeScreenArea;
  double SideLength;
  char* LengthUnit;

  // Methods to manipulate the cursor
  virtual void Scale(const double* p1, const double* p2, const double eventPos[2]);
  virtual void MoveFocus(const double* p1, const double* p2);

  void CreateDefaultProperties();

  /**
   * If adaptive scaling is enabled, rescale the cube so that its
   * representational area in the display window falls between
   * <MinRelativeCubeScreenArea> and <MaxRelativeCubeScreenArea>.
   */
  void ScaleIfNecessary(svtkViewport*);

  /**
   * Given a motion vector defined by p1 --> p2 (p1 and p2 are in
   * world coordinates), the new display position of the handle center is
   * populated into requestedDisplayPos. This is again only a request for the
   * new display position. It is up to the point placer to deduce the
   * appropriate world co-ordinates that this display position will map into.
   * The placer may even disallow such a movement.
   * If "SmoothMotion" is OFF, the returned requestedDisplayPos is the same
   * as the event position, ie the location of the mouse cursor. If its OFF,
   * incremental offsets as described above are used to compute it.
   */
  void MoveFocusRequest(
    const double* p1, const double* p2, const double eventPos[2], double requestedDisplayPos[3]);

  /**
   * The handle may be scaled uniformly in all three dimensions using this
   * API. The handle can also be scaled interactively using the right
   * mouse button.
   */
  virtual void SetUniformScale(double scale);

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
  svtkTypeBool SelectedLabelVisibility;
  svtkBillboardTextActor3D* LabelText;
  bool LabelAnnotationTextScaleInitialized;
  svtkTypeBool SmoothMotion;

private:
  svtkMeasurementCubeHandleRepresentation3D(
    const svtkMeasurementCubeHandleRepresentation3D&) = delete;
  void operator=(const svtkMeasurementCubeHandleRepresentation3D&) = delete;
};

#endif
