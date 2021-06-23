/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeFieldAggregator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkTreeFieldAggregator
 * @brief   aggregate field values from the leaves up the tree
 *
 *
 * svtkTreeFieldAggregator may be used to assign sizes to all the vertices in the
 * tree, based on the sizes of the leaves.  The size of a vertex will equal
 * the sum of the sizes of the child vertices.  If you have a data array with
 * values for all leaves, you may specify that array, and the values will
 * be filled in for interior tree vertices.  If you do not yet have an array,
 * you may tell the filter to create a new array, assuming that the size
 * of each leaf vertex is 1.  You may optionally set a flag to first take the
 * log of all leaf values before aggregating.
 */

#ifndef svtkTreeFieldAggregator_h
#define svtkTreeFieldAggregator_h

class svtkPoints;
class svtkTree;

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkTreeFieldAggregator : public svtkTreeAlgorithm
{
public:
  static svtkTreeFieldAggregator* New();

  svtkTypeMacro(svtkTreeFieldAggregator, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The field to aggregate.  If this is a string array, the entries are converted to double.
   * TODO: Remove this field and use the ArrayToProcess in svtkAlgorithm.
   */
  svtkGetStringMacro(Field);
  svtkSetStringMacro(Field);
  //@}

  //@{
  /**
   * If the value of the vertex is less than MinValue then consider it's value to be minVal.
   */
  svtkGetMacro(MinValue, double);
  svtkSetMacro(MinValue, double);
  //@}

  //@{
  /**
   * If set, the algorithm will assume a size of 1 for each leaf vertex.
   */
  svtkSetMacro(LeafVertexUnitSize, bool);
  svtkGetMacro(LeafVertexUnitSize, bool);
  svtkBooleanMacro(LeafVertexUnitSize, bool);
  //@}

  //@{
  /**
   * If set, the leaf values in the tree will be logarithmically scaled (base 10).
   */
  svtkSetMacro(LogScale, bool);
  svtkGetMacro(LogScale, bool);
  svtkBooleanMacro(LogScale, bool);
  //@}

protected:
  svtkTreeFieldAggregator();
  ~svtkTreeFieldAggregator() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  char* Field;
  bool LeafVertexUnitSize;
  bool LogScale;
  double MinValue;
  svtkTreeFieldAggregator(const svtkTreeFieldAggregator&) = delete;
  void operator=(const svtkTreeFieldAggregator&) = delete;
  double GetDoubleValue(svtkAbstractArray* arr, svtkIdType id);
  static void SetDoubleValue(svtkAbstractArray* arr, svtkIdType id, double value);
};

#endif
