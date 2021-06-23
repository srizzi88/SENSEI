/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageTracerWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageTracerWidget
 * @brief   3D widget for tracing on planar props.
 *
 * svtkImageTracerWidget is different from other widgets in three distinct ways:
 * 1) any sub-class of svtkProp can be input rather than just svtkProp3D, so that
 * svtkImageActor can be set as the prop and then traced over, 2) the widget fires
 * pick events at the input prop to decide where to move its handles, 3) the
 * widget has 2D glyphs for handles instead of 3D spheres as is done in other
 * sub-classes of svtk3DWidget. This widget is primarily designed for manually
 * tracing over image data.
 * The button actions and key modifiers are as follows for controlling the
 * widget:
 * 1) left button click over the image, hold and drag draws a free hand line.
 * 2) left button click and release erases the widget line,
 * if it exists, and repositions the first handle.
 * 3) middle button click starts a snap drawn line.  The line is terminated by
 * clicking the middle button while depressing the ctrl key.
 * 4) when tracing a continuous or snap drawn line, if the last cursor position
 * is within a specified tolerance to the first handle, the widget line will form
 * a closed loop.
 * 5) right button clicking and holding on any handle that is part of a snap
 * drawn line allows handle dragging: existing line segments are updated
 * accordingly.  If the path is open and AutoClose is set to On, the path can
 * be closed by repositioning the first and last points over one another.
 * 6) ctrl key + right button down on any handle will erase it: existing
 * snap drawn line segments are updated accordingly.  If the line was formed by
 * continuous tracing, the line is deleted leaving one handle.
 * 7) shift key + right button down on any snap drawn line segment will insert
 * a handle at the cursor position.  The line segment is split accordingly.
 *
 * @warning
 * the input svtkDataSet should be svtkImageData.
 *
 * @sa
 * svtk3DWidget svtkBoxWidget svtkLineWidget svtkPointWidget svtkSphereWidget
 * svtkImagePlaneWidget svtkImplicitPlaneWidget svtkPlaneWidget
 */

#ifndef svtkImageTracerWidget_h
#define svtkImageTracerWidget_h

#include "svtk3DWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkAbstractPropPicker;
class svtkActor;
class svtkCellArray;
class svtkCellPicker;
class svtkFloatArray;
class svtkGlyphSource2D;
class svtkPoints;
class svtkPolyData;
class svtkProp;
class svtkProperty;
class svtkPropPicker;
class svtkTransform;
class svtkTransformPolyDataFilter;

#define SVTK_ITW_PROJECTION_YZ 0
#define SVTK_ITW_PROJECTION_XZ 1
#define SVTK_ITW_PROJECTION_XY 2
#define SVTK_ITW_SNAP_CELLS 0
#define SVTK_ITW_SNAP_POINTS 1

class SVTKINTERACTIONWIDGETS_EXPORT svtkImageTracerWidget : public svtk3DWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkImageTracerWidget* New();

  svtkTypeMacro(svtkImageTracerWidget, svtk3DWidget);
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
   * Set/Get the handle properties (the 2D glyphs are the handles). The
   * properties of the handles when selected and normal can be manipulated.
   */
  virtual void SetHandleProperty(svtkProperty*);
  svtkGetObjectMacro(HandleProperty, svtkProperty);
  virtual void SetSelectedHandleProperty(svtkProperty*);
  svtkGetObjectMacro(SelectedHandleProperty, svtkProperty);
  //@}

  //@{
  /**
   * Set/Get the line properties. The properties of the line when selected
   * and unselected can be manipulated.
   */
  virtual void SetLineProperty(svtkProperty*);
  svtkGetObjectMacro(LineProperty, svtkProperty);
  virtual void SetSelectedLineProperty(svtkProperty*);
  svtkGetObjectMacro(SelectedLineProperty, svtkProperty);
  //@}

  /**
   * Set the prop, usually a svtkImageActor, to trace over.
   */
  void SetViewProp(svtkProp* prop);

  //@{
  /**
   * Force handles to be on a specific ortho plane. Default is Off.
   */
  svtkSetMacro(ProjectToPlane, svtkTypeBool);
  svtkGetMacro(ProjectToPlane, svtkTypeBool);
  svtkBooleanMacro(ProjectToPlane, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the projection normal.  The normal in SetProjectionNormal is 0,1,2
   * for YZ,XZ,XY planes respectively.  Since the handles are 2D glyphs, it is
   * necessary to specify a plane on which to generate them, even though
   * ProjectToPlane may be turned off.
   */
  svtkSetClampMacro(ProjectionNormal, int, SVTK_ITW_PROJECTION_YZ, SVTK_ITW_PROJECTION_XY);
  svtkGetMacro(ProjectionNormal, int);
  void SetProjectionNormalToXAxes() { this->SetProjectionNormal(0); }
  void SetProjectionNormalToYAxes() { this->SetProjectionNormal(1); }
  void SetProjectionNormalToZAxes() { this->SetProjectionNormal(2); }
  //@}

  //@{
  /**
   * Set the position of the widgets' handles in terms of a plane's position.
   * e.g., if ProjectionNormal is 0, all of the x-coordinate values of the
   * handles are set to ProjectionPosition.  No attempt is made to ensure that
   * the position is within the bounds of either the underlying image data or
   * the prop on which tracing is performed.
   */
  void SetProjectionPosition(double position);
  svtkGetMacro(ProjectionPosition, double);
  //@}

  //@{
  /**
   * Force snapping to image data while tracing. Default is Off.
   */
  void SetSnapToImage(svtkTypeBool snap);
  svtkGetMacro(SnapToImage, svtkTypeBool);
  svtkBooleanMacro(SnapToImage, svtkTypeBool);
  //@}

  //@{
  /**
   * In concert with a CaptureRadius value, automatically
   * form a closed path by connecting first to last path points.
   * Default is Off.
   */
  svtkSetMacro(AutoClose, svtkTypeBool);
  svtkGetMacro(AutoClose, svtkTypeBool);
  svtkBooleanMacro(AutoClose, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the capture radius for automatic path closing.  For image
   * data, capture radius should be half the distance between voxel/pixel
   * centers.
   * Default is 1.0
   */
  svtkSetMacro(CaptureRadius, double);
  svtkGetMacro(CaptureRadius, double);
  //@}

  /**
   * Grab the points and lines that define the traced path. These point values
   * are guaranteed to be up-to-date when either the InteractionEvent or
   * EndInteraction events are invoked. The user provides the svtkPolyData and
   * the points and cells representing the line are added to it.
   */
  void GetPath(svtkPolyData* pd);

  /**
   * Get the handles' geometric representation via svtkGlyphSource2D.
   */
  svtkGlyphSource2D* GetGlyphSource() { return this->HandleGenerator; }

  //@{
  /**
   * Set/Get the type of snapping to image data: center of a pixel/voxel or
   * nearest point defining a pixel/voxel.
   */
  svtkSetClampMacro(ImageSnapType, int, SVTK_ITW_SNAP_CELLS, SVTK_ITW_SNAP_POINTS);
  svtkGetMacro(ImageSnapType, int);
  //@}

  //@{
  /**
   * Set/Get the handle position in terms of a zero-based array of handles.
   */
  void SetHandlePosition(int handle, double xyz[3]);
  void SetHandlePosition(int handle, double x, double y, double z);
  void GetHandlePosition(int handle, double xyz[3]);
  double* GetHandlePosition(int handle) SVTK_SIZEHINT(3);
  //@}

  //@{
  /**
   * Get the number of handles.
   */
  svtkGetMacro(NumberOfHandles, int);
  //@}

  //@{
  /**
   * Enable/disable mouse interaction when the widget is visible.
   */
  void SetInteraction(svtkTypeBool interact);
  svtkGetMacro(Interaction, svtkTypeBool);
  svtkBooleanMacro(Interaction, svtkTypeBool);
  //@}

  /**
   * Initialize the widget with a set of points and generate
   * lines between them.  If AutoClose is on it will handle the
   * case wherein the first and last points are congruent.
   */
  void InitializeHandles(svtkPoints*);

  /**
   * Is the path closed or open?
   */
  int IsClosed();

  //@{
  /**
   * Enable/Disable mouse button events
   */
  svtkSetMacro(HandleLeftMouseButton, svtkTypeBool);
  svtkGetMacro(HandleLeftMouseButton, svtkTypeBool);
  svtkBooleanMacro(HandleLeftMouseButton, svtkTypeBool);
  svtkSetMacro(HandleMiddleMouseButton, svtkTypeBool);
  svtkGetMacro(HandleMiddleMouseButton, svtkTypeBool);
  svtkBooleanMacro(HandleMiddleMouseButton, svtkTypeBool);
  svtkSetMacro(HandleRightMouseButton, svtkTypeBool);
  svtkGetMacro(HandleRightMouseButton, svtkTypeBool);
  svtkBooleanMacro(HandleRightMouseButton, svtkTypeBool);
  //@}

protected:
  svtkImageTracerWidget();
  ~svtkImageTracerWidget() override;

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Start = 0,
    Tracing,
    Snapping,
    Erasing,
    Inserting,
    Moving,
    Translating,
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

  void AddObservers();

  // Controlling ivars
  svtkTypeBool Interaction;
  int ProjectionNormal;
  double ProjectionPosition;
  svtkTypeBool ProjectToPlane;
  int ImageSnapType;
  svtkTypeBool SnapToImage;
  double CaptureRadius; // tolerance for auto path close
  svtkTypeBool AutoClose;
  int IsSnapping;
  int LastX;
  int LastY;

  void Trace(int, int);
  void Snap(double*);
  void MovePoint(const double*, const double*);
  void Translate(const double*, const double*);
  void ClosePath();

  // 2D glyphs representing hot spots (e.g., handles)
  svtkActor** Handle;
  svtkPolyData** HandleGeometry;
  svtkGlyphSource2D* HandleGenerator;

  // Transforms required as 2D glyphs are generated in the x-y plane
  svtkTransformPolyDataFilter* TransformFilter;
  svtkTransform* Transform;
  svtkFloatArray* TemporaryHandlePoints;

  void AppendHandles(double*);
  void ResetHandles();
  void AllocateHandles(const int&);
  void AdjustHandlePosition(const int&, double*);
  int HighlightHandle(svtkProp*); // returns handle index or -1 on fail
  void EraseHandle(const int&);
  void SizeHandles() override;
  void InsertHandleOnLine(double*);

  int NumberOfHandles;
  svtkActor* CurrentHandle;
  int CurrentHandleIndex;

  svtkProp* ViewProp;         // the prop we want to pick on
  svtkPropPicker* PropPicker; // the prop's picker

  // Representation of the line
  svtkPoints* LinePoints;
  svtkCellArray* LineCells;
  svtkActor* LineActor;
  svtkPolyData* LineData;
  svtkIdType CurrentPoints[2];

  void HighlightLine(const int&);
  void BuildLinesFromHandles();
  void ResetLine(double*);
  void AppendLine(double*);
  int PickCount;

  // Do the picking of the handles and the lines
  svtkCellPicker* HandlePicker;
  svtkCellPicker* LinePicker;
  svtkAbstractPropPicker* CurrentPicker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* HandleProperty;
  svtkProperty* SelectedHandleProperty;
  svtkProperty* LineProperty;
  svtkProperty* SelectedLineProperty;
  void CreateDefaultProperties();

  // Enable/Disable mouse button events
  svtkTypeBool HandleLeftMouseButton;
  svtkTypeBool HandleMiddleMouseButton;
  svtkTypeBool HandleRightMouseButton;

private:
  svtkImageTracerWidget(const svtkImageTracerWidget&) = delete;
  void operator=(const svtkImageTracerWidget&) = delete;
};

#endif
