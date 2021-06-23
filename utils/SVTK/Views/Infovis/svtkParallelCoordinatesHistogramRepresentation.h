/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelCoordinatesHistogramRepresentation.h

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
 * @class   svtkParallelCoordinatesHistogramRepresentation
 * @brief   Data representation
 *  that takes generic multivariate data and produces a parallel coordinates plot.
 *  This plot optionally can draw a histogram-based plot summary.
 *
 *
 *  A parallel coordinates plot represents each variable in a multivariate
 *  data set as a separate axis.  Individual samples of that data set are
 *  represented as a polyline that pass through each variable axis at
 *  positions that correspond to data values.  This class can generate
 *  parallel coordinates plots identical to its superclass
 *  (svtkParallelCoordinatesRepresentation) and has the same interaction
 *  styles.
 *
 *  In addition to the standard parallel coordinates plot, this class also
 *  can draw a histogram summary of the parallel coordinates plot.
 *  Rather than draw every row in an input data set, first it computes
 *  a 2D histogram for all neighboring variable axes, then it draws
 *  bar (thickness corresponds to bin size) for each bin the histogram
 *  with opacity weighted by the number of rows contained in the bin.
 *  The result is essentially a density map.
 *
 *  Because this emphasizes dense regions over sparse outliers, this class
 *  also uses a svtkComputeHistogram2DOutliers instance to identify outlier
 *  table rows and draws those as standard parallel coordinates lines.
 *
 * @sa
 *  svtkParallelCoordinatesView svtkParallelCoordinatesRepresentation
 *  svtkExtractHistogram2D svtkComputeHistogram2DOutliers
 *
 * @par Thanks:
 *  Developed by David Feng at Sandia National Laboratories
 */

#ifndef svtkParallelCoordinatesHistogramRepresentation_h
#define svtkParallelCoordinatesHistogramRepresentation_h

#include "svtkParallelCoordinatesRepresentation.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkComputeHistogram2DOutliers;
class svtkPairwiseExtractHistogram2D;
class svtkExtractHistogram2D;
class svtkInformationVector;
class svtkLookupTable;

class SVTKVIEWSINFOVIS_EXPORT svtkParallelCoordinatesHistogramRepresentation
  : public svtkParallelCoordinatesRepresentation
{
public:
  static svtkParallelCoordinatesHistogramRepresentation* New();
  svtkTypeMacro(svtkParallelCoordinatesHistogramRepresentation, svtkParallelCoordinatesRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Apply the theme to this view.
   */
  void ApplyViewTheme(svtkViewTheme* theme) override;

  //@{
  /**
   * Whether to use the histogram rendering mode or the superclass's line rendering mode
   */
  virtual void SetUseHistograms(svtkTypeBool);
  svtkGetMacro(UseHistograms, svtkTypeBool);
  svtkBooleanMacro(UseHistograms, svtkTypeBool);
  //@}

  //@{
  /**
   * Whether to compute and show outlier lines
   */
  virtual void SetShowOutliers(svtkTypeBool);
  svtkGetMacro(ShowOutliers, svtkTypeBool);
  svtkBooleanMacro(ShowOutliers, svtkTypeBool);
  //@}

  //@{
  /**
   * Control over the range of the lookup table used to draw the histogram quads.
   */
  svtkSetVector2Macro(HistogramLookupTableRange, double);
  svtkGetVector2Macro(HistogramLookupTableRange, double);
  //@}

  //@{
  /**
   * The number of histogram bins on either side of each pair of axes.
   */
  void SetNumberOfHistogramBins(int, int);
  void SetNumberOfHistogramBins(int*);
  svtkGetVector2Macro(NumberOfHistogramBins, int);
  //@}

  //@{
  /**
   * Target maximum number of outliers to be drawn, although not guaranteed.
   */
  void SetPreferredNumberOfOutliers(int);
  svtkGetMacro(PreferredNumberOfOutliers, int);
  //@}

  /**
   * Calls superclass swap, and assures that only histograms affected by the
   * swap get recomputed.
   */
  int SwapAxisPositions(int position1, int position2) override;

  /**
   * Calls the superclass method, and assures that only the two histograms
   * affect by this call get recomputed.
   */
  int SetRangeAtPosition(int position, double range[2]) override;

protected:
  svtkParallelCoordinatesHistogramRepresentation();
  ~svtkParallelCoordinatesHistogramRepresentation() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool AddToView(svtkView* view) override;

  bool RemoveFromView(svtkView* view) override;

  /**
   * Flag deciding if histograms will be drawn.
   */
  svtkTypeBool UseHistograms;

  /**
   * The range applied to the lookup table used to draw histogram quads
   */
  double HistogramLookupTableRange[2];

  /**
   * How many bins are used during the 2D histogram computation
   */
  int NumberOfHistogramBins[2];

  svtkSmartPointer<svtkPairwiseExtractHistogram2D> HistogramFilter;
  svtkSmartPointer<svtkLookupTable> HistogramLookupTable;

  /**
   * Whether or not to draw outlier lines
   */
  svtkTypeBool ShowOutliers;

  /**
   * How many outlier lines to draw, approximately.
   */
  int PreferredNumberOfOutliers;

  svtkSmartPointer<svtkComputeHistogram2DOutliers> OutlierFilter;
  svtkSmartPointer<svtkPolyData> OutlierData;
  svtkSmartPointer<svtkPolyDataMapper2D> OutlierMapper;
  svtkSmartPointer<svtkActor2D> OutlierActor;

  /**
   * Correctly forwards the superclass call to draw lines to the internal
   * PlaceHistogramLineQuads call.
   */
  int PlaceLines(svtkPolyData* polyData, svtkTable* data, svtkIdTypeArray* idsToPlot) override;

  /**
   * Correctly forwards the superclass call to draw curves to the internal
   * PlaceHistogramLineCurves call.
   */
  int PlaceCurves(svtkPolyData* polyData, svtkTable* data, svtkIdTypeArray* idsToPlot) override;

  /**
   * Draw a selection node referencing the row ids of a table into a poly data object.
   */
  int PlaceSelection(
    svtkPolyData* polyData, svtkTable* data, svtkSelectionNode* selectionNode) override;

  /**
   * Take the input 2D histogram images and draw one quad for each bin
   */
  virtual int PlaceHistogramLineQuads(svtkPolyData* polyData);

  /**
   * Take the input 2D histogram images and draw one triangle strip that
   * is the curved version of the regular quad drawn via PlaceHistogramLineQuads
   */
  virtual int PlaceHistogramCurveQuads(svtkPolyData* polyData);

  //@{
  /**
   * Compute the number of axes and their individual ranges, as well
   * as histograms if requested.
   */
  int ComputeDataProperties() override;
  int UpdatePlotProperties(svtkStringArray*) override;
  //@}

  /**
   * Access the input data object containing the histograms and
   * pull out the image data for the idx'th histogram.
   */
  virtual svtkImageData* GetHistogramImage(int idx);

  /**
   * get the table containing just the outlier rows from the input table.
   */
  virtual svtkTable* GetOutlierData();

private:
  svtkParallelCoordinatesHistogramRepresentation(
    const svtkParallelCoordinatesHistogramRepresentation&) = delete;
  void operator=(const svtkParallelCoordinatesHistogramRepresentation&) = delete;
};

#endif
