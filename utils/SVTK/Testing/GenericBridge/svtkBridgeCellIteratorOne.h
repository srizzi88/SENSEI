/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorOne.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeCellIteratorOne
 * @brief   Iterate over one cell only of a dataset.
 * @sa
 * svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorStrategy
 */

#ifndef svtkBridgeCellIteratorOne_h
#define svtkBridgeCellIteratorOne_h

#include "svtkBridgeCellIteratorStrategy.h"

class svtkBridgeCell;
class svtkBridgeDataSet;
class svtkBridgeCell;
class svtkIdList;
class svtkPoints;
class svtkCell;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgeCellIteratorOne : public svtkBridgeCellIteratorStrategy
{
public:
  static svtkBridgeCellIteratorOne* New();
  svtkTypeMacro(svtkBridgeCellIteratorOne, svtkBridgeCellIteratorStrategy);
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
   * Iterate on one cell `id' of `ds'.
   * \pre ds_exists: ds!=0
   * \pre valid_id: (id>=0)&&(id<=ds->GetNumberOfCells())
   */
  void InitWithOneCell(svtkBridgeDataSet* ds, svtkIdType cellid);

  /**
   * Used internally by svtkBridgeCell.
   * Iterate on one cell `c'.
   * \pre c_exists: c!=0
   */
  void InitWithOneCell(svtkBridgeCell* c);

  /**
   * Used internally by svtkBridgeCell.
   * Iterate on a boundary cell (defined by its points `pts' with coordinates
   * `coords', dimension `dim' and unique id `cellid') of a cell.
   * \pre coords_exist: coords!=0
   * \pre pts_exist: pts!=0
   * \pre valid_dim: dim>=0 && dim<=2
   * \pre valid_points: pts->GetNumberOfIds()>dim
   */
  void InitWithPoints(svtkPoints* coords, svtkIdList* pts, int dim, svtkIdType cellid);

protected:
  svtkBridgeCellIteratorOne();
  ~svtkBridgeCellIteratorOne() override;

  int cIsAtEnd;
  svtkBridgeDataSet* DataSet; // the structure on which the object iterates.
  svtkIdType Id;              // the id at current position.
  svtkBridgeCell* Cell;       // cell at current position.
  svtkCell* InternalCell;

private:
  svtkBridgeCellIteratorOne(const svtkBridgeCellIteratorOne&) = delete;
  void operator=(const svtkBridgeCellIteratorOne&) = delete;
};

#endif
