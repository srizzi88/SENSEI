/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkComputeHistogram2DOutliers.h

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
 * @class   svtkComputeHistogram2DOutliers
 * @brief   compute the outliers in a set
 *  of 2D histograms and extract the corresponding row data.
 *
 *
 *  This class takes a table and one or more svtkImageData histograms as input
 *  and computes the outliers in that data.  In general it does so by
 *  identifying histogram bins that are removed by a median (salt and pepper)
 *  filter and below a threshold.  This threshold is automatically identified
 *  to retrieve a number of outliers close to a user-determined value.  This
 *  value is set by calling SetPreferredNumberOfOutliers(int).
 *
 *  The image data input can come either as a multiple svtkImageData via the
 *  repeatable INPUT_HISTOGRAM_IMAGE_DATA port, or as a single
 *  svtkMultiBlockDataSet containing svtkImageData objects as blocks.  One
 *  or the other must be set, not both (or neither).
 *
 *  The output can be retrieved as a set of row ids in a svtkSelection or
 *  as a svtkTable containing the actual outlier row data.
 *
 * @sa
 *  svtkExtractHistogram2D svtkPComputeHistogram2DOutliers
 *
 * @par Thanks:
 *  Developed by David Feng at Sandia National Laboratories
 *------------------------------------------------------------------------------
 */

#ifndef svtkComputeHistogram2DOutliers_h
#define svtkComputeHistogram2DOutliers_h
//------------------------------------------------------------------------------
#include "svtkFiltersImagingModule.h" // For export macro
#include "svtkSelectionAlgorithm.h"

//------------------------------------------------------------------------------
class svtkCollection;
class svtkDoubleArray;
class svtkIdTypeArray;
class svtkImageData;
class svtkTable;
//------------------------------------------------------------------------------
class SVTKFILTERSIMAGING_EXPORT svtkComputeHistogram2DOutliers : public svtkSelectionAlgorithm
{
public:
  static svtkComputeHistogram2DOutliers* New();
  svtkTypeMacro(svtkComputeHistogram2DOutliers, svtkSelectionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkSetMacro(PreferredNumberOfOutliers, int);
  svtkGetMacro(PreferredNumberOfOutliers, int);

  //
  svtkTable* GetOutputTable();

  enum InputPorts
  {
    INPUT_TABLE_DATA = 0,
    INPUT_HISTOGRAMS_IMAGE_DATA,
    INPUT_HISTOGRAMS_MULTIBLOCK
  };
  enum OutputPorts
  {
    OUTPUT_SELECTED_ROWS = 0,
    OUTPUT_SELECTED_TABLE_DATA
  };

  /**
   * Set the source table data, from which data will be filtered.
   */
  void SetInputTableConnection(svtkAlgorithmOutput* cxn)
  {
    this->SetInputConnection(INPUT_TABLE_DATA, cxn);
  }

  /**
   * Set the input histogram data as a (repeatable) svtkImageData
   */
  void SetInputHistogramImageDataConnection(svtkAlgorithmOutput* cxn)
  {
    this->SetInputConnection(INPUT_HISTOGRAMS_IMAGE_DATA, cxn);
  }

  /**
   * Set the input histogram data as a svtkMultiBlockData set
   * containing multiple svtkImageData objects.
   */
  void SetInputHistogramMultiBlockConnection(svtkAlgorithmOutput* cxn)
  {
    this->SetInputConnection(INPUT_HISTOGRAMS_MULTIBLOCK, cxn);
  }

protected:
  svtkComputeHistogram2DOutliers();
  ~svtkComputeHistogram2DOutliers() override;

  int PreferredNumberOfOutliers;
  svtkTimeStamp BuildTime;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Compute the thresholds (essentially bin extents) that contain outliers for
   * a collection of svtkImageData histograms.
   */
  virtual int ComputeOutlierThresholds(svtkCollection* histograms, svtkCollection* thresholds);

  /**
   * Compute the thresholds (bin extents) that contain outliers for a single svtkImageData histogram
   */
  virtual int ComputeOutlierThresholds(
    svtkImageData* histogram, svtkDoubleArray* thresholds, double threshold);

  /**
   * Take a set of range thresholds (bin extents) and filter out rows from the input table data that
   * fits inside those thresholds.
   */
  virtual int FillOutlierIds(
    svtkTable* data, svtkCollection* thresholds, svtkIdTypeArray* rowIds, svtkTable* outTable);

private:
  svtkComputeHistogram2DOutliers(const svtkComputeHistogram2DOutliers&) = delete;
  void operator=(const svtkComputeHistogram2DOutliers&) = delete;
};

#endif
