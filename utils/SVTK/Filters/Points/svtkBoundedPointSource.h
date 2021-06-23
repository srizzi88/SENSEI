/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoundedPointSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBoundedPointSource
 * @brief   create a random cloud of points within a
 * specified bounding box
 *
 *
 * svtkBoundedPointSource is a source object that creates a user-specified
 * number of points within a specified bounding box. The points are scattered
 * randomly throughout the box. Optionally, the user can produce a
 * svtkPolyVertex cell as well as random scalar values within a specified
 * range. The class is typically used for debugging and testing, as well as
 * seeding streamlines.
 */

#ifndef svtkBoundedPointSource_h
#define svtkBoundedPointSource_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSPOINTS_EXPORT svtkBoundedPointSource : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information and printing.
   */
  static svtkBoundedPointSource* New();
  svtkTypeMacro(svtkBoundedPointSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set the number of points to generate.
   */
  svtkSetClampMacro(NumberOfPoints, svtkIdType, 1, SVTK_ID_MAX);
  svtkGetMacro(NumberOfPoints, svtkIdType);
  //@}

  //@{
  /**
   * Set the bounding box for the point distribution. By default the bounds is
   * (-1,1,-1,1,-1,1).
   */
  svtkSetVector6Macro(Bounds, double);
  svtkGetVectorMacro(Bounds, double, 6);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

  //@{
  /**
   * Indicate whether to produce a svtkPolyVertex cell to go along with the
   * output svtkPoints generated. By default a cell is NOT produced. Some filters
   * do not need the svtkPolyVertex which just consumes a lot of memory.
   */
  svtkSetMacro(ProduceCellOutput, bool);
  svtkGetMacro(ProduceCellOutput, bool);
  svtkBooleanMacro(ProduceCellOutput, bool);
  //@}

  //@{
  /**
   * Indicate whether to produce random point scalars in the output. By default
   * this is off.
   */
  svtkSetMacro(ProduceRandomScalars, bool);
  svtkGetMacro(ProduceRandomScalars, bool);
  svtkBooleanMacro(ProduceRandomScalars, bool);
  //@}

  //@{
  /**
   * Set the range in which the random scalars should be produced. By default the
   * scalar range is (0,1).
   */
  svtkSetVector2Macro(ScalarRange, double);
  svtkGetVectorMacro(ScalarRange, double, 2);
  //@}

protected:
  svtkBoundedPointSource();
  ~svtkBoundedPointSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkIdType NumberOfPoints;
  double Bounds[6];
  int OutputPointsPrecision;
  bool ProduceCellOutput;
  bool ProduceRandomScalars;
  double ScalarRange[2];

private:
  svtkBoundedPointSource(const svtkBoundedPointSource&) = delete;
  void operator=(const svtkBoundedPointSource&) = delete;
};

#endif
