/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorOnCellBoundaries.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeCellIteratorOnCellBoundaries
 * @brief   Iterate over boundary cells of
 * a cell.
 *
 * @sa
 * svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorStrategy
 */

#ifndef svtkBridgeCellIteratorOnCellBoundaries_h
#define svtkBridgeCellIteratorOnCellBoundaries_h

#include "svtkBridgeCellIteratorStrategy.h"

class svtkBridgeCell;
class svtkBridgeDataSet;
class svtkBridgeCell;
class svtkIdList;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgeCellIteratorOnCellBoundaries
  : public svtkBridgeCellIteratorStrategy
{
public:
  static svtkBridgeCellIteratorOnCellBoundaries* New();
  svtkTypeMacro(svtkBridgeCellIteratorOnCellBoundaries, svtkBridgeCellIteratorStrategy);
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
   * Iterate on boundary cells of a cell.
   * \pre cell_exists: cell!=0
   * \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<cell->GetDimension()))
   */
  void InitWithCellBoundaries(svtkBridgeCell* cell, int dim);

protected:
  svtkBridgeCellIteratorOnCellBoundaries();
  ~svtkBridgeCellIteratorOnCellBoundaries() override;

  int Dim; // Dimension of cells over which to iterate (-1 to 3)

  svtkBridgeCell* DataSetCell; // the structure on which the object iterates.
  svtkIdType Id;               // the id at current position.
  svtkBridgeCell* Cell;        // cell at current position.
  svtkIdType NumberOfFaces;
  svtkIdType NumberOfEdges;
  svtkIdType NumberOfVertices;

private:
  svtkBridgeCellIteratorOnCellBoundaries(const svtkBridgeCellIteratorOnCellBoundaries&) = delete;
  void operator=(const svtkBridgeCellIteratorOnCellBoundaries&) = delete;
};

#endif
