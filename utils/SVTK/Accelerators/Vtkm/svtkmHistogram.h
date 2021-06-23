//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
/**
 * @class   svtkmHistogram
 * @brief   generate a histogram out of a scalar data
 *
 * svtkmHistogram is a filter that generates a histogram out of a scalar data.
 * The histogram consists of a certain number of bins specified by the user, and
 * the user can fetch the range and bin delta after completion.
 *
 */

#ifndef svtkmHistogram_h
#define svtkmHistogram_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct export
#include "svtkTableAlgorithm.h"

class svtkDoubleArray;

class SVTKACCELERATORSSVTKM_EXPORT svtkmHistogram : public svtkTableAlgorithm
{
public:
  svtkTypeMacro(svtkmHistogram, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmHistogram* New();

  //@{
  /**
   * Specify number of bins.  Default is 10.
   */
  svtkSetMacro(NumberOfBins, size_t);
  svtkGetMacro(NumberOfBins, size_t);
  //@}

  //@{
  /**
   * Specify the range to use to generate the histogram. They are only used when
   * UseCustomBinRanges is set to true.
   */
  svtkSetVector2Macro(CustomBinRange, double);
  svtkGetVector2Macro(CustomBinRange, double);
  //@}

  //@{
  /**
   * When set to true, CustomBinRanges will  be used instead of using the full
   * range for the selected array. By default, set to false.
   */
  svtkSetMacro(UseCustomBinRanges, bool);
  svtkGetMacro(UseCustomBinRanges, bool);
  svtkBooleanMacro(UseCustomBinRanges, bool);
  //@}

  //@{
  /**
   * Get/Set if first and last bins must be centered around the min and max
   * data. This is only used when UseCustomBinRanges is set to false.
   * Default is false.
   */
  svtkSetMacro(CenterBinsAroundMinAndMax, bool);
  svtkGetMacro(CenterBinsAroundMinAndMax, bool);
  svtkBooleanMacro(CenterBinsAroundMinAndMax, bool);
  //@}

  //@{
  /**
   * Return the range used to generate the histogram.
   */
  svtkGetVectorMacro(ComputedRange, double, 2);
  //@}

  //@{
  /**
   * Return the bin delta of the computed field.
   */
  svtkGetMacro(BinDelta, double);
  //@}

protected:
  svtkmHistogram();
  ~svtkmHistogram() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkmHistogram(const svtkmHistogram&) = delete;
  void operator=(const svtkmHistogram&) = delete;

  void FillBinExtents(svtkDoubleArray* binExtents);

  size_t NumberOfBins;
  double BinDelta;
  double CustomBinRange[2];
  bool UseCustomBinRanges;
  bool CenterBinsAroundMinAndMax;
  double ComputedRange[2];
};

#endif // svtkmHistogram_h
// SVTK-HeaderTest-Exclude: svtkmHistogram.h
