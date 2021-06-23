/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeCellIterator
 * @brief   Implementation of svtkGenericCellIterator.
 * It is just an example that show how to implement the Generic API. It is also
 * used for testing and evaluating the Generic framework.
 * @sa
 * svtkGenericCellIterator, svtkBridgeDataSet
 */

#ifndef svtkBridgeCellIterator_h
#define svtkBridgeCellIterator_h

#include "svtkBridgeExport.h" //for module export macro
#include "svtkGenericCellIterator.h"

class svtkBridgeCell;
class svtkBridgeDataSet;
class svtkBridgeCell;
class svtkIdList;
class svtkBridgeDataSet;
class svtkPoints;

class svtkBridgeCellIteratorStrategy;
class svtkBridgeCellIteratorOnDataSet;
class svtkBridgeCellIteratorOne;
class svtkBridgeCellIteratorOnCellBoundaries;
class svtkBridgeCellIteratorOnCellList;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgeCellIterator : public svtkGenericCellIterator
{
public:
  static svtkBridgeCellIterator* New();
  svtkTypeMacro(svtkBridgeCellIterator, svtkGenericCellIterator);
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
   * Create an empty cell.
   * \post result_exists: result!=0
   */
  svtkGenericAdaptorCell* NewCell() override;

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

  /**
   * Used internally by svtkBridgeDataSet.
   * Iterate over boundary cells of `ds' of some dimension `dim'.
   * \pre ds_exists: ds!=0
   * \pre valid_dim_range: (dim>=-1) && (dim<=3)
   */
  void InitWithDataSetBoundaries(svtkBridgeDataSet* ds, int dim, int exterior_only);

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
   * Iterate on boundary cells of a cell.
   * \pre cell_exists: cell!=0
   * \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<cell->GetDimension()))
   */
  void InitWithCellBoundaries(svtkBridgeCell* cell, int dim);

  /**
   * Used internally by svtkBridgeCell.
   * Iterate on neighbors defined by `cells' over the dataset `ds'.
   * \pre cells_exist: cells!=0
   * \pre ds_exists: ds!=0
   */
  void InitWithCells(svtkIdList* cells, svtkBridgeDataSet* ds);

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
  svtkBridgeCellIterator();
  ~svtkBridgeCellIterator() override;

  svtkBridgeCellIteratorStrategy* CurrentIterator;
  svtkBridgeCellIteratorOnDataSet* IteratorOnDataSet;
  svtkBridgeCellIteratorOne* IteratorOneCell;
  svtkBridgeCellIteratorOnCellBoundaries* IteratorOnCellBoundaries;
  svtkBridgeCellIteratorOnCellList* IteratorOnCellList;

  svtkBridgeDataSet* DataSet; // the structure on which the object iterates.
  svtkIdType Id;              // the id at current position.
  int OneCell;               // Is in one cell mode?
  svtkIdType Size;            // size of the structure.
  svtkBridgeCell* Cell;       // cell at current position.

private:
  svtkBridgeCellIterator(const svtkBridgeCellIterator&) = delete;
  void operator=(const svtkBridgeCellIterator&) = delete;
};

#endif
