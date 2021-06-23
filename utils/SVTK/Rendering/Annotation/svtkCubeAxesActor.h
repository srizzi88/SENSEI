/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCubeAxesActor.h
  Language:  C++

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen
All rights reserve
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
/**
 * @class   svtkCubeAxesActor
 * @brief   create a plot of a bounding box edges -
 * used for navigation
 *
 * svtkCubeAxesActor is a composite actor that draws axes of the
 * bounding box of an input dataset. The axes include labels and titles
 * for the x-y-z axes. The algorithm selects which axes to draw based
 * on the user-defined 'fly' mode.  (STATIC is default).
 * 'STATIC' constructs axes from all edges of the bounding box.
 * 'CLOSEST_TRIAD' consists of the three axes x-y-z forming a triad that
 * lies closest to the specified camera.
 * 'FURTHEST_TRIAD' consists of the three axes x-y-z forming a triad that
 * lies furthest from the specified camera.
 * 'OUTER_EDGES' is constructed from edges that are on the "exterior" of the
 * bounding box, exterior as determined from examining outer edges of the
 * bounding box in projection (display) space.
 *
 * To use this object you must define a bounding box and the camera used
 * to render the svtkCubeAxesActor. You can optionally turn on/off labels,
 * ticks, gridlines, and set tick location, number of labels, and text to
 * use for axis-titles.  A 'corner offset' can also be set.  This allows
 * the axes to be set partially away from the actual bounding box to perhaps
 * prevent overlap of labels between the various axes.
 *
 * The Bounds instance variable (an array of six doubles) is used to determine
 * the bounding box.
 *
 * @par Thanks:
 * This class was written by:
 * Hank Childs, Kathleen Bonnell, Amy Squillacote, Brad Whitlock, Will Schroeder,
 * Eric Brugger, Daniel Aguilera, Claire Guilbaud, Nicolas Dolegieviez,
 * Aashish Chaudhary, Philippe Pebay, David Gobbi, David Partyka, Utkarsh Ayachit
 * David Cole, Francois Bertel, and Mark Olesen
 * Part of this work was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France.
 *
 * @sa
 * svtkActor svtkAxisActor svtkCubeAxesActor2D
 */

#ifndef svtkCubeAxesActor_h
#define svtkCubeAxesActor_h

#include "svtkActor.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkAxisActor;
class svtkCamera;
class svtkTextProperty;
class svtkStringArray;

class SVTKRENDERINGANNOTATION_EXPORT svtkCubeAxesActor : public svtkActor
{
public:
  svtkTypeMacro(svtkCubeAxesActor, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with label format "6.3g" and the number of labels
   * per axis set to 3.
   */
  static svtkCubeAxesActor* New();

  //@{
  /**
   * Draw the axes as per the svtkProp superclass' API.
   */
  int RenderOpaqueGeometry(svtkViewport*) override;
  virtual int RenderTranslucentGeometry(svtkViewport*);
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  int RenderOverlay(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Gets/Sets the RebuildAxes flag
   */
  svtkSetMacro(RebuildAxes, bool);
  svtkGetMacro(RebuildAxes, bool);
  //@}

  //@{
  /**
   * Explicitly specify the region in space around which to draw the bounds.
   * The bounds is used only when no Input or Prop is specified. The bounds
   * are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
   * sure that the min's are less than the max's.
   */
  svtkSetVector6Macro(Bounds, double);
  using Superclass::GetBounds;
  double* GetBounds() SVTK_SIZEHINT(6) override { return this->Bounds; }
  //@}

  //@{
  /**
   * Method used to properly return the bounds of the cube axis itself with all
   * its labels.
   */
  virtual void GetRenderedBounds(double rBounds[6]);
  virtual double* GetRenderedBounds();
  //@}

  //@{
  /**
   * Explicitly specify the range of each axes that's used to define the prop.
   * The default, (if you do not use these methods) is to use the bounds
   * specified, or use the bounds of the Input Prop if one is specified. This
   * method allows you to separate the notion of extent of the axes in physical
   * space (bounds) and the extent of the values it represents. In other words,
   * you can have the ticks and labels show a different range.
   */
  svtkSetVector2Macro(XAxisRange, double);
  svtkSetVector2Macro(YAxisRange, double);
  svtkSetVector2Macro(ZAxisRange, double);
  svtkGetVector2Macro(XAxisRange, double);
  svtkGetVector2Macro(YAxisRange, double);
  //@}
  //@{
  /**
   * Explicitly specify the axis labels along an axis as an array of strings
   * instead of using the values.
   */
  svtkStringArray* GetAxisLabels(int axis);
  void SetAxisLabels(int axis, svtkStringArray* value);
  //@}

  svtkGetVector2Macro(ZAxisRange, double);

  //@{
  /**
   * Explicitly specify the screen size of title and label text.
   * ScreenSize determines the size of the text in terms of screen
   * pixels. Default is 10.0.
   */
  void SetScreenSize(double screenSize);
  svtkGetMacro(ScreenSize, double);
  //@}

  //@{
  /**
   * Explicitly specify the distance between labels and the axis.
   * Default is 20.0.
   */
  void SetLabelOffset(double offset);
  svtkGetMacro(LabelOffset, double);
  //@}

  //@{
  /**
   * Explicitly specify the distance between title and labels.
   * Default is 20.0.
   */
  void SetTitleOffset(double offset);
  svtkGetMacro(TitleOffset, double);
  //@}

  //@{
  /**
   * Set/Get the camera to perform scaling and translation of the
   * svtkCubeAxesActor.
   */
  virtual void SetCamera(svtkCamera*);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  enum FlyMode
  {
    SVTK_FLY_OUTER_EDGES = 0,
    SVTK_FLY_CLOSEST_TRIAD = 1,
    SVTK_FLY_FURTHEST_TRIAD = 2,
    SVTK_FLY_STATIC_TRIAD = 3,
    SVTK_FLY_STATIC_EDGES = 4
  };

  //@{
  /**
   * Specify a mode to control how the axes are drawn: either static,
   * closest triad, furthest triad or outer edges in relation to the
   * camera position.
   */
  svtkSetClampMacro(FlyMode, int, SVTK_FLY_OUTER_EDGES, SVTK_FLY_STATIC_EDGES);
  svtkGetMacro(FlyMode, int);
  void SetFlyModeToOuterEdges() { this->SetFlyMode(SVTK_FLY_OUTER_EDGES); }
  void SetFlyModeToClosestTriad() { this->SetFlyMode(SVTK_FLY_CLOSEST_TRIAD); }
  void SetFlyModeToFurthestTriad() { this->SetFlyMode(SVTK_FLY_FURTHEST_TRIAD); }
  void SetFlyModeToStaticTriad() { this->SetFlyMode(SVTK_FLY_STATIC_TRIAD); }
  void SetFlyModeToStaticEdges() { this->SetFlyMode(SVTK_FLY_STATIC_EDGES); }
  //@}

  //@{
  /**
   * Set/Get the labels for the x, y, and z axes. By default,
   * use "X-Axis", "Y-Axis" and "Z-Axis".
   */
  svtkSetStringMacro(XTitle);
  svtkGetStringMacro(XTitle);
  svtkSetStringMacro(XUnits);
  svtkGetStringMacro(XUnits);
  svtkSetStringMacro(YTitle);
  svtkGetStringMacro(YTitle);
  svtkSetStringMacro(YUnits);
  svtkGetStringMacro(YUnits);
  svtkSetStringMacro(ZTitle);
  svtkGetStringMacro(ZTitle);
  svtkSetStringMacro(ZUnits);
  svtkGetStringMacro(ZUnits);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on each of the
   * x-y-z axes.
   */
  svtkSetStringMacro(XLabelFormat);
  svtkGetStringMacro(XLabelFormat);
  svtkSetStringMacro(YLabelFormat);
  svtkGetStringMacro(YLabelFormat);
  svtkSetStringMacro(ZLabelFormat);
  svtkGetStringMacro(ZLabelFormat);
  //@}

  //@{
  /**
   * Set/Get the inertial factor that controls how often (i.e, how
   * many renders) the axes can switch position (jump from one axes
   * to another).
   */
  svtkSetClampMacro(Inertia, int, 1, SVTK_INT_MAX);
  svtkGetMacro(Inertia, int);
  //@}

  //@{
  /**
   * Specify an offset value to "pull back" the axes from the corner at
   * which they are joined to avoid overlap of axes labels. The
   * "CornerOffset" is the fraction of the axis length to pull back.
   */
  svtkSetMacro(CornerOffset, double);
  svtkGetMacro(CornerOffset, double);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * Enable and disable the use of distance based LOD for titles and labels.
   */
  svtkSetMacro(EnableDistanceLOD, int);
  svtkGetMacro(EnableDistanceLOD, int);
  //@}

  //@{
  /**
   * Set distance LOD threshold [0.0 - 1.0] for titles and labels.
   */
  svtkSetClampMacro(DistanceLODThreshold, double, 0.0, 1.0);
  svtkGetMacro(DistanceLODThreshold, double);
  //@}

  //@{
  /**
   * Enable and disable the use of view angle based LOD for titles and labels.
   */
  svtkSetMacro(EnableViewAngleLOD, int);
  svtkGetMacro(EnableViewAngleLOD, int);
  //@}

  //@{
  /**
   * Set view angle LOD threshold [0.0 - 1.0] for titles and labels.
   */
  svtkSetClampMacro(ViewAngleLODThreshold, double, 0., 1.);
  svtkGetMacro(ViewAngleLODThreshold, double);
  //@}

  //@{
  /**
   * Turn on and off the visibility of each axis.
   */
  svtkSetMacro(XAxisVisibility, svtkTypeBool);
  svtkGetMacro(XAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(XAxisVisibility, svtkTypeBool);
  svtkSetMacro(YAxisVisibility, svtkTypeBool);
  svtkGetMacro(YAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(YAxisVisibility, svtkTypeBool);
  svtkSetMacro(ZAxisVisibility, svtkTypeBool);
  svtkGetMacro(ZAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(ZAxisVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of labels for each axis.
   */
  svtkSetMacro(XAxisLabelVisibility, svtkTypeBool);
  svtkGetMacro(XAxisLabelVisibility, svtkTypeBool);
  svtkBooleanMacro(XAxisLabelVisibility, svtkTypeBool);
  //@}

  svtkSetMacro(YAxisLabelVisibility, svtkTypeBool);
  svtkGetMacro(YAxisLabelVisibility, svtkTypeBool);
  svtkBooleanMacro(YAxisLabelVisibility, svtkTypeBool);

  svtkSetMacro(ZAxisLabelVisibility, svtkTypeBool);
  svtkGetMacro(ZAxisLabelVisibility, svtkTypeBool);
  svtkBooleanMacro(ZAxisLabelVisibility, svtkTypeBool);

  //@{
  /**
   * Turn on and off the visibility of ticks for each axis.
   */
  svtkSetMacro(XAxisTickVisibility, svtkTypeBool);
  svtkGetMacro(XAxisTickVisibility, svtkTypeBool);
  svtkBooleanMacro(XAxisTickVisibility, svtkTypeBool);
  //@}

  svtkSetMacro(YAxisTickVisibility, svtkTypeBool);
  svtkGetMacro(YAxisTickVisibility, svtkTypeBool);
  svtkBooleanMacro(YAxisTickVisibility, svtkTypeBool);

  svtkSetMacro(ZAxisTickVisibility, svtkTypeBool);
  svtkGetMacro(ZAxisTickVisibility, svtkTypeBool);
  svtkBooleanMacro(ZAxisTickVisibility, svtkTypeBool);

  //@{
  /**
   * Turn on and off the visibility of minor ticks for each axis.
   */
  svtkSetMacro(XAxisMinorTickVisibility, svtkTypeBool);
  svtkGetMacro(XAxisMinorTickVisibility, svtkTypeBool);
  svtkBooleanMacro(XAxisMinorTickVisibility, svtkTypeBool);
  //@}

  svtkSetMacro(YAxisMinorTickVisibility, svtkTypeBool);
  svtkGetMacro(YAxisMinorTickVisibility, svtkTypeBool);
  svtkBooleanMacro(YAxisMinorTickVisibility, svtkTypeBool);

  svtkSetMacro(ZAxisMinorTickVisibility, svtkTypeBool);
  svtkGetMacro(ZAxisMinorTickVisibility, svtkTypeBool);
  svtkBooleanMacro(ZAxisMinorTickVisibility, svtkTypeBool);

  svtkSetMacro(DrawXGridlines, svtkTypeBool);
  svtkGetMacro(DrawXGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawXGridlines, svtkTypeBool);

  svtkSetMacro(DrawYGridlines, svtkTypeBool);
  svtkGetMacro(DrawYGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawYGridlines, svtkTypeBool);

  svtkSetMacro(DrawZGridlines, svtkTypeBool);
  svtkGetMacro(DrawZGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawZGridlines, svtkTypeBool);

  svtkSetMacro(DrawXInnerGridlines, svtkTypeBool);
  svtkGetMacro(DrawXInnerGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawXInnerGridlines, svtkTypeBool);

  svtkSetMacro(DrawYInnerGridlines, svtkTypeBool);
  svtkGetMacro(DrawYInnerGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawYInnerGridlines, svtkTypeBool);

  svtkSetMacro(DrawZInnerGridlines, svtkTypeBool);
  svtkGetMacro(DrawZInnerGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawZInnerGridlines, svtkTypeBool);

  svtkSetMacro(DrawXGridpolys, svtkTypeBool);
  svtkGetMacro(DrawXGridpolys, svtkTypeBool);
  svtkBooleanMacro(DrawXGridpolys, svtkTypeBool);

  svtkSetMacro(DrawYGridpolys, svtkTypeBool);
  svtkGetMacro(DrawYGridpolys, svtkTypeBool);
  svtkBooleanMacro(DrawYGridpolys, svtkTypeBool);

  svtkSetMacro(DrawZGridpolys, svtkTypeBool);
  svtkGetMacro(DrawZGridpolys, svtkTypeBool);
  svtkBooleanMacro(DrawZGridpolys, svtkTypeBool);

  /**
   * Returns the text property for the title on an axis.
   */
  svtkTextProperty* GetTitleTextProperty(int);

  /**
   * Returns the text property for the labels on an axis.
   */
  svtkTextProperty* GetLabelTextProperty(int);

  //@{
  /**
   * Get/Set axes actors properties.
   */
  void SetXAxesLinesProperty(svtkProperty*);
  svtkProperty* GetXAxesLinesProperty();
  void SetYAxesLinesProperty(svtkProperty*);
  svtkProperty* GetYAxesLinesProperty();
  void SetZAxesLinesProperty(svtkProperty*);
  svtkProperty* GetZAxesLinesProperty();
  //@}

  //@{
  /**
   * Get/Set axes (outer) gridlines actors properties.
   */
  void SetXAxesGridlinesProperty(svtkProperty*);
  svtkProperty* GetXAxesGridlinesProperty();
  void SetYAxesGridlinesProperty(svtkProperty*);
  svtkProperty* GetYAxesGridlinesProperty();
  void SetZAxesGridlinesProperty(svtkProperty*);
  svtkProperty* GetZAxesGridlinesProperty();
  //@}

  //@{
  /**
   * Get/Set axes inner gridlines actors properties.
   */
  void SetXAxesInnerGridlinesProperty(svtkProperty*);
  svtkProperty* GetXAxesInnerGridlinesProperty();
  void SetYAxesInnerGridlinesProperty(svtkProperty*);
  svtkProperty* GetYAxesInnerGridlinesProperty();
  void SetZAxesInnerGridlinesProperty(svtkProperty*);
  svtkProperty* GetZAxesInnerGridlinesProperty();
  //@}

  //@{
  /**
   * Get/Set axes gridPolys actors properties.
   */
  void SetXAxesGridpolysProperty(svtkProperty*);
  svtkProperty* GetXAxesGridpolysProperty();
  void SetYAxesGridpolysProperty(svtkProperty*);
  svtkProperty* GetYAxesGridpolysProperty();
  void SetZAxesGridpolysProperty(svtkProperty*);
  svtkProperty* GetZAxesGridpolysProperty();
  //@}

  enum TickLocation
  {
    SVTK_TICKS_INSIDE = 0,
    SVTK_TICKS_OUTSIDE = 1,
    SVTK_TICKS_BOTH = 2
  };

  //@{
  /**
   * Set/Get the location of ticks marks.
   */
  svtkSetClampMacro(TickLocation, int, SVTK_TICKS_INSIDE, SVTK_TICKS_BOTH);
  svtkGetMacro(TickLocation, int);
  //@}

  void SetTickLocationToInside(void) { this->SetTickLocation(SVTK_TICKS_INSIDE); }
  void SetTickLocationToOutside(void) { this->SetTickLocation(SVTK_TICKS_OUTSIDE); }
  void SetTickLocationToBoth(void) { this->SetTickLocation(SVTK_TICKS_BOTH); }

  void SetLabelScaling(bool, int, int, int);

  //@{
  /**
   * Use or not svtkTextActor3D for titles and labels.
   * See Also:
   * svtkAxisActor::SetUseTextActor3D(), svtkAxisActor::GetUseTextActor3D()
   */
  void SetUseTextActor3D(int val);
  int GetUseTextActor3D();
  //@}

  //@{
  /**
   * Get/Set 2D mode
   * NB: Use svtkTextActor for titles in 2D instead of svtkAxisFollower
   */
  void SetUse2DMode(int val);
  int GetUse2DMode();
  //@}

  /**
   * For 2D mode only: save axis title positions for later use
   */
  void SetSaveTitlePosition(int val);

  //@{
  /**
   * Provide an oriented bounded box when using AxisBaseFor.
   */
  svtkSetVector6Macro(OrientedBounds, double);
  svtkGetVector6Macro(OrientedBounds, double);
  //@}

  //@{
  /**
   * Enable/Disable the usage of the OrientedBounds
   */
  svtkSetMacro(UseOrientedBounds, int);
  svtkGetMacro(UseOrientedBounds, int);
  //@}

  //@{
  /**
   * Vector that should be use as the base for X
   */
  svtkSetVector3Macro(AxisBaseForX, double);
  svtkGetVector3Macro(AxisBaseForX, double);
  //@}

  //@{
  /**
   * Vector that should be use as the base for Y
   */
  svtkSetVector3Macro(AxisBaseForY, double);
  svtkGetVector3Macro(AxisBaseForY, double);
  //@}

  //@{
  /**
   * Vector that should be use as the base for Z
   */
  svtkSetVector3Macro(AxisBaseForZ, double);
  svtkGetVector3Macro(AxisBaseForZ, double);
  //@}

  //@{
  /**
   * Provide a custom AxisOrigin. This point must be inside the bounding box and
   * will represent the point where the 3 axes will intersect
   */
  svtkSetVector3Macro(AxisOrigin, double);
  svtkGetVector3Macro(AxisOrigin, double);
  //@}

  //@{
  /**
   * Enable/Disable the usage of the AxisOrigin
   */
  svtkSetMacro(UseAxisOrigin, int);
  svtkGetMacro(UseAxisOrigin, int);
  //@}

  //@{
  /**
   * Specify the mode in which the cube axes should render its gridLines
   */
  svtkSetMacro(GridLineLocation, int);
  svtkGetMacro(GridLineLocation, int);
  //@}

  //@{
  /**
   * Enable/Disable axis stickiness. When on, the axes will be adjusted to always
   * be visible in the viewport unless the original bounds of the axes are entirely
   * outside the viewport. Defaults to off.
   */
  svtkSetMacro(StickyAxes, svtkTypeBool);
  svtkGetMacro(StickyAxes, svtkTypeBool);
  svtkBooleanMacro(StickyAxes, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/Disable centering of axes when the Sticky option is
   * on. If on, the axes bounds will be centered in the
   * viewport. Otherwise, the axes can move about the longer of the
   * horizontal or verical directions of the viewport to follow the
   * data. Defaults to on.
   */
  svtkSetMacro(CenterStickyAxes, svtkTypeBool);
  svtkGetMacro(CenterStickyAxes, svtkTypeBool);
  svtkBooleanMacro(CenterStickyAxes, svtkTypeBool);
  //@}

  enum GridVisibility
  {
    SVTK_GRID_LINES_ALL = 0,
    SVTK_GRID_LINES_CLOSEST = 1,
    SVTK_GRID_LINES_FURTHEST = 2
  };

protected:
  svtkCubeAxesActor();
  ~svtkCubeAxesActor() override;

  /**
   * Computes a bounding sphere used to determine the sticky bounding box.
   * Sphere center and sphere radius are return parameters and can remain uninitialized
   * prior to calling this method.
   */
  void ComputeStickyAxesBoundingSphere(
    svtkViewport* viewport, const double bounds[6], double sphereCenter[3], double& sphereRadius);

  /**
   * Get bounds such that the axes are entirely within a viewport
   */
  void GetViewportLimitedBounds(svtkViewport* viewport, double bounds[6]);

  /**
   * Get the bits for a bounds point. 0 means the lower side for a
   * coordinate, 1 means the higher side.
   */
  static void GetBoundsPointBits(
    unsigned int pointIndex, unsigned int& xBit, unsigned int& yBit, unsigned int& zBit);

  /**
   * Get a point on the bounding box by point index
   */
  static void GetBoundsPoint(unsigned int pointIndex, const double bounds[6], double point[3]);

  int LabelExponent(double min, double max);

  int Digits(double min, double max);

  double MaxOf(double, double);
  double MaxOf(double, double, double, double);

  double FFix(double);
  double FSign(double, double);
  int FRound(double fnt);
  int GetNumTicks(double range, double fxt);

  void UpdateLabels(svtkAxisActor** axis, int index);

  svtkCamera* Camera;

  int FlyMode;

  // Expose internally closest axis index computation
  int FindClosestAxisIndex(double pts[8][3]);

  // Expose internally furthest axis index computation
  int FindFurtherstAxisIndex(double pts[8][3]);

  // Expose internally the boundary edge fly mode axis index computation
  void FindBoundaryEdge(int& indexOfAxisX, int& indexOfAxisY, int& indexOfAxisZ, double pts[8][3]);

  /**
   * This will Update AxisActors with GridVisibility when those should be
   * dynamaic regarding the viewport.
   * GridLineLocation = [SVTK_CLOSEST_GRID_LINES, SVTK_FURTHEST_GRID_LINES]
   */
  void UpdateGridLineVisibility(int axisIndex);

  // SVTK_ALL_GRID_LINES      0
  // SVTK_CLOSEST_GRID_LINES  1
  // SVTK_FURTHEST_GRID_LINES 2
  int GridLineLocation;

  /**
   * Flag for axes stickiness
   */
  svtkTypeBool StickyAxes;

  /**
   * Flag for centering sticky axes
   */
  svtkTypeBool CenterStickyAxes;

  /**
   * If enabled the actor will not be visible at a certain distance from the camera.
   * Default is true
   */
  int EnableDistanceLOD;

  /**
   * Default is 0.80
   * This determines at what fraction of camera far clip range, actor is not visible.
   */
  double DistanceLODThreshold;

  /**
   * If enabled the actor will not be visible at a certain view angle.
   * Default is true.
   */
  int EnableViewAngleLOD;

  /**
   * This determines at what view angle to geometry will make the geometry not visible.
   * Default is 0.3.
   */
  double ViewAngleLODThreshold;

  enum NumberOfAlignedAxis
  {
    NUMBER_OF_ALIGNED_AXIS = 4
  };

  //@{
  /**
   * Control variables for all axes
   * NB: [0] always for 'Major' axis during non-static fly modes.
   */
  svtkAxisActor* XAxes[NUMBER_OF_ALIGNED_AXIS];
  svtkAxisActor* YAxes[NUMBER_OF_ALIGNED_AXIS];
  svtkAxisActor* ZAxes[NUMBER_OF_ALIGNED_AXIS];
  //@}

  bool RebuildAxes;

  char* XTitle;
  char* XUnits;
  char* YTitle;
  char* YUnits;
  char* ZTitle;
  char* ZUnits;

  char* ActualXLabel;
  char* ActualYLabel;
  char* ActualZLabel;

  int TickLocation;

  svtkTypeBool XAxisVisibility;
  svtkTypeBool YAxisVisibility;
  svtkTypeBool ZAxisVisibility;

  svtkTypeBool XAxisTickVisibility;
  svtkTypeBool YAxisTickVisibility;
  svtkTypeBool ZAxisTickVisibility;

  svtkTypeBool XAxisMinorTickVisibility;
  svtkTypeBool YAxisMinorTickVisibility;
  svtkTypeBool ZAxisMinorTickVisibility;

  svtkTypeBool XAxisLabelVisibility;
  svtkTypeBool YAxisLabelVisibility;
  svtkTypeBool ZAxisLabelVisibility;

  svtkTypeBool DrawXGridlines;
  svtkTypeBool DrawYGridlines;
  svtkTypeBool DrawZGridlines;

  svtkTypeBool DrawXInnerGridlines;
  svtkTypeBool DrawYInnerGridlines;
  svtkTypeBool DrawZInnerGridlines;

  svtkTypeBool DrawXGridpolys;
  svtkTypeBool DrawYGridpolys;
  svtkTypeBool DrawZGridpolys;

  char* XLabelFormat;
  char* YLabelFormat;
  char* ZLabelFormat;

  double CornerOffset;

  int Inertia;

  int RenderCount;

  int InertiaLocs[3];

  int RenderSomething;

  svtkTextProperty* TitleTextProperty[3];
  svtkStringArray* AxisLabels[3];

  svtkTextProperty* LabelTextProperty[3];

  svtkProperty* XAxesLinesProperty;
  svtkProperty* YAxesLinesProperty;
  svtkProperty* ZAxesLinesProperty;
  svtkProperty* XAxesGridlinesProperty;
  svtkProperty* YAxesGridlinesProperty;
  svtkProperty* ZAxesGridlinesProperty;
  svtkProperty* XAxesInnerGridlinesProperty;
  svtkProperty* YAxesInnerGridlinesProperty;
  svtkProperty* ZAxesInnerGridlinesProperty;
  svtkProperty* XAxesGridpolysProperty;
  svtkProperty* YAxesGridpolysProperty;
  svtkProperty* ZAxesGridpolysProperty;

  double RenderedBounds[6];
  double OrientedBounds[6];
  int UseOrientedBounds;

  double AxisOrigin[3];
  int UseAxisOrigin;

  double AxisBaseForX[3];
  double AxisBaseForY[3];
  double AxisBaseForZ[3];

private:
  svtkCubeAxesActor(const svtkCubeAxesActor&) = delete;
  void operator=(const svtkCubeAxesActor&) = delete;

  svtkSetStringMacro(ActualXLabel);
  svtkSetStringMacro(ActualYLabel);
  svtkSetStringMacro(ActualZLabel);

  svtkTimeStamp BuildTime;
  int LastUseOrientedBounds;
  int LastXPow;
  int LastYPow;
  int LastZPow;

  int UserXPow;
  int UserYPow;
  int UserZPow;

  bool AutoLabelScaling;

  int LastXAxisDigits;
  int LastYAxisDigits;
  int LastZAxisDigits;

  double LastXRange[2];
  double LastYRange[2];
  double LastZRange[2];
  double LastBounds[6];

  int LastFlyMode;

  int RenderAxesX[NUMBER_OF_ALIGNED_AXIS];
  int RenderAxesY[NUMBER_OF_ALIGNED_AXIS];
  int RenderAxesZ[NUMBER_OF_ALIGNED_AXIS];

  int NumberOfAxesX;
  int NumberOfAxesY;
  int NumberOfAxesZ;

  bool MustAdjustXValue;
  bool MustAdjustYValue;
  bool MustAdjustZValue;

  bool ForceXLabelReset;
  bool ForceYLabelReset;
  bool ForceZLabelReset;

  double XAxisRange[2];
  double YAxisRange[2];
  double ZAxisRange[2];

  double LabelScale;
  double TitleScale;

  double ScreenSize;
  double LabelOffset;
  double TitleOffset;

  //@{
  /**
   * Major start and delta values, in each direction.
   * These values are needed for inner grid lines generation
   */
  double MajorStart[3];
  double DeltaMajor[3];
  //@}

  int RenderGeometry(bool& initialRender, svtkViewport* viewport, bool checkAxisVisibility,
    int (svtkAxisActor::*renderMethod)(svtkViewport*));

  void TransformBounds(svtkViewport* viewport, const double bounds[6], double pts[8][3]);
  void AdjustAxes(double bounds[6], double xCoords[NUMBER_OF_ALIGNED_AXIS][6],
    double yCoords[NUMBER_OF_ALIGNED_AXIS][6], double zCoords[NUMBER_OF_ALIGNED_AXIS][6],
    double xRange[2], double yRange[2], double zRange[2]);

  bool ComputeTickSize(double bounds[6]);
  void AdjustValues(const double xRange[2], const double yRange[2], const double zRange[2]);
  void AdjustRange(const double bounds[6]);
  void BuildAxes(svtkViewport*);
  void DetermineRenderAxes(svtkViewport*);
  void SetNonDependentAttributes(void);
  void BuildLabels(svtkAxisActor* axes[NUMBER_OF_ALIGNED_AXIS]);
  void AdjustTicksComputeRange(
    svtkAxisActor* axes[NUMBER_OF_ALIGNED_AXIS], double rangeMin, double rangeMax);

  void AutoScale(svtkViewport* viewport);
  void AutoScale(svtkViewport* viewport, svtkAxisActor* axes[NUMBER_OF_ALIGNED_AXIS]);
  double AutoScale(svtkViewport* viewport, double screenSize, double position[3]);
};

#endif
