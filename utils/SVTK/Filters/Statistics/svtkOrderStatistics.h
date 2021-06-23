/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOrderStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkOrderStatistics
 * @brief   A class for univariate order statistics
 *
 *
 * Given a selection of columns of interest in an input data table, this
 * class provides the following functionalities, depending on the
 * execution mode it is executed in:
 * * Learn: calculate histogram.
 * * Derive: calculate PDFs and arbitrary quantiles. Provide specific names when 5-point
 *   statistics (minimum, 1st quartile, median, third quartile, maximum) requested.
 * * Assess: given an input data set and a set of q-quantiles, label each datum
 *   either with the quantile interval to which it belongs, or 0 if it is smaller
 *   than smaller quantile, or q if it is larger than largest quantile.
 * * Test: calculate Kolmogorov-Smirnov goodness-of-fit statistic between CDF based on
 *   model quantiles, and empirical CDF
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef svtkOrderStatistics_h
#define svtkOrderStatistics_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkStatisticsAlgorithm.h"

class svtkMultiBlockDataSet;
class svtkStringArray;
class svtkTable;
class svtkVariant;

class SVTKFILTERSSTATISTICS_EXPORT svtkOrderStatistics : public svtkStatisticsAlgorithm
{
public:
  svtkTypeMacro(svtkOrderStatistics, svtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkOrderStatistics* New();

  /**
   * The type of quantile definition.
   */
  enum QuantileDefinitionType
  {
    InverseCDF = 0,              // Identical to method 1 of R
    InverseCDFAveragedSteps = 1, // Identical to method 2 of R, ignored for non-numeric types
    NearestObservation = 2       // Identical to method 3 of R
  };

  //@{
  /**
   * Set/Get the number of quantiles (with uniform spacing).
   */
  svtkSetMacro(NumberOfIntervals, svtkIdType);
  svtkGetMacro(NumberOfIntervals, svtkIdType);
  //@}

  //@{
  /**
   * Set the quantile definition.
   */
  svtkSetMacro(QuantileDefinition, QuantileDefinitionType);
  void SetQuantileDefinition(int);
  //@}

  //@{
  /**
   * Set/Get whether quantization will be allowed to enforce maximum histogram size.
   */
  svtkSetMacro(Quantize, bool);
  svtkGetMacro(Quantize, bool);
  //@}

  //@{
  /**
   * Set/Get the maximum histogram size.
   * This maximum size is enforced only when Quantize is TRUE.
   */
  svtkSetMacro(MaximumHistogramSize, svtkIdType);
  svtkGetMacro(MaximumHistogramSize, svtkIdType);
  //@}

  /**
   * Get the quantile definition.
   */
  svtkIdType GetQuantileDefinition() { return static_cast<svtkIdType>(this->QuantileDefinition); }

  /**
   * A convenience method (in particular for access from other applications) to
   * set parameter values.
   * Return true if setting of requested parameter name was executed, false otherwise.
   */
  bool SetParameter(const char* parameter, int index, svtkVariant value) override;

  /**
   * Given a collection of models, calculate aggregate model
   * NB: not implemented
   */
  void Aggregate(svtkDataObjectCollection*, svtkMultiBlockDataSet*) override { return; }

protected:
  svtkOrderStatistics();
  ~svtkOrderStatistics() override;

  /**
   * Execute the calculations required by the Learn option.
   */
  void Learn(svtkTable*, svtkTable*, svtkMultiBlockDataSet*) override;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive(svtkMultiBlockDataSet*) override;

  /**
   * Execute the calculations required by the Test option.
   */
  void Test(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override;

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess(svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outData) override
  {
    this->Superclass::Assess(inData, inMeta, outData, 1);
  }

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor(svtkTable* outData, svtkDataObject* inMeta, svtkStringArray* rowNames,
    AssessFunctor*& dfunc) override;

  svtkIdType NumberOfIntervals;
  QuantileDefinitionType QuantileDefinition;
  bool Quantize;
  svtkIdType MaximumHistogramSize;

private:
  svtkOrderStatistics(const svtkOrderStatistics&) = delete;
  void operator=(const svtkOrderStatistics&) = delete;
};

#endif
