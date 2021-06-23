/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPairwiseExtractHistogram2D.h

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
 * @class   svtkPairwiseExtractHistogram2D
 * @brief   compute a 2D histogram between
 *  all adjacent columns of an input svtkTable.
 *
 *
 *  This class computes a 2D histogram between all adjacent pairs of columns
 *  of an input svtkTable. Internally it creates multiple svtkExtractHistogram2D
 *  instances (one for each pair of adjacent table columns).  It also
 *  manages updating histogram computations intelligently, only recomputing
 *  those histograms for whom a relevant property has been altered.
 *
 *  Note that there are two different outputs from this filter.  One is a
 *  table for which each column contains a flattened 2D histogram array.
 *  The other is a svtkMultiBlockDataSet for which each block is a
 *  svtkImageData representation of the 2D histogram.
 *
 * @sa
 *  svtkExtractHistogram2D svtkPPairwiseExtractHistogram2D
 *
 * @par Thanks:
 *  Developed by David Feng and Philippe Pebay at Sandia National Laboratories
 *------------------------------------------------------------------------------
 */

#ifndef svtkPairwiseExtractHistogram2D_h
#define svtkPairwiseExtractHistogram2D_h

#include "svtkFiltersImagingModule.h" // For export macro
#include "svtkSmartPointer.h"         //needed for smart pointer ivars
#include "svtkStatisticsAlgorithm.h"
class svtkCollection;
class svtkExtractHistogram2D;
class svtkImageData;
class svtkIdTypeArray;
class svtkMultiBlockDataSet;

class SVTKFILTERSIMAGING_EXPORT svtkPairwiseExtractHistogram2D : public svtkStatisticsAlgorithm
{
public:
  static svtkPairwiseExtractHistogram2D* New();
  svtkTypeMacro(svtkPairwiseExtractHistogram2D, svtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the bin dimensions of the histograms to compute
   */
  svtkSetVector2Macro(NumberOfBins, int);
  svtkGetVector2Macro(NumberOfBins, int);
  //@}

  //@{
  /**
   * Strange method for setting an index to be used for setting custom
   * column range. This was (probably) necessary to get this class
   * to interact with the ParaView client/server message passing interface.
   */
  svtkSetMacro(CustomColumnRangeIndex, int);
  void SetCustomColumnRangeByIndex(double, double);
  //@}

  //@{
  /**
   * More standard way to set the custom range for a particular column.
   * This makes sure that only the affected histograms know that they
   * need to be updated.
   */
  void SetCustomColumnRange(int col, double range[2]);
  void SetCustomColumnRange(int col, double rmin, double rmax);
  //@}

  //@{
  /**
   * Set the scalar type for each of the computed histograms.
   */
  svtkSetMacro(ScalarType, int);
  void SetScalarTypeToUnsignedInt() { this->SetScalarType(SVTK_UNSIGNED_INT); }
  void SetScalarTypeToUnsignedLong() { this->SetScalarType(SVTK_UNSIGNED_LONG); }
  void SetScalarTypeToUnsignedShort() { this->SetScalarType(SVTK_UNSIGNED_SHORT); }
  void SetScalarTypeToUnsignedChar() { this->SetScalarType(SVTK_UNSIGNED_CHAR); }
  svtkGetMacro(ScalarType, int);
  //@}

  /**
   * Get the maximum bin count for a single histogram
   */
  double GetMaximumBinCount(int idx);

  /**
   * Get the maximum bin count over all histograms
   */
  double GetMaximumBinCount();

  /**
   * Compute the range of the bin located at position (binX,binY) in
   * the 2D histogram at idx.
   */
  int GetBinRange(int idx, svtkIdType binX, svtkIdType binY, double range[4]);

  /**
   * Get the range of the of the bin located at 1D position index bin
   * in the 2D histogram array at idx.
   */
  int GetBinRange(int idx, svtkIdType bin, double range[4]);

  /**
   * Get the width of all of the bins. Also stored in the spacing
   * ivar of the histogram image output at idx.
   */
  void GetBinWidth(int idx, double bw[2]);

  /**
   * Get the histogram extents currently in use, either computed
   * or set by the user for the idx'th histogram.
   */
  double* GetHistogramExtents(int idx);

  /**
   * Get the svtkImageData output of the idx'th histogram filter
   */
  svtkImageData* GetOutputHistogramImage(int idx);

  /**
   * Get a pointer to the idx'th histogram filter.
   */
  svtkExtractHistogram2D* GetHistogramFilter(int idx);

  enum OutputIndices
  {
    HISTOGRAM_IMAGE = 3
  };

  /**
   * Given a collection of models, calculate aggregate model.  Not used
   */
  void Aggregate(svtkDataObjectCollection*, svtkMultiBlockDataSet*) override {}

protected:
  svtkPairwiseExtractHistogram2D();
  ~svtkPairwiseExtractHistogram2D() override;

  int NumberOfBins[2];
  int ScalarType;
  int CustomColumnRangeIndex;

  svtkSmartPointer<svtkIdTypeArray> OutputOutlierIds;
  svtkSmartPointer<svtkCollection> HistogramFilters;
  class Internals;
  Internals* Implementation;

  /**
   * Execute the calculations required by the Learn option.
   * Does the actual histogram computation works.
   */
  void Learn(svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta) override;

  /**
   * Execute the calculations required by the Derive option. Not used.
   */
  void Derive(svtkMultiBlockDataSet*) override {}

  /**
   * Execute the assess option. Not implemented.
   */
  void Assess(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override {}

  /**
   * Execute the calculations required by the Test option.
   */
  void Test(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override { return; }

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor(svtkTable* svtkNotUsed(outData), svtkDataObject* svtkNotUsed(inMeta),
    svtkStringArray* svtkNotUsed(rowNames), AssessFunctor*& svtkNotUsed(dfunc)) override
  {
  }

  /**
   * Generate a new histogram filter
   */
  virtual svtkExtractHistogram2D* NewHistogramFilter();

  int FillOutputPortInformation(int port, svtkInformation* info) override;

  svtkTimeStamp BuildTime;

private:
  svtkPairwiseExtractHistogram2D(const svtkPairwiseExtractHistogram2D&) = delete;
  void operator=(const svtkPairwiseExtractHistogram2D&) = delete;
};

#endif
