/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgeCellIteratorStrategy - Interface used by svtkBridgeCellIterator
// svtkBridgeCellIterator has different behaviors depending on the way it is
// initialized. svtkBridgeCellIteratorStrategy is the interface for one of those
// behaviors. Concrete classes are svtkBridgeCellIteratorOnDataSet,
// svtkBridgeCellIteratorOnDataSetBoundaries,
// svtkBridgeCellIteratorOnCellBoundaries,
// svtkBridgeCellIteratorOnCellNeighbors,
// .SECTION See Also
// svtkCellIterator, svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorOnDataSet,
// svtkBridgeCellIteratorOnDataSetBoundaries, svtkBridgeCellIteratorOnCellBoundaries,
// svtkBridgeCellIteratorOnCellNeighbors

#include "svtkBridgeCellIteratorStrategy.h"

#include <cassert>

//-----------------------------------------------------------------------------
void svtkBridgeCellIteratorStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
// Description:
// Create an empty cell. NOT USED
// \post result_exists: result!=0
svtkGenericAdaptorCell* svtkBridgeCellIteratorStrategy::NewCell()
{
  assert("check: should not be called: see svtkBridgeCellIterator::NewCell()" && 0);
  return nullptr;
}
