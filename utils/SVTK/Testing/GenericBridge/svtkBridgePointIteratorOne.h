/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgePointIteratorOne.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgePointIteratorOne
 * @brief   Iterate over one point of a dataset.
 * @sa
 * svtkGenericPointIterator, svtkBridgeDataSet
 */

#ifndef svtkBridgePointIteratorOne_h
#define svtkBridgePointIteratorOne_h

#include "svtkBridgeExport.h" //for module export macro
#include "svtkGenericPointIterator.h"

class svtkBridgeDataSet;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgePointIteratorOne : public svtkGenericPointIterator
{
public:
  static svtkBridgePointIteratorOne* New();
  svtkTypeMacro(svtkBridgePointIteratorOne, svtkGenericPointIterator);
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
   * Iterate over one point of identifier `id' on dataset `ds'.
   * \pre ds_can_be_null: ds!=0 || ds==0
   * \pre valid_id: svtkImplies(ds!=0,(id>=0)&&(id<=ds->GetNumberOfCells()))
   */
  void InitWithOnePoint(svtkBridgeDataSet* ds, svtkIdType id);

protected:
  /**
   * Default constructor.
   */
  svtkBridgePointIteratorOne();

  /**
   * Destructor.
   */
  ~svtkBridgePointIteratorOne() override;

  svtkBridgeDataSet* DataSet; // the structure on which the object iterates.
  svtkIdType Id;              // the id at current position.
  int cIsAtEnd;

private:
  svtkBridgePointIteratorOne(const svtkBridgePointIteratorOne&) = delete;
  void operator=(const svtkBridgePointIteratorOne&) = delete;
};

#endif
