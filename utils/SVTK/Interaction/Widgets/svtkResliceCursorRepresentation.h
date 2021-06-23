/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceCursorRepresentation
 * @brief   represent the svtkResliceCursorWidget
 *
 * This class is the base class for the reslice cursor representation
 * subclasses. It represents a cursor that may be interactively translated,
 * rotated through an image and perform thick / thick reformats.
 * @sa
 * svtkResliceCursorLineRepresentation svtkResliceCursorThickLineRepresentation
 * svtkResliceCursorWidget svtkResliceCursor
 */

#ifndef svtkResliceCursorRepresentation_h
#define svtkResliceCursorRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkTextProperty;
class svtkActor2D;
class svtkTextMapper;
class svtkImageData;
class svtkImageReslice;
class svtkPlane;
class svtkPlaneSource;
class svtkResliceCursorPolyDataAlgorithm;
class svtkResliceCursor;
class svtkMatrix4x4;
class svtkScalarsToColors;
class svtkImageMapToColors;
class svtkActor;
class svtkImageActor;
class svtkTexture;
class svtkTextActor;
class svtkImageAlgorithm;

// Private.
#define SVTK_RESLICE_CURSOR_REPRESENTATION_MAX_TEXTBUFF 128

class SVTKINTERACTIONWIDGETS_EXPORT svtkResliceCursorRepresentation : public svtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkResliceCursorRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * The tolerance representing the distance to the representation (in
   * pixels) in which the cursor is considered near enough to the
   * representation to be active.
   */
  svtkSetClampMacro(Tolerance, int, 1, 100);
  svtkGetMacro(Tolerance, int);
  //@}

  //@{
  /**
   * Show the resliced image ?
   */
  svtkSetMacro(ShowReslicedImage, svtkTypeBool);
  svtkGetMacro(ShowReslicedImage, svtkTypeBool);
  svtkBooleanMacro(ShowReslicedImage, svtkTypeBool);
  //@}

  //@{
  /**
   * Make sure that the resliced image remains within the volume.
   * Default is On.
   */
  svtkSetMacro(RestrictPlaneToVolume, svtkTypeBool);
  svtkGetMacro(RestrictPlaneToVolume, svtkTypeBool);
  svtkBooleanMacro(RestrictPlaneToVolume, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the format to use for labelling the distance. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the thickness value.
   */
  svtkSetStringMacro(ThicknessLabelFormat);
  svtkGetStringMacro(ThicknessLabelFormat);
  //@}

  // Used to communicate about the state of the representation
  enum
  {
    Outside = 0,
    NearCenter,
    NearAxis1,
    NearAxis2,
    OnCenter,
    OnAxis1,
    OnAxis2
  };
  enum
  {
    None = 0,
    PanAndRotate,
    RotateBothAxes,
    ResizeThickness,
    WindowLevelling
  };

  /**
   * Get the text shown in the widget's label.
   */
  virtual char* GetThicknessLabelText();

  //@{
  /**
   * Get the position of the widget's label in display coordinates.
   */
  virtual double* GetThicknessLabelPosition();
  virtual void GetThicknessLabelPosition(double pos[3]);
  virtual void GetWorldThicknessLabelPosition(double pos[3]);
  //@}

  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void BuildRepresentation() override;

  //@{
  /**
   * Get the current reslice class and reslice axes
   */
  svtkGetObjectMacro(ResliceAxes, svtkMatrix4x4);
  svtkGetObjectMacro(Reslice, svtkImageAlgorithm);
  //@}

  //@{
  /**
   * Get the displayed image actor
   */
  svtkGetObjectMacro(ImageActor, svtkImageActor);
  //@}

  //@{
  /**
   * Set/Get the internal lookuptable (lut) to one defined by the user, or,
   * alternatively, to the lut of another Reslice cusror widget.  In this way,
   * a set of three orthogonal planes can share the same lut so that
   * window-levelling is performed uniformly among planes.  The default
   * internal lut can be re- set/allocated by setting to 0 (nullptr).
   */
  virtual void SetLookupTable(svtkScalarsToColors*);
  svtkGetObjectMacro(LookupTable, svtkScalarsToColors);
  //@}

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

  virtual svtkResliceCursor* GetResliceCursor() = 0;

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
   * Set/Get the text property for the image data and window-level annotation.
   */
  void SetTextProperty(svtkTextProperty* tprop);
  svtkTextProperty* GetTextProperty();
  //@}

  //@{
  /**
   * Render as a 2D image, or render as a plane with a texture in physical
   * space.
   */
  svtkSetMacro(UseImageActor, svtkTypeBool);
  svtkGetMacro(UseImageActor, svtkTypeBool);
  svtkBooleanMacro(UseImageActor, svtkTypeBool);
  //@}

  //@{
  /**
   * INTERNAL - Do not use
   * Set the manipulation mode. This is done by the widget
   */
  void SetManipulationMode(int m);
  svtkGetMacro(ManipulationMode, int);
  //@}

  //@{
  /**
   * INTERNAL - Do not use.
   * Internal methods used by the widget to manage text displays
   * for annotations.
   */
  void ActivateText(int);
  void ManageTextDisplay();
  //@}

  //@{
  /**
   * Initialize the reslice planes and the camera center. This is done
   * automatically, the first time we render.
   */
  virtual void InitializeReslicePlane();
  virtual void ResetCamera();
  //@}

  /**
   * Get the underlying cursor source.
   */
  virtual svtkResliceCursorPolyDataAlgorithm* GetCursorAlgorithm() = 0;

  //@{
  /**
   * Get the plane source on which the texture (the thin/thick resliced
   * image is displayed)
   */
  svtkGetObjectMacro(PlaneSource, svtkPlaneSource);
  //@}

protected:
  svtkResliceCursorRepresentation();
  ~svtkResliceCursorRepresentation() override;

  //@{
  /**
   * Create New Reslice plane. Allows subclasses to override and crate
   * their own reslice filters to respond to the widget.
   */
  virtual void CreateDefaultResliceAlgorithm();
  virtual void SetResliceParameters(
    double outputSpacingX, double outputSpacingY, int extentX, int extentY);
  //@}

  /**
   * Process window level
   */
  virtual void WindowLevel(double x, double y);

  /**
   * Update the reslice plane
   */
  virtual void UpdateReslicePlane();

  /**
   * Compute the origin of the planes so as to capture the entire image.
   */
  virtual void ComputeReslicePlaneOrigin();

  // for negative window values.
  void InvertTable();

  // recompute origin to make the location of the reslice cursor consistent
  // with its physical location
  virtual void ComputeOrigin(svtkMatrix4x4*);

  //@{
  void GetVector1(double d[3]);
  void GetVector2(double d[3]);
  //@}

  /**
   * The widget sets the manipulation mode. This can be one of :
   * None, PanAndRotate, RotateBothAxes, ResizeThickness
   */
  int ManipulationMode;

  // Keep track if modifier is set
  int Modifier;

  // Selection tolerance for the handles
  int Tolerance;

  // Format for printing the distance
  char* ThicknessLabelFormat;

  svtkImageAlgorithm* Reslice;
  svtkPlaneSource* PlaneSource;
  svtkTypeBool RestrictPlaneToVolume;
  svtkTypeBool ShowReslicedImage;
  svtkTextProperty* ThicknessTextProperty;
  svtkTextMapper* ThicknessTextMapper;
  svtkActor2D* ThicknessTextActor;
  svtkMatrix4x4* ResliceAxes;
  svtkMatrix4x4* NewResliceAxes;
  svtkImageMapToColors* ColorMap;
  svtkActor* TexturePlaneActor;
  svtkTexture* Texture;
  svtkScalarsToColors* LookupTable;
  svtkImageActor* ImageActor;
  svtkTextActor* TextActor;
  double OriginalWindow;
  double OriginalLevel;
  double CurrentWindow;
  double CurrentLevel;
  double InitialWindow;
  double InitialLevel;
  double LastEventPosition[2];
  svtkTypeBool UseImageActor;
  char TextBuff[SVTK_RESLICE_CURSOR_REPRESENTATION_MAX_TEXTBUFF];
  svtkTypeBool DisplayText;

  svtkScalarsToColors* CreateDefaultLookupTable();
  void GenerateText();

private:
  svtkResliceCursorRepresentation(const svtkResliceCursorRepresentation&) = delete;
  void operator=(const svtkResliceCursorRepresentation&) = delete;
};

#endif
