/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractDataOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractDataOverTime
 * @brief   extract point data from a time sequence for
 * a specified point id.
 *
 * This filter extracts the point data from a time sequence and specified index
 * and creates an output of the same type as the input but with Points
 * containing "number of time steps" points; the point and PointData
 * corresponding to the PointIndex are extracted at each time step and added to
 * the output.  A PointData array is added called "Time" (or "TimeData" if
 * there is already an array called "Time"), which is the time at each index.
 */

#ifndef svtkExtractDataOverTime_h
#define svtkExtractDataOverTime_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractDataOverTime : public svtkPointSetAlgorithm
{
public:
  static svtkExtractDataOverTime* New();
  svtkTypeMacro(svtkExtractDataOverTime, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Index of point to extract at each time step
   */
  svtkSetMacro(PointIndex, int);
  svtkGetMacro(PointIndex, int);
  //@}

  //@{
  /**
   * Get the number of time steps
   */
  svtkGetMacro(NumberOfTimeSteps, int);
  //@}

protected:
  svtkExtractDataOverTime();
  ~svtkExtractDataOverTime() override {}

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int AllocateOutputData(svtkPointSet* input, svtkPointSet* output);

  int PointIndex;
  int CurrentTimeIndex;
  int NumberOfTimeSteps;

private:
  svtkExtractDataOverTime(const svtkExtractDataOverTime&) = delete;
  void operator=(const svtkExtractDataOverTime&) = delete;
};

#endif
