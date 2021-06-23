/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCurveRepresentation

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCurveRepresentation
 * @brief   svtkWidgetRepresentation
 * base class for a widget that represents an curve that connects control
 * points.
 *
 * Base class for widgets used to define curves from points, such as
 * svtkPolyLineRepresentation and svtkSplineRepresentation.  This class
 * uses handles, the number of which can be changed, to represent the
 * points that define the curve. The handles can be picked can be
 * picked on the curve itself to translate or rotate it in the scene.
 */

#ifndef svtkCurveRepresentation_h
#define svtkCurveRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"        // needed for svtkPolyDataAlgorithm
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkCellPicker;
class svtkConeSource;
class svtkDoubleArray;
class svtkPlaneSource;
class svtkPoints;
class svtkPolyData;
class svtkProp;
class svtkProperty;
class svtkSphereSource;
class svtkTransform;

#define SVTK_PROJECTION_YZ 0
#define SVTK_PROJECTION_XZ 1
#define SVTK_PROJECTION_XY 2
#define SVTK_PROJECTION_OBLIQUE 3
class SVTKINTERACTIONWIDGETS_EXPORT svtkCurveRepresentation : public svtkWidgetRepresentation
{
public:
  svtkTypeMacro(svtkCurveRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Used to manage the InteractionState of the widget
  enum _InteractionState
  {
    Outside = 0,
    OnHandle,
    OnLine,
    Moving,
    Scaling,
    Spinning,
    Inserting,
    Erasing,
    Pushing
  };

  //@{
  /**
   * Set the interaction state
   */
  svtkSetMacro(InteractionState, int);
  //@}

  //@{
  /**
   * Force the widget to be projected onto one of the orthogonal
   * planes.  Remember that when the InteractionState changes, a
   * ModifiedEvent is invoked.  This can be used to snap the curve to
   * the plane if it is originally not aligned.  The normal in
   * SetProjectionNormal is 0,1,2 for YZ,XZ,XY planes respectively and
   * 3 for arbitrary oblique planes when the widget is tied to a
   * svtkPlaneSource.
   */
  svtkSetMacro(ProjectToPlane, svtkTypeBool);
  svtkGetMacro(ProjectToPlane, svtkTypeBool);
  svtkBooleanMacro(ProjectToPlane, svtkTypeBool);
  //@}

  /**
   * Set up a reference to a svtkPlaneSource that could be from another widget
   * object, e.g. a svtkPolyDataSourceWidget.
   */
  void SetPlaneSource(svtkPlaneSource* plane);

  svtkSetClampMacro(ProjectionNormal, int, SVTK_PROJECTION_YZ, SVTK_PROJECTION_OBLIQUE);
  svtkGetMacro(ProjectionNormal, int);
  void SetProjectionNormalToXAxes() { this->SetProjectionNormal(0); }
  void SetProjectionNormalToYAxes() { this->SetProjectionNormal(1); }
  void SetProjectionNormalToZAxes() { this->SetProjectionNormal(2); }
  void SetProjectionNormalToOblique() { this->SetProjectionNormal(3); }

  //@{
  /**
   * Set the position of poly line handles and points in terms of a plane's
   * position. i.e., if ProjectionNormal is 0, all of the x-coordinate
   * values of the points are set to position. Any value can be passed (and is
   * ignored) to update the poly line points when Projection normal is set to 3
   * for arbitrary plane orientations.
   */
  void SetProjectionPosition(double position);
  svtkGetMacro(ProjectionPosition, double);
  //@}

  /**
   * Grab the polydata (including points) that defines the
   * interpolating curve. Points are guaranteed to be up-to-date when
   * either the InteractionEvent or EndInteraction events are
   * invoked. The user provides the svtkPolyData and the points and
   * polyline are added to it.
   */
  virtual void GetPolyData(svtkPolyData* pd) = 0;

  //@{
  /**
   * Set/Get the handle properties (the spheres are the handles). The
   * properties of the handles when selected and unselected can be manipulated.
   */
  svtkGetObjectMacro(HandleProperty, svtkProperty);
  svtkGetObjectMacro(SelectedHandleProperty, svtkProperty);
  //@}

  //@{
  /**
   * Set/Get the line properties. The properties of the line when selected
   * and unselected can be manipulated.
   */
  svtkGetObjectMacro(LineProperty, svtkProperty);
  svtkGetObjectMacro(SelectedLineProperty, svtkProperty);
  //@}

  //@{
  /**
   * Set/Get the number of handles for this widget.
   */
  virtual void SetNumberOfHandles(int npts) = 0;
  svtkGetMacro(NumberOfHandles, int);
  //@}

  //@{
  /**
   * Sets the representation to be a directional curve with the end represented
   * as a cone.
   */
  void SetDirectionalLine(bool val);
  svtkGetMacro(DirectionalLine, bool);
  svtkBooleanMacro(DirectionalLine, bool);
  //@}

  //@{
  /**
   * Set/Get the position of the handles. Call GetNumberOfHandles
   * to determine the valid range of handle indices.
   */
  virtual void SetHandlePosition(int handle, double x, double y, double z);
  virtual void SetHandlePosition(int handle, double xyz[3]);
  virtual void GetHandlePosition(int handle, double xyz[3]);
  virtual double* GetHandlePosition(int handle);
  virtual svtkDoubleArray* GetHandlePositions() = 0;
  //@}

  //@{
  /**
   * Control whether the curve is open or closed. A closed forms a
   * continuous loop: the first and last points are the same.  A
   * minimum of 3 handles are required to form a closed loop.
   */
  void SetClosed(svtkTypeBool closed);
  svtkGetMacro(Closed, svtkTypeBool);
  svtkBooleanMacro(Closed, svtkTypeBool);
  //@}

  /**
   * Convenience method to determine whether the curve is
   * closed in a geometric sense.  The widget may be set "closed" but still
   * be geometrically open (e.g., a straight line).
   */
  svtkTypeBool IsClosed();

  /**
   * Get the approximate vs. the true arc length of the curve. Calculated as
   * the summed lengths of the individual straight line segments. Use
   * SetResolution to control the accuracy.
   */
  virtual double GetSummedLength() = 0;

  /**
   * Convenience method to allocate and set the handles from a
   * svtkPoints instance.  If the first and last points are the same,
   * the curve sets Closed to the on InteractionState and disregards
   * the last point, otherwise Closed remains unchanged.
   */
  virtual void InitializeHandles(svtkPoints* points) = 0;

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's
   * API. Note that a version of place widget is available where the
   * center and handle position are specified.
   */
  void BuildRepresentation() override = 0;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  void EndWidgetInteraction(double e[2]) override;
  double* GetBounds() override;
  //@}

  //@{
  /**
   * Methods supporting, and required by, the rendering process.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  int RenderOverlay(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /**
   * Convenience method to set the line color.
   * Ideally one should use GetLineProperty()->SetColor().
   */
  void SetLineColor(double r, double g, double b);

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  //@{
  /**
   * Get/Set the current handle index. Setting the current handle index will
   * also result in the handle being highlighted. Set to `-1` to remove the
   * highlight.
   */
  void SetCurrentHandleIndex(int index);
  svtkGetMacro(CurrentHandleIndex, int);
  //@}

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
  svtkCurveRepresentation();
  ~svtkCurveRepresentation() override;

  class HandleSource;

  double LastEventPosition[3];
  double Bounds[6];

  // Controlling vars
  int ProjectionNormal;
  double ProjectionPosition;
  svtkTypeBool ProjectToPlane;
  svtkPlaneSource* PlaneSource;

  // Projection capabilities
  void ProjectPointsToPlane();
  void ProjectPointsToOrthoPlane();
  void ProjectPointsToObliquePlane();

  int NumberOfHandles;
  svtkTypeBool Closed;

  // The line segments
  svtkActor* LineActor;
  void HighlightLine(int highlight);

  // Glyphs representing hot spots (e.g., handles)
  svtkActor** Handle;
  HandleSource** HandleGeometry;
  void Initialize();
  int HighlightHandle(svtkProp* prop); // returns handle index or -1 on fail
  int GetHandleIndex(svtkProp* prop);  // returns handle index or -1 on fail
  virtual void SizeHandles();

  /**
   * Returns the position of insertion or -1 on fail.
   */
  virtual int InsertHandleOnLine(double* pos) = 0;

  virtual void PushHandle(double* pos);
  void EraseHandle(const int&);

  // Do the picking
  svtkCellPicker* HandlePicker;
  svtkCellPicker* LinePicker;
  double LastPickPosition[3];
  svtkActor* CurrentHandle;
  int CurrentHandleIndex;
  bool FirstSelected;

  // Methods to manipulate the curve.
  void MovePoint(double* p1, double* p2);
  void Scale(double* p1, double* p2, int X, int Y);
  void Translate(double* p1, double* p2);
  void Spin(double* p1, double* p2, double* vpn);

  // Transform the control points (used for spinning)
  svtkTransform* Transform;

  // Manage how the representation appears
  bool DirectionalLine;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* HandleProperty;
  svtkProperty* SelectedHandleProperty;
  svtkProperty* LineProperty;
  svtkProperty* SelectedLineProperty;
  void CreateDefaultProperties();

  // For efficient spinning
  double Centroid[3];
  void CalculateCentroid();

  int TranslationAxis;

  class HandleSource : public svtkPolyDataAlgorithm
  {
  public:
    static HandleSource* New();

    svtkSetMacro(UseSphere, bool);
    svtkGetMacro(UseSphere, bool);
    svtkBooleanMacro(UseSphere, bool);

    svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
    svtkGetMacro(Radius, double);

    svtkSetVector3Macro(Center, double);
    svtkGetVectorMacro(Center, double, 3);

    svtkSetVector3Macro(Direction, double);
    svtkGetVectorMacro(Direction, double, 3);

  protected:
    HandleSource();
    ~HandleSource() override {}
    int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  private:
    bool UseSphere;
    // Used by both
    double Radius;
    double Center[3];
    // Cone only
    double Direction[3];
  };

private:
  svtkCurveRepresentation(const svtkCurveRepresentation&) = delete;
  void operator=(const svtkCurveRepresentation&) = delete;
};

#endif
