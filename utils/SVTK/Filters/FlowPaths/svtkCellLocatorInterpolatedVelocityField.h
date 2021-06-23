/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellLocatorInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellLocatorInterpolatedVelocityField
 * @brief   A concrete class for
 *  obtaining the interpolated velocity values at a point.
 *
 *
 *  svtkCellLocatorInterpolatedVelocityField acts as a continuous velocity
 *  field via cell interpolation on a svtkDataSet, NumberOfIndependentVariables
 *  = 4 (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). As a concrete sub-class
 *  of svtkCompositeInterpolatedVelocityField, it adopts svtkAbstractCellLocator's
 *  sub-classes, e.g., svtkCellLocator and svtkModifiedBSPTree, without the use
 *  of svtkPointLocator ( employed by svtkDataSet/svtkPointSet::FindCell() in
 *  svtkInterpolatedVelocityField ). svtkCellLocatorInterpolatedVelocityField
 *  adopts one level of cell caching. Specifically, if the next point is still
 *  within the previous cell, cell location is then simply skipped and svtkCell::
 *  EvaluatePosition() is called to obtain the new parametric coordinates and
 *  weights that are used to interpolate the velocity function values across the
 *  vertices of this cell. Otherwise a global cell (the target containing the next
 *  point) location is instead directly invoked, without exploiting the clue that
 *  svtkInterpolatedVelocityField makes use of from the previous cell (an immediate
 *  neighbor). Although ignoring the neighbor cell may incur a relatively high
 *  computational cost, svtkCellLocatorInterpolatedVelocityField is more robust in
 *  locating the target cell than its sibling class svtkInterpolatedVelocityField.
 *
 * @warning
 *  svtkCellLocatorInterpolatedVelocityField is not thread safe. A new instance
 *  should be created by each thread.
 *
 * @sa
 *  svtkCompositeInterpolatedVelocityField svtkInterpolatedVelocityField
 *  svtkGenericInterpolatedVelocityField svtkCachingInterpolatedVelocityField
 *  svtkTemporalInterpolatedVelocityField svtkFunctionSet svtkStreamTracer
 */

#ifndef svtkCellLocatorInterpolatedVelocityField_h
#define svtkCellLocatorInterpolatedVelocityField_h

#include "svtkCompositeInterpolatedVelocityField.h"
#include "svtkFiltersFlowPathsModule.h" // For export macro

class svtkAbstractCellLocator;
class svtkCellLocatorInterpolatedVelocityFieldCellLocatorsType;

class SVTKFILTERSFLOWPATHS_EXPORT svtkCellLocatorInterpolatedVelocityField
  : public svtkCompositeInterpolatedVelocityField
{
public:
  svtkTypeMacro(svtkCellLocatorInterpolatedVelocityField, svtkCompositeInterpolatedVelocityField);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a svtkCellLocatorInterpolatedVelocityField without an initial
   * dataset. Caching is set on and LastCellId is set to -1.
   */
  static svtkCellLocatorInterpolatedVelocityField* New();

  //@{
  /**
   * Get the cell locator attached to the most recently visited dataset.
   */
  svtkGetObjectMacro(LastCellLocator, svtkAbstractCellLocator);
  //@}

  //@{
  /**
   * Set/Get the prototype of the cell locator that is used for interpolating
   * the velocity field during integration. The prototype is used to
   * instantiate locators for performing interpolation. By default, a
   * svtkModifiedBSPTree is used - other classes such as svtkStaticCellLocator
   * can be faster.
   */
  void SetCellLocatorPrototype(svtkAbstractCellLocator* prototype);
  svtkGetObjectMacro(CellLocatorPrototype, svtkAbstractCellLocator);
  //@}

  /**
   * Import parameters. Sub-classes can add more after chaining.
   */
  void CopyParameters(svtkAbstractInterpolatedVelocityField* from) override;

  /**
   * Add a dataset coupled with a cell locator (of type svtkAbstractCellLocator)
   * for vector function evaluation. Note the use of a svtkAbstractCellLocator
   * enables robust cell location. If more than one dataset is added, the
   * evaluation point is searched in all until a match is found. THIS FUNCTION
   * DOES NOT CHANGE THE REFERENCE COUNT OF dataset FOR THREAD SAFETY REASONS.
   */
  void AddDataSet(svtkDataSet* dataset) override;

  using Superclass::FunctionValues;
  /**
   * Evaluate the velocity field f at point (x, y, z).
   */
  int FunctionValues(double* x, double* f) override;

  /**
   * Set the cell id cached by the last evaluation within a specified dataset.
   */
  void SetLastCellId(svtkIdType c, int dataindex) override;

  /**
   * Set the cell id cached by the last evaluation.
   */
  void SetLastCellId(svtkIdType c) override { this->Superclass::SetLastCellId(c); }

protected:
  svtkCellLocatorInterpolatedVelocityField();
  ~svtkCellLocatorInterpolatedVelocityField() override;

  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * (actually of type svtkPointSet only) through the use of the associated
   * svtkAbstractCellLocator::FindCell() (instead of involving svtkPointLocator)
   * to locate the next cell if the given point is outside the current cell.
   */
  int FunctionValues(svtkDataSet* ds, svtkAbstractCellLocator* loc, double* x, double* f);

  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * (of type svtkImageData or svtkRectilinearGrid only) by invoking FindCell()
   * to locate the next cell if the given point is outside the current cell.
   */
  int FunctionValues(svtkDataSet* ds, double* x, double* f) override
  {
    return this->Superclass::FunctionValues(ds, x, f);
  }

private:
  svtkAbstractCellLocator* LastCellLocator;
  svtkAbstractCellLocator* CellLocatorPrototype;
  svtkCellLocatorInterpolatedVelocityFieldCellLocatorsType* CellLocators;

  svtkCellLocatorInterpolatedVelocityField(const svtkCellLocatorInterpolatedVelocityField&) = delete;
  void operator=(const svtkCellLocatorInterpolatedVelocityField&) = delete;
};

#endif
