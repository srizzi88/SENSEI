/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInterpolatedVelocityField
 * @brief   A concrete class for obtaining
 *  the interpolated velocity values at a point.
 *
 *
 * svtkInterpolatedVelocityField acts as a continuous velocity field via
 * cell interpolation on a svtkDataSet, NumberOfIndependentVariables = 4
 * (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). As a concrete sub-class
 * of svtkCompositeInterpolatedVelocityField, this class adopts two levels
 * of cell caching for faster though less robust cell location than its
 * sibling class svtkCellLocatorInterpolatedVelocityField. Level #0 begins
 * with intra-cell caching. Specifically, if the previous cell is valid
 * and the next point is still within it, ( svtkCell::EvaluatePosition()
 * returns 1, coupled with the new parametric coordinates and weights ),
 * the function values are interpolated and svtkCell::EvaluatePosition()
 * is invoked only. If it fails, level #1 follows by inter-cell location
 * of the target cell (that contains the next point). By inter-cell, the
 * previous cell gives an important clue / guess or serves as an immediate
 * neighbor to aid in the location of the target cell (as is typically the
 * case with integrating a streamline across cells) by means of svtkDataSet::
 * FindCell(). If this still fails, a global cell search is invoked via
 * svtkDataSet::FindCell().
 *
 * Regardless of inter-cell or global search, a point locator is employed as
 * a crucial tool underlying the interpolation process. The use of a point
 * locator, while faster than a cell locator, is not optimal and may cause
 * svtkInterpolatedVelocityField to return incorrect results (i.e., premature
 * streamline termination) for datasets defined on complex grids (especially
 * those this discontinuous/incompatible cells). In these cases, try
 * svtkCellLocatorInterpolatedVelocityField which produces the best results at
 * the cost of speed.
 *
 * @warning
 * svtkInterpolatedVelocityField is not thread safe. A new instance should be
 * created by each thread.
 *
 * @sa
 *  svtkCompositeInterpolatedVelocityField svtkCellLocatorInterpolatedVelocityField
 *  svtkGenericInterpolatedVelocityField svtkCachingInterpolatedVelocityField
 *  svtkTemporalInterpolatedVelocityField svtkFunctionSet svtkStreamTracer
 */

#ifndef svtkInterpolatedVelocityField_h
#define svtkInterpolatedVelocityField_h

#include "svtkCompositeInterpolatedVelocityField.h"
#include "svtkFiltersFlowPathsModule.h" // For export macro

class SVTKFILTERSFLOWPATHS_EXPORT svtkInterpolatedVelocityField
  : public svtkCompositeInterpolatedVelocityField
{
public:
  /**
   * Construct a svtkInterpolatedVelocityField without an initial dataset.
   * Caching is set on and LastCellId is set to -1.
   */
  static svtkInterpolatedVelocityField* New();

  //@{
  /**
   * Standard methods for type information and printing.
   */
  svtkTypeMacro(svtkInterpolatedVelocityField, svtkCompositeInterpolatedVelocityField);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Add a dataset used for the implicit function evaluation. If more than
   * one dataset is added, the evaluation point is searched in all until a
   * match is found. THIS FUNCTION DOES NOT CHANGE THE REFERENCE COUNT OF
   * DATASET FOR THREAD SAFETY REASONS.
   */
  void AddDataSet(svtkDataSet* dataset) override;

  using Superclass::FunctionValues;
  /**
   * Evaluate the velocity field f at point (x, y, z).
   */
  int FunctionValues(double* x, double* f) override;

  /**
   * Project the provided point on current cell, current dataset.
   */
  virtual int SnapPointOnCell(double* pOrigin, double* pProj);

  /**
   * Set the cell id cached by the last evaluation within a specified dataset.
   */
  void SetLastCellId(svtkIdType c, int dataindex) override;

  /**
   * Set the cell id cached by the last evaluation.
   */
  void SetLastCellId(svtkIdType c) override { this->Superclass::SetLastCellId(c); }

protected:
  svtkInterpolatedVelocityField() {}
  ~svtkInterpolatedVelocityField() override {}

  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * by either involving svtkPointLocator, via svtkPointSet::FindCell(), in
   * locating the next cell (for datasets of type svtkPointSet) or simply
   * invoking svtkImageData/svtkRectilinearGrid::FindCell() to fulfill the same
   * task if the point is outside the current cell.
   */
  int FunctionValues(svtkDataSet* ds, double* x, double* f) override
  {
    return this->Superclass::FunctionValues(ds, x, f);
  }

private:
  svtkInterpolatedVelocityField(const svtkInterpolatedVelocityField&) = delete;
  void operator=(const svtkInterpolatedVelocityField&) = delete;
};

#endif
