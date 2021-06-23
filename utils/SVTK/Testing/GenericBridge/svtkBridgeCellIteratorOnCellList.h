/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorOnCellList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeCellIteratorOnCellList
 * @brief   Iterate over a list of cells defined
 * on a dataset.
 * See InitWithCells().
 * @sa
 * svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorStrategy
 */

#ifndef svtkBridgeCellIteratorOnCellList_h
#define svtkBridgeCellIteratorOnCellList_h

#include "svtkBridgeCellIteratorStrategy.h"

class svtkBridgeCell;
class svtkIdList;
class svtkBridgeDataSet;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgeCellIteratorOnCellList
  : public svtkBridgeCellIteratorStrategy
{
public:
  static svtkBridgeCellIteratorOnCellList* New();
  svtkTypeMacro(svtkBridgeCellIteratorOnCellList, svtkBridgeCellIteratorStrategy);
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
   * Used internally by svtkBridgeCell.
   * Iterate on neighbors defined by `cells' over the dataset `ds'.
   * \pre cells_exist: cells!=0
   * \pre ds_exists: ds!=0
   */
  void InitWithCells(svtkIdList* cells, svtkBridgeDataSet* ds);

protected:
  svtkBridgeCellIteratorOnCellList();
  ~svtkBridgeCellIteratorOnCellList() override;

  svtkIdList* Cells; // cells traversed by the iterator.
  svtkBridgeDataSet* DataSet;
  svtkIdType Id;        // the id at current position.
  svtkBridgeCell* Cell; // cell at current position.

private:
  svtkBridgeCellIteratorOnCellList(const svtkBridgeCellIteratorOnCellList&) = delete;
  void operator=(const svtkBridgeCellIteratorOnCellList&) = delete;
};

#endif
