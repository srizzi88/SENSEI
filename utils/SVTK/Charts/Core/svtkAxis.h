/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxis.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkAxis
 * @brief   takes care of drawing 2D axes
 *
 *
 * The svtkAxis is drawn in screen coordinates. It is usually one of the last
 * elements of a chart to be drawn. It renders the axis label, tick marks and
 * tick labels.
 * The tick marks and labels span the range of values between
 * \a Minimum and \a Maximum.
 * The \a Minimum and \a Maximum values are not allowed to extend beyond the
 * \a MinimumLimit and \a MaximumLimit values, respectively.
 *
 * Note that many other chart elements (e.g., svtkPlotPoints) refer to
 * svtkAxis instances to determine how to scale raw data for presentation.
 * In particular, care must be taken with logarithmic scaling.
 * The axis Minimum, Maximum, and Limit values are stored both unscaled
 * and scaled (with log(x) applied when GetLogScaleActive() returns true).
 * User interfaces will most likely present the unscaled values as they
 * represent the values provided by the user.
 * Other chart elements may need the scaled values in order to draw
 * in the same coordinate system.
 *
 * Just because LogScale is set to true does not guarantee that the axis
 * will use logarithmic scaling -- the Minimum and Maximum values for the
 * axis must both lie to the same side of origin (and not include the origin).
 * Also, this switch from linear- to log-scaling may occur during a rendering
 * pass if autoscaling is enabled.
 * Because the log and pow functions are not invertible and the axis itself
 * decides when to switch between them without offering any external class
 * managing the axis a chance to save the old values, it saves
 * old Limit values in NonLogUnscaled{Min,Max}Limit so that behavior is
 * consistent when LogScale is changed from false to true and back again.
 */

#ifndef svtkAxis_h
#define svtkAxis_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkContextItem.h"
#include "svtkRect.h"         // For bounding rect
#include "svtkSmartPointer.h" // For svtkSmartPointer
#include "svtkStdString.h"    // For svtkStdString ivars
#include "svtkVector.h"       // For position variables

class svtkContext2D;
class svtkPen;
class svtkFloatArray;
class svtkDoubleArray;
class svtkStringArray;
class svtkTextProperty;

class SVTKCHARTSCORE_EXPORT svtkAxis : public svtkContextItem
{
public:
  svtkTypeMacro(svtkAxis, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Enumeration of the axis locations in a conventional XY chart. Other
   * layouts are possible.
   */
  enum Location
  {
    LEFT = 0,
    BOTTOM,
    RIGHT,
    TOP,
    PARALLEL
  };

  enum
  {
    TICK_SIMPLE = 0,
    TICK_WILKINSON_EXTENDED
  };

  /**
   * Creates a 2D Chart object.
   */
  static svtkAxis* New();

  //@{
  /**
   * Get/set the position of the axis (LEFT, BOTTOM, RIGHT, TOP, PARALLEL).
   */
  virtual void SetPosition(int position);
  svtkGetMacro(Position, int);
  //@}

  //@{
  /**
   * Set point 1 of the axis (in pixels), this is usually the origin.
   */
  void SetPoint1(const svtkVector2f& pos);
  void SetPoint1(float x, float y);
  //@}

  //@{
  /**
   * Get point 1 of the axis (in pixels), this is usually the origin.
   */
  svtkGetVector2Macro(Point1, float);
  svtkVector2f GetPosition1();
  //@}

  //@{
  /**
   * Set point 2 of the axis (in pixels), this is usually the terminus.
   */
  void SetPoint2(const svtkVector2f& pos);
  void SetPoint2(float x, float y);
  //@}

  //@{
  /**
   * Get point 2 of the axis (in pixels), this is usually the terminus.
   */
  svtkGetVector2Macro(Point2, float);
  svtkVector2f GetPosition2();
  //@}

  /**
   * Set the number of tick marks for this axis. Default is -1, which leads to
   * automatic calculation of nicely spaced tick marks.
   */
  virtual void SetNumberOfTicks(int numberOfTicks);

  //@{
  /**
   * Get the number of tick marks for this axis.
   */
  svtkGetMacro(NumberOfTicks, int);
  //@}

  //@{
  /**
   * Get/set the length of tick marks (in pixels).
   */
  svtkSetMacro(TickLength, float);
  svtkGetMacro(TickLength, float);
  //@}

  //@{
  /**
   * Get the svtkTextProperty that governs how the axis labels are displayed.
   * Note that the alignment properties are not used.
   */
  svtkGetObjectMacro(LabelProperties, svtkTextProperty);
  //@}

  /**
   * Set the logical minimum value of the axis, in plot coordinates.
   * If LogScaleActive is true (not just LogScale), then this
   * sets the minimum base-10 <b>exponent</b>.
   */
  virtual void SetMinimum(double minimum);

  //@{
  /**
   * Get the logical minimum value of the axis, in plot coordinates.
   * If LogScaleActive is true (not just LogScale), then this
   * returns the minimum base-10 <b>exponent</b>.
   */
  svtkGetMacro(Minimum, double);
  //@}

  /**
   * Set the logical maximum value of the axis, in plot coordinates.
   * If LogScaleActive is true (not just LogScale), then this
   * sets the maximum base-10 <b>exponent</b>.
   */
  virtual void SetMaximum(double maximum);

  //@{
  /**
   * Get the logical maximum value of the axis, in plot coordinates.
   * If LogScaleActive is true (not just LogScale), then this
   * returns the maximum base-10 <b>exponent</b>.
   */
  svtkGetMacro(Maximum, double);
  //@}

  /**
   * Set the logical, unscaled minimum value of the axis, in plot coordinates.
   * Use this instead of SetMinimum() if you wish to provide the actual minimum
   * instead of log10(the minimum) as part of the axis scale.
   */
  virtual void SetUnscaledMinimum(double minimum);

  //@{
  /**
   * Get the logical minimum value of the axis, in plot coordinates.
   */
  svtkGetMacro(UnscaledMinimum, double);
  //@}

  /**
   * Set the logical maximum value of the axis, in plot coordinates.
   */
  virtual void SetUnscaledMaximum(double maximum);

  //@{
  /**
   * Get the logical maximum value of the axis, in plot coordinates.
   */
  svtkGetMacro(UnscaledMaximum, double);
  //@}

  //@{
  /**
   * Set the logical range of the axis, in plot coordinates.

   * The unscaled range will always be in the same coordinate system of
   * the data being plotted, regardless of whether LogScale is true or false.
   * When calling SetRange() and LogScale is true, the range must be specified
   * in logarithmic coordinates.
   * Using SetUnscaledRange(), you may ignore the value of LogScale.
   */
  virtual void SetRange(double minimum, double maximum);
  virtual void SetRange(double range[2]);
  virtual void SetUnscaledRange(double minimum, double maximum);
  virtual void SetUnscaledRange(double range[2]);
  //@}

  //@{
  /**
   * Get the logical range of the axis, in plot coordinates.

   * The unscaled range will always be in the same coordinate system of
   * the data being plotted, regardless of whether LogScale is true or false.
   * Calling GetRange() when LogScale is true will return the log10({min, max}).
   */
  virtual void GetRange(double* range);
  virtual void GetUnscaledRange(double* range);
  //@}

  /**
   * Set the logical lowest possible value for \a Minimum, in plot coordinates.
   */
  virtual void SetMinimumLimit(double lowest);

  //@{
  /**
   * Get the logical lowest possible value for \a Minimum, in plot coordinates.
   */
  svtkGetMacro(MinimumLimit, double);
  //@}

  /**
   * Set the logical highest possible value for \a Maximum, in plot coordinates.
   */
  virtual void SetMaximumLimit(double highest);

  //@{
  /**
   * Get the logical highest possible value for \a Maximum, in plot coordinates.
   */
  svtkGetMacro(MaximumLimit, double);
  //@}

  /**
   * Set the logical lowest possible value for \a Minimum, in plot coordinates.
   */
  virtual void SetUnscaledMinimumLimit(double lowest);

  //@{
  /**
   * Get the logical lowest possible value for \a Minimum, in plot coordinates.
   */
  svtkGetMacro(UnscaledMinimumLimit, double);
  //@}

  /**
   * Set the logical highest possible value for \a Maximum, in plot coordinates.
   */
  virtual void SetUnscaledMaximumLimit(double highest);

  //@{
  /**
   * Get the logical highest possible value for \a Maximum, in plot coordinates.
   */
  svtkGetMacro(UnscaledMaximumLimit, double);
  //@}

  //@{
  /**
   * Get the margins of the axis, in pixels.
   */
  svtkGetVector2Macro(Margins, int);
  //@}

  //@{
  /**
   * Set the margins of the axis, in pixels.
   */
  svtkSetVector2Macro(Margins, int);
  //@}

  //@{
  /**
   * Get/set the title text of the axis.
   */
  virtual void SetTitle(const svtkStdString& title);
  virtual svtkStdString GetTitle();
  //@}

  //@{
  /**
   * Get the svtkTextProperty that governs how the axis title is displayed.
   */
  svtkGetObjectMacro(TitleProperties, svtkTextProperty);
  //@}

  //@{
  /**
   * Get whether the axis is using a log scale.
   * This will always be false when LogScale is false.
   * It is only true when LogScale is true <b>and</b> the \a UnscaledRange
   * does not cross or include the origin (zero).

   * The limits (\a MinimumLimit, \a MaximumLimit, and their
   * unscaled counterparts) do not prevent LogScaleActive from becoming
   * true; they are adjusted if they cross or include the origin
   * and the original limits are preserved for when LogScaleActive
   * becomes false again.
   */
  svtkGetMacro(LogScaleActive, bool);
  //@}

  //@{
  /**
   * Get/set whether the axis should <b>attempt</b> to use a log scale.

   * The default is false.
   * \sa{LogScaleActive}.
   */
  svtkGetMacro(LogScale, bool);
  virtual void SetLogScale(bool logScale);
  svtkBooleanMacro(LogScale, bool);
  //@}

  //@{
  /**
   * Get/set whether the axis grid lines should be drawn, default is true.
   */
  svtkSetMacro(GridVisible, bool);
  svtkGetMacro(GridVisible, bool);
  //@}

  //@{
  /**
   * Get/set whether the axis labels should be visible.
   */
  svtkSetMacro(LabelsVisible, bool);
  svtkGetMacro(LabelsVisible, bool);
  //@}

  //@{
  /**
   * Get/set whether the labels for the range should be visible.
   */
  svtkSetMacro(RangeLabelsVisible, bool);
  svtkGetMacro(RangeLabelsVisible, bool);
  //@}

  //@{
  /**
   * Get/set the offset (in pixels) of the label text position from the axis
   */
  svtkSetMacro(LabelOffset, float);
  svtkGetMacro(LabelOffset, float);
  //@}

  //@{
  /**
   * Get/set whether the tick marks should be visible.
   */
  svtkSetMacro(TicksVisible, bool);
  svtkGetMacro(TicksVisible, bool);
  //@}

  //@{
  /**
   * Get/set whether the axis line should be visible.
   */
  svtkSetMacro(AxisVisible, bool);
  svtkGetMacro(AxisVisible, bool);
  //@}

  //@{
  /**
   * Get/set whether the axis title should be visible.
   */
  svtkSetMacro(TitleVisible, bool);
  svtkGetMacro(TitleVisible, bool);
  //@}

  //@{
  /**
   * Get/set the numerical precision to use, default is 2. This is ignored
   * when Notation is STANDARD_NOTATION or PRINTF_NOTATION.
   */
  virtual void SetPrecision(int precision);
  svtkGetMacro(Precision, int);
  //@}

  /**
   * Enumeration of the axis notations available.
   */
  enum
  {
    STANDARD_NOTATION = 0,
    SCIENTIFIC_NOTATION,
    FIXED_NOTATION,
    PRINTF_NOTATION
  };

  //@{
  /**
   * Get/Set the printf-style format string used when TickLabelAlgorithm is
   * TICK_SIMPLE and Notation is PRINTF_NOTATION. The default is "%g".
   */
  virtual void SetLabelFormat(const std::string& fmt);
  svtkGetMacro(LabelFormat, std::string);
  //@}

  //@{
  /**
   * Get/Set the printf-style format string used for range labels.
   * This format is always used regardless of TickLabelAlgorithm and
   * Notation. Default is "%g".
   */
  svtkSetMacro(RangeLabelFormat, std::string);
  svtkGetMacro(RangeLabelFormat, std::string);
  //@}

  //@{
  /**
   * Get/set the numerical notation, standard, scientific, fixed, or a
   * printf-style format string.
   * \sa SetPrecision SetLabelFormat
   */
  virtual void SetNotation(int notation);
  svtkGetMacro(Notation, int);
  //@}

  /**
   * Enumeration of the axis behaviors.
   */
  enum
  {
    AUTO = 0, // Automatically scale the axis to view all data that is visible.
    FIXED,    // Use a fixed axis range and make no attempt to rescale.
    CUSTOM    // Deprecated, use the tick label settings instead.
  };

  //@{
  /**
   * Get/set the behavior of the axis (auto or fixed). The default is 0 (auto).
   */
  svtkSetMacro(Behavior, int);
  svtkGetMacro(Behavior, int);
  //@}

  //@{
  /**
   * Get a pointer to the svtkPen object that controls the way this axis is drawn.
   */
  svtkGetObjectMacro(Pen, svtkPen);
  //@}

  //@{
  /**
   * Get a pointer to the svtkPen object that controls the way this axis is drawn.
   */
  svtkGetObjectMacro(GridPen, svtkPen);
  //@}

  //@{
  /**
   * Get/set the tick label algorithm that is used to calculate the min, max
   * and tick spacing. There are currently two algoriths, svtkAxis::TICK_SIMPLE
   * is the default and uses a simple algorithm. The second option is
   * svtkAxis::TICK_WILKINSON which uses an extended Wilkinson algorithm to find
   * the optimal range, spacing and font parameters.
   */
  svtkSetMacro(TickLabelAlgorithm, int);
  svtkGetMacro(TickLabelAlgorithm, int);
  //@}

  //@{
  /**
   * Get/set the scaling factor used for the axis, this defaults to 1.0 (no
   * scaling), and is used to coordinate scaling with the plots, charts, etc.
   */
  svtkSetMacro(ScalingFactor, double);
  svtkGetMacro(ScalingFactor, double);
  svtkSetMacro(Shift, double);
  svtkGetMacro(Shift, double);
  //@}

  /**
   * Update the geometry of the axis. Takes care of setting up the tick mark
   * locations etc. Should be called by the scene before rendering.
   */
  void Update() override;

  /**
   * Paint event for the axis, called whenever the axis needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Use this function to autoscale the axes after setting the minimum and
   * maximum values. This will cause the axes to select the nicest numbers
   * that enclose the minimum and maximum values, and to select an appropriate
   * number of tick marks.
   */
  virtual void AutoScale();

  /**
   * Recalculate the spacing of the tick marks - typically useful to do after
   * scaling the axis.
   */
  virtual void RecalculateTickSpacing();

  /**
   * An array with the positions of the tick marks along the axis line.
   * The positions are specified in the plot coordinates of the axis.
   */
  virtual svtkDoubleArray* GetTickPositions();

  /**
   * An array with the positions of the tick marks along the axis line.
   * The positions are specified in scene coordinates.
   */
  virtual svtkFloatArray* GetTickScenePositions();

  /**
   * A string array containing the tick labels for the axis.
   */
  virtual svtkStringArray* GetTickLabels();

  /**
   * Set the tick positions, and optionally custom tick labels. If the labels
   * and positions are null then automatic tick labels will be assigned. If
   * only positions are supplied then appropriate labels will be generated
   * according to the axis settings. If positions and labels are supplied they
   * must be of the same length. Returns true on success, false on failure.
   */
  virtual bool SetCustomTickPositions(svtkDoubleArray* positions, svtkStringArray* labels = nullptr);

  /**
   * Request the space the axes require to be drawn. This is returned as a
   * svtkRectf, with the corner being the offset from Point1, and the width/
   * height being the total width/height required by the axis. In order to
   * ensure the numbers are correct, Update() should be called on the axis.
   */
  svtkRectf GetBoundingRect(svtkContext2D* painter);

  /**
   * Return a "nice number", often defined as 1, 2 or 5. If roundUp is true then
   * the nice number will be rounded up, false it is rounded down. The supplied
   * number should be between 0.0 and 9.9.
   */
  static double NiceNumber(double number, bool roundUp);

  /**
   * Static function to calculate "nice" minimum, maximum, and tick spacing
   * values.
   */
  static double NiceMinMax(double& min, double& max, float pixelRange, float tickPixelSpacing);

  /**
   * Generate a single label using the current settings when TickLabelAlgorithm
   * is TICK_SIMPLE.
   */
  virtual svtkStdString GenerateSimpleLabel(double val);

protected:
  svtkAxis();
  ~svtkAxis() override;

  /**
   * Update whether log scaling will be used for layout and rendering.

   * Log scaling is only active when LogScaling is true <b>and</b> the closed,
   * unscaled range does not contain the origin.
   * The boolean parameter determines whether the minimum and maximum values
   * are set from their unscaled counterparts.
   */
  void UpdateLogScaleActive(bool updateMinMaxFromUnscaled);

  /**
   * Calculate and assign nice labels/logical label positions.
   */
  virtual void GenerateTickLabels(double min, double max);

  /**
   * Generate tick labels from the supplied double array of tick positions.
   */
  virtual void GenerateTickLabels();

  virtual void GenerateLabelFormat(int notation, double n);

  /**
   * Generate label using a printf-style format string.
   */
  virtual svtkStdString GenerateSprintfLabel(double value, const std::string& format);

  /**
   * Calculate the next "nicest" numbers above and below the current minimum.
   * \return the "nice" spacing of the numbers.
   */
  double CalculateNiceMinMax(double& min, double& max);

  /**
   * Return a tick mark for a logarithmic axis.
   * If roundUp is true then the upper tick mark is returned.
   * Otherwise the lower tick mark is returned.
   * Tick marks will be: ... 0.1 0.2 .. 0.9 1 2 .. 9 10 20 .. 90 100 ...
   * Parameter nicevalue will be set to true if tick mark is in:
   * ... 0.1 0.2 0.5 1 2 5 10 20 50 100 ...
   * Parameter order is set to the detected order of magnitude of the number.
   */
  double LogScaleTickMark(double number, bool roundUp, bool& niceValue, int& order);

  /**
   * Generate logarithmically-spaced tick marks with linear-style labels.

   * This is for the case when log scaling is active, but the axis min and max
   * span less than an order of magnitude.
   * In this case, the most significant digit that varies is identified and
   * ticks generated for each value that digit may take on. If that results
   * in only 2 tick marks, the next-most-significant digit is varied.
   * If more than 20 tick marks would result, the stride for the varying digit
   * is increased.
   */
  virtual void GenerateLogSpacedLinearTicks(int order, double min, double max);

  /**
   * Generate tick marks for logarithmic scale for specific order of magnitude.
   * Mark generation is limited by parameters min and max.
   * Tick marks will be: ... 0.1 0.2 .. 0.9 1 2 .. 9 10 20 .. 90 100 ...
   * Tick labels will be: ... 0.1 0.2 0.5 1 2 5 10 20 50 100 ...
   * If Parameter detaillabels is disabled tick labels will be:
   * ... 0.1 1 10 100 ...
   * If min/max is not in between 1.0 and 9.0 defaults will be used.
   * If min and max do not differ 1 defaults will be used.
   */
  void GenerateLogScaleTickMarks(
    int order, double min = 1.0, double max = 9.0, bool detailLabels = true);

  int Position;  // The position of the axis (LEFT, BOTTOM, RIGHT, TOP)
  float* Point1; // The position of point 1 (usually the origin)
  float* Point2; // The position of point 2 (usually the terminus)
  svtkVector2f Position1, Position2;
  double TickInterval;              // Interval between tick marks in plot space
  int NumberOfTicks;                // The number of tick marks to draw
  float TickLength;                 // The length of the tick marks
  svtkTextProperty* LabelProperties; // Text properties for the labels.
  double Minimum;                   // Minimum value of the axis
  double Maximum;                   // Maximum values of the axis
  double MinimumLimit;              // Lowest possible value for Minimum
  double MaximumLimit;              // Highest possible value for Maximum
  double UnscaledMinimum;           // UnscaledMinimum value of the axis
  double UnscaledMaximum;           // UnscaledMaximum values of the axis
  double UnscaledMinimumLimit;      // Lowest possible value for UnscaledMinimum
  double UnscaledMaximumLimit;      // Highest possible value for UnscaledMaximum
  double NonLogUnscaledMinLimit;    // Saved UnscaledMinimumLimit (when !LogActive)
  double NonLogUnscaledMaxLimit;    // Saved UnscaledMinimumLimit (when !LogActive)
  int Margins[2];                   // Horizontal/vertical margins for the axis
  svtkStdString Title;               // The text label drawn on the axis
  svtkTextProperty* TitleProperties; // Text properties for the axis title
  bool LogScale;                    // *Should* the axis use a log scale?
  bool LogScaleActive;              // *Is* the axis using a log scale?
  bool GridVisible;                 // Whether the grid for the axis should be drawn
  bool LabelsVisible;               // Should the axis labels be visible
  bool RangeLabelsVisible;          // Should range labels be visible?
  float LabelOffset;                // Offset of label from the tick mark
  bool TicksVisible;                // Should the tick marks be visible.
  bool AxisVisible;                 // Should the axis line be visible.
  bool TitleVisible;                // Should the title be visible.
  int Precision;                    // Numerical precision to use, defaults to 2.
  int Notation;                     // The notation to use (standard, scientific, mixed)
  std::string LabelFormat;          // The printf-style format string used for labels.
  std::string RangeLabelFormat;     // The printf-style format string used for range labels.
  int Behavior;                     // The behaviour of the axis (auto, fixed, custom).
  float MaxLabel[2];                // The widest/tallest axis label.
  bool TitleAppended;               // Track if the title is updated when the label formats
                                    // are changed in the Extended Axis Labeling algorithm

  //@{
  /**
   * Scaling factor used on this axis, this is used to accurately render very
   * small/large numbers accurately by converting the underlying range by the
   * scaling factor.
   */
  double ScalingFactor;
  double Shift;
  //@}

  /**
   * Are we using custom tick labels, or should the axis generate them?
   */
  bool CustomTickLabels;

  /**
   * This object stores the svtkPen that controls how the axis is drawn.
   */
  svtkPen* Pen;

  /**
   * This object stores the svtkPen that controls how the grid lines are drawn.
   */
  svtkPen* GridPen;

  /**
   * Position of tick marks in screen coordinates
   */
  svtkSmartPointer<svtkDoubleArray> TickPositions;

  /**
   * Position of tick marks in screen coordinates
   */
  svtkSmartPointer<svtkFloatArray> TickScenePositions;

  /**
   * The labels for the tick marks
   */
  svtkSmartPointer<svtkStringArray> TickLabels;

  /**
   * Hint as to whether a nice min/max was set, otherwise labels may not be
   * present at the top/bottom of the axis.
   */
  bool UsingNiceMinMax;

  /**
   * Mark the tick labels as dirty when the min/max value is changed.
   */
  bool TickMarksDirty;

  /**
   * Flag to indicate that the axis has been resized.
   */
  bool Resized;

  /**
   * The algorithm being used to tick label placement.
   */
  int TickLabelAlgorithm;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

private:
  svtkAxis(const svtkAxis&) = delete;
  void operator=(const svtkAxis&) = delete;

  /**
   * Return true if the value is in range, false otherwise.
   */
  bool InRange(double value);
};

#endif // svtkAxis_h
