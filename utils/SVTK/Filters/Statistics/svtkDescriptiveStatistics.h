/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkDescriptiveStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkDescriptiveStatistics
 * @brief   A class for univariate descriptive statistics
 *
 *
 * Given a selection of columns of interest in an input data table, this
 * class provides the following functionalities, depending on the chosen
 * execution options:
 * * Learn: calculate extremal values, sample mean, and M2, M3, and M4 aggregates
 *   (cf. P. Pebay, Formulas for robust, one-pass parallel computation of covariances
 *   and Arbitrary-Order Statistical Moments, Sandia Report SAND2008-6212, Sep 2008,
 *   http://infoserve.sandia.gov/sand_doc/2008/086212.pdf for details)
 * * Derive: calculate unbiased variance estimator, standard deviation estimator,
 *   two skewness estimators, and two kurtosis excess estimators.
 * * Assess: given an input data set, a reference value and a non-negative deviation,
 *   mark each datum with corresponding relative deviation (1-dimensional Mahlanobis
 *   distance). If the deviation is zero, then mark each datum which are equal to the
 *   reference value with 0, and all others with 1. By default, the reference value
 *   and the deviation are, respectively, the mean and the standard deviation of the
 *   input model.
 * * Test: calculate Jarque-Bera statistic and, if SVTK to R interface is available,
 *   retrieve corresponding p-value for normality testing.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef svtkDescriptiveStatistics_h
#define svtkDescriptiveStatistics_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkStatisticsAlgorithm.h"

class svtkMultiBlockDataSet;
class svtkStringArray;
class svtkTable;
class svtkVariant;
class svtkDoubleArray;

class SVTKFILTERSSTATISTICS_EXPORT svtkDescriptiveStatistics : public svtkStatisticsAlgorithm
{
public:
  svtkTypeMacro(svtkDescriptiveStatistics, svtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkDescriptiveStatistics* New();

  //@{
  /**
   * Set/get whether the unbiased estimator for the variance should be used, or if
   * the population variance will be calculated.
   * The default is that the unbiased estimator will be used.
   */
  svtkSetMacro(UnbiasedVariance, svtkTypeBool);
  svtkGetMacro(UnbiasedVariance, svtkTypeBool);
  svtkBooleanMacro(UnbiasedVariance, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get whether the G1 estimator for the skewness should be used, or if
   * the g1 skewness will be calculated.
   * The default is that the g1 skewness estimator will be used.
   */
  svtkSetMacro(G1Skewness, svtkTypeBool);
  svtkGetMacro(G1Skewness, svtkTypeBool);
  svtkBooleanMacro(G1Skewness, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get whether the G2 estimator for the kurtosis should be used, or if
   * the g2 kurtosis will be calculated.
   * The default is that the g2 kurtosis estimator will be used.
   */
  svtkSetMacro(G2Kurtosis, svtkTypeBool);
  svtkGetMacro(G2Kurtosis, svtkTypeBool);
  svtkBooleanMacro(G2Kurtosis, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get whether the deviations returned should be signed, or should
   * only have their magnitude reported.
   * The default is that signed deviations will be computed.
   */
  svtkSetMacro(SignedDeviations, svtkTypeBool);
  svtkGetMacro(SignedDeviations, svtkTypeBool);
  svtkBooleanMacro(SignedDeviations, svtkTypeBool);
  //@}

  /**
   * Given a collection of models, calculate aggregate model
   */
  void Aggregate(svtkDataObjectCollection*, svtkMultiBlockDataSet*) override;

protected:
  svtkDescriptiveStatistics();
  ~svtkDescriptiveStatistics() override;

  /**
   * Execute the calculations required by the Learn option, given some input Data
   * NB: input parameters are unused.
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
   * Calculate p-value. This will be overridden using the object factory with an
   * R implementation if R is present.
   */
  virtual svtkDoubleArray* CalculatePValues(svtkDoubleArray*);

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor(svtkTable* outData, svtkDataObject* inMeta, svtkStringArray* rowNames,
    AssessFunctor*& dfunc) override;

  svtkTypeBool UnbiasedVariance;
  svtkTypeBool G1Skewness;
  svtkTypeBool G2Kurtosis;
  svtkTypeBool SignedDeviations;

private:
  svtkDescriptiveStatistics(const svtkDescriptiveStatistics&) = delete;
  void operator=(const svtkDescriptiveStatistics&) = delete;
};

#endif
