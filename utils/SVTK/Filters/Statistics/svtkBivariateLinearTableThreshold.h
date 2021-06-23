/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkBivariateLinearTableThreshold.h

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
 * @class   svtkBivariateLinearTableThreshold
 * @brief   performs line-based thresholding
 * for svtkTable data.
 *
 *
 * Class for filtering the rows of a two numeric columns of a svtkTable.  The
 * columns are treated as the two variables of a line.  This filter will
 * then iterate through the rows of the table determining if X,Y values pairs
 * are above/below/between/near one or more lines.
 *
 * The "between" mode checks to see if a row is contained within the convex
 * hull of all of the specified lines.  The "near" mode checks if a row is
 * within a distance threshold two one of the specified lines.  This class
 * is used in conjunction with various plotting classes, so it is useful
 * to rescale the X,Y axes to a particular range of values.  Distance
 * comparisons can be performed in the scaled space by setting the CustomRanges
 * ivar and enabling UseNormalizedDistance.
 */

#ifndef svtkBivariateLinearTableThreshold_h
#define svtkBivariateLinearTableThreshold_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkSmartPointer.h"            //Required for smart pointer internal ivars
#include "svtkTableAlgorithm.h"

class svtkDataArrayCollection;
class svtkDoubleArray;
class svtkIdTypeArray;
class svtkTable;

class SVTKFILTERSSTATISTICS_EXPORT svtkBivariateLinearTableThreshold : public svtkTableAlgorithm
{
public:
  static svtkBivariateLinearTableThreshold* New();
  svtkTypeMacro(svtkBivariateLinearTableThreshold, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Include the line in the threshold.  Essentially whether the threshold operation
   * uses > versus >=.
   */
  svtkSetMacro(Inclusive, int);
  svtkGetMacro(Inclusive, int);
  //@}

  /**
   * Add a numeric column to the pair of columns to be thresholded.  Call twice.
   */
  void AddColumnToThreshold(svtkIdType column, svtkIdType component);

  /**
   * Return how many columns have been added.  Hopefully 2.
   */
  int GetNumberOfColumnsToThreshold();

  /**
   * Return the column number from the input table for the idx'th added column.
   */
  void GetColumnToThreshold(svtkIdType idx, svtkIdType& column, svtkIdType& component);

  /**
   * Reset the columns to be thresholded.
   */
  void ClearColumnsToThreshold();

  /**
   * Get the output as a table of row ids.
   */
  svtkIdTypeArray* GetSelectedRowIds(int selection = 0);

  enum OutputPorts
  {
    OUTPUT_ROW_IDS = 0,
    OUTPUT_ROW_DATA
  };
  enum LinearThresholdType
  {
    BLT_ABOVE = 0,
    BLT_BELOW,
    BLT_NEAR,
    BLT_BETWEEN
  };

  /**
   * Reset the columns to threshold, column ranges, etc.
   */
  void Initialize();

  /**
   * Add a line for thresholding from two x,y points.
   */
  void AddLineEquation(double* p1, double* p2);

  /**
   * Add a line for thresholding in point-slope form.
   */
  void AddLineEquation(double* p, double slope);

  /**
   * Add a line for thresholding in implicit form (ax + by + c = 0)
   */
  void AddLineEquation(double a, double b, double c);

  /**
   * Reset the list of line equations.
   */
  void ClearLineEquations();

  //@{
  /**
   * Set the threshold type.  Above: find all rows that are above the specified
   * lines.  Below: find all rows that are below the specified lines.  Near:
   * find all rows that are near the specified lines.  Between: find all rows
   * that are between the specified lines.
   */
  svtkGetMacro(LinearThresholdType, int);
  svtkSetMacro(LinearThresholdType, int);
  void SetLinearThresholdTypeToAbove()
  {
    this->SetLinearThresholdType(svtkBivariateLinearTableThreshold::BLT_ABOVE);
  }
  void SetLinearThresholdTypeToBelow()
  {
    this->SetLinearThresholdType(svtkBivariateLinearTableThreshold::BLT_BELOW);
  }
  void SetLinearThresholdTypeToNear()
  {
    this->SetLinearThresholdType(svtkBivariateLinearTableThreshold::BLT_NEAR);
  }
  void SetLinearThresholdTypeToBetween()
  {
    this->SetLinearThresholdType(svtkBivariateLinearTableThreshold::BLT_BETWEEN);
  }
  //@}

  //@{
  /**
   * Manually access the maximum/minimum x,y values.  This is used in
   * conjunction with UseNormalizedDistance when determining if a row
   * passes the threshold.
   */
  svtkSetVector2Macro(ColumnRanges, double);
  svtkGetVector2Macro(ColumnRanges, double);
  //@}

  //@{
  /**
   * The Cartesian distance within which a point will pass the near threshold.
   */
  svtkSetMacro(DistanceThreshold, double);
  svtkGetMacro(DistanceThreshold, double);
  //@}

  //@{
  /**
   * Renormalize the space of the data such that the X and Y axes are
   * "square" over the specified ColumnRanges.  This essentially scales
   * the data space so that ColumnRanges[1]-ColumnRanges[0] = 1.0 and
   * ColumnRanges[3]-ColumnRanges[2] = 1.0.  Used for scatter plot distance
   * calculations.  Be sure to set DistanceThreshold accordingly, when used.
   */
  svtkSetMacro(UseNormalizedDistance, svtkTypeBool);
  svtkGetMacro(UseNormalizedDistance, svtkTypeBool);
  svtkBooleanMacro(UseNormalizedDistance, svtkTypeBool);
  //@}

  /**
   * Convert the two-point line formula to implicit form.
   */
  static void ComputeImplicitLineFunction(double* p1, double* p2, double* abc);

  /**
   * Convert the point-slope line formula to implicit form.
   */
  static void ComputeImplicitLineFunction(double* p, double slope, double* abc);

protected:
  svtkBivariateLinearTableThreshold();
  ~svtkBivariateLinearTableThreshold() override;

  double ColumnRanges[2];
  double DistanceThreshold;
  int Inclusive;
  int LinearThresholdType;
  int NumberOfLineEquations;
  svtkTypeBool UseNormalizedDistance;

  svtkSmartPointer<svtkDoubleArray> LineEquations;
  class Internals;
  Internals* Implementation;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Apply the current threshold to a svtkTable.  Fills acceptedIds on success.
   */
  virtual int ApplyThreshold(svtkTable* tableToThreshold, svtkIdTypeArray* acceptedIds);

  //@{
  /**
   * Determine if x,y is above all specified lines.
   */
  int ThresholdAbove(double x, double y);

  /**
   * Determine if x,y is below all specified lines.
   */
  int ThresholdBelow(double x, double y);

  /**
   * Determine if x,y is near ONE specified line (not all).
   */
  int ThresholdNear(double x, double y);

  /**
   * Determine if x,y is between ANY TWO of the specified lines.
   */
  int ThresholdBetween(double x, double y);
  //@}

private:
  svtkBivariateLinearTableThreshold(const svtkBivariateLinearTableThreshold&) = delete;
  void operator=(const svtkBivariateLinearTableThreshold&) = delete;
};

#endif
