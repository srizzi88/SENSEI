/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorOnDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeCellIteratorOnDataSet
 * @brief   Iterate over cells of a dataset.
 * @sa
 * svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorStrategy
 */

#ifndef svtkBridgeCellIteratorOnDataSet_h
#define svtkBridgeCellIteratorOnDataSet_h

#include "svtkBridgeCellIteratorStrategy.h"

class svtkBridgeCell;
class svtkBridgeDataSet;
class svtkBridgeCell;
class svtkIdList;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgeCellIteratorOnDataSet
  : public svtkBridgeCellIteratorStrategy
{
public:
  static svtkBridgeCellIteratorOnDataSet* New();
  svtkTypeMacro(svtkBridgeCellIteratorOnDataSet, svtkBridgeCellIteratorStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Move iterator to first position if any (loop initialization).
   */
  void Begin() override;

  /**
   * Is there no cell at iterator position? (exit condition).
   */
  svtkTypeBool IsAtEnd() override;

  /**
   * Cell at current position
   * \pre not_at_end: !IsAtEnd()
   * \pre c_exists: c!=0
   * THREAD SAFE
   */
  void GetCell(svtkGenericAdaptorCell* c) override;

  /**
   * Cell at current position.
   * NOT THREAD SAFE
   * \pre not_at_end: !IsAtEnd()
   * \post result_exits: result!=0
   */
  svtkGenericAdaptorCell* GetCell() override;

  /**
   * Move iterator to next position. (loop progression).
   * \pre not_at_end: !IsAtEnd()
   */
  void Next() override;

  /**
   * Used internally by svtkBridgeDataSet.
   * Iterate over cells of `ds' of some dimension `dim'.
   * \pre ds_exists: ds!=0
   * \pre valid_dim_range: (dim>=-1) && (dim<=3)
   */
  void InitWithDataSet(svtkBridgeDataSet* ds, int dim);

protected:
  svtkBridgeCellIteratorOnDataSet();
  ~svtkBridgeCellIteratorOnDataSet() override;

  int Dim; // Dimension of cells over which to iterate (-1 to 3)

  svtkBridgeDataSet* DataSet; // the structure on which the object iterates.
  svtkIdType Id;              // the id at current position.
  svtkIdType Size;            // size of the structure.
  svtkBridgeCell* Cell;       // cell at current position.

private:
  svtkBridgeCellIteratorOnDataSet(const svtkBridgeCellIteratorOnDataSet&) = delete;
  void operator=(const svtkBridgeCellIteratorOnDataSet&) = delete;
};

#endif
