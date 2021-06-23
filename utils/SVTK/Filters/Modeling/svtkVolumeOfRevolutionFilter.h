/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeOfRevolutionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVolumeOfRevolutionFilter
 * @brief   sweep data about a line to create a volume
 *
 * svtkVolumeOfRevolutionFilter is a modeling filter. It takes a 2-dimensional
 * dataset as input and generates an unstructured grid on output. The input
 * dataset is swept around the axis of rotation to create dimension-elevated
 * primitives. For example, sweeping a vertex creates a series of lines;
 * sweeping a line creates a series of quads, etc.
 *
 * @warning
 * The user must take care to ensure that the axis of revolution does not cross
 * through the geometry, otherwise there will be intersecting cells in the
 * output.
 *
 * @sa
 * svtkRotationalExtrusionFilter
 */

#ifndef svtkVolumeOfRevolutionFilter_h
#define svtkVolumeOfRevolutionFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class SVTKFILTERSMODELING_EXPORT svtkVolumeOfRevolutionFilter : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkVolumeOfRevolutionFilter, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create object with sweep angle of 360 degrees, resolution = 12,
   * axis position (0,0,0) and axis direction (0,0,1).
   */
  static svtkVolumeOfRevolutionFilter* New();

  //@{
  /**
   * Set/Get resolution of sweep operation. Resolution controls the number
   * of intermediate node points.
   */
  svtkSetClampMacro(Resolution, int, 1, SVTK_INT_MAX);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Set/Get angle of rotation in degrees.
   */
  svtkSetClampMacro(SweepAngle, double, -360., 360.);
  svtkGetMacro(SweepAngle, double);
  //@}

  //@{
  /**
   * Set/Get the position of the axis of revolution.
   */
  svtkSetVector3Macro(AxisPosition, double);
  svtkGetVector3Macro(AxisPosition, double);
  //@}

  //@{
  /**
   * Set/Get the direction of the axis of revolution.
   */
  svtkSetVector3Macro(AxisDirection, double);
  svtkGetVector3Macro(AxisDirection, double);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkVolumeOfRevolutionFilter();
  ~svtkVolumeOfRevolutionFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int Resolution;
  double SweepAngle;
  double AxisPosition[3];
  double AxisDirection[3];
  int OutputPointsPrecision;

private:
  svtkVolumeOfRevolutionFilter(const svtkVolumeOfRevolutionFilter&) = delete;
  void operator=(const svtkVolumeOfRevolutionFilter&) = delete;
};

#endif
