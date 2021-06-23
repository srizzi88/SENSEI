/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThresholdTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkThresholdTable
 * @brief   Thresholds table rows.
 *
 *
 * svtkThresholdTable uses minimum and/or maximum values to threshold
 * table rows based on the values in a particular column.
 * The column to threshold is specified using SetInputArrayToProcess(0, ...).
 */

#ifndef svtkThresholdTable_h
#define svtkThresholdTable_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"
#include "svtkVariant.h" // For svtkVariant arguments

class SVTKINFOVISCORE_EXPORT svtkThresholdTable : public svtkTableAlgorithm
{
public:
  static svtkThresholdTable* New();
  svtkTypeMacro(svtkThresholdTable, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    ACCEPT_LESS_THAN = 0,
    ACCEPT_GREATER_THAN = 1,
    ACCEPT_BETWEEN = 2,
    ACCEPT_OUTSIDE = 3
  };

  //@{
  /**
   * The mode of the threshold filter.  Options are:
   * ACCEPT_LESS_THAN (0) accepts rows with values < MaxValue;
   * ACCEPT_GREATER_THAN (1) accepts rows with values > MinValue;
   * ACCEPT_BETWEEN (2) accepts rows with values > MinValue and < MaxValue;
   * ACCEPT_OUTSIDE (3) accepts rows with values < MinValue or > MaxValue.
   */
  svtkSetClampMacro(Mode, int, 0, 3);
  svtkGetMacro(Mode, int);
  //@}

  //@{
  /**
   * The minimum value for the threshold.
   * This may be any data type stored in a svtkVariant.
   */
  virtual void SetMinValue(svtkVariant v)
  {
    this->MinValue = v;
    this->Modified();
  }
  virtual svtkVariant GetMinValue() { return this->MinValue; }
  //@}

  //@{
  /**
   * The maximum value for the threshold.
   * This may be any data type stored in a svtkVariant.
   */
  virtual void SetMaxValue(svtkVariant v)
  {
    this->MaxValue = v;
    this->Modified();
  }
  virtual svtkVariant GetMaxValue() { return this->MaxValue; }
  //@}

  /**
   * Criterion is rows whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(svtkVariant lower, svtkVariant upper);

  /**
   * The minimum value for the threshold as a double.
   */
  void SetMinValue(double v) { this->SetMinValue(svtkVariant(v)); }

  /**
   * The maximum value for the threshold as a double.
   */
  void SetMaxValue(double v) { this->SetMaxValue(svtkVariant(v)); }

  /**
   * Criterion is rows whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper)
  {
    this->ThresholdBetween(svtkVariant(lower), svtkVariant(upper));
  }

protected:
  svtkThresholdTable();
  ~svtkThresholdTable() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkVariant MinValue;
  svtkVariant MaxValue;
  int Mode;

private:
  svtkThresholdTable(const svtkThresholdTable&) = delete;
  void operator=(const svtkThresholdTable&) = delete;
};

#endif
