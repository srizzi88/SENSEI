/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkCorrelativeStatistics.h

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
 * @class   svtkCorrelativeStatistics
 * @brief   A class for bivariate linear correlation
 *
 *
 * Given a selection of pairs of columns of interest, this class provides the
 * following functionalities, depending on the chosen execution options:
 * * Learn: calculate sample mean and M2 aggregates for each pair of variables
 *   (cf. P. Pebay, Formulas for robust, one-pass parallel computation of covariances
 *   and Arbitrary-Order Statistical Moments, Sandia Report SAND2008-6212, Sep 2008,
 *   http://infoserve.sandia.gov/sand_doc/2008/086212.pdf for details)
 * * Derive: calculate unbiased covariance matrix estimators and its determinant,
 *   linear regressions, and Pearson correlation coefficient.
 * * Assess: given an input data set, two means and a 2x2 covariance matrix,
 *   mark each datum with corresponding relative deviation (2-dimensional Mahlanobis
 *   distance).
 * * Test: Perform Jarque-Bera-Srivastava test of 2-d normality
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef svtkCorrelativeStatistics_h
#define svtkCorrelativeStatistics_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkStatisticsAlgorithm.h"

class svtkMultiBlockDataSet;
class svtkStringArray;
class svtkTable;
class svtkVariant;
class svtkDoubleArray;

class SVTKFILTERSSTATISTICS_EXPORT svtkCorrelativeStatistics : public svtkStatisticsAlgorithm
{
public:
  svtkTypeMacro(svtkCorrelativeStatistics, svtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkCorrelativeStatistics* New();

  /**
   * Given a collection of models, calculate aggregate model
   */
  void Aggregate(svtkDataObjectCollection*, svtkMultiBlockDataSet*) override;

protected:
  svtkCorrelativeStatistics();
  ~svtkCorrelativeStatistics() override;

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
    this->Superclass::Assess(inData, inMeta, outData, 2);
  }

  /**
   * Calculate p-value. This will be overridden using the object factory with an
   * R implementation if R is present.
   */
  virtual svtkDoubleArray* CalculatePValues(svtkDoubleArray*);

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor(svtkTable* outData, svtkDataObject* inMeta, svtkStringArray* rowNames,
    AssessFunctor*& dfunc) override;

private:
  svtkCorrelativeStatistics(const svtkCorrelativeStatistics&) = delete;
  void operator=(const svtkCorrelativeStatistics&) = delete;
};

#endif
