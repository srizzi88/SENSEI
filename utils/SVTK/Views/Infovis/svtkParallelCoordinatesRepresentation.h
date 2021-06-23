/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelCoordinatesRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkParallelCoordinatesRepresentation
 * @brief   Data representation that
 *  takes generic multivariate data and produces a parallel coordinates plot.
 *
 *
 *  A parallel coordinates plot represents each variable in a multivariate
 *  data set as a separate axis.  Individual samples of that data set are
 *  represented as a polyline that pass through each variable axis at
 *  positions that correspond to data values.  svtkParallelCoordinatesRepresentation
 *  generates this plot when added to a svtkParallelCoordinatesView, which handles
 *  interaction and highlighting.  Sample polylines can alternatively
 *  be represented as s-curves by enabling the UseCurves flag.
 *
 *  There are three selection modes: lasso, angle, and function. Lasso selection
 *  picks sample lines that pass through a polyline.  Angle selection picks sample
 *  lines that have similar slope to a line segment.  Function selection picks
 *  sample lines that are near a linear function defined on two variables.  This
 *  function specified by passing two (x,y) variable value pairs.
 *
 *  All primitives are plotted in normalized view coordinates [0,1].
 *
 * @sa
 *  svtkParallelCoordinatesView svtkParallelCoordinatesHistogramRepresentation
 *  svtkSCurveSpline
 *
 * @par Thanks:
 *  Developed by David Feng at Sandia National Laboratories
 */

#ifndef svtkParallelCoordinatesRepresentation_h
#define svtkParallelCoordinatesRepresentation_h

#include "svtkRenderedRepresentation.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkActor;
class svtkActor2D;
class svtkArrayData;
class svtkAxisActor2D;
class svtkBivariateLinearTableThreshold;
class svtkCollection;
class svtkCoordinate;
class svtkExtractSelectedPolyDataIds;
class svtkFieldData;
class svtkDataArray;
class svtkDataObject;
class svtkDoubleArray;
class svtkIdList;
class svtkIdTypeArray;
class svtkIntArray;
class svtkLookupTable;
class svtkOutlineCornerSource;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkPropCollection;
class svtkSelection;
class svtkSelectionNode;
class svtkTextMapper;
class svtkTimeStamp;
class svtkUnsignedIntArray;
class svtkViewport;
class svtkWindow;

class SVTKVIEWSINFOVIS_EXPORT svtkParallelCoordinatesRepresentation : public svtkRenderedRepresentation
{
public:
  static svtkParallelCoordinatesRepresentation* New();
  svtkTypeMacro(svtkParallelCoordinatesRepresentation, svtkRenderedRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Apply the theme to this view.  CellColor is used for line coloring
   * and titles.  EdgeLabelColor is used for axis color. CellOpacity is
   * used for line opacity.
   */
  void ApplyViewTheme(svtkViewTheme* theme) override;

  /**
   * Returns the hover text at an x,y location.
   */
  virtual const char* GetHoverText(svtkView* view, int x, int y);

  //@{
  /**
   * Change the position of the plot
   */
  int SetPositionAndSize(double* position, double* size);
  int GetPositionAndSize(double* position, double* size);
  //@}

  //@{
  /**
   * Set/Get the axis titles
   */
  void SetAxisTitles(svtkStringArray*);
  void SetAxisTitles(svtkAlgorithmOutput*);
  //@}

  /**
   * Set the title for the entire plot
   */
  void SetPlotTitle(const char*);

  //@{
  /**
   * Get the number of axes in the plot
   */
  svtkGetMacro(NumberOfAxes, int);
  //@}

  //@{
  /**
   * Get the number of samples in the plot
   */
  svtkGetMacro(NumberOfSamples, int);
  //@}

  //@{
  /**
   * Set/Get the number of labels to display on each axis
   */
  void SetNumberOfAxisLabels(int num);
  svtkGetMacro(NumberOfAxisLabels, int);
  //@}

  //@{
  /**
   * Move an axis to a particular screen position.  Using these
   * methods requires an Update() before they will work properly.
   */
  virtual int SwapAxisPositions(int position1, int position2);
  int SetXCoordinateOfPosition(int position, double xcoord);
  double GetXCoordinateOfPosition(int axis);
  void GetXCoordinatesOfPositions(double* coords);
  int GetPositionNearXCoordinate(double xcoord);
  //@}

  //@{
  /**
   * Whether or not to display using curves
   */
  svtkSetMacro(UseCurves, svtkTypeBool);
  svtkGetMacro(UseCurves, svtkTypeBool);
  svtkBooleanMacro(UseCurves, svtkTypeBool);
  //@}

  //@{
  /**
   * Resolution of the curves displayed, enabled by setting UseCurves
   */
  svtkSetMacro(CurveResolution, int);
  svtkGetMacro(CurveResolution, int);
  //@}

  //@{
  /**
   * Access plot properties
   */
  svtkGetMacro(LineOpacity, double);
  svtkGetMacro(FontSize, double);
  svtkGetVector3Macro(LineColor, double);
  svtkGetVector3Macro(AxisColor, double);
  svtkGetVector3Macro(AxisLabelColor, double);
  svtkSetMacro(LineOpacity, double);
  svtkSetMacro(FontSize, double);
  svtkSetVector3Macro(LineColor, double);
  svtkSetVector3Macro(AxisColor, double);
  svtkSetVector3Macro(AxisLabelColor, double);
  //@}

  //@{
  /**
   * Maximum angle difference (in degrees) of selection using angle/function brushes
   */
  svtkSetMacro(AngleBrushThreshold, double);
  svtkGetMacro(AngleBrushThreshold, double);
  //@}

  //@{
  /**
   * Maximum angle difference (in degrees) of selection using angle/function brushes
   */
  svtkSetMacro(FunctionBrushThreshold, double);
  svtkGetMacro(FunctionBrushThreshold, double);
  //@}

  //@{
  /**
   * Set/get the value range of the axis at a particular screen position
   */
  int GetRangeAtPosition(int position, double range[2]);
  virtual int SetRangeAtPosition(int position, double range[2]);
  //@}

  /**
   * Reset the axes to their default positions and orders
   */
  void ResetAxes();

  //@{
  /**
   * Do a selection of the lines.  See the main description for how to use these functions.
   * RangeSelect is currently stubbed out.
   */
  virtual void LassoSelect(int brushClass, int brushOperator, svtkPoints* brushPoints);
  virtual void AngleSelect(int brushClass, int brushOperator, double* p1, double* p2);
  virtual void FunctionSelect(
    int brushClass, int brushOperator, double* p1, double* p2, double* q1, double* q2);
  virtual void RangeSelect(int brushClass, int brushOperator, double* p1, double* p2);
  //@}

  enum InputPorts
  {
    INPUT_DATA = 0,
    INPUT_TITLES,
    NUM_INPUT_PORTS
  };

protected:
  svtkParallelCoordinatesRepresentation();
  ~svtkParallelCoordinatesRepresentation() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  //@{
  /**
   * Add/remove the props and actors to/from a view
   */
  bool AddToView(svtkView* view) override;
  bool RemoveFromView(svtkView* view) override;
  void PrepareForRendering(svtkRenderView* view) override;
  //@}

  /**
   * This function is not actually used, but as left as a stub in case
   * it becomes useful at some point.
   */
  void UpdateHoverHighlight(svtkView* view, int x, int y);

  /**
   * Allocate the cells/points/scalars for a svtkPolyData
   */
  virtual int AllocatePolyData(svtkPolyData* polyData, int numLines, int numPointsPerLine,
    int numStrips, int numPointsPerStrip, int numQuads, int numPoints, int numCellScalars,
    int numPointScalars);

  /**
   * Put the axis actors in their correct positions.
   */
  int PlaceAxes();

  //@{
  /**
   * Place line primitives into a svtkPolyData from the input data.  idsToPlot
   * is a list of which rows/samples should be plotted.  If nullptr, all
   * rows/samples are plotted.
   */
  virtual int PlaceLines(svtkPolyData* polyData, svtkTable* data, svtkIdTypeArray* idsToPlot);
  virtual int PlaceCurves(svtkPolyData* polyData, svtkTable* data, svtkIdTypeArray* idsToPlot);
  //@}

  /**
   * Takes the selection list (assumed to be a svtkIdTypeArray) from a svtkSelectionNode
   * and plots lines/curves into polyData for just those row/sample ids.
   */
  virtual int PlaceSelection(
    svtkPolyData* polyData, svtkTable* data, svtkSelectionNode* selectionNode);

  /**
   * Compute the number of axes and their individual ranges
   */
  virtual int ComputeDataProperties();

  /**
   * Set plot actor properties (line thickness, opacity, etc)
   */
  virtual int UpdatePlotProperties(svtkStringArray* inputTitles);

  /**
   * Delete and reallocate the internals, resetting to default values
   */
  virtual int ReallocateInternals();

  //@{
  /**
   * Compute which screen position a point belongs to (returns the left position)
   */
  int ComputePointPosition(double* p);
  int ComputeLinePosition(double* p1, double* p2);
  //@}

  //@{
  /**
   * Select a set of points using the prescribed operator (add, subtract, etc.) and class
   */
  virtual void SelectRows(svtkIdType brushClass, svtkIdType brushOperator, svtkIdTypeArray* rowIds);
  svtkSelection* ConvertSelection(svtkView* view, svtkSelection* selection) override;
  virtual void BuildInverseSelection();
  virtual svtkPolyDataMapper2D* InitializePlotMapper(
    svtkPolyData* input, svtkActor2D* actor, bool forceStandard = false);
  //@}

  /**
   * Build an s-curve passing through (0,0) and (1,1) with a specified number of
   * values.  This is used as a lookup table when plotting curved primitives.
   */
  void BuildDefaultSCurve(svtkDoubleArray* array, int numValues);

  /**
   * same as public version, but assumes that the brushpoints coming in
   * are all within two neighboring axes.
   */
  virtual void LassoSelectInternal(svtkPoints* brushPoints, svtkIdTypeArray* outIds);

  /**
   * todo
   */
  virtual void UpdateSelectionActors();

  svtkPolyDataMapper2D* GetSelectionMapper(int idx);
  int GetNumberOfSelections();

  svtkSmartPointer<svtkPolyData> PlotData;
  svtkSmartPointer<svtkPolyDataMapper2D> PlotMapper;
  svtkSmartPointer<svtkActor2D> PlotActor;
  svtkSmartPointer<svtkTextMapper> PlotTitleMapper;
  svtkSmartPointer<svtkActor2D> PlotTitleActor;
  svtkSmartPointer<svtkTextMapper> FunctionTextMapper;
  svtkSmartPointer<svtkActor2D> FunctionTextActor;

  svtkSmartPointer<svtkSelection> InverseSelection;
  svtkSmartPointer<svtkBivariateLinearTableThreshold> LinearThreshold;

  class Internals;
  Internals* I;

  int NumberOfAxes;
  int NumberOfAxisLabels;
  int NumberOfSamples;
  double YMin;
  double YMax;

  int CurveResolution;
  svtkTypeBool UseCurves;
  double AngleBrushThreshold;
  double FunctionBrushThreshold;
  double SwapThreshold;

  // Indexed by screen position
  double* Xs;
  double* Mins;
  double* Maxs;
  double* MinOffsets;
  double* MaxOffsets;

  svtkSmartPointer<svtkAxisActor2D>* Axes;
  svtkSmartPointer<svtkTable> InputArrayTable;
  svtkSmartPointer<svtkStringArray> AxisTitles;

  svtkTimeStamp BuildTime;

  double LineOpacity;
  double FontSize;
  double LineColor[3];
  double AxisColor[3];
  double AxisLabelColor[3];

  svtkGetStringMacro(InternalHoverText);
  svtkSetStringMacro(InternalHoverText);
  char* InternalHoverText;

private:
  svtkParallelCoordinatesRepresentation(const svtkParallelCoordinatesRepresentation&) = delete;
  void operator=(const svtkParallelCoordinatesRepresentation&) = delete;
};

#endif
