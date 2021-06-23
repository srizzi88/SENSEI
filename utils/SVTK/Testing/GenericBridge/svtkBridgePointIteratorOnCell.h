/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgePointIteratorOnCell.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgePointIteratorOnCell
 * @brief   Implementation of svtkGenericPointIterator.
 *
 * It iterates over the corner points of a cell.
 * @sa
 * svtkGenericPointIterator, svtkBridgeDataSet
 */

#ifndef svtkBridgePointIteratorOnCell_h
#define svtkBridgePointIteratorOnCell_h

#include "svtkBridgeExport.h" //for module export macro
#include "svtkGenericPointIterator.h"

class svtkBridgeDataSet;
class svtkBridgeCell;
class svtkIdList;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgePointIteratorOnCell : public svtkGenericPointIterator
{
public:
  static svtkBridgePointIteratorOnCell* New();
  svtkTypeMacro(svtkBridgePointIteratorOnCell, svtkGenericPointIterator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Move iterator to first position if any (loop initialization).
   */
  void Begin() override;

  /**
   * Is there no point at iterator position? (exit condition).
   */
  svtkTypeBool IsAtEnd() override;

  /**
   * Move iterator to next position. (loop progression).
   * \pre not_off: !IsAtEnd()
   */
  void Next() override;

  /**
   * Point at iterator position.
   * \pre not_off: !IsAtEnd()
   * \post result_exists: result!=0
   */
  double* GetPosition() override;

  /**
   * Point at iterator position.
   * \pre not_off: !IsAtEnd()
   * \pre x_exists: x!=0
   */
  void GetPosition(double x[3]) override;

  /**
   * Unique identifier for the point, could be non-contiguous
   * \pre not_off: !IsAtEnd()
   */
  svtkIdType GetId() override;

  /**
   * The iterator will iterate over the point of a cell
   * \pre cell_exists: cell!=0
   */
  void InitWithCell(svtkBridgeCell* cell);

protected:
  /**
   * Default constructor.
   */
  svtkBridgePointIteratorOnCell();

  /**
   * Destructor.
   */
  ~svtkBridgePointIteratorOnCell() override;

  svtkBridgeDataSet* DataSet; // the structure on which the object iterates.
  svtkIdType Cursor;          // current position

  svtkIdList* PtIds; // list of points of the cell

private:
  svtkBridgePointIteratorOnCell(const svtkBridgePointIteratorOnCell&) = delete;
  void operator=(const svtkBridgePointIteratorOnCell&) = delete;
};

#endif
