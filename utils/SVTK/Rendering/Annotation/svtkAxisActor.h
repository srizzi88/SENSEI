/*=========================================================================
Program:   Visualization Toolkit
Module:    svtkAxisActor.h
Language:  C++

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
/**
 * @class   svtkAxisActor
 * @brief   Create an axis with tick marks and labels
 *
 * svtkAxisActor creates an axis with tick marks, labels, and/or a title,
 * depending on the particular instance variable settings. It is assumed that
 * the axes is part of a bounding box and is orthogonal to one of the
 * coordinate axes.  To use this class, you typically specify two points
 * defining the start and end points of the line (xyz definition using
 * svtkCoordinate class), the axis type (X, Y or Z), the axis location in
 * relation to the bounding box, the bounding box, the number of labels, and
 * the data range (min,max). You can also control what parts of the axis are
 * visible including the line, the tick marks, the labels, and the title. It
 * is also possible to control gridlines, and specify on which 'side' the
 * tickmarks are drawn (again with respect to the underlying assumed
 * bounding box). You can also specify the label format (a printf style format).
 *
 * This class decides how to locate the labels, and how to create reasonable
 * tick marks and labels.
 *
 * Labels follow the camera so as to be legible from any viewpoint.
 *
 * The instance variables Point1 and Point2 are instances of svtkCoordinate.
 * All calculations and references are in World Coordinates.
 *
 * @par Thanks:
 * This class was written by:
 * Hank Childs, Kathleen Bonnell, Amy Squillacote, Brad Whitlock,
 * Eric Brugger, Claire Guilbaud, Nicolas Dolegieviez, Will Schroeder,
 * Karthik Krishnan, Aashish Chaudhary, Philippe Pebay, David Gobbi,
 * David Partyka, Utkarsh Ayachit David Cole, Francois Bertel, and Mark Olesen
 * Part of this work was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France.
 *
 * @sa
 * svtkActor svtkVectorText svtkPolyDataMapper svtkAxisActor2D svtkCoordinate
 */

#ifndef svtkAxisActor_h
#define svtkAxisActor_h

#include "svtkActor.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkAxisFollower;
class svtkCamera;
class svtkCoordinate;
class svtkFollower;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkProp3DAxisFollower;
class svtkProperty2D;
class svtkStringArray;
class svtkTextActor;
class svtkTextActor3D;
class svtkTextProperty;
class svtkVectorText;

class SVTKRENDERINGANNOTATION_EXPORT svtkAxisActor : public svtkActor
{
public:
  svtkTypeMacro(svtkAxisActor, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object.
   */
  static svtkAxisActor* New();

  //@{
  /**
   * Specify the position of the first point defining the axis.
   */
  virtual svtkCoordinate* GetPoint1Coordinate();
  virtual void SetPoint1(double x[3]) { this->SetPoint1(x[0], x[1], x[2]); }
  virtual void SetPoint1(double x, double y, double z);
  virtual double* GetPoint1();
  //@}

  //@{
  /**
   * Specify the position of the second point defining the axis.
   */
  virtual svtkCoordinate* GetPoint2Coordinate();
  virtual void SetPoint2(double x[3]) { this->SetPoint2(x[0], x[1], x[2]); }
  virtual void SetPoint2(double x, double y, double z);
  virtual double* GetPoint2();
  //@}

  //@{
  /**
   * Specify the (min,max) axis range. This will be used in the generation
   * of labels, if labels are visible.
   */
  svtkSetVector2Macro(Range, double);
  svtkGetVectorMacro(Range, double, 2);
  //@}

  //@{
  /**
   * Set or get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  void SetBounds(const double bounds[6]);
  void SetBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  double* GetBounds(void) SVTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on the axis.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Render text as polygons (svtkVectorText) or as sprites (svtkTextActor3D).
   * In 2D mode, the value is ignored and text is rendered as svtkTextActor.
   * False(0) by default.
   * See Also:
   * GetUse2DMode(), SetUse2DMode
   */
  svtkSetMacro(UseTextActor3D, int);
  svtkGetMacro(UseTextActor3D, int);
  //@}

  //@{
  /**
   * Set/Get the flag that controls whether the minor ticks are visible.
   */
  svtkSetMacro(MinorTicksVisible, svtkTypeBool);
  svtkGetMacro(MinorTicksVisible, svtkTypeBool);
  svtkBooleanMacro(MinorTicksVisible, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the title of the axis actor,
   */
  void SetTitle(const char* t);
  svtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the common exponent of the labels values
   */
  void SetExponent(const char* t);
  svtkGetStringMacro(Exponent);
  //@}

  //@{
  /**
   * Set/Get the size of the major tick marks
   */
  svtkSetMacro(MajorTickSize, double);
  svtkGetMacro(MajorTickSize, double);
  //@}

  //@{
  /**
   * Set/Get the size of the major tick marks
   */
  svtkSetMacro(MinorTickSize, double);
  svtkGetMacro(MinorTickSize, double);
  //@}

  enum TickLocation
  {
    SVTK_TICKS_INSIDE = 0,
    SVTK_TICKS_OUTSIDE = 1,
    SVTK_TICKS_BOTH = 2
  };

  //@{
  /**
   * Set/Get the location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   */
  svtkSetClampMacro(TickLocation, int, SVTK_TICKS_INSIDE, SVTK_TICKS_BOTH);
  svtkGetMacro(TickLocation, int);
  //@}

  void SetTickLocationToInside(void) { this->SetTickLocation(SVTK_TICKS_INSIDE); }
  void SetTickLocationToOutside(void) { this->SetTickLocation(SVTK_TICKS_OUTSIDE); }
  void SetTickLocationToBoth(void) { this->SetTickLocation(SVTK_TICKS_BOTH); }

  //@{
  /**
   * Set/Get visibility of the axis line.
   */
  svtkSetMacro(AxisVisibility, svtkTypeBool);
  svtkGetMacro(AxisVisibility, svtkTypeBool);
  svtkBooleanMacro(AxisVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis major tick marks.
   */
  svtkSetMacro(TickVisibility, svtkTypeBool);
  svtkGetMacro(TickVisibility, svtkTypeBool);
  svtkBooleanMacro(TickVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis labels.
   */
  svtkSetMacro(LabelVisibility, svtkTypeBool);
  svtkGetMacro(LabelVisibility, svtkTypeBool);
  svtkBooleanMacro(LabelVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis title.
   */
  svtkSetMacro(TitleVisibility, svtkTypeBool);
  svtkGetMacro(TitleVisibility, svtkTypeBool);
  svtkBooleanMacro(TitleVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis detached exponent.
   */
  svtkSetMacro(ExponentVisibility, bool);
  svtkGetMacro(ExponentVisibility, bool);
  svtkBooleanMacro(ExponentVisibility, bool);
  //@}

  //@{
  /**
   * Set/Get visibility of the axis detached exponent.
   */
  svtkSetMacro(LastMajorTickPointCorrection, bool);
  svtkGetMacro(LastMajorTickPointCorrection, bool);
  svtkBooleanMacro(LastMajorTickPointCorrection, bool);
  //@}

  enum AlignLocation
  {
    SVTK_ALIGN_TOP = 0,
    SVTK_ALIGN_BOTTOM = 1,
    SVTK_ALIGN_POINT1 = 2,
    SVTK_ALIGN_POINT2 = 3
  };

  //@{
  /**
   * Get/Set the alignment of the title related to the axis.
   * Possible Alignment: SVTK_ALIGN_TOP, SVTK_ALIGN_BOTTOM, SVTK_ALIGN_POINT1, SVTK_ALIGN_POINT2
   */
  virtual void SetTitleAlignLocation(int location);
  svtkGetMacro(TitleAlignLocation, int);
  //@}

  //@{
  /**
   * Get/Set the location of the Detached Exponent related to the axis.
   * Possible Location: SVTK_ALIGN_TOP, SVTK_ALIGN_BOTTOM, SVTK_ALIGN_POINT1, SVTK_ALIGN_POINT2
   */
  virtual void SetExponentLocation(int location);
  svtkGetMacro(ExponentLocation, int);
  //@}

  //@{
  /**
   * Set/Get the axis title text property.
   */
  virtual void SetTitleTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TitleTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the axis labels text property.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(LabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Get/Set axis actor property (axis and its ticks) (kept for compatibility)
   */
  void SetAxisLinesProperty(svtkProperty*);
  svtkProperty* GetAxisLinesProperty();
  //@}

  //@{
  /**
   * Get/Set main line axis actor property
   */
  void SetAxisMainLineProperty(svtkProperty*);
  svtkProperty* GetAxisMainLineProperty();
  //@}

  //@{
  /**
   * Get/Set axis actor property (axis and its ticks)
   */
  void SetAxisMajorTicksProperty(svtkProperty*);
  svtkProperty* GetAxisMajorTicksProperty();
  //@}

  //@{
  /**
   * Get/Set axis actor property (axis and its ticks)
   */
  void SetAxisMinorTicksProperty(svtkProperty*);
  svtkProperty* GetAxisMinorTicksProperty();
  //@}

  //@{
  /**
   * Get/Set gridlines actor property (outer grid lines)
   */
  void SetGridlinesProperty(svtkProperty*);
  svtkProperty* GetGridlinesProperty();
  //@}

  //@{
  /**
   * Get/Set inner gridlines actor property
   */
  void SetInnerGridlinesProperty(svtkProperty*);
  svtkProperty* GetInnerGridlinesProperty();
  //@}

  //@{
  /**
   * Get/Set gridPolys actor property (grid quads)
   */
  void SetGridpolysProperty(svtkProperty*);
  svtkProperty* GetGridpolysProperty();
  //@}

  //@{
  /**
   * Set/Get whether gridlines should be drawn.
   */
  svtkSetMacro(DrawGridlines, svtkTypeBool);
  svtkGetMacro(DrawGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawGridlines, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether ONLY the gridlines should be drawn.
   * This will only draw GridLines and will skip any other part of the rendering
   * such as Axis/Tick/Title/...
   */
  svtkSetMacro(DrawGridlinesOnly, svtkTypeBool);
  svtkGetMacro(DrawGridlinesOnly, svtkTypeBool);
  svtkBooleanMacro(DrawGridlinesOnly, svtkTypeBool);
  //@}

  svtkSetMacro(DrawGridlinesLocation, int);
  svtkGetMacro(DrawGridlinesLocation, int);

  //@{
  /**
   * Set/Get whether inner gridlines should be drawn.
   */
  svtkSetMacro(DrawInnerGridlines, svtkTypeBool);
  svtkGetMacro(DrawInnerGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawInnerGridlines, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the length to use when drawing gridlines.
   */
  svtkSetMacro(GridlineXLength, double);
  svtkGetMacro(GridlineXLength, double);
  svtkSetMacro(GridlineYLength, double);
  svtkGetMacro(GridlineYLength, double);
  svtkSetMacro(GridlineZLength, double);
  svtkGetMacro(GridlineZLength, double);
  //@}

  //@{
  /**
   * Set/Get whether gridpolys should be drawn.
   */
  svtkSetMacro(DrawGridpolys, svtkTypeBool);
  svtkGetMacro(DrawGridpolys, svtkTypeBool);
  svtkBooleanMacro(DrawGridpolys, svtkTypeBool);
  //@}

  enum AxisType
  {
    SVTK_AXIS_TYPE_X = 0,
    SVTK_AXIS_TYPE_Y = 1,
    SVTK_AXIS_TYPE_Z = 2
  };

  //@{
  /**
   * Set/Get the type of this axis.
   */
  svtkSetClampMacro(AxisType, int, SVTK_AXIS_TYPE_X, SVTK_AXIS_TYPE_Z);
  svtkGetMacro(AxisType, int);
  void SetAxisTypeToX(void) { this->SetAxisType(SVTK_AXIS_TYPE_X); }
  void SetAxisTypeToY(void) { this->SetAxisType(SVTK_AXIS_TYPE_Y); }
  void SetAxisTypeToZ(void) { this->SetAxisType(SVTK_AXIS_TYPE_Z); }
  //@}

  enum AxisPosition
  {
    SVTK_AXIS_POS_MINMIN = 0,
    SVTK_AXIS_POS_MINMAX = 1,
    SVTK_AXIS_POS_MAXMAX = 2,
    SVTK_AXIS_POS_MAXMIN = 3
  };

  //@{
  /**
   * Set/Get The type of scale, enable logarithmic scale or linear by default
   */
  svtkSetMacro(Log, bool);
  svtkGetMacro(Log, bool);
  svtkBooleanMacro(Log, bool);
  //@}

  //@{
  /**
   * Set/Get the position of this axis (in relation to an an
   * assumed bounding box).  For an x-type axis, MINMIN corresponds
   * to the x-edge in the bounding box where Y values are minimum and
   * Z values are minimum. For a y-type axis, MAXMIN corresponds to the
   * y-edge where X values are maximum and Z values are minimum.
   */
  svtkSetClampMacro(AxisPosition, int, SVTK_AXIS_POS_MINMIN, SVTK_AXIS_POS_MAXMIN);
  svtkGetMacro(AxisPosition, int);
  //@}

  void SetAxisPositionToMinMin(void) { this->SetAxisPosition(SVTK_AXIS_POS_MINMIN); }
  void SetAxisPositionToMinMax(void) { this->SetAxisPosition(SVTK_AXIS_POS_MINMAX); }
  void SetAxisPositionToMaxMax(void) { this->SetAxisPosition(SVTK_AXIS_POS_MAXMAX); }
  void SetAxisPositionToMaxMin(void) { this->SetAxisPosition(SVTK_AXIS_POS_MAXMIN); }

  //@{
  /**
   * Set/Get the camera for this axis.  The camera is used by the
   * labels to 'follow' the camera and be legible from any viewpoint.
   */
  virtual void SetCamera(svtkCamera*);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  //@{
  /**
   * Draw the axis.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  virtual int RenderTranslucentGeometry(svtkViewport* viewport);
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  int RenderOverlay(svtkViewport* viewport) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  double ComputeMaxLabelLength(const double[3]);
  double ComputeTitleLength(const double[3]);

  void SetLabelScale(const double scale);
  void SetLabelScale(int labelIndex, const double scale);
  void SetTitleScale(const double scale);

  //@{
  /**
   * Set/Get the starting position for minor and major tick points,
   * and the delta values that determine their spacing.
   */
  svtkSetMacro(MinorStart, double);
  svtkGetMacro(MinorStart, double);
  double GetMajorStart(int axis);
  void SetMajorStart(int axis, double value);
  // svtkSetMacro(MajorStart, double);
  // svtkGetMacro(MajorStart, double);
  svtkSetMacro(DeltaMinor, double);
  svtkGetMacro(DeltaMinor, double);
  double GetDeltaMajor(int axis);
  void SetDeltaMajor(int axis, double value);
  // svtkSetMacro(DeltaMajor, double);
  // svtkGetMacro(DeltaMajor, double);
  //@}

  //@{
  /**
   * Set/Get the starting position for minor and major tick points on
   * the range and the delta values that determine their spacing. The
   * range and the position need not be identical. ie the displayed
   * values need not match the actual positions in 3D space.
   */
  svtkSetMacro(MinorRangeStart, double);
  svtkGetMacro(MinorRangeStart, double);
  svtkSetMacro(MajorRangeStart, double);
  svtkGetMacro(MajorRangeStart, double);
  svtkSetMacro(DeltaRangeMinor, double);
  svtkGetMacro(DeltaRangeMinor, double);
  svtkSetMacro(DeltaRangeMajor, double);
  svtkGetMacro(DeltaRangeMajor, double);
  //@}

  void SetLabels(svtkStringArray* labels);

  void BuildAxis(svtkViewport* viewport, bool);

  //@{
  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  svtkGetObjectMacro(TitleActor, svtkAxisFollower);
  //@}

  //@{
  /**
   * Get exponent follower actor
   */
  svtkGetObjectMacro(ExponentActor, svtkAxisFollower);
  //@}

  /**
   * Get label actors responsigle for drawing label text.
   */
  inline svtkAxisFollower** GetLabelActors() { return this->LabelActors; }

  //@{
  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  svtkGetObjectMacro(TitleProp3D, svtkProp3DAxisFollower);
  //@}

  /**
   * Get label actors responsigle for drawing label text.
   */
  inline svtkProp3DAxisFollower** GetLabelProps3D() { return this->LabelProps3D; }

  //@{
  /**
   * Get title actor and it is responsible for drawing
   * title text.
   */
  svtkGetObjectMacro(ExponentProp3D, svtkProp3DAxisFollower);
  //@}

  //@{
  /**
   * Get total number of labels built. Once built
   * this count does not change.
   */
  svtkGetMacro(NumberOfLabelsBuilt, int);
  //@}

  //@{
  /**
   * Set/Get flag whether to calculate title offset.
   * Default is true.
   */
  svtkSetMacro(CalculateTitleOffset, svtkTypeBool);
  svtkGetMacro(CalculateTitleOffset, svtkTypeBool);
  svtkBooleanMacro(CalculateTitleOffset, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get flag whether to calculate label offset.
   * Default is true.
   */
  svtkSetMacro(CalculateLabelOffset, svtkTypeBool);
  svtkGetMacro(CalculateLabelOffset, svtkTypeBool);
  svtkBooleanMacro(CalculateLabelOffset, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the 2D mode
   */
  svtkSetMacro(Use2DMode, int);
  svtkGetMacro(Use2DMode, int);
  //@}

  //@{
  /**
   * Set/Get the 2D mode the vertical offset for X title in 2D mode
   */
  svtkSetMacro(VerticalOffsetXTitle2D, double);
  svtkGetMacro(VerticalOffsetXTitle2D, double);
  //@}

  //@{
  /**
   * Set/Get the 2D mode the horizontal offset for Y title in 2D mode
   */
  svtkSetMacro(HorizontalOffsetYTitle2D, double);
  svtkGetMacro(HorizontalOffsetYTitle2D, double);
  //@}

  //@{
  /**
   * Set/Get whether title position must be saved in 2D mode
   */
  svtkSetMacro(SaveTitlePosition, int);
  svtkGetMacro(SaveTitlePosition, int);
  //@}

  //@{
  /**
   * Provide real vector for non aligned axis
   */
  svtkSetVector3Macro(AxisBaseForX, double);
  svtkGetVector3Macro(AxisBaseForX, double);
  //@}

  //@{
  /**
   * Provide real vector for non aligned axis
   */
  svtkSetVector3Macro(AxisBaseForY, double);
  svtkGetVector3Macro(AxisBaseForY, double);
  //@}

  //@{
  /**
   * Provide real vector for non aligned axis
   */
  svtkSetVector3Macro(AxisBaseForZ, double);
  svtkGetVector3Macro(AxisBaseForZ, double);
  //@}

  //@{
  /**
   * Notify the axes that is not part of a cube anymore
   */
  svtkSetMacro(AxisOnOrigin, int);
  svtkGetMacro(AxisOnOrigin, int);
  //@}

  //@{
  /**
   * Set/Get the offsets used to position texts.
   */
  svtkSetMacro(LabelOffset, double);
  svtkGetMacro(LabelOffset, double);
  svtkSetMacro(TitleOffset, double);
  svtkGetMacro(TitleOffset, double);
  svtkSetMacro(ExponentOffset, double);
  svtkGetMacro(ExponentOffset, double);
  svtkSetMacro(ScreenSize, double);
  svtkGetMacro(ScreenSize, double);
  //@}

protected:
  svtkAxisActor();
  ~svtkAxisActor() override;

  char* Title;
  char* Exponent;
  double Range[2];
  double LastRange[2];
  char* LabelFormat;
  int UseTextActor3D;
  int NumberOfLabelsBuilt;
  svtkTypeBool MinorTicksVisible;
  int LastMinorTicksVisible;

  /**
   * The location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   */
  int TickLocation;

  /**
   * Hold the alignment property of the title related to the axis.
   * Possible Alignment: SVTK_ALIGN_BOTTOM, SVTK_ALIGN_TOP, SVTK_ALIGN_POINT1, SVTK_ALIGN_POINT2.
   */
  int TitleAlignLocation;

  /**
   * Hold the alignment property of the exponent coming from the label values.
   * Possible Alignment: SVTK_ALIGN_BOTTOM, SVTK_ALIGN_TOP, SVTK_ALIGN_POINT1, SVTK_ALIGN_POINT2.
   */
  int ExponentLocation;

  svtkTypeBool DrawGridlines;
  svtkTypeBool DrawGridlinesOnly;
  int LastDrawGridlines;
  int DrawGridlinesLocation;     // 0: all | 1: closest | 2: farest
  int LastDrawGridlinesLocation; // 0: all | 1: closest | 2: farest
  double GridlineXLength;
  double GridlineYLength;
  double GridlineZLength;

  svtkTypeBool DrawInnerGridlines;
  int LastDrawInnerGridlines;

  svtkTypeBool DrawGridpolys;
  int LastDrawGridpolys;

  svtkTypeBool AxisVisibility;
  svtkTypeBool TickVisibility;
  int LastTickVisibility;
  svtkTypeBool LabelVisibility;
  svtkTypeBool TitleVisibility;
  bool ExponentVisibility;
  bool LastMajorTickPointCorrection;

  bool Log;
  int AxisType;
  int AxisPosition;

  // coordinate system for axisAxtor, relative to world coordinates
  double AxisBaseForX[3];
  double AxisBaseForY[3];
  double AxisBaseForZ[3];

private:
  svtkAxisActor(const svtkAxisActor&) = delete;
  void operator=(const svtkAxisActor&) = delete;

  void TransformBounds(svtkViewport*, double bnds[6]);

  void BuildLabels(svtkViewport*, bool);
  void BuildLabels2D(svtkViewport*, bool);
  void SetLabelPositions(svtkViewport*, bool);
  void SetLabelPositions2D(svtkViewport*, bool);

  /**
   * Set orientation of the actor 2D (follower) to keep the axis orientation and stay on the right
   * size
   */
  void RotateActor2DFromAxisProjection(svtkTextActor* pActor2D);

  /**
   * Init the geometry of the title. (no positioning or orientation)
   */
  void InitTitle();

  /**
   * Init the geometry of the common exponent of the labels values. (no positioning or orientation)
   */
  void InitExponent();

  /**
   * This methdod set the text and set the base position of the follower from the axis
   * The position will be modified in svtkAxisFollower::Render() sub-functions according to the
   * camera position
   * for convenience purpose.
   */
  void BuildTitle(bool);

  /**
   * Build the actor to display the exponent in case it should appear next to the title or next to
   * p2 coordinate.
   */
  void BuildExponent(bool force);

  void BuildExponent2D(svtkViewport* viewport, bool force);

  void BuildTitle2D(svtkViewport* viewport, bool);

  void SetAxisPointsAndLines(void);

  bool BuildTickPoints(double p1[3], double p2[3], bool force);

  // Build major ticks for linear scale.
  void BuildMajorTicks(double p1[3], double p2[3], double localCoordSys[3][3]);

  // Build major ticks for logarithmic scale.
  void BuildMajorTicksLog(double p1[3], double p2[3], double localCoordSys[3][3]);

  // Build minor ticks for linear scale.
  void BuildMinorTicks(double p1[3], double p2[3], double localCoordSys[3][3]);

  // Build minor ticks for logarithmic scale enabled
  void BuildMinorTicksLog(double p1[3], double p2[3], double localCoordSys[3][3]);

  void BuildAxisGridLines(double p1[3], double p2[3], double localCoordSys[3][3]);

  bool TickVisibilityChanged(void);
  svtkProperty* NewTitleProperty();
  svtkProperty2D* NewTitleProperty2D();
  svtkProperty* NewLabelProperty();

  bool BoundsDisplayCoordinateChanged(svtkViewport* viewport);

  svtkCoordinate* Point1Coordinate;
  svtkCoordinate* Point2Coordinate;

  double MajorTickSize;
  double MinorTickSize;

  // For each axis (for the inner gridline generation)
  double MajorStart[3];
  double DeltaMajor[3];
  double MinorStart;
  double DeltaMinor;

  // For the ticks, w.r.t to the set range
  double MajorRangeStart;
  double MinorRangeStart;

  /**
   * step between 2 minor ticks, in range value (values displayed on the axis)
   */
  double DeltaRangeMinor;

  /**
   * step between 2 major ticks, in range value (values displayed on the axis)
   */
  double DeltaRangeMajor;

  int LastAxisPosition;
  int LastAxisType;
  int LastTickLocation;
  double LastLabelStart;

  svtkPoints* MinorTickPts;
  svtkPoints* MajorTickPts;
  svtkPoints* GridlinePts;
  svtkPoints* InnerGridlinePts;
  svtkPoints* GridpolyPts;

  svtkVectorText* TitleVector;
  svtkPolyDataMapper* TitleMapper;
  svtkAxisFollower* TitleActor;
  svtkTextActor* TitleActor2D;
  svtkProp3DAxisFollower* TitleProp3D;
  svtkTextActor3D* TitleActor3D;
  svtkTextProperty* TitleTextProperty;

  //@{
  /**
   * Mapper/Actor used to display a common exponent of the label values
   */
  svtkVectorText* ExponentVector;
  svtkPolyDataMapper* ExponentMapper;
  svtkAxisFollower* ExponentActor;
  svtkTextActor* ExponentActor2D;
  svtkProp3DAxisFollower* ExponentProp3D;
  svtkTextActor3D* ExponentActor3D;
  //@}

  svtkVectorText** LabelVectors;
  svtkPolyDataMapper** LabelMappers;
  svtkAxisFollower** LabelActors;
  svtkProp3DAxisFollower** LabelProps3D;
  svtkTextActor** LabelActors2D;
  svtkTextActor3D** LabelActors3D;
  svtkTextProperty* LabelTextProperty;

  // Main line axis
  svtkPolyData* AxisLines;
  svtkPolyDataMapper* AxisLinesMapper;
  svtkActor* AxisLinesActor;

  // Ticks of the axis
  svtkPolyData *AxisMajorTicks, *AxisMinorTicks;
  svtkPolyDataMapper *AxisMajorTicksMapper, *AxisMinorTicksMapper;
  svtkActor *AxisMajorTicksActor, *AxisMinorTicksActor;

  svtkPolyData* Gridlines;
  svtkPolyDataMapper* GridlinesMapper;
  svtkActor* GridlinesActor;
  svtkPolyData* InnerGridlines;
  svtkPolyDataMapper* InnerGridlinesMapper;
  svtkActor* InnerGridlinesActor;
  svtkPolyData* Gridpolys;
  svtkPolyDataMapper* GridpolysMapper;
  svtkActor* GridpolysActor;

  svtkCamera* Camera;
  svtkTimeStamp BuildTime;
  svtkTimeStamp BuildTickPointsTime;
  svtkTimeStamp BoundsTime;
  svtkTimeStamp LabelBuildTime;
  svtkTimeStamp TitleTextTime;
  svtkTimeStamp ExponentTextTime;

  int AxisOnOrigin;

  int AxisHasZeroLength;

  svtkTypeBool CalculateTitleOffset;
  svtkTypeBool CalculateLabelOffset;

  /**
   * Use xy-axis only when Use2DMode=1:
   */
  int Use2DMode;

  /**
   * Vertical offset in display coordinates for X axis title (used in 2D mode only)
   * Default: -40
   */
  double VerticalOffsetXTitle2D;

  /**
   * Vertical offset in display coordinates for X axis title (used in 2D mode only)
   * Default: -50
   */
  double HorizontalOffsetYTitle2D;

  /**
   * Save title position (used in 2D mode only):
   * val = 0 : no need to save position (doesn't stick actors in a position)
   * val = 1 : positions have to be saved during the next render pass
   * val = 2 : positions are saved; use them
   */
  int SaveTitlePosition;

  /**
   * Constant position for the title (used in 2D mode only)
   */
  double TitleConstantPosition[2];

  /**
   * True if the 2D title has to be built, false otherwise
   */
  bool NeedBuild2D;

  double LastMinDisplayCoordinate[3];
  double LastMaxDisplayCoordinate[3];
  double TickVector[3];

  //@{
  /**
   * Offsets used to position text.
   */
  double ScreenSize;
  double LabelOffset;
  double TitleOffset;
  double ExponentOffset;
};
//@}

#endif
