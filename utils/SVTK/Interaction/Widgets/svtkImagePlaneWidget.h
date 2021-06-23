/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImagePlaneWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImagePlaneWidget
 * @brief   3D widget for reslicing image data
 *
 * This 3D widget defines a plane that can be interactively placed in an
 * image volume. A nice feature of the object is that the
 * svtkImagePlaneWidget, like any 3D widget, will work with the current
 * interactor style. That is, if svtkImagePlaneWidget does not handle an
 * event, then all other registered observers (including the interactor
 * style) have an opportunity to process the event. Otherwise, the
 * svtkImagePlaneWidget will terminate the processing of the event that it
 * handles.
 *
 * The core functionality of the widget is provided by a svtkImageReslice
 * object which passes its output onto a texture mapping pipeline for fast
 * slicing through volumetric data. See the key methods: GenerateTexturePlane()
 * and UpdatePlane() for implementation details.
 *
 * To use this object, just invoke SetInteractor() with the argument of the
 * method a svtkRenderWindowInteractor.  You may also wish to invoke
 * "PlaceWidget()" to initially position the widget. If the "i" key (for
 * "interactor") is pressed, the svtkImagePlaneWidget will appear. (See
 * superclass documentation for information about changing this behavior.)
 *
 * Selecting the widget with the middle mouse button with and without holding
 * the shift or control keys enables complex reslicing capablilites.
 * To facilitate use, a set of 'margins' (left, right, top, bottom) are shown as
 * a set of plane-axes aligned lines, the properties of which can be changed
 * as a group.
 * Without keyboard modifiers: selecting in the middle of the margins
 * enables translation of the plane along its normal. Selecting one of the
 * corners within the margins enables spinning around the plane's normal at its
 * center.  Selecting within a margin allows rotating about the center of the
 * plane around an axis aligned with the margin (i.e., selecting left margin
 * enables rotating around the plane's local y-prime axis).
 * With control key modifier: margin selection enables edge translation (i.e., a
 * constrained form of scaling). Selecting within the margins enables
 * translation of the entire plane.
 * With shift key modifier: uniform plane scaling is enabled.  Moving the mouse
 * up enlarges the plane while downward movement shrinks it.
 *
 * Window-level is achieved by using the right mouse button.  Window-level
 * values can be reset by shift + 'r' or control + 'r' while regular reset
 * camera is maintained with 'r' or 'R'.
 * The left mouse button can be used to query the underlying image data
 * with a snap-to cross-hair cursor.  Currently, the nearest point in the input
 * image data to the mouse cursor generates the cross-hairs.  With oblique
 * slicing, this behaviour may appear unsatisfactory. Text display of
 * window-level and image coordinates/data values are provided by a text
 * actor/mapper pair.
 *
 * Events that occur outside of the widget (i.e., no part of the widget is
 * picked) are propagated to any other registered obsevers (such as the
 * interaction style). Turn off the widget by pressing the "i" key again
 * (or invoke the Off() method). To support interactive manipulation of
 * objects, this class invokes the events StartInteractionEvent,
 * InteractionEvent, and EndInteractionEvent as well as StartWindowLevelEvent,
 * WindowLevelEvent, EndWindowLevelEvent and ResetWindowLevelEvent.
 *
 * The svtkImagePlaneWidget has several methods that can be used in
 * conjunction with other SVTK objects. The GetPolyData() method can be used
 * to get the polygonal representation of the plane and can be used as input
 * for other SVTK objects. Typical usage of the widget is to make use of the
 * StartInteractionEvent, InteractionEvent, and EndInteractionEvent
 * events. The InteractionEvent is called on mouse motion; the other two
 * events are called on button down and button up (either left or right
 * button).
 *
 * Some additional features of this class include the ability to control the
 * properties of the widget. You can set the properties of: the selected and
 * unselected representations of the plane's outline; the text actor via its
 * svtkTextProperty; the cross-hair cursor. In addition there are methods to
 * constrain the plane so that it is aligned along the x-y-z axes.  Finally,
 * one can specify the degree of interpolation (svtkImageReslice): nearest
 * neighbour, linear, and cubic.
 *
 * @par Thanks:
 * Thanks to Dean Inglis for developing and contributing this class.
 * Based on the Python SlicePlaneFactory from Atamai, Inc.
 *
 * @sa
 * svtk3DWidget svtkBoxWidget svtkLineWidget  svtkPlaneWidget svtkPointWidget
 * svtkPolyDataSourceWidget svtkSphereWidget svtkImplicitPlaneWidget
 */

#ifndef svtkImagePlaneWidget_h
#define svtkImagePlaneWidget_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPolyDataSourceWidget.h"

class svtkActor;
class svtkAbstractPropPicker;
class svtkDataSetMapper;
class svtkImageData;
class svtkImageMapToColors;
class svtkImageReslice;
class svtkLookupTable;
class svtkMatrix4x4;
class svtkPlaneSource;
class svtkPoints;
class svtkPolyData;
class svtkProperty;
class svtkTextActor;
class svtkTextProperty;
class svtkTexture;
class svtkTransform;

#define SVTK_NEAREST_RESLICE 0
#define SVTK_LINEAR_RESLICE 1
#define SVTK_CUBIC_RESLICE 2

// Private.
#define SVTK_IMAGE_PLANE_WIDGET_MAX_TEXTBUFF 128

class SVTKINTERACTIONWIDGETS_EXPORT svtkImagePlaneWidget : public svtkPolyDataSourceWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkImagePlaneWidget* New();

  svtkTypeMacro(svtkImagePlaneWidget, svtkPolyDataSourceWidget);
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

  /**
   * Set the svtkImageData* input for the svtkImageReslice.
   */
  void SetInputConnection(svtkAlgorithmOutput* aout) override;

  //@{
  /**
   * Set/Get the origin of the plane.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double xyz[3]);
  double* GetOrigin() SVTK_SIZEHINT(3);
  void GetOrigin(double xyz[3]);
  //@}

  //@{
  /**
   * Set/Get the position of the point defining the first axis of the plane.
   */
  void SetPoint1(double x, double y, double z);
  void SetPoint1(double xyz[3]);
  double* GetPoint1() SVTK_SIZEHINT(3);
  void GetPoint1(double xyz[3]);
  //@}

  //@{
  /**
   * Set/Get the position of the point defining the second axis of the plane.
   */
  void SetPoint2(double x, double y, double z);
  void SetPoint2(double xyz[3]);
  double* GetPoint2() SVTK_SIZEHINT(3);
  void GetPoint2(double xyz[3]);
  //@}

  //@{
  /**
   * Get the center of the plane.
   */
  double* GetCenter() SVTK_SIZEHINT(3);
  void GetCenter(double xyz[3]);
  //@}

  //@{
  /**
   * Get the normal to the plane.
   */
  double* GetNormal() SVTK_SIZEHINT(3);
  void GetNormal(double xyz[3]);
  //@}

  /**
   * Get the vector from the plane origin to point1.
   */
  void GetVector1(double v1[3]);

  /**
   * Get the vector from the plane origin to point2.
   */
  void GetVector2(double v2[3]);

  /**
   * Get the slice position in terms of the data extent.
   */
  int GetSliceIndex();

  /**
   * Set the slice position in terms of the data extent.
   */
  void SetSliceIndex(int index);

  /**
   * Get the position of the slice along its normal.
   */
  double GetSlicePosition();

  /**
   * Set the position of the slice along its normal.
   */
  void SetSlicePosition(double position);

  //@{
  /**
   * Set the interpolation to use when texturing the plane.
   */
  void SetResliceInterpolate(int);
  svtkGetMacro(ResliceInterpolate, int);
  void SetResliceInterpolateToNearestNeighbour()
  {
    this->SetResliceInterpolate(SVTK_NEAREST_RESLICE);
  }
  void SetResliceInterpolateToLinear() { this->SetResliceInterpolate(SVTK_LINEAR_RESLICE); }
  void SetResliceInterpolateToCubic() { this->SetResliceInterpolate(SVTK_CUBIC_RESLICE); }
  //@}

  /**
   * Convenience method to get the svtkImageReslice output.
   */
  svtkImageData* GetResliceOutput();

  //@{
  /**
   * Make sure that the plane remains within the volume.
   * Default is On.
   */
  svtkSetMacro(RestrictPlaneToVolume, svtkTypeBool);
  svtkGetMacro(RestrictPlaneToVolume, svtkTypeBool);
  svtkBooleanMacro(RestrictPlaneToVolume, svtkTypeBool);
  //@}

  //@{
  /**
   * Let the user control the lookup table. NOTE: apply this method BEFORE
   * applying the SetLookupTable method.
   * Default is Off.
   */
  svtkSetMacro(UserControlledLookupTable, svtkTypeBool);
  svtkGetMacro(UserControlledLookupTable, svtkTypeBool);
  svtkBooleanMacro(UserControlledLookupTable, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether to interpolate the texture or not. When off, the
   * reslice interpolation is nearest neighbour regardless of how the
   * interpolation is set through the API. Set before setting the
   * svtkImageData input. Default is On.
   */
  svtkSetMacro(TextureInterpolate, svtkTypeBool);
  svtkGetMacro(TextureInterpolate, svtkTypeBool);
  svtkBooleanMacro(TextureInterpolate, svtkTypeBool);
  //@}

  //@{
  /**
   * Control the visibility of the actual texture mapped reformatted plane.
   * in some cases you may only want the plane outline for example.
   */
  virtual void SetTextureVisibility(svtkTypeBool);
  svtkGetMacro(TextureVisibility, svtkTypeBool);
  svtkBooleanMacro(TextureVisibility, svtkTypeBool);
  //@}

  /**
   * Grab the polydata (including points) that defines the plane.  The
   * polydata consists of (res+1)*(res+1) points, and res*res quadrilateral
   * polygons, where res is the resolution of the plane. These point values
   * are guaranteed to be up-to-date when either the InteractionEvent or
   * EndInteraction events are invoked. The user provides the svtkPolyData and
   * the points and polygons are added to it.
   */
  void GetPolyData(svtkPolyData* pd);

  /**
   * Satisfies superclass API.  This returns a pointer to the underlying
   * svtkPolyData.  Make changes to this before calling the initial PlaceWidget()
   * to have the initial placement follow suit.  Or, make changes after the
   * widget has been initialised and call UpdatePlacement() to realise.
   */
  svtkPolyDataAlgorithm* GetPolyDataAlgorithm() override;

  /**
   * Satisfies superclass API.  This will change the state of the widget to
   * match changes that have been made to the underlying svtkPolyDataSource
   */
  void UpdatePlacement(void) override;

  /**
   * Convenience method to get the texture used by this widget.  This can be
   * used in external slice viewers.
   */
  svtkTexture* GetTexture();

  //@{
  /**
   * Convenience method to get the svtkImageMapToColors filter used by this
   * widget.  The user can properly render other transparent actors in a
   * scene by calling the filter's SetOutputFormatToRGB and
   * PassAlphaToOutputOff.
   */
  svtkGetObjectMacro(ColorMap, svtkImageMapToColors);
  virtual void SetColorMap(svtkImageMapToColors*);
  //@}

  //@{
  /**
   * Set/Get the plane's outline properties. The properties of the plane's
   * outline when selected and unselected can be manipulated.
   */
  virtual void SetPlaneProperty(svtkProperty*);
  svtkGetObjectMacro(PlaneProperty, svtkProperty);
  virtual void SetSelectedPlaneProperty(svtkProperty*);
  svtkGetObjectMacro(SelectedPlaneProperty, svtkProperty);
  //@}

  //@{
  /**
   * Convenience method sets the plane orientation normal to the
   * x, y, or z axes.  Default is XAxes (0).
   */
  void SetPlaneOrientation(int);
  svtkGetMacro(PlaneOrientation, int);
  void SetPlaneOrientationToXAxes() { this->SetPlaneOrientation(0); }
  void SetPlaneOrientationToYAxes() { this->SetPlaneOrientation(1); }
  void SetPlaneOrientationToZAxes() { this->SetPlaneOrientation(2); }
  //@}

  /**
   * Set the internal picker to one defined by the user.  In this way,
   * a set of three orthogonal planes can share the same picker so that
   * picking is performed correctly.  The default internal picker can be
   * re-set/allocated by setting to 0 (nullptr).
   */
  void SetPicker(svtkAbstractPropPicker*);

  //@{
  /**
   * Set/Get the internal lookuptable (lut) to one defined by the user, or,
   * alternatively, to the lut of another svtkImgePlaneWidget.  In this way,
   * a set of three orthogonal planes can share the same lut so that
   * window-levelling is performed uniformly among planes.  The default
   * internal lut can be re- set/allocated by setting to 0 (nullptr).
   */
  virtual void SetLookupTable(svtkLookupTable*);
  svtkGetObjectMacro(LookupTable, svtkLookupTable);
  //@}

  //@{
  /**
   * Enable/disable text display of window-level, image coordinates and
   * scalar values in a render window.
   */
  svtkSetMacro(DisplayText, svtkTypeBool);
  svtkGetMacro(DisplayText, svtkTypeBool);
  svtkBooleanMacro(DisplayText, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the properties of the cross-hair cursor.
   */
  virtual void SetCursorProperty(svtkProperty*);
  svtkGetObjectMacro(CursorProperty, svtkProperty);
  //@}

  //@{
  /**
   * Set the properties of the margins.
   */
  virtual void SetMarginProperty(svtkProperty*);
  svtkGetObjectMacro(MarginProperty, svtkProperty);
  //@}

  //@{
  /**
   * Set the size of the margins based on a percentage of the
   * plane's width and height, limited between 0 and 50%.
   */
  svtkSetClampMacro(MarginSizeX, double, 0.0, 0.5);
  svtkGetMacro(MarginSizeX, double);
  svtkSetClampMacro(MarginSizeY, double, 0.0, 0.5);
  svtkGetMacro(MarginSizeY, double);
  //@}

  //@{
  /**
   * Set/Get the text property for the image data and window-level annotation.
   */
  void SetTextProperty(svtkTextProperty* tprop);
  svtkTextProperty* GetTextProperty();
  //@}

  //@{
  /**
   * Set/Get the property for the resliced image.
   */
  virtual void SetTexturePlaneProperty(svtkProperty*);
  svtkGetObjectMacro(TexturePlaneProperty, svtkProperty);
  //@}

  //@{
  /**
   * Set/Get the current window and level values.  SetWindowLevel should
   * only be called after SetInput.  If a shared lookup table is being used,
   * a callback is required to update the window level values without having
   * to update the lookup table again.
   */
  void SetWindowLevel(double window, double level, int copy = 0);
  void GetWindowLevel(double wl[2]);
  double GetWindow() { return this->CurrentWindow; }
  double GetLevel() { return this->CurrentLevel; }
  //@}

  /**
   * Get the image coordinate position and voxel value.  Currently only
   * supports single component image data.
   */
  int GetCursorData(double xyzv[4]);

  /**
   * Get the status of the cursor data.  If this returns 1 the
   * CurrentCursorPosition and CurrentImageValue will have current
   * data.  If it returns 0, these values are invalid.
   */
  int GetCursorDataStatus();

  //@{
  /**
   * Get the current cursor position.  To be used in conjunction with
   * GetCursorDataStatus.
   */
  svtkGetVectorMacro(CurrentCursorPosition, double, 3);
  //@}

  //@{
  /**
   * Get the current image value at the current cursor position.  To
   * be used in conjunction with GetCursorDataStatus.  The value is
   * SVTK_DOUBLE_MAX when the data is invalid.
   */
  svtkGetMacro(CurrentImageValue, double);
  //@}

  //@{
  /**
   * Get the current reslice class and reslice axes
   */
  svtkGetObjectMacro(ResliceAxes, svtkMatrix4x4);
  svtkGetObjectMacro(Reslice, svtkImageReslice);
  //@}

  //@{
  /**
   * Choose between voxel centered or continuous cursor probing.  With voxel
   * centered probing, the cursor snaps to the nearest voxel and the reported
   * cursor coordinates are extent based.  With continuous probing, voxel data
   * is interpolated using svtkDataSetAttributes' InterpolatePoint method and
   * the reported coordinates are 3D spatial continuous.
   */
  svtkSetMacro(UseContinuousCursor, svtkTypeBool);
  svtkGetMacro(UseContinuousCursor, svtkTypeBool);
  svtkBooleanMacro(UseContinuousCursor, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable mouse interaction so the widget remains on display.
   */
  void SetInteraction(svtkTypeBool interact);
  svtkGetMacro(Interaction, svtkTypeBool);
  svtkBooleanMacro(Interaction, svtkTypeBool);
  //@}

  //@{
  /**
   * Set action associated to buttons.
   */
  enum
  {
    SVTK_CURSOR_ACTION = 0,
    SVTK_SLICE_MOTION_ACTION = 1,
    SVTK_WINDOW_LEVEL_ACTION = 2
  };
  svtkSetClampMacro(LeftButtonAction, int, SVTK_CURSOR_ACTION, SVTK_WINDOW_LEVEL_ACTION);
  svtkGetMacro(LeftButtonAction, int);
  svtkSetClampMacro(MiddleButtonAction, int, SVTK_CURSOR_ACTION, SVTK_WINDOW_LEVEL_ACTION);
  svtkGetMacro(MiddleButtonAction, int);
  svtkSetClampMacro(RightButtonAction, int, SVTK_CURSOR_ACTION, SVTK_WINDOW_LEVEL_ACTION);
  svtkGetMacro(RightButtonAction, int);
  //@}

  //@{
  /**
   * Set the auto-modifiers associated to buttons.
   * This allows users to bind some buttons to actions that are usually
   * triggered by a key modifier. For example, if you do not need cursoring,
   * you can bind the left button action to SVTK_SLICE_MOTION_ACTION (see above)
   * and the left button auto modifier to SVTK_CONTROL_MODIFIER: you end up with
   * the left button controlling panning without pressing a key.
   */
  enum
  {
    SVTK_NO_MODIFIER = 0,
    SVTK_SHIFT_MODIFIER = 1,
    SVTK_CONTROL_MODIFIER = 2
  };
  svtkSetClampMacro(LeftButtonAutoModifier, int, SVTK_NO_MODIFIER, SVTK_CONTROL_MODIFIER);
  svtkGetMacro(LeftButtonAutoModifier, int);
  svtkSetClampMacro(MiddleButtonAutoModifier, int, SVTK_NO_MODIFIER, SVTK_CONTROL_MODIFIER);
  svtkGetMacro(MiddleButtonAutoModifier, int);
  svtkSetClampMacro(RightButtonAutoModifier, int, SVTK_NO_MODIFIER, SVTK_CONTROL_MODIFIER);
  svtkGetMacro(RightButtonAutoModifier, int);
  //@}

protected:
  svtkImagePlaneWidget();
  ~svtkImagePlaneWidget() override;

  svtkTypeBool TextureVisibility;

  int LeftButtonAction;
  int MiddleButtonAction;
  int RightButtonAction;

  int LeftButtonAutoModifier;
  int MiddleButtonAutoModifier;
  int RightButtonAutoModifier;

  enum
  {
    SVTK_NO_BUTTON = 0,
    SVTK_LEFT_BUTTON = 1,
    SVTK_MIDDLE_BUTTON = 2,
    SVTK_RIGHT_BUTTON = 3
  };
  int LastButtonPressed;

  // Manage the state of the widget
  int State;
  enum WidgetState
  {
    Start = 0,
    Cursoring,
    WindowLevelling,
    Pushing,
    Spinning,
    Rotating,
    Moving,
    Scaling,
    Outside
  };

  // Handles the events
  static void ProcessEvents(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // internal utility method that adds observers to the RenderWindowInteractor
  // so that our ProcessEvents is eventually called.  this method is called
  // by SetEnabled as well as SetInteraction
  void AddObservers();

  // ProcessEvents() dispatches to these methods.
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();
  void OnChar() override;

  virtual void StartCursor();
  virtual void StopCursor();
  virtual void StartSliceMotion();
  virtual void StopSliceMotion();
  virtual void StartWindowLevel();
  virtual void StopWindowLevel();

  // controlling ivars
  svtkTypeBool Interaction; // Is the widget responsive to mouse events
  int PlaneOrientation;
  svtkTypeBool RestrictPlaneToVolume;
  double OriginalWindow;
  double OriginalLevel;
  double CurrentWindow;
  double CurrentLevel;
  double InitialWindow;
  double InitialLevel;
  int StartWindowLevelPositionX;
  int StartWindowLevelPositionY;
  int ResliceInterpolate;
  svtkTypeBool TextureInterpolate;
  svtkTypeBool UserControlledLookupTable;
  svtkTypeBool DisplayText;

  // The geometric representation of the plane and it's outline
  svtkPlaneSource* PlaneSource;
  svtkPolyData* PlaneOutlinePolyData;
  svtkActor* PlaneOutlineActor;
  void HighlightPlane(int highlight);
  void GeneratePlaneOutline();

  // Re-builds the plane outline based on the plane source
  void BuildRepresentation();

  // Do the picking
  svtkAbstractPropPicker* PlanePicker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // for negative window values.
  void InvertTable();

  // Methods to manipulate the plane
  void WindowLevel(int X, int Y);
  void Push(double* p1, double* p2);
  void Spin(double* p1, double* p2);
  void Rotate(double* p1, double* p2, double* vpn);
  void Scale(double* p1, double* p2, int X, int Y);
  void Translate(double* p1, double* p2);

  svtkImageData* ImageData;
  svtkImageReslice* Reslice;
  svtkMatrix4x4* ResliceAxes;
  svtkTransform* Transform;
  svtkActor* TexturePlaneActor;
  svtkImageMapToColors* ColorMap;
  svtkTexture* Texture;
  svtkLookupTable* LookupTable;
  svtkLookupTable* CreateDefaultLookupTable();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.  The plane property is actually that for
  // the outline.  The TexturePlaneProperty can be used to control the
  // lighting etc. of the resliced image data.
  svtkProperty* PlaneProperty;
  svtkProperty* SelectedPlaneProperty;
  svtkProperty* CursorProperty;
  svtkProperty* MarginProperty;
  svtkProperty* TexturePlaneProperty;
  void CreateDefaultProperties();

  // Reslice and texture management
  void UpdatePlane();
  void GenerateTexturePlane();

  // The cross-hair cursor
  svtkPolyData* CursorPolyData;
  svtkActor* CursorActor;
  double CurrentCursorPosition[3];
  double CurrentImageValue; // Set to SVTK_DOUBLE_MAX when invalid
  void GenerateCursor();
  void UpdateCursor(int, int);
  void ActivateCursor(int);
  int UpdateContinuousCursor(double* q);
  int UpdateDiscreteCursor(double* q);
  svtkTypeBool UseContinuousCursor;

  // The text to display W/L, image data
  svtkTextActor* TextActor;
  char TextBuff[SVTK_IMAGE_PLANE_WIDGET_MAX_TEXTBUFF];
  void GenerateText();
  void ManageTextDisplay();
  void ActivateText(int);

  // Oblique reslice control
  double RotateAxis[3];
  double RadiusVector[3];
  void AdjustState();

  // Visible margins to assist user interaction
  svtkPolyData* MarginPolyData;
  svtkActor* MarginActor;
  int MarginSelectMode;
  void GenerateMargins();
  void UpdateMargins();
  void ActivateMargins(int);
  double MarginSizeX;
  double MarginSizeY;

private:
  svtkImagePlaneWidget(const svtkImagePlaneWidget&) = delete;
  void operator=(const svtkImagePlaneWidget&) = delete;
};

#endif
