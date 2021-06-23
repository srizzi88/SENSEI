/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewiseFunctionShiftScale.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPiecewiseFunctionShiftScale
 *
 *
 */

#ifndef svtkPiecewiseFunctionShiftScale_h
#define svtkPiecewiseFunctionShiftScale_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkPiecewiseFunctionAlgorithm.h"

class svtkPiecewiseFunction;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkPiecewiseFunctionShiftScale
  : public svtkPiecewiseFunctionAlgorithm
{
public:
  static svtkPiecewiseFunctionShiftScale* New();
  svtkTypeMacro(svtkPiecewiseFunctionShiftScale, svtkPiecewiseFunctionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkSetMacro(PositionShift, double);
  svtkSetMacro(PositionScale, double);
  svtkSetMacro(ValueShift, double);
  svtkSetMacro(ValueScale, double);

  svtkGetMacro(PositionShift, double);
  svtkGetMacro(PositionScale, double);
  svtkGetMacro(ValueShift, double);
  svtkGetMacro(ValueScale, double);

protected:
  svtkPiecewiseFunctionShiftScale();
  ~svtkPiecewiseFunctionShiftScale() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double PositionShift;
  double PositionScale;
  double ValueShift;
  double ValueScale;

private:
  svtkPiecewiseFunctionShiftScale(const svtkPiecewiseFunctionShiftScale&) = delete;
  void operator=(const svtkPiecewiseFunctionShiftScale&) = delete;
};

#endif
