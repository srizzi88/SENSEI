/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTimeSourceExample.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTimeSource
 * @brief   creates a simple time varying data set.
 *
 * Creates a small easily understood time varying data set for testing.
 * The output is a svtkUntructuredGrid in which the point and cell values vary
 * over time in a sin wave. The analytic ivar controls whether the output
 * corresponds to a step function over time or is continuous.
 * The X and Y Amplitude ivars make the output move in the X and Y directions
 * over time. The Growing ivar makes the number of cells in the output grow
 * and then shrink over time.
 */

#ifndef svtkTimeSourceExample_h
#define svtkTimeSourceExample_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkTimeSourceExample : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkTimeSourceExample* New();
  svtkTypeMacro(svtkTimeSourceExample, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * When off (the default) this source produces a discrete set of values.
   * When on, this source produces a value analytically for any queried time.
   */
  svtkSetClampMacro(Analytic, svtkTypeBool, 0, 1);
  svtkGetMacro(Analytic, svtkTypeBool);
  svtkBooleanMacro(Analytic, svtkTypeBool);
  //@}

  //@{
  /**
   * When 0.0 (the default) this produces a data set that is stationary.
   * When on the data set moves in the X/Y plane over a sin wave over time,
   * amplified by the value.
   */
  svtkSetMacro(XAmplitude, double);
  svtkGetMacro(XAmplitude, double);
  svtkSetMacro(YAmplitude, double);
  svtkGetMacro(YAmplitude, double);
  //@}

  //@{
  /**
   * When off (the default) this produces a single cell data set.
   * When on the number of cells (in the Y direction) grows
   * and shrinks over time along a hat function.
   */
  svtkSetClampMacro(Growing, svtkTypeBool, 0, 1);
  svtkGetMacro(Growing, svtkTypeBool);
  svtkBooleanMacro(Growing, svtkTypeBool);
  //@}

protected:
  svtkTimeSourceExample();
  ~svtkTimeSourceExample() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void LookupTimeAndValue(double& time, double& value);
  double ValueFunction(double time);
  double XFunction(double time);
  double YFunction(double time);
  int NumCellsFunction(double time);

  svtkTypeBool Analytic;
  double XAmplitude;
  double YAmplitude;
  svtkTypeBool Growing;

  int NumSteps;
  double* Steps;
  double* Values;

private:
  svtkTimeSourceExample(const svtkTimeSourceExample&) = delete;
  void operator=(const svtkTimeSourceExample&) = delete;
};

#endif
