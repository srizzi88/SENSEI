/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToSparseArray.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkTableToSparseArray
 * @brief   converts a svtkTable into a sparse array.
 *
 *
 * Converts a svtkTable into a sparse array.  Use AddCoordinateColumn() to
 * designate one-to-many table columns that contain coordinates for each
 * array value, and SetValueColumn() to designate the table column that
 * contains array values.
 *
 * Thus, the number of dimensions in the output array will equal the number
 * of calls to AddCoordinateColumn().
 *
 * The coordinate columns will also be used to populate dimension labels
 * in the output array.
 *
 * By default, the extent of the output array will be set to the range
 * [0, largest coordinate + 1) along each dimension.  In some situations
 * you may prefer to set the extents explicitly, using the
 * SetOutputExtents() method.  This is useful when the output array should
 * be larger than its largest coordinates, or when working with partitioned
 * data.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkTableToSparseArray_h
#define svtkTableToSparseArray_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkTableToSparseArray : public svtkArrayDataAlgorithm
{
public:
  static svtkTableToSparseArray* New();
  svtkTypeMacro(svtkTableToSparseArray, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the set of input table columns that will be mapped to coordinates
   * in the output sparse array.
   */
  void ClearCoordinateColumns();
  void AddCoordinateColumn(const char* name);
  //@}

  //@{
  /**
   * Specify the input table column that will be mapped to values in the output array.
   */
  void SetValueColumn(const char* name);
  const char* GetValueColumn();
  //@}

  //@{
  /**
   * Explicitly specify the extents of the output array.
   */
  void ClearOutputExtents();
  void SetOutputExtents(const svtkArrayExtents& extents);
  //@}

protected:
  svtkTableToSparseArray();
  ~svtkTableToSparseArray() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkTableToSparseArray(const svtkTableToSparseArray&) = delete;
  void operator=(const svtkTableToSparseArray&) = delete;

  class implementation;
  implementation* const Implementation;
};

#endif
