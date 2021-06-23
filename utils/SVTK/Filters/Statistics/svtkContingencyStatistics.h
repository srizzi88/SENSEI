/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkContingencyStatistics.h

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
 * @class   svtkContingencyStatistics
 * @brief   A class for bivariate correlation contigency
 * tables, conditional probabilities, and information entropy
 *
 *
 * Given a pair of columns of interest, this class provides the
 * following functionalities, depending on the operation in which it is executed:
 * * Learn: calculate contigency tables and corresponding discrete joint
 *   probability distribution.
 * * Derive: calculate conditional probabilities, information entropies, and
 *   pointwise mutual information.
 * * Assess: given two columns of interest with the same number of entries as
 *   input in port INPUT_DATA, and a corresponding bivariate probability distribution,
 * * Test: calculate Chi-square independence statistic and, if SVTK to R interface is available,
 *   retrieve corresponding p-value for independence testing.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef svtkContingencyStatistics_h
#define svtkContingencyStatistics_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkStatisticsAlgorithm.h"

class svtkMultiBlockDataSet;
class svtkStringArray;
class svtkTable;
class svtkVariant;
class svtkIdTypeArray;
class svtkDoubleArray;

class SVTKFILTERSSTATISTICS_EXPORT svtkContingencyStatistics : public svtkStatisticsAlgorithm
{
public:
  svtkTypeMacro(svtkContingencyStatistics, svtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkContingencyStatistics* New();

  /**
   * Given a collection of models, calculate aggregate model
   * NB: not implemented
   */
  void Aggregate(svtkDataObjectCollection*, svtkMultiBlockDataSet*) override { return; }

protected:
  svtkContingencyStatistics();
  ~svtkContingencyStatistics() override;

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
  void Assess(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override;

  /**
   * Calculate p-value. This will be overridden using the object factory with an
   * R implementation if R is present.
   */
  virtual void CalculatePValues(svtkTable*);

  /**
   * Provide the appropriate assessment functor.
   * This one does nothing because the API is not sufficient for tables indexed
   * by a separate summary table.
   */
  void SelectAssessFunctor(svtkTable* outData, svtkDataObject* inMeta, svtkStringArray* rowNames,
    AssessFunctor*& dfunc) override;
  /**
   * Provide the appropriate assessment functor.
   * This one is the one that is actually used.
   */
  virtual void SelectAssessFunctor(svtkTable* outData, svtkMultiBlockDataSet* inMeta,
    svtkIdType pairKey, svtkStringArray* rowNames, AssessFunctor*& dfunc);

private:
  svtkContingencyStatistics(const svtkContingencyStatistics&) = delete;
  void operator=(const svtkContingencyStatistics&) = delete;
};

#endif
