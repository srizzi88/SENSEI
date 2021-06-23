/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAppendPoints
 * @brief   appends points of one or more svtkPolyData data sets
 *
 *
 * svtkAppendPoints is a filter that appends the points and associated data
 * of one or more polygonal (svtkPolyData) datasets. This filter can optionally
 * add a new array marking the input index that the point came from.
 *
 * @sa
 * svtkAppendFilter svtkAppendPolyData
 */

#ifndef svtkAppendPoints_h
#define svtkAppendPoints_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkAppendPoints : public svtkPolyDataAlgorithm
{
public:
  static svtkAppendPoints* New();
  svtkTypeMacro(svtkAppendPoints, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Sets the output array name to fill with the input connection index
   * for each point. This provides a way to trace a point back to a
   * particular input. If this is nullptr (the default), the array is not generated.
   */
  svtkSetStringMacro(InputIdArrayName);
  svtkGetStringMacro(InputIdArrayName);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output type. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings. If the desired precision is
   * DEFAULT_PRECISION and any of the inputs are double precision, then the
   * output precision will be double precision. Otherwise, if the desired
   * precision is DEFAULT_PRECISION and all the inputs are single precision,
   * then the output will be single precision.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkAppendPoints();
  ~svtkAppendPoints() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  char* InputIdArrayName;
  int OutputPointsPrecision;

private:
  svtkAppendPoints(const svtkAppendPoints&) = delete;
  void operator=(const svtkAppendPoints&) = delete;
};

#endif
