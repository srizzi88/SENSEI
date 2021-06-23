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
 * @class   svtkPolarAxesActor
 * @brief   create an actor of a polar axes -
 *
 *
 * svtkPolarAxesActor is a composite actor that draws polar axes in a
 * specified plane for a give pole.
 * Currently the plane has to be the xy plane.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2011.
 * This work was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France.
 *
 * @sa
 * svtkActor svtkAxisActor svtkPolarAxesActor
 */

#ifndef svtkPolarAxesActor_h
#define svtkPolarAxesActor_h

#define SVTK_MAXIMUM_NUMBER_OF_RADIAL_AXES 50
#define SVTK_DEFAULT_NUMBER_OF_RADIAL_AXES 5
#define SVTK_MAXIMUM_NUMBER_OF_POLAR_AXIS_TICKS 200
#define SVTK_MAXIMUM_RATIO 1000.0
#define SVTK_POLAR_ARC_RESOLUTION_PER_DEG 0.2

#include "svtkActor.h"
#include "svtkAxisActor.h"                 // access to enum values
#include "svtkRenderingAnnotationModule.h" // For export macro
#include <list>                           // To process exponent list as reference
#include <string>                         // used for ivar

class svtkCamera;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkProperty;
class svtkStringArray;
class svtkTextProperty;

class SVTKRENDERINGANNOTATION_EXPORT svtkPolarAxesActor : public svtkActor
{
public:
  svtkTypeMacro(svtkPolarAxesActor, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with label format "6.3g" and the number of labels
   * per axis set to 3.
   */
  static svtkPolarAxesActor* New();

  //@{
  /**
   * Draw the polar axes
   */
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  //@}

  //@{
  /**
   * Explicitly specify the coordinate of the pole.
   */
  virtual void SetPole(double[3]);
  virtual void SetPole(double, double, double);
  svtkGetVector3Macro(Pole, double);
  //@}

  //@{
  /**
   * Enable/Disable log scale
   * Default: false
   */
  svtkSetMacro(Log, bool);
  svtkGetMacro(Log, bool);
  svtkBooleanMacro(Log, bool);
  //@}

  //@{
  /**
   * Gets/Sets the number of radial axes
   */
  svtkSetClampMacro(RequestedNumberOfRadialAxes, svtkIdType, 0, SVTK_MAXIMUM_NUMBER_OF_RADIAL_AXES);
  svtkGetMacro(RequestedNumberOfRadialAxes, svtkIdType);
  //@}

  //@{
  /**
   * Set/Get a number of ticks that one would like to display along polar axis
   * NB: it modifies DeltaRangeMajor to correspond to this number
   */
  virtual void SetNumberOfPolarAxisTicks(int);
  int GetNumberOfPolarAxisTicks();
  //@}

  //@{
  /**
   * Set/Get whether the number of polar axis ticks and arcs should be automatically calculated
   * Default: true
   */
  svtkSetMacro(AutoSubdividePolarAxis, bool);
  svtkGetMacro(AutoSubdividePolarAxis, bool);
  svtkBooleanMacro(AutoSubdividePolarAxis, bool);
  //@}

  //@{
  /**
   * Define the range values displayed on the polar Axis.
   */
  svtkSetVector2Macro(Range, double);
  svtkGetVectorMacro(Range, double, 2);
  //@}

  //@{
  /**
   * Set/Get the minimal radius of the polar coordinates.
   */
  virtual void SetMinimumRadius(double);
  svtkGetMacro(MinimumRadius, double);
  //@}

  //@{
  /**
   * Set/Get the maximum radius of the polar coordinates.
   */
  virtual void SetMaximumRadius(double);
  svtkGetMacro(MaximumRadius, double);
  //@}

  //@{
  /**
   * Set/Get the minimum radius of the polar coordinates (in degrees).
   */
  virtual void SetMinimumAngle(double);
  svtkGetMacro(MinimumAngle, double);
  //@}

  //@{
  /**
   * Set/Get the maximum radius of the polar coordinates (in degrees).
   */
  virtual void SetMaximumAngle(double);
  svtkGetMacro(MaximumAngle, double);
  //@}

  //@{
  /**
   * Set/Get the minimum radial angle distinguishable from polar axis
   * NB: This is used only when polar axis is visible
   * Default: 0.5
   */
  svtkSetClampMacro(SmallestVisiblePolarAngle, double, 0., 5.);
  svtkGetMacro(SmallestVisiblePolarAngle, double);
  //@}

  //@{
  /**
   * Set/Get the location of the ticks.
   * Inside: tick end toward positive direction of perpendicular axes.
   * Outside: tick end toward negative direction of perpendicular axes.
   */
  svtkSetClampMacro(TickLocation, int, svtkAxisActor::SVTK_TICKS_INSIDE, svtkAxisActor::SVTK_TICKS_BOTH);
  svtkGetMacro(TickLocation, int);
  //@}

  //@{
  /**
   * Default: true
   */
  svtkSetMacro(RadialUnits, bool);
  svtkGetMacro(RadialUnits, bool);
  //@}

  //@{
  /**
   * Explicitly specify the screen size of title and label text.
   * ScreenSize determines the size of the text in terms of screen
   * pixels.
   * Default: 10.0.
   */
  svtkSetMacro(ScreenSize, double);
  svtkGetMacro(ScreenSize, double);
  //@}

  //@{
  /**
   * Set/Get the camera to perform scaling and translation of the
   * svtkPolarAxesActor.
   */
  virtual void SetCamera(svtkCamera*);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  //@{
  /**
   * Set/Get the labels for the polar axis.
   * Default: "Radial Distance".
   */
  svtkSetStringMacro(PolarAxisTitle);
  svtkGetStringMacro(PolarAxisTitle);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the polar axis labels.
   */
  svtkSetStringMacro(PolarLabelFormat);
  svtkGetStringMacro(PolarLabelFormat);
  //@}

  enum ExponentLocation
  {
    SVTK_EXPONENT_BOTTOM = 0,
    SVTK_EXPONENT_EXTERN = 1,
    SVTK_EXPONENT_LABELS = 2
  };

  //@{
  /**
   * Get/Set the location of the exponent (if any) of the polar axis values.
   * Possible location: SVTK_EXPONENT_BOTTOM, SVTK_EXPONENT_EXTERN,
   * SVTK_EXPONENT_LABELS
   */
  svtkSetClampMacro(ExponentLocation, int, SVTK_EXPONENT_BOTTOM, SVTK_EXPONENT_LABELS);
  svtkGetMacro(ExponentLocation, int);
  //@}

  //@{
  /**
   * String to format angle values displayed on the radial axes.
   */
  svtkSetStringMacro(RadialAngleFormat);
  svtkGetStringMacro(RadialAngleFormat);
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
   * Turn on and off the visibility of the polar axis.
   */
  svtkSetMacro(PolarAxisVisibility, svtkTypeBool);
  svtkGetMacro(PolarAxisVisibility, svtkTypeBool);
  svtkBooleanMacro(PolarAxisVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of inner radial grid lines
   */
  svtkSetMacro(DrawRadialGridlines, svtkTypeBool);
  svtkGetMacro(DrawRadialGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawRadialGridlines, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of inner polar arcs grid lines
   */
  svtkSetMacro(DrawPolarArcsGridlines, svtkTypeBool);
  svtkGetMacro(DrawPolarArcsGridlines, svtkTypeBool);
  svtkBooleanMacro(DrawPolarArcsGridlines, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of titles for polar axis.
   */
  svtkSetMacro(PolarTitleVisibility, svtkTypeBool);
  svtkGetMacro(PolarTitleVisibility, svtkTypeBool);
  svtkBooleanMacro(PolarTitleVisibility, svtkTypeBool);
  //@}

  enum TitleLocation
  {
    SVTK_TITLE_BOTTOM = 0,
    SVTK_TITLE_EXTERN = 1
  };

  //@{
  /**
   * Get/Set the alignment of the radial axes title related to the axis.
   * Possible Alignment: SVTK_TITLE_BOTTOM, SVTK_TITLE_EXTERN
   */
  svtkSetClampMacro(RadialAxisTitleLocation, int, SVTK_TITLE_BOTTOM, SVTK_TITLE_EXTERN);
  svtkGetMacro(RadialAxisTitleLocation, int);
  //@}

  //@{
  /**
   * Get/Set the alignment of the polar axes title related to the axis.
   * Possible Alignment: SVTKTITLE_BOTTOM, SVTK_TITLE_EXTERN
   */
  svtkSetClampMacro(PolarAxisTitleLocation, int, SVTK_TITLE_BOTTOM, SVTK_TITLE_EXTERN);
  svtkGetMacro(PolarAxisTitleLocation, int);
  //@}

  //@{
  /**
   * Turn on and off the visibility of labels for polar axis.
   */
  svtkSetMacro(PolarLabelVisibility, svtkTypeBool);
  svtkGetMacro(PolarLabelVisibility, svtkTypeBool);
  svtkBooleanMacro(PolarLabelVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * If On, the ticks are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngle Major/Minor. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  svtkSetMacro(ArcTicksOriginToPolarAxis, svtkTypeBool);
  svtkGetMacro(ArcTicksOriginToPolarAxis, svtkTypeBool);
  svtkBooleanMacro(ArcTicksOriginToPolarAxis, svtkTypeBool);
  //@}

  //@{
  /**
   * If On, the radial axes are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngleRadialAxes. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  svtkSetMacro(RadialAxesOriginToPolarAxis, svtkTypeBool);
  svtkGetMacro(RadialAxesOriginToPolarAxis, svtkTypeBool);
  svtkBooleanMacro(RadialAxesOriginToPolarAxis, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the overall visibility of ticks.
   */
  svtkSetMacro(PolarTickVisibility, svtkTypeBool);
  svtkGetMacro(PolarTickVisibility, svtkTypeBool);
  svtkBooleanMacro(PolarTickVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of major ticks on polar axis and last radial axis.
   */
  svtkSetMacro(AxisTickVisibility, svtkTypeBool);
  svtkGetMacro(AxisTickVisibility, svtkTypeBool);
  svtkBooleanMacro(AxisTickVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of minor ticks on polar axis and last radial axis.
   */
  svtkSetMacro(AxisMinorTickVisibility, svtkTypeBool);
  svtkGetMacro(AxisMinorTickVisibility, svtkTypeBool);
  svtkBooleanMacro(AxisMinorTickVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of major ticks on the last arc.
   */
  svtkSetMacro(ArcTickVisibility, svtkTypeBool);
  svtkGetMacro(ArcTickVisibility, svtkTypeBool);
  svtkBooleanMacro(ArcTickVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of minor ticks on the last arc.
   */
  svtkSetMacro(ArcMinorTickVisibility, svtkTypeBool);
  svtkGetMacro(ArcMinorTickVisibility, svtkTypeBool);
  svtkBooleanMacro(ArcMinorTickVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the size of the major ticks on the last arc.
   */
  svtkSetMacro(ArcMajorTickSize, double);
  svtkGetMacro(ArcMajorTickSize, double);
  //@}

  //@{
  /**
   * Set/Get the size of the major ticks on the polar axis.
   */
  svtkSetMacro(PolarAxisMajorTickSize, double);
  svtkGetMacro(PolarAxisMajorTickSize, double);
  //@}

  //@{
  /**
   * Set/Get the size of the major ticks on the last radial axis.
   */
  svtkSetMacro(LastRadialAxisMajorTickSize, double);
  svtkGetMacro(LastRadialAxisMajorTickSize, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Polar Axis ticks size
   */
  svtkSetMacro(PolarAxisTickRatioSize, double);
  svtkGetMacro(PolarAxisTickRatioSize, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Last Radial axis ticks size
   */
  svtkSetMacro(LastAxisTickRatioSize, double);
  svtkGetMacro(LastAxisTickRatioSize, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Arc ticks size
   */
  svtkSetMacro(ArcTickRatioSize, double);
  svtkGetMacro(ArcTickRatioSize, double);
  //@}

  //@{
  /**
   * Set/Get the size of the thickness of polar axis ticks
   */
  svtkSetMacro(PolarAxisMajorTickThickness, double);
  svtkGetMacro(PolarAxisMajorTickThickness, double);
  //@}

  //@{
  /**
   * Set/Get the size of the thickness of last radial axis ticks
   */
  svtkSetMacro(LastRadialAxisMajorTickThickness, double);
  svtkGetMacro(LastRadialAxisMajorTickThickness, double);
  //@}

  //@{
  /**
   * Set/Get the size of the thickness of the last arc ticks
   */
  svtkSetMacro(ArcMajorTickThickness, double);
  svtkGetMacro(ArcMajorTickThickness, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Polar Axis ticks thickness
   */
  svtkSetMacro(PolarAxisTickRatioThickness, double);
  svtkGetMacro(PolarAxisTickRatioThickness, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Last Radial axis ticks thickness
   */
  svtkSetMacro(LastAxisTickRatioThickness, double);
  svtkGetMacro(LastAxisTickRatioThickness, double);
  //@}

  //@{
  /**
   * Set/Get the ratio between major and minor Arc ticks thickness
   */
  svtkSetMacro(ArcTickRatioThickness, double);
  svtkGetMacro(ArcTickRatioThickness, double);
  //@}

  //@{
  /**
   * Set/Get the step between 2 major ticks, in range value (values displayed on the axis).
   */
  svtkSetMacro(DeltaRangeMajor, double);
  svtkGetMacro(DeltaRangeMajor, double);
  //@}

  //@{
  /**
   * Set/Get the step between 2 minor ticks, in range value (values displayed on the axis).
   */
  svtkSetMacro(DeltaRangeMinor, double);
  svtkGetMacro(DeltaRangeMinor, double);
  //@}

  //@{
  /**
   * Set/Get the angle between 2 major ticks on the last arc.
   */
  svtkSetMacro(DeltaAngleMajor, double);
  svtkGetMacro(DeltaAngleMajor, double);
  //@}

  //@{
  /**
   * Set/Get the angle between 2 minor ticks on the last arc.
   */
  svtkSetMacro(DeltaAngleMinor, double);
  svtkGetMacro(DeltaAngleMinor, double);
  //@}

  //@{
  /**
   * Set/Get the angle between 2 radial axes.
   */
  svtkSetMacro(DeltaAngleRadialAxes, double);
  svtkGetMacro(DeltaAngleRadialAxes, double);
  //@}

  //------------------------------------------------

  //@{
  /**
   * Turn on and off the visibility of non-polar radial axes.
   */
  svtkSetMacro(RadialAxesVisibility, svtkTypeBool);
  svtkGetMacro(RadialAxesVisibility, svtkTypeBool);
  svtkBooleanMacro(RadialAxesVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of titles for non-polar radial axes.
   */
  svtkSetMacro(RadialTitleVisibility, svtkTypeBool);
  svtkGetMacro(RadialTitleVisibility, svtkTypeBool);
  svtkBooleanMacro(RadialTitleVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on and off the visibility of arcs for polar axis.
   */
  svtkSetMacro(PolarArcsVisibility, svtkTypeBool);
  svtkGetMacro(PolarArcsVisibility, svtkTypeBool);
  svtkBooleanMacro(PolarArcsVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/Disable labels 2D mode (always facing the camera).
   */
  void SetUse2DMode(int val);
  int GetUse2DMode();
  //@}

  //@{
  /**
   * Set/Get the polar axis title text property.
   */
  virtual void SetPolarAxisTitleTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(PolarAxisTitleTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the polar axis labels text property.
   */
  virtual void SetPolarAxisLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(PolarAxisLabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the last radial axis text property.
   */
  virtual void SetLastRadialAxisTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(LastRadialAxisTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the secondary radial axes text property.
   */
  virtual void SetSecondaryRadialAxesTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(SecondaryRadialAxesTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Get/Set polar axis actor properties.
   */
  virtual void SetPolarAxisProperty(svtkProperty*);
  svtkGetObjectMacro(PolarAxisProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get/Set last radial axis actor properties.
   */
  virtual void SetLastRadialAxisProperty(svtkProperty* p);
  svtkGetObjectMacro(LastRadialAxisProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get/Set secondary radial axes actors properties.
   */
  virtual void SetSecondaryRadialAxesProperty(svtkProperty* p);
  svtkGetObjectMacro(SecondaryRadialAxesProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get/Set principal polar arc actor property.
   */
  virtual void SetPolarArcsProperty(svtkProperty* p);
  svtkProperty* GetPolarArcsProperty();
  //@}

  //@{
  /**
   * Get/Set secondary polar arcs actors property.
   */
  virtual void SetSecondaryPolarArcsProperty(svtkProperty* p);
  svtkProperty* GetSecondaryPolarArcsProperty();
  //@}

  //@{
  /**
   * Explicitly specify the region in space around which to draw the bounds.
   * The bounds are used only when no Input or Prop is specified. The bounds
   * are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
   * sure that the min's are less than the max's.
   */
  svtkSetVector6Macro(Bounds, double);
  double* GetBounds() override;
  void GetBounds(
    double& xmin, double& xmax, double& ymin, double& ymax, double& zmin, double& zmax);
  void GetBounds(double bounds[6]);
  //@}

  //@{
  /**
   * Ratio
   */
  svtkSetClampMacro(Ratio, double, 0.001, 100.0);
  svtkGetMacro(Ratio, double);
  //@}

protected:
  svtkPolarAxesActor();
  ~svtkPolarAxesActor() override;

  /**
   * Check consistency of svtkPolarAxesActor members.
   */
  bool CheckMembersConsistency();

  /**
   * Build the axes.
   * Determine coordinates, position, etc.
   */
  void BuildAxes(svtkViewport*);

  /**
   * Calculate bounds based on maximum radius and angular sector
   */
  void CalculateBounds();

  /**
   * Send attributes which are common to all axes, both polar and radial
   */
  void SetCommonAxisAttributes(svtkAxisActor*);

  /**
   * Set properties specific to PolarAxis
   */
  void SetPolarAxisAttributes(svtkAxisActor*);

  /**
   * Create requested number of type X axes.
   */
  void CreateRadialAxes(int axisCount);

  /**
   * Build requested number of radial axes with respect to specified pole.
   */
  void BuildRadialAxes();

  /**
   * Set Range and PolarAxis members value to build axis ticks
   * this function doesn't actually build PolarAxis ticks, it set the DeltaRangeMajor and DeltaMajor
   * attributes
   * then PolarAxis itself is in charge of ticks drawing
   */
  void AutoComputeTicksProperties();

  /**
   * return a step attempting to be as rounded as possible according to input parameters
   */
  double ComputeIdealStep(int subDivsRequired, double rangeLength, int maxSubDivs = 1000);

  /**
   * Build Arc ticks
   */
  void BuildArcTicks();

  /**
   * Init tick point located on an ellipse at angleEllipseRad angle and according to "a" major
   * radius
   */
  void StoreTicksPtsFromParamEllipse(
    double a, double angleEllipseRad, double tickSize, svtkPoints* tickPts);

  /**
   * Build polar axis labels and arcs with respect to specified pole.
   */
  void BuildPolarAxisLabelsArcs();

  /**
   * Build labels and arcs with log scale axis
   */
  void BuildPolarAxisLabelsArcsLog();

  /**
   * Define label values
   */
  void BuildLabelsLog();

  void BuildPolarArcsLog();

  /**
   * Find a common exponent for label values.
   */
  std::string FindExponentAndAdjustValues(std::list<double>& valuesList);

  /**
   * Yield a string array with the float part of each values. 0.01e-2 -> 0.0001
   */
  void GetSignificantPartFromValues(svtkStringArray* valuesStr, std::list<double>& valuesList);

  //@{
  /**
   * Convenience methods
   */
  double FFix(double);
  double FSign(double, double);
  //@}

  /**
   * Automatically rescale titles and labels
   * NB: Current implementation only for perspective projections.
   */
  void AutoScale(svtkViewport* viewport);

  /**
   * convert section angle to an angle applied to ellipse equation.
   * the result point with ellipse angle, is the point located on section angle
   */
  static double ComputeEllipseAngle(double angleInDegrees, double ratio);

  /**
   * Compute delta angle of radial axes.
   */
  virtual void ComputeDeltaAngleRadialAxes(svtkIdType);
  /**
   * Coordinates of the pole
   * Default: (0,0,0).
   */
  double Pole[3];

  /**
   * Number of radial axes
   */
  int NumberOfRadialAxes;

  /**
   * Requested Number of radial axes
   */
  int RequestedNumberOfRadialAxes;

  /**
   * Whether the number of polar axis ticks and arcs should be automatically calculated.
   * Default: TRUE
   */
  bool AutoSubdividePolarAxis;

  /**
   * Ratio for elliptical representation of the polar axes actor.
   */
  double Ratio;

  /**
   * Define the range values displayed on the polar Axis.
   */
  double Range[2];

  /**
   * Step between 2 minor ticks, in range value (values displayed on the axis).
   */
  double DeltaRangeMinor;

  /**
   * Step between 2 major ticks, in range value (values displayed on the axis).
   */
  double DeltaRangeMajor;

  /**
   * Angle between 2 minor ticks on the last arc.
   */
  double DeltaAngleMinor;

  /**
   * Angle between 2 major ticks on the last arc.
   */
  double DeltaAngleMajor;

  /**
   * Angle between 2 radial Axes.
   */
  double DeltaAngleRadialAxes;

  /**
   * Minimum polar radius.
   * Default: 0.0
   */
  double MinimumRadius;

  /**
   * Maximum polar radius.
   * Default: 1
   */
  double MaximumRadius;

  /**
   * Enable/Disable log scale
   * Default: 0
   */
  bool Log;

  /**
   * Auto-scale polar radius (with respect to average length scale of x-y bounding box).
   */
  bool AutoScaleRadius;

  /**
   * Minimum polar angle
   * Default: 0.
   */
  double MinimumAngle;

  /**
   * Maximum polar angle
   * Default: 90.
   */
  double MaximumAngle;

  /**
   * Smallest radial angle distinguishable from polar axis
   */
  double SmallestVisiblePolarAngle;

  // Structures for principal polar arc
  svtkPolyData* PolarArcs;
  svtkPolyDataMapper* PolarArcsMapper;
  svtkActor* PolarArcsActor;

  //@{
  /**
   * Structures for secondary polar arcs
   */
  svtkPolyData* SecondaryPolarArcs;
  svtkPolyDataMapper* SecondaryPolarArcsMapper;
  svtkActor* SecondaryPolarArcsActor;
  //@}

  /**
   * Camera attached to the polar axes system
   */
  svtkCamera* Camera;

  /**
   * Control variables for polar axis
   */
  svtkAxisActor* PolarAxis;

  /**
   * Control variables for non-polar radial axes
   */
  svtkAxisActor** RadialAxes;

  //@{
  /**
   * Title to be used for the polar axis
   * NB: Non-polar radial axes use the polar angle as title and have no labels
   */
  char* PolarAxisTitle;
  char* PolarLabelFormat;
  //@}

  /**
   * String to format angle values displayed on the radial axes.
   */
  char* RadialAngleFormat;

  /**
   * Display angle units (degrees) to label radial axes
   * Default is true
   */
  bool RadialUnits;

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

  //@{
  /**
   * Visibility of polar axis and its title, labels, ticks (major only)
   */
  svtkTypeBool PolarAxisVisibility;
  svtkTypeBool PolarTitleVisibility;
  svtkTypeBool PolarLabelVisibility;
  //@}

  /**
   * Describes the tick orientation for the graph elements involved by this property.
   * The ticks are drawn according to the direction of the 2 orthogonal axes, of the axisBase
   * defined for a svtkAxisActor.
   * For an ellipse, tick directions are defined from ellipse center to tick origin and
   * the orthogonal direction of the ellipse plane.
   */
  int TickLocation;

  /**
   * Hold visibility for all present ticks
   */
  svtkTypeBool PolarTickVisibility;

  /**
   * If On, the ticks are drawn from the angle of the polarAxis (i.e. this->MinimumAngle)
   * and continue counterclockwise with the step DeltaAngle Major/Minor. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  int ArcTicksOriginToPolarAxis;

  /**
   * If On, the radial axes are drawn from the angle of the polarAxis (i.e. this->MinimalRadius)
   * and continue counterclockwise with the step DeltaAngleRadialAxes. if Off, the start angle is
   * 0.0, i.e.
   * the angle on the major radius of the ellipse.
   */
  int RadialAxesOriginToPolarAxis;

  /**
   * Hold visibility of major/minor ticks for the polar axis and the last radial axis
   */
  svtkTypeBool AxisTickVisibility, AxisMinorTickVisibility;

  /**
   * Enable / Disable major/minor tick visibility on the last arc displayed
   */
  svtkTypeBool ArcTickVisibility, ArcMinorTickVisibility;

  /**
   * Defines the length of the ticks located on the last arc
   */
  double PolarAxisMajorTickSize, LastRadialAxisMajorTickSize, ArcMajorTickSize;

  /**
   * Set the ratios between major tick Size for each ticks location
   */
  double PolarAxisTickRatioSize, LastAxisTickRatioSize, ArcTickRatioSize;

  /**
   * Defines the tickness of the major ticks.
   */
  double PolarAxisMajorTickThickness, LastRadialAxisMajorTickThickness, ArcMajorTickThickness;

  /**
   * Set the ratios between major tick thickness for each ticks location
   */
  double PolarAxisTickRatioThickness, LastAxisTickRatioThickness, ArcTickRatioThickness;

  //@{
  /**
   * Visibility of radial axes and their titles
   */
  svtkTypeBool RadialAxesVisibility;
  svtkTypeBool RadialTitleVisibility;
  //@}

  /**
   * Define the alignment of the title related to the radial axis. (BOTTOM or EXTERN)
   */
  int RadialAxisTitleLocation;

  /**
   * Define the alignment of the title related to the polar axis. (BOTTOM or EXTERN)
   */
  int PolarAxisTitleLocation;

  /**
   * Define the location of the exponent of the labels values, located on the polar axis.
   * it could be: LABEL, EXTERN, BOTTOM
   */
  int ExponentLocation;

  /**
   * Visibility of polar arcs
   */
  svtkTypeBool PolarArcsVisibility;

  /**
   * Visibility of the inner axes (overridden to 0 if RadialAxesVisibility is set to 0)
   */
  svtkTypeBool DrawRadialGridlines;

  /**
   * Visibility of the inner arcs (overridden to 0 if PolarArcsVisibility is set to 0)
   */
  svtkTypeBool DrawPolarArcsGridlines;

  /**
   * Keep the arc major ticks svtkPoints instances
   */
  svtkPoints* ArcMajorTickPts;

  /**
   * Keep the arc minor ticks svtkPoints instances
   */
  svtkPoints* ArcMinorTickPts;

  //@{
  /**
   * svtk object for arc Ticks
   */
  svtkPolyData* ArcTickPolyData;
  svtkPolyData* ArcMinorTickPolyData;
  svtkPolyDataMapper* ArcTickPolyDataMapper;
  svtkPolyDataMapper* ArcMinorTickPolyDataMapper;
  svtkActor* ArcTickActor;
  svtkActor* ArcMinorTickActor;
  //@}

  //@{
  /**
   * Text properties of polar axis title and labels
   */
  svtkTextProperty* PolarAxisTitleTextProperty;
  svtkTextProperty* PolarAxisLabelTextProperty;
  //@}

  /**
   * Text properties of last radial axis
   */
  svtkTextProperty* LastRadialAxisTextProperty;

  /**
   * Text properties of secondary radial axes
   */
  svtkTextProperty* SecondaryRadialAxesTextProperty;

  /**
   * General properties of polar axis
   * Behavior may be override by polar axis ticks 's actor property.
   */
  svtkProperty* PolarAxisProperty;

  /**
   * General properties of last radial axis
   */
  svtkProperty* LastRadialAxisProperty;

  /**
   * General properties of radial axes
   */
  svtkProperty* SecondaryRadialAxesProperty;

  svtkTimeStamp BuildTime;

  /**
   * Title scale factor
   */
  double TitleScale;

  /**
   * Label scale factor
   */
  double LabelScale;

  /**
   * Text screen size
   */
  double ScreenSize;

private:
  svtkPolarAxesActor(const svtkPolarAxesActor&) = delete;
  void operator=(const svtkPolarAxesActor&) = delete;
};

#endif
