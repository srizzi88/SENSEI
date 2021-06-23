/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgePointIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgePointIterator
 * @brief   Implementation of svtkGenericPointIterator.
 *
 * It is just an example that show how to implement the Generic API. It is also
 * used for testing and evaluating the Generic framework.
 * @sa
 * svtkGenericPointIterator, svtkBridgeDataSet
 */

#ifndef svtkBridgePointIterator_h
#define svtkBridgePointIterator_h

#include "svtkBridgeExport.h" //for module export macro
#include "svtkGenericPointIterator.h"

class svtkBridgeDataSet;
class svtkBridgeCell;
class svtkBridgePointIteratorOnDataSet;
class svtkBridgePointIteratorOne;
class svtkBridgePointIteratorOnCell;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgePointIterator : public svtkGenericPointIterator
{
public:
  static svtkBridgePointIterator* New();
  svtkTypeMacro(svtkBridgePointIterator, svtkGenericPointIterator);
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
   * Used internally by svtkBridgeDataSet.
   * Iterate over points of `ds'.
   * \pre ds_exists: ds!=0
   */
  void InitWithDataSet(svtkBridgeDataSet* ds);

  /**
   * Used internally by svtkBridgeDataSet.
   * Iterate over one point of identifier `id' on dataset `ds'.
   * \pre ds_can_be_null: ds!=0 || ds==0
   * \pre valid_id: svtkImplies(ds!=0,(id>=0)&&(id<=ds->GetNumberOfCells()))
   */
  void InitWithOnePoint(svtkBridgeDataSet* ds, svtkIdType id);

  /**
   * The iterator will iterate over the point of a cell
   * \pre cell_exists: cell!=0
   */
  void InitWithCell(svtkBridgeCell* cell);

protected:
  /**
   * Default constructor.
   */
  svtkBridgePointIterator();

  /**
   * Destructor.
   */
  ~svtkBridgePointIterator() override;

  svtkGenericPointIterator* CurrentIterator;
  svtkBridgePointIteratorOnDataSet* IteratorOnDataSet;
  svtkBridgePointIteratorOne* IteratorOne;
  svtkBridgePointIteratorOnCell* IteratorOnCell;

private:
  svtkBridgePointIterator(const svtkBridgePointIterator&) = delete;
  void operator=(const svtkBridgePointIterator&) = delete;
};

#endif
