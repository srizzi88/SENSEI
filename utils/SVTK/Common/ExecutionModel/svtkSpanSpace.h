/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSpanSpace.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSpanSpace
 * @brief   organize data according to scalar span space
 *
 * This is a helper class used to accelerate contouring operations. Given an
 * dataset, it organizes the dataset cells into a 2D binned space, with
 * coordinate axes (scalar_min,scalar_max). This so-called span space can
 * then be traversed quickly to find the cells that intersect a specified
 * contour value.
 *
 * This class has an API that supports both serial and parallel
 * operation.  The parallel API enables the using class to grab arrays
 * (or batches) of cells that lie along a particular row in the span
 * space. These arrays can then be processed separately or in parallel.
 *
 * Learn more about span space in these two publications: 1) "A Near
 * Optimal Isosorface Extraction Algorithm Using the Span Space."
 * Yarden Livnat et al. and 2) Isosurfacing in Span Space with Utmost
 * Efficiency." Han-Wei Shen et al.
 *
 * @sa
 * svtkScalarTree svtkSimpleScalarTree
 */

#ifndef svtkSpanSpace_h
#define svtkSpanSpace_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkScalarTree.h"

class svtkSpanSpace;
struct svtkInternalSpanSpace;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkSpanSpace : public svtkScalarTree
{
public:
  /**
   * Instantiate a scalar tree with default resolution of 100 and automatic
   * scalar range computation.
   */
  static svtkSpanSpace* New();

  //@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  svtkTypeMacro(svtkSpanSpace, svtkScalarTree);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This method is used to copy data members when cloning an instance of the
   * class. It does not copy heavy data.
   */
  void ShallowCopy(svtkScalarTree* stree) override;

  //----------------------------------------------------------------------
  // The following methods are specific to the creation and configuration of
  // svtkSpanSpace.

  //@{
  /**
   * Specify the scalar range in terms of minimum and maximum values
   * (smin,smax). These values are used to build the span space. Note that
   * setting the range can have significant impact on the performance of the
   * span space as it controls the effective resolution near important
   * isocontour values. By default the range is computed automatically; turn
   * off ComputeScalarRange is you wish to manually specify it.
   */
  svtkSetVector2Macro(ScalarRange, double);
  svtkGetVectorMacro(ScalarRange, double, 2);
  //@}

  //@{
  /**
   * This boolean controls whether the determination of the scalar range is
   * computed from the input scalar data. By default this is enabled.
   */
  svtkSetMacro(ComputeScalarRange, svtkTypeBool);
  svtkGetMacro(ComputeScalarRange, svtkTypeBool);
  svtkBooleanMacro(ComputeScalarRange, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the resolution N of the span space. The span space can be
   * envisioned as a rectangular lattice of NXN buckets/bins (i.e., N rows
   * and N columns), where each bucket stores a list of cell ids. The i-j
   * coordinate of each cell (hence its location in the lattice) is
   * determined from the cell's 2-tuple (smin,smax) scalar range.  By default
   * Resolution = 100, with a clamp of 10,000.
   */
  svtkSetClampMacro(Resolution, svtkIdType, 1, 10000);
  svtkGetMacro(Resolution, svtkIdType);
  //@}

  //@{
  /**
   * Boolean controls whether the resolution of span space is computed
   * automatically from the average number of cells falling in each bucket.
   */
  svtkSetMacro(ComputeResolution, svtkTypeBool);
  svtkGetMacro(ComputeResolution, svtkTypeBool);
  svtkBooleanMacro(ComputeResolution, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the average number of cells in each bucket. This is used to
   * indirectly control the resolution if ComputeResolution is enabled.
   */
  svtkSetClampMacro(NumberOfCellsPerBucket, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfCellsPerBucket, int);
  //@}

  //----------------------------------------------------------------------
  // The following methods satisfy the svtkScalarTree abstract API.

  /**
   * Initialize the span space. Frees memory and resets object as
   * appropriate.
   */
  void Initialize() override;

  /**
   * Construct the scalar tree from the dataset provided. Checks build times
   * and modified time from input and reconstructs the tree if necessary.
   */
  void BuildTree() override;

  /**
   * Begin to traverse the cells based on a scalar value. Returned cells will
   * have scalar values that span the scalar value specified (within the
   * resolution of the span space). Note this method must be called prior to
   * parallel or serial traversal since it specifies the scalar value to be
   * extracted.
   */
  void InitTraversal(double scalarValue) override;

  /**
   * Return the next cell that may contain scalar value specified to
   * InitTraversal(). The value nullptr is returned if the list is
   * exhausted. Make sure that InitTraversal() has been invoked first or
   * you'll get undefined behavior. This is inherently a serial operation.
   */
  svtkCell* GetNextCell(svtkIdType& cellId, svtkIdList*& ptIds, svtkDataArray* cellScalars) override;

  // The following methods supports parallel (threaded) traversal. Basically
  // batches of cells (which are a portion of the whole dataset) are available for
  // processing in a parallel For() operation.

  /**
   * Get the number of cell batches available for processing as a function of
   * the specified scalar value. Each batch contains a list of candidate
   * cells that may contain the specified isocontour value.
   */
  svtkIdType GetNumberOfCellBatches(double scalarValue) override;

  /**
   * Return the array of cell ids in the specified batch. The method
   * also returns the number of cell ids in the array. Make sure to
   * call GetNumberOfCellBatches() beforehand.
   */
  const svtkIdType* GetCellBatch(svtkIdType batchNum, svtkIdType& numCells) override;

  //@{
  /**
   * Set/Get the size of the cell batches when processing in
   * parallel. By default the batch size = 100 cells in each batch.
   */
  svtkSetClampMacro(BatchSize, svtkIdType, 100, SVTK_INT_MAX);
  svtkGetMacro(BatchSize, svtkIdType);
  //@}

protected:
  svtkSpanSpace();
  ~svtkSpanSpace() override;

  double ScalarRange[2];
  svtkTypeBool ComputeScalarRange;
  svtkIdType Resolution;
  svtkTypeBool ComputeResolution;
  int NumberOfCellsPerBucket;
  svtkInternalSpanSpace* SpanSpace;
  svtkIdType BatchSize;

private:
  // Internal variables supporting span space traversal
  svtkIdType RMin[2]; // span space lower left corner
  svtkIdType RMax[2]; // span space upper right corner

  // This supports serial traversal via GetNextCell()
  svtkIdType CurrentRow;      // the span space row currently being processed
  svtkIdType* CurrentSpan;    // pointer to current span row
  svtkIdType CurrentIdx;      // position into the current span row
  svtkIdType CurrentNumCells; // number of cells on the current span row

private:
  svtkSpanSpace(const svtkSpanSpace&) = delete;
  void operator=(const svtkSpanSpace&) = delete;
};

#endif
