/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearToQuadraticCellsFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkLinearToQuadraticCellsFilter
 * @brief   degree elevate the cells of a linear unstructured grid.
 *
 *
 * svtkLinearToQuadraticCellsFilter takes an unstructured grid comprised of
 * linear cells and degree elevates each of the cells to quadratic. Additional
 * points are simply interpolated from the existing points (there is no snapping
 * to an external model).
 */

#ifndef svtkLinearToQuadraticCellsFilter_h
#define svtkLinearToQuadraticCellsFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkIncrementalPointLocator;

class SVTKFILTERSGEOMETRY_EXPORT svtkLinearToQuadraticCellsFilter
  : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkLinearToQuadraticCellsFilter, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkLinearToQuadraticCellsFilter* New();

  //@{
  /**
   * Specify a spatial locator for merging points. By default, an
   * instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   * OutputPointsPrecision is DEFAULT_PRECISION by default.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

  /**
   * Return the mtime also considering the locator.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkLinearToQuadraticCellsFilter();
  ~svtkLinearToQuadraticCellsFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkIncrementalPointLocator* Locator;
  int OutputPointsPrecision;

private:
  svtkLinearToQuadraticCellsFilter(const svtkLinearToQuadraticCellsFilter&) = delete;
  void operator=(const svtkLinearToQuadraticCellsFilter&) = delete;
};

#endif
