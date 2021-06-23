/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScatterPlotMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkScatterPlotMatrix
 * @brief   container for a matrix of charts.
 *
 *
 * This class contains a matrix of charts. These charts will be of type
 * svtkChartXY by default, but this can be overridden. The class will manage
 * their layout and object lifetime.
 */

#ifndef svtkScatterPlotMatrix_h
#define svtkScatterPlotMatrix_h

#include "svtkChartMatrix.h"
#include "svtkChartsCoreModule.h" // For export macro
#include "svtkColor.h"            // For member function return
#include "svtkNew.h"              // For ivars
#include "svtkSmartPointer.h"     // For ivars
#include "svtkStdString.h"        // For ivars
#include "svtkWeakPointer.h"      // For currentPainter

class svtkStringArray;
class svtkTable;
class svtkAxis;
class svtkAnnotationLink;
class svtkTextProperty;
class svtkTooltipItem;
class svtkRenderWindowInteractor;

class SVTKCHARTSCORE_EXPORT svtkScatterPlotMatrix : public svtkChartMatrix
{
public:
  enum
  {
    SCATTERPLOT,
    HISTOGRAM,
    ACTIVEPLOT,
    NOPLOT
  };

  svtkTypeMacro(svtkScatterPlotMatrix, svtkChartMatrix);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a new object.
   */
  static svtkScatterPlotMatrix* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   */
  void Update() override;

  /**
   * Paint event for the chart matrix.
   */
  bool Paint(svtkContext2D* painter) override;

  void SetScene(svtkContextScene* scene) override;

  /**
   * Set the active plot, the one that will be displayed in the top-right.
   * This defaults to (0, n-2), the plot below the first histogram on the left.
   * \return false is the position specified is not valid.
   */
  virtual bool SetActivePlot(const svtkVector2i& position);

  /**
   * Reset ActivePlotSet flag and call superclass method
   */
  void SetSize(const svtkVector2i& size) override;

  /**
   * Get the position of the active plot.
   */
  virtual svtkVector2i GetActivePlot();

  /**
   * Get the AnnotationLink for the scatter plot matrix, this gives you access
   * to the currently selected points in the scatter plot matrix.
   */
  svtkAnnotationLink* GetAnnotationLink();

  /**
   * Set the input table for the scatter plot matrix. This will cause all
   * columns to be plotted against each other - a square scatter plot matrix.
   */
  virtual void SetInput(svtkTable* table);

  /**
   * Set the visibility of the specified column.
   */
  void SetColumnVisibility(const svtkStdString& name, bool visible);

  /**
   * Insert the specified column at the index position of
   * the visible columns.
   */
  void InsertVisibleColumn(const svtkStdString& name, int index);

  /**
   * Get the visibility of the specified column.
   */
  bool GetColumnVisibility(const svtkStdString& name);

  /**
   * Set the visibility of all columns (true will make them all visible, false
   * will remove all visible columns).
   */
  void SetColumnVisibilityAll(bool visible);

  /**
   * Get a list of the columns, and the order in which they are displayed.
   */
  virtual svtkStringArray* GetVisibleColumns();

  /**
   * Set the list of visible columns, and the order in which they will be displayed.
   */
  virtual void SetVisibleColumns(svtkStringArray* visColumns);

  /**
   * Set the number of bins in the histograms along the central diagonal of the
   * scatter plot matrix.
   */
  virtual void SetNumberOfBins(int numberOfBins);

  /**
   * Get the number of bins the histograms along the central diagonal scatter
   * plot matrix. The default value is 10.
   */
  virtual int GetNumberOfBins() const { return this->NumberOfBins; }

  /**
   * Set the color for the specified plotType.
   */
  void SetPlotColor(int plotType, const svtkColor4ub& color);

  /**
   * Sets the marker style for the specified plotType.
   */
  void SetPlotMarkerStyle(int plotType, int style);

  /**
   * Sets the marker size for the specified plotType.
   */
  void SetPlotMarkerSize(int plotType, float size);

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse) override;

  //@{
  /**
   * Returns the type of the plot at the given position. The return
   * value is one of: SCATTERPLOT, HISTOGRAM, ACTIVEPLOT, or NOPLOT.
   */
  int GetPlotType(const svtkVector2i& pos);
  int GetPlotType(int row, int column);
  //@}

  //@{
  /**
   * Set/get the scatter plot title.
   */
  void SetTitle(const svtkStdString& title);
  svtkStdString GetTitle();
  //@}

  //@{
  /**
   * Set/get the text properties for the chart title, i.e. color, font, size.
   */
  void SetTitleProperties(svtkTextProperty* prop);
  svtkTextProperty* GetTitleProperties();
  //@}

  //@{
  /**
   * Sets whether or not the grid for the given axis is visible given a plot
   * type, which refers to
   * svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetGridVisibility(int plotType, bool visible);
  bool GetGridVisibility(int plotType);
  //@}

  //@{
  /**
   * Sets the background color for the chart given a plot type, which refers to
   * svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetBackgroundColor(int plotType, const svtkColor4ub& color);
  svtkColor4ub GetBackgroundColor(int plotType);
  //@}

  //@{
  /**
   * Sets the color for the axes given a plot type, which refers to
   * svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetAxisColor(int plotType, const svtkColor4ub& color);
  svtkColor4ub GetAxisColor(int plotType);
  //@}

  //@{
  /**
   * Sets the color for the axes given a plot type, which refers to
   * svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetGridColor(int plotType, const svtkColor4ub& color);
  svtkColor4ub GetGridColor(int plotType);
  //@}

  //@{
  /**
   * Sets whether or not the labels for the axes are visible, given a plot type,
   * which refers to
   * svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetAxisLabelVisibility(int plotType, bool visible);
  bool GetAxisLabelVisibility(int plotType);
  //@}

  //@{
  /**
   * Set/get the text property for the axis labels of the given plot type,
   * possible types are svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetAxisLabelProperties(int plotType, svtkTextProperty* prop);
  svtkTextProperty* GetAxisLabelProperties(int plotType);
  //@}

  //@{
  /**
   * Sets the axis label notation for the axes given a plot type, which refers to
   * svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetAxisLabelNotation(int plotType, int notation);
  int GetAxisLabelNotation(int plotType);
  //@}

  //@{
  /**
   * Sets the axis label precision for the axes given a plot type, which refers to
   * svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetAxisLabelPrecision(int plotType, int precision);
  int GetAxisLabelPrecision(int plotType);
  //@}

  //@{
  /**
   * Set chart's tooltip notation and precision, given a plot type, which refers to
   * svtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
   */
  void SetTooltipNotation(int plotType, int notation);
  void SetTooltipPrecision(int plotType, int precision);
  int GetTooltipNotation(int plotType);
  int GetTooltipPrecision(int plotType);
  //@}

  /**
   * Set the svtkTooltipItem object that will be displayed by the active chart.
   */
  void SetTooltip(svtkTooltipItem* tooltip);

  /**
   * Get the svtkTooltipItem object that will be displayed by the active chart.
   */
  svtkTooltipItem* GetTooltip() const;

  /**
   * Set indexed labels array.
   */
  void SetIndexedLabels(svtkStringArray* labels);

  /**
   * Get the indexed labels array.
   */
  svtkStringArray* GetIndexedLabels() const;

  //@{
  /**
   * Set the scatter plot selected row/column charts' background color.
   */
  void SetScatterPlotSelectedRowColumnColor(const svtkColor4ub& color);
  svtkColor4ub GetScatterPlotSelectedRowColumnColor();
  //@}

  //@{
  /**
   * Set the scatter plot selected active chart background color.
   */
  void SetScatterPlotSelectedActiveColor(const svtkColor4ub& color);
  svtkColor4ub GetScatterPlotSelectedActiveColor();
  //@}

  /**
   * Convenient method to update all the chart settings
   */
  void UpdateSettings();

  /**
   * Update charts based on settings given the plot type
   */
  void UpdateChartSettings(int plotType);

  //@{
  /**
   * Set/get the Selection Mode that will be used by the chart while doing
   * selection. The only valid enums are svtkContextScene::SELECTION_NONE,
   * SELECTION_DEFAULT, SELECTION_ADDITION, SELECTION_SUBTRACTION, SELECTION_TOGGLE
   */
  virtual void SetSelectionMode(int);
  svtkGetMacro(SelectionMode, int);
  //@}

  /**
   * Get the column name for the supplied index.
   */
  svtkStdString GetColumnName(int column);

  /**
   * Get the column name for the supplied index.
   */
  svtkStdString GetRowName(int row);

  /**
   * Set the number of animation frames in each transition. Default is 25,
   * and 0 means to animations between axes.
   */
  void SetNumberOfFrames(int frames);

  /**
   * Get the number of animation frames in each transition. Default is 25,
   * and 0 means to animations between axes.
   */
  int GetNumberOfFrames();

  /**
   * Clear the animation path.
   */
  void ClearAnimationPath();

  /**
   * Add a move to the animation path. Note that a move can only change i or j,
   * not both. If the proposed move does not satisfy those criteria it will
   * be rejected and the animation path will not be extended.
   */
  bool AddAnimationPath(const svtkVector2i& move);

  /**
   * Get the number of elements (transitions) in the animation path.
   */
  svtkIdType GetNumberOfAnimationPathElements();

  /**
   * Get the element specified from the animation path.
   */
  svtkVector2i GetAnimationPathElement(svtkIdType i);

  /**
   * Trigger the animation of the scatter plot matrix to begin.
   */
  bool BeginAnimationPath(svtkRenderWindowInteractor* interactor);

  /**
   * Advance the animation in response to the timer events. This is public to
   * allow the animation to be manually advanced when timers are not a
   */
  virtual void AdvanceAnimation();

  /**
   * Get the main plot (the one in the top-right of the matrix.
   */
  virtual svtkChart* GetMainChart();

protected:
  svtkScatterPlotMatrix();
  ~svtkScatterPlotMatrix() override;

  /**
   * Internal helper to do the layout of the charts in the scatter plot matrix.
   */
  void UpdateLayout();

  /**
   * Compute and set big chart resize
   */
  void ResizeBigChart();

  //@{
  /**
   * Attach axis range listener so we can forward to dependent axes in matrix.
   */
  void AttachAxisRangeListener(svtkAxis*);
  void AxisRangeForwarderCallback(svtkObject*, unsigned long, void*);
  //@}

  /**
   * The callback function when SelectionChangedEvent is invoked from
   * the Big chart. This class will just forward the event.
   */
  void BigChartSelectionCallback(svtkObject*, unsigned long, void*);

  /**
   * Given a new position for the active plot, calculate a
   * an animation path from the old active plot to the new
   * active plot.
   */
  virtual void UpdateAnimationPath(const svtkVector2i& newActivePos);

  /**
   * Given the render window interactor, start animation of the
   * animation path calculated above.
   */
  virtual void StartAnimation(svtkRenderWindowInteractor* interactor);

  /**
   * Process events and dispatch to the appropriate member functions.
   */
  static void ProcessEvents(
    svtkObject* caller, unsigned long event, void* clientData, void* callerData);

  /**
   * Called when drawing a chart, does nothing at this level.
   */
  virtual void AddSupplementaryPlot(svtkChart* svtkNotUsed(chart), int svtkNotUsed(plotType),
    svtkStdString svtkNotUsed(row), svtkStdString svtkNotUsed(column), int svtkNotUsed(plotCorner) = 0)
  {
  }

  // The position of the active plot (defaults to 0, 1).
  svtkVector2i ActivePlot;

  // A flag to show if the ActivePlot vector is valid or not
  bool ActivePlotValid;

  // Weakly owned input data for the scatter plot matrix.
  svtkSmartPointer<svtkTable> Input;

  // Strongly owned internal data for the column visibility.
  svtkNew<svtkStringArray> VisibleColumns;

  // The number of bins in the histograms.
  int NumberOfBins;

  // The title of the scatter plot matrix.
  svtkStdString Title;
  svtkSmartPointer<svtkTextProperty> TitleProperties;

  // The mode when the chart is doing selection.
  int SelectionMode;

  // How many frames should animations consist of, 0 means no transitions.
  int NumberOfFrames;

  // A flag to know if we are animating the scatter plot along an animation path
  bool Animating;

private:
  svtkScatterPlotMatrix(const svtkScatterPlotMatrix&) = delete;
  void operator=(const svtkScatterPlotMatrix&) = delete;

  class PIMPL;
  PIMPL* Private;
  friend class PIMPL;

  svtkWeakPointer<svtkContext2D> CurrentPainter;
  svtkMTimeType LayoutUpdatedTime;

  // Go through the process of calculating axis ranges, etc...
  void UpdateAxes();
  void ApplyAxisSetting(svtkChart* chart, const svtkStdString& x, const svtkStdString& y);
};

#endif // svtkScatterPlotMatrix_h
