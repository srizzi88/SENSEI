/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeCellIteratorStrategy
 * @brief   Interface used by svtkBridgeCellIterator
 * svtkBridgeCellIterator has different behaviors depending on the way it is
 * initialized. svtkBridgeCellIteratorStrategy is the interface for one of those
 * behaviors. Concrete classes are svtkBridgeCellIteratorOnDataSet,
 * svtkBridgeCellIteratorOnDataSetBoundaries,
 * svtkBridgeCellIteratorOnCellBoundaries,
 * svtkBridgeCellIteratorOnCellNeighbors,
 * @sa
 * svtkGenericCellIterator, svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorOnDataSet,
 * svtkBridgeCellIteratorOnDataSetBoundaries, svtkBridgeCellIteratorOnCellBoundaries,
 * svtkBridgeCellIteratorOnCellNeighbors
 */

#ifndef svtkBridgeCellIteratorStrategy_h
#define svtkBridgeCellIteratorStrategy_h

#include "svtkBridgeExport.h" //for module export macro
#include "svtkGenericCellIterator.h"

class svtkBridgeCell;
class svtkBridgeDataSet;
class svtkBridgeCell;
class svtkIdList;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgeCellIteratorStrategy : public svtkGenericCellIterator
{
public:
  svtkTypeMacro(svtkBridgeCellIteratorStrategy, svtkGenericCellIterator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create an empty cell. NOT USED
   * \post result_exists: result!=0
   */
  svtkGenericAdaptorCell* NewCell() override;

protected:
  svtkBridgeCellIteratorStrategy() {}
  ~svtkBridgeCellIteratorStrategy() override {}

private:
  svtkBridgeCellIteratorStrategy(const svtkBridgeCellIteratorStrategy&) = delete;
  void operator=(const svtkBridgeCellIteratorStrategy&) = delete;
};

#endif
