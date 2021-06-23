/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractInterpolatedVelocityField
 * @brief   An abstract class for
 *  obtaining the interpolated velocity values at a point
 *
 *
 *  svtkAbstractInterpolatedVelocityField acts as a continuous velocity field
 *  by performing cell interpolation on the underlying svtkDataSet. This is an
 *  abstract sub-class of svtkFunctionSet, NumberOfIndependentVariables = 4
 *  (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). With a brute-force scheme,
 *  every time an evaluation is performed, the target cell containing point
 *  (x,y,z) needs to be found by calling FindCell(), via either svtkDataSet or
 *  svtkAbstractCelllocator's sub-classes (svtkCellLocator & svtkModifiedBSPTree).
 *  As it incurs a large cost, one (for svtkCellLocatorInterpolatedVelocityField
 *  via svtkAbstractCellLocator) or two (for svtkInterpolatedVelocityField via
 *  svtkDataSet that involves svtkPointLocator in addressing svtkPointSet) levels
 *  of cell caching may be exploited to increase the performance.
 *
 *  For svtkInterpolatedVelocityField, level #0 begins with intra-cell caching.
 *  Specifically if the previous cell is valid and the next point is still in
 *  it ( i.e., svtkCell::EvaluatePosition() returns 1, coupled with newly created
 *  parametric coordinates & weights ), the function values can be interpolated
 *  and only svtkCell::EvaluatePosition() is invoked. If this fails, then level #1
 *  follows by inter-cell search for the target cell that contains the next point.
 *  By an inter-cell search, the previous cell provides an important clue or serves
 *  as an immediate neighbor to aid in locating the target cell via svtkPointSet::
 *  FindCell(). If this still fails, a global cell location / search is invoked via
 *  svtkPointSet::FindCell(). Here regardless of either inter-cell or global search,
 *  svtkPointLocator is in fact employed (for datasets of type svtkPointSet only, note
 *  svtkImageData and svtkRectilinearGrid are able to provide rapid and robust cell
 *  location due to the simple mesh topology) as a crucial tool underlying the cell
 *  locator. However, the use of svtkPointLocator makes svtkInterpolatedVelocityField
 *  non-robust in cell location for svtkPointSet.
 *
 *  For svtkCellLocatorInterpolatedVelocityField, the only caching (level #0) works
 *  by intra-cell trial. In case of failure, a global search for the target cell is
 *  invoked via svtkAbstractCellLocator::FindCell() and the actual work is done by
 *  either svtkCellLocator or svtkModifiedBSPTree (for datasets of type svtkPointSet
 *  only, while svtkImageData and svtkRectilinearGrid themselves are able to provide
 *  fast robust cell location). Without the involvement of svtkPointLocator, robust
 *  cell location is achieved for svtkPointSet.
 *
 * @warning
 *  svtkAbstractInterpolatedVelocityField is not thread safe. A new instance
 *  should be created by each thread.
 *
 * @sa
 *  svtkInterpolatedVelocityField svtkCellLocatorInterpolatedVelocityField
 *  svtkGenericInterpolatedVelocityField svtkCachingInterpolatedVelocityField
 *  svtkTemporalInterpolatedVelocityField svtkFunctionSet svtkStreamTracer
 */

#ifndef svtkAbstractInterpolatedVelocityField_h
#define svtkAbstractInterpolatedVelocityField_h

#include "svtkFunctionSet.h"

class svtkDataSet;
class svtkDataArray;
class svtkPointData;
class svtkGenericCell;
class svtkAbstractInterpolatedVelocityFieldDataSetsType;
class svtkFindCellStrategy;
struct svtkStrategyMap;

#include "svtkFiltersFlowPathsModule.h" // For export macro

class SVTKFILTERSFLOWPATHS_EXPORT svtkAbstractInterpolatedVelocityField : public svtkFunctionSet
{
public:
  svtkTypeMacro(svtkAbstractInterpolatedVelocityField, svtkFunctionSet);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the caching flag. If this flag is turned ON, there are two levels
   * of caching for derived concrete class svtkInterpolatedVelocityField and one
   * level of caching for derived concrete class svtkCellLocatorInterpolatedVelocityField.
   * Otherwise a global cell location is always invoked for evaluating the
   * function values at any point.
   */
  svtkSetMacro(Caching, bool);
  svtkGetMacro(Caching, bool);
  //@}

  //@{
  /**
   * Get the caching statistics. CacheHit refers to the number of level #0 cache
   * hits while CacheMiss is the number of level #0 cache misses.
   */
  svtkGetMacro(CacheHit, int);
  svtkGetMacro(CacheMiss, int);
  //@}

  svtkGetObjectMacro(LastDataSet, svtkDataSet);

  //@{
  /**
   * Get/Set the id of the cell cached from last evaluation.
   */
  svtkGetMacro(LastCellId, svtkIdType);
  virtual void SetLastCellId(svtkIdType c) { this->LastCellId = c; }
  //@}

  /**
   * Set the id of the most recently visited cell of a dataset.
   */
  virtual void SetLastCellId(svtkIdType c, int dataindex) = 0;

  //@{
  /**
   * Get/Set the name of a spcified vector array. By default it is nullptr, with
   * the active vector array for use.
   */
  svtkGetStringMacro(VectorsSelection);
  svtkGetMacro(VectorsType, int);
  //@}

  /**
   * the association type (see svtkDataObject::FieldAssociations)
   * and the name of the velocity data field
   */
  void SelectVectors(int fieldAssociation, const char* fieldName);

  //@{
  /**
   * Set/Get the flag indicating vector post-normalization (following vector
   * interpolation). Vector post-normalization is required to avoid the
   * 'curve-overshooting' problem (caused by high velocity magnitude) that
   * occurs when Cell-Length is used as the step size unit (particularly the
   * Minimum step size unit). Furthermore, it is required by RK45 to achieve,
   * as expected, high numerical accuracy (or high smoothness of flow lines)
   * through adaptive step sizing. Note this operation is performed (when
   * NormalizeVector TRUE) right after vector interpolation such that the
   * differing amount of contribution of each node (of a cell) to the
   * resulting direction of the interpolated vector, due to the possibly
   * significantly-differing velocity magnitude values at the nodes (which is
   * the case with large cells), can be reflected as is. Also note that this
   * flag needs to be turned to FALSE after svtkInitialValueProblemSolver::
   * ComputeNextStep() as subsequent operations, e.g., vorticity computation,
   * may need non-normalized vectors.
   */
  svtkSetMacro(NormalizeVector, bool);
  svtkGetMacro(NormalizeVector, bool);
  //@}

  //@{
  /**
   * If set to true, the first three point of the cell will be used to compute a normal to the cell,
   * this normal will then be removed from the vorticity so the resulting vector in tangent to the
   * cell.
   */
  svtkSetMacro(ForceSurfaceTangentVector, bool);
  svtkGetMacro(ForceSurfaceTangentVector, bool);
  //@}

  //@{
  /**
   * If set to true, cell within tolerance factor will always be found, except for edges.
   */
  svtkSetMacro(SurfaceDataset, bool);
  svtkGetMacro(SurfaceDataset, bool);
  //@}

  /**
   * Import parameters. Sub-classes can add more after chaining.
   */
  virtual void CopyParameters(svtkAbstractInterpolatedVelocityField* from);

  using Superclass::FunctionValues;
  /**
   * Evaluate the velocity field f at point (x, y, z).
   */
  int FunctionValues(double* x, double* f) override = 0;

  /**
   * Set the last cell id to -1 to incur a global cell search for the next point.
   */
  void ClearLastCellId() { this->LastCellId = -1; }

  //@{
  /**
   * Get the interpolation weights cached from last evaluation. Return 1 if the
   * cached cell is valid and 0 otherwise.
   */
  int GetLastWeights(double* w);
  int GetLastLocalCoordinates(double pcoords[3]);
  //@}

  //@{
  /**
   * Set / get the strategy used to perform the FindCell() operation. This
   * strategy is used when operating on svtkPointSet subclasses. Note if the
   * input is a composite dataset then the strategy will be used to clone
   * one strategy per leaf dataset.
   */
  virtual void SetFindCellStrategy(svtkFindCellStrategy*);
  svtkGetObjectMacro(FindCellStrategy, svtkFindCellStrategy);
  //@}

protected:
  svtkAbstractInterpolatedVelocityField();
  ~svtkAbstractInterpolatedVelocityField() override;

  static const double TOLERANCE_SCALE;
  static const double SURFACE_TOLERANCE_SCALE;

  int CacheHit;
  int CacheMiss;
  int WeightsSize;
  bool Caching;
  bool NormalizeVector;
  bool ForceSurfaceTangentVector;
  bool SurfaceDataset;
  int VectorsType;
  char* VectorsSelection;
  double* Weights;
  double LastPCoords[3];
  int LastSubId;
  svtkIdType LastCellId;
  svtkDataSet* LastDataSet;
  svtkGenericCell* Cell;
  svtkGenericCell* GenCell; // the current cell

  // Define a FindCell() strategy, keep track of the strategies assigned to
  // each dataset
  svtkFindCellStrategy* FindCellStrategy;
  svtkStrategyMap* StrategyMap;

  //@{
  /**
   * Set the name of a specific vector to be interpolated.
   */
  svtkSetStringMacro(VectorsSelection);
  //@}

  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * by invoking svtkDataSet::FindCell() to locate the next cell if the given
   * point is outside the current cell. To address svtkPointSet, svtkPointLocator
   * is involved via svtkPointSet::FindCell() in svtkInterpolatedVelocityField
   * for cell location. In svtkCellLocatorInterpolatedVelocityField, this function
   * is invoked just to handle svtkImageData and svtkRectilinearGrid that are not
   * assigned with any svtkAbstractCellLocatot-type cell locator.
   * If activated, returned vector will be tangential to the first
   * three point of the cell
   */
  virtual int FunctionValues(svtkDataSet* ds, double* x, double* f);

  /**
   * Check that all three pcoords are between 0 and 1 included.
   */
  virtual bool CheckPCoords(double pcoords[3]);

  /**
   * Try to find the cell closest to provided x point in provided dataset,
   * By first testing inclusion in it's cached cell and neighbor
   * Then testing globally
   * Then , only if surfacic is activated finding the closest cell
   * using FindPoint and comparing distance with tolerance
   */
  virtual bool FindAndUpdateCell(svtkDataSet* ds, double* x);

  friend class svtkTemporalInterpolatedVelocityField;
  //@{
  /**
   * If all weights have been computed (parametric coords etc all valid), a
   * scalar/vector can be quickly interpolated using the known weights and
   * the cached generic cell. This function is primarily reserved for use by
   * svtkTemporalInterpolatedVelocityField
   */
  void FastCompute(svtkDataArray* vectors, double f[3]);
  bool InterpolatePoint(svtkPointData* outPD, svtkIdType outIndex);
  svtkGenericCell* GetLastCell() { return (this->LastCellId != -1) ? this->GenCell : nullptr; }
  //@}

private:
  svtkAbstractInterpolatedVelocityField(const svtkAbstractInterpolatedVelocityField&) = delete;
  void operator=(const svtkAbstractInterpolatedVelocityField&) = delete;
};

#endif
