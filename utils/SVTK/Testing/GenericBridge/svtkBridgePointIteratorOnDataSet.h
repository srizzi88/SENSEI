/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgePointIteratorOnDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgePointIteratorOnDataSet
 * @brief   Implementation of svtkGenericPointIterator.
 *
 * It iterates over the points of a dataset (can be corner points of cells or
 * isolated points)
 * @sa
 * svtkGenericPointIterator, svtkBridgeDataSet
 */

#ifndef svtkBridgePointIteratorOnDataSet_h
#define svtkBridgePointIteratorOnDataSet_h

#include "svtkBridgeExport.h" //for module export macro
#include "svtkGenericPointIterator.h"

class svtkBridgeDataSet;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgePointIteratorOnDataSet
  : public svtkGenericPointIterator
{
public:
  static svtkBridgePointIteratorOnDataSet* New();
  svtkTypeMacro(svtkBridgePointIteratorOnDataSet, svtkGenericPointIterator);
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

protected:
  /**
   * Default constructor.
   */
  svtkBridgePointIteratorOnDataSet();

  /**
   * Destructor.
   */
  ~svtkBridgePointIteratorOnDataSet() override;

  svtkBridgeDataSet* DataSet; // the structure on which the object iterates.
  svtkIdType Id;              // the id at current position.
  int Size;                  // size of the structure.

private:
  svtkBridgePointIteratorOnDataSet(const svtkBridgePointIteratorOnDataSet&) = delete;
  void operator=(const svtkBridgePointIteratorOnDataSet&) = delete;
};

#endif
