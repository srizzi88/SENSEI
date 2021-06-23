/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThresholdPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkThresholdPoints
 * @brief   extracts points whose scalar value satisfies threshold criterion
 *
 * svtkThresholdPoints is a filter that extracts points from a dataset that
 * satisfy a threshold criterion. The criterion can take three forms:
 * 1) greater than a particular value; 2) less than a particular value; or
 * 3) between a particular value. The output of the filter is polygonal data.
 *
 * @sa
 * svtkThreshold svtkSelectEnclosedPoints svtkExtractEnclosedPoints
 */

#ifndef svtkThresholdPoints_h
#define svtkThresholdPoints_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkThresholdPoints : public svtkPolyDataAlgorithm
{
public:
  static svtkThresholdPoints* New();
  svtkTypeMacro(svtkThresholdPoints, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Criterion is cells whose scalars are less or equal to lower threshold.
   */
  void ThresholdByLower(double lower);

  /**
   * Criterion is cells whose scalars are greater or equal to upper threshold.
   */
  void ThresholdByUpper(double upper);

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Set/Get the upper threshold.
   */
  svtkSetMacro(UpperThreshold, double);
  svtkGetMacro(UpperThreshold, double);
  //@}

  //@{
  /**
   * Set/Get the lower threshold.
   */
  svtkSetMacro(LowerThreshold, double);
  svtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkThresholdPoints();
  ~svtkThresholdPoints() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  double LowerThreshold;
  double UpperThreshold;
  int OutputPointsPrecision;

  int (svtkThresholdPoints::*ThresholdFunction)(double s);

  int Lower(double s) { return (s <= this->LowerThreshold ? 1 : 0); }
  int Upper(double s) { return (s >= this->UpperThreshold ? 1 : 0); }
  int Between(double s)
  {
    return (s >= this->LowerThreshold ? (s <= this->UpperThreshold ? 1 : 0) : 0);
  }

private:
  svtkThresholdPoints(const svtkThresholdPoints&) = delete;
  void operator=(const svtkThresholdPoints&) = delete;
};

#endif
