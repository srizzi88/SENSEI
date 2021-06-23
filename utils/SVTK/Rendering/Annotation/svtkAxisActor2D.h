/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxisActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAxisActor2D
 * @brief   Create an axis with tick marks and labels
 *
 * svtkAxisActor2D creates an axis with tick marks, labels, and/or a title,
 * depending on the particular instance variable settings. svtkAxisActor2D is
 * a 2D actor; that is, it is drawn on the overlay plane and is not
 * occluded by 3D geometry. To use this class, you typically specify two
 * points defining the start and end points of the line (x-y definition using
 * svtkCoordinate class), the number of labels, and the data range
 * (min,max). You can also control what parts of the axis are visible
 * including the line, the tick marks, the labels, and the title.  You can
 * also specify the label format (a printf style format).
 *
 * This class decides what font size to use and how to locate the labels. It
 * also decides how to create reasonable tick marks and labels. The number
 * of labels and the range of values may not match the number specified, but
 * should be close.
 *
 * Labels are drawn on the "right" side of the axis. The "right" side is
 * the side of the axis on the right as you move from Position to Position2.
 * The way the labels and title line up with the axis and tick marks depends on
 * whether the line is considered horizontal or vertical.
 *
 * The svtkActor2D instance variables Position and Position2 are instances of
 * svtkCoordinate. Note that the Position2 is an absolute position in that
 * class (it was by default relative to Position in svtkActor2D).
 *
 * What this means is that you can specify the axis in a variety of coordinate
 * systems. Also, the axis does not have to be either horizontal or vertical.
 * The tick marks are created so that they are perpendicular to the axis.
 *
 * Set the text property/attributes of the title and the labels through the
 * svtkTextProperty objects associated to this actor.
 *
 * @sa
 * svtkCubeAxesActor2D can be used to create axes in world coordinate space.
 *
 * @sa
 * svtkActor2D svtkTextMapper svtkPolyDataMapper2D svtkScalarBarActor
 * svtkCoordinate svtkTextProperty
 */

#ifndef svtkAxisActor2D_h
#define svtkAxisActor2D_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkPolyDataMapper2D;
class svtkPolyData;
class svtkTextMapper;
class svtkTextProperty;

class SVTKRENDERINGANNOTATION_EXPORT svtkAxisActor2D : public svtkActor2D
{
public:
  svtkTypeMacro(svtkAxisActor2D, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object.
   */
  static svtkAxisActor2D* New();

  //@{
  /**
   * Specify the position of the first point defining the axis.
   * Note: backward compatibility only, use svtkActor2D's Position instead.
   */
  virtual svtkCoordinate* GetPoint1Coordinate() { return this->GetPositionCoordinate(); }
  virtual void SetPoint1(double x[2]) { this->SetPosition(x); }
  virtual void SetPoint1(double x, double y) { this->SetPosition(x, y); }
  virtual double* GetPoint1() { return this->GetPosition(); }
  //@}

  //@{
  /**
   * Specify the position of the second point defining the axis. Note that
   * the order from Point1 to Point2 controls which side the tick marks
   * are drawn on (ticks are drawn on the right, if visible).
   * Note: backward compatibility only, use svtkActor2D's Position2 instead.
   */
  virtual svtkCoordinate* GetPoint2Coordinate() { return this->GetPosition2Coordinate(); }
  virtual void SetPoint2(double x[2]) { this->SetPosition2(x); }
  virtual void SetPoint2(double x, double y) { this->SetPosition2(x, y); }
  virtual double* GetPoint2() { return this->GetPosition2(); }
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
   * Specify whether this axis should act like a measuring tape (or ruler) with
   * specified major tick spacing. If enabled, the distance between major ticks
   * is controlled by the RulerDistance ivar.
   */
  svtkSetMacro(RulerMode, svtkTypeBool);
  svtkGetMacro(RulerMode, svtkTypeBool);
  svtkBooleanMacro(RulerMode, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the RulerDistance which indicates the spacing of the major ticks.
   * This ivar only has effect when the RulerMode is on.
   */
  svtkSetClampMacro(RulerDistance, double, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(RulerDistance, double);
  //@}

  enum LabelMax
  {
    SVTK_MAX_LABELS = 25
  };

  //@{
  /**
   * Set/Get the number of annotation labels to show. This also controls the
   * number of major ticks shown. Note that this ivar only holds meaning if
   * the RulerMode is off.
   */
  svtkSetClampMacro(NumberOfLabels, int, 2, SVTK_MAX_LABELS);
  svtkGetMacro(NumberOfLabels, int);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on the scalar
   * bar.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Set/Get the flag that controls whether the labels and ticks are
   * adjusted for "nice" numerical values to make it easier to read
   * the labels. The adjustment is based in the Range instance variable.
   * Call GetAdjustedRange and GetAdjustedNumberOfLabels to get the adjusted
   * range and number of labels. Note that if RulerMode is on, then the
   * number of labels is a function of the range and ruler distance.
   */
  svtkSetMacro(AdjustLabels, svtkTypeBool);
  svtkGetMacro(AdjustLabels, svtkTypeBool);
  svtkBooleanMacro(AdjustLabels, svtkTypeBool);
  virtual double* GetAdjustedRange()
  {
    this->UpdateAdjustedRange();
    return this->AdjustedRange;
  }
  virtual void GetAdjustedRange(double& _arg1, double& _arg2)
  {
    this->UpdateAdjustedRange();
    _arg1 = this->AdjustedRange[0];
    _arg2 = this->AdjustedRange[1];
  };
  virtual void GetAdjustedRange(double _arg[2]) { this->GetAdjustedRange(_arg[0], _arg[1]); }
  virtual int GetAdjustedNumberOfLabels()
  {
    this->UpdateAdjustedRange();
    return this->AdjustedNumberOfLabels;
  }
  //@}

  //@{
  /**
   * Set/Get the title of the scalar bar actor,
   */
  svtkSetStringMacro(Title);
  svtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the title text property.
   */
  virtual void SetTitleTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TitleTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the labels text property.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(LabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the length of the tick marks (expressed in pixels or display
   * coordinates).
   */
  svtkSetClampMacro(TickLength, int, 0, 100);
  svtkGetMacro(TickLength, int);
  //@}

  //@{
  /**
   * Number of minor ticks to be displayed between each tick. Default
   * is 0.
   */
  svtkSetClampMacro(NumberOfMinorTicks, int, 0, 20);
  svtkGetMacro(NumberOfMinorTicks, int);
  //@}

  //@{
  /**
   * Set/Get the length of the minor tick marks (expressed in pixels or
   * display coordinates).
   */
  svtkSetClampMacro(MinorTickLength, int, 0, 100);
  svtkGetMacro(MinorTickLength, int);
  //@}

  //@{
  /**
   * Set/Get the offset of the labels (expressed in pixels or display
   * coordinates). The offset is the distance of labels from tick marks
   * or other objects.
   */
  svtkSetClampMacro(TickOffset, int, 0, 100);
  svtkGetMacro(TickOffset, int);
  //@}

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
   * Set/Get visibility of the axis tick marks.
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
   * Set/Get position of the axis title. 0 is at the start of the
   * axis whereas 1 is at the end.
   */
  svtkSetMacro(TitlePosition, double);
  svtkGetMacro(TitlePosition, double);
  //@}

  //@{
  /**
   * Set/Get the factor that controls the overall size of the fonts used
   * to label and title the axes. This ivar used in conjunction with
   * the LabelFactor can be used to control font sizes.
   */
  svtkSetClampMacro(FontFactor, double, 0.1, 2.0);
  svtkGetMacro(FontFactor, double);
  //@}

  //@{
  /**
   * Set/Get the factor that controls the relative size of the axis labels
   * to the axis title.
   */
  svtkSetClampMacro(LabelFactor, double, 0.1, 2.0);
  svtkGetMacro(LabelFactor, double);
  //@}

  //@{
  /**
   * Draw the axis.
   */
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * This method computes the range of the axis given an input range.
   * It also computes the number of tick marks given a suggested number.
   * (The number of tick marks includes end ticks as well.)
   * The number of tick marks computed (in conjunction with the output
   * range) will yield "nice" tick values. For example, if the input range
   * is (0.25,96.7) and the number of ticks requested is 10, the output range
   * will be (0,100) with the number of computed ticks to 11 to yield tick
   * values of (0,10,20,...,100).
   */
  static void ComputeRange(
    double inRange[2], double outRange[2], int inNumTicks, int& outNumTicks, double& interval);

  /**
   * General method to computes font size from a representative size on the
   * viewport (given by size[2]). The method returns the font size (in points)
   * and the string height/width (in pixels). It also sets the font size of the
   * instance of svtkTextMapper provided. The factor is used when you're trying
   * to create text of different size-factor (it is usually = 1 but you can
   * adjust the font size by making factor larger or smaller).
   */
  static int SetMultipleFontSize(svtkViewport* viewport, svtkTextMapper** textMappers,
    int nbOfMappers, int* targetSize, double factor, int* stringSize);

  //@{
  /**
   * Specify whether to size the fonts relative to the viewport or relative to
   * length of the axis. By default, fonts are resized relative to the viewport.
   */
  svtkSetMacro(SizeFontRelativeToAxis, svtkTypeBool);
  svtkGetMacro(SizeFontRelativeToAxis, svtkTypeBool);
  svtkBooleanMacro(SizeFontRelativeToAxis, svtkTypeBool);
  //@}

  //@{
  /**
   * By default the AxisActor controls the font size of the axis label.  If this
   * option is set to true, it will instead use whatever font size is set in the
   * svtkTextProperty, allowing external control of the axis size.
   */
  svtkSetMacro(UseFontSizeFromProperty, svtkTypeBool);
  svtkGetMacro(UseFontSizeFromProperty, svtkTypeBool);
  svtkBooleanMacro(UseFontSizeFromProperty, svtkTypeBool);
  //@}

  /**
   * Shallow copy of an axis actor. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

protected:
  svtkAxisActor2D();
  ~svtkAxisActor2D() override;

  svtkTextProperty* TitleTextProperty;
  svtkTextProperty* LabelTextProperty;

  char* Title;
  double Range[2];
  double TitlePosition;
  svtkTypeBool RulerMode;
  double RulerDistance;
  int NumberOfLabels;
  char* LabelFormat;
  svtkTypeBool AdjustLabels;
  double FontFactor;
  double LabelFactor;
  int TickLength;
  int MinorTickLength;
  int TickOffset;
  int NumberOfMinorTicks;

  double AdjustedRange[2];
  int AdjustedNumberOfLabels;
  int NumberOfLabelsBuilt;

  svtkTypeBool AxisVisibility;
  svtkTypeBool TickVisibility;
  svtkTypeBool LabelVisibility;
  svtkTypeBool TitleVisibility;

  int LastPosition[2];
  int LastPosition2[2];

  int LastSize[2];
  int LastMaxLabelSize[2];

  int SizeFontRelativeToAxis;
  svtkTypeBool UseFontSizeFromProperty;

  virtual void BuildAxis(svtkViewport* viewport);
  static double ComputeStringOffset(double width, double height, double theta);
  static void SetOffsetPosition(double xTick[3], double theta, int stringWidth, int stringHeight,
    int offset, svtkActor2D* actor);
  virtual void UpdateAdjustedRange();

  svtkTextMapper* TitleMapper;
  svtkActor2D* TitleActor;

  svtkTextMapper** LabelMappers;
  svtkActor2D** LabelActors;

  svtkPolyData* Axis;
  svtkPolyDataMapper2D* AxisMapper;
  svtkActor2D* AxisActor;

  svtkTimeStamp AdjustedRangeBuildTime;
  svtkTimeStamp BuildTime;

private:
  svtkAxisActor2D(const svtkAxisActor2D&) = delete;
  void operator=(const svtkAxisActor2D&) = delete;
};

#endif
