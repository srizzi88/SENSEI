/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkScalarTree
 * @brief   organize data according to scalar values (used to accelerate contouring operations)
 *
 *
 * svtkScalarTree is an abstract class that defines the API to concrete
 * scalar tree subclasses. A scalar tree is a data structure that organizes
 * data according to its scalar value. This allows rapid access to data for
 * those algorithms that access the data based on scalar value. For example,
 * isocontouring operates on cells based on the scalar (isocontour) value.
 *
 * To use subclasses of this class, you must specify a dataset to operate on,
 * and then specify a scalar value in the InitTraversal() method. Then
 * calls to GetNextCell() return cells whose scalar data contains the
 * scalar value specified. (This describes serial traversal.)
 *
 * Methods supporting parallel traversal (such as threading) are also
 * supported. Basically thread-safe batches of cells (which are a
 * portion of the whole dataset) are available for processing using a
 * parallel For() operation. First request the number of batches, and
 * then for each batch, retrieve the array of cell ids in that batch. These
 * batches contain cell ids that are likely to contain the isosurface.
 *
 * @sa
 * svtkSimpleScalarTree svtkSpanSpace
 */

#ifndef svtkScalarTree_h
#define svtkScalarTree_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkObject.h"

class svtkCell;
class svtkDataArray;
class svtkDataSet;
class svtkIdList;
class svtkTimeStamp;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkScalarTree : public svtkObject
{
public:
  //@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  svtkTypeMacro(svtkScalarTree, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This method is used to copy data members when cloning an instance of the
   * class. It does not copy heavy data.
   */
  virtual void ShallowCopy(svtkScalarTree* stree);

  //@{
  /**
   * Build the tree from the points/cells and scalars defining this
   * dataset.
   */
  virtual void SetDataSet(svtkDataSet*);
  svtkGetObjectMacro(DataSet, svtkDataSet);
  //@}

  //@{
  /**
   * Build the tree from the points/cells and scalars defining the
   * dataset and scalars provided. Typically the scalars come from
   * the svtkDataSet specified, but sometimes a separate svtkDataArray
   * is provided to specify the scalars. If the scalar array is
   * explicitly set, then it takes precedence over the scalars held
   * in the svtkDataSet.
   */
  virtual void SetScalars(svtkDataArray*);
  svtkGetObjectMacro(Scalars, svtkDataArray);
  //@}

  /**
   * Construct the scalar tree from the dataset provided. Checks build times
   * and modified time from input and reconstructs the tree if necessary.
   */
  virtual void BuildTree() = 0;

  /**
   * Initialize locator. Frees memory and resets object as appropriate.
   */
  virtual void Initialize() = 0;

  /**
   * Begin to traverse the cells based on a scalar value (serial
   * traversal). Returned cells will have scalar values that span the scalar
   * value specified. Note that changing the scalarValue does not cause the
   * scalar tree to be modified, and hence it does not rebuild.
   */
  virtual void InitTraversal(double scalarValue) = 0;

  /**
   * Return the next cell that may contain scalar value specified to
   * InitTraversal() (serial traversal). The value nullptr is returned if the
   * list is exhausted. Make sure that InitTraversal() has been invoked first
   * or you'll get erratic behavior.
   */
  virtual svtkCell* GetNextCell(svtkIdType& cellId, svtkIdList*& ptIds, svtkDataArray* cellScalars) = 0;

  /**
   * Return the current scalar value over which tree traversal is proceeding.
   * This is the scalar value provided in InitTraversal().
   */
  double GetScalarValue() { return this->ScalarValue; }

  // The following methods supports parallel (threaded) traversal. Basically
  // batches of cells (which are a portion of the whole dataset) are available for
  // processing in a parallel For() operation.

  /**
   * Get the number of cell batches available for processing as a function of
   * the specified scalar value. Each batch contains a list of candidate
   * cells that may contain the specified isocontour value.
   */
  virtual svtkIdType GetNumberOfCellBatches(double scalarValue) = 0;

  /**
   * Return the array of cell ids in the specified batch. The method
   * also returns the number of cell ids in the array. Make sure to
   * call GetNumberOfCellBatches() beforehand.
   */
  virtual const svtkIdType* GetCellBatch(svtkIdType batchNum, svtkIdType& numCells) = 0;

protected:
  svtkScalarTree();
  ~svtkScalarTree() override;

  svtkDataSet* DataSet;   // the dataset over which the scalar tree is built
  svtkDataArray* Scalars; // the scalars of the DataSet
  double ScalarValue;    // current scalar value for traversal

  svtkTimeStamp BuildTime; // time at which tree was built

private:
  svtkScalarTree(const svtkScalarTree&) = delete;
  void operator=(const svtkScalarTree&) = delete;
};

#endif
