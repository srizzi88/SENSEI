/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSparseArrayToTable.h

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
 * @class   svtkSparseArrayToTable
 * @brief   Converts a sparse array to a svtkTable.
 *
 *
 * Converts any sparse array to a svtkTable containing one row for each value
 * stored in the array.  The table will contain one column of coordinates for each
 * dimension in the source array, plus one column of array values.  A common use-case
 * for svtkSparseArrayToTable would be converting a sparse array into a table
 * suitable for use as an input to svtkTableToGraph.
 *
 * The coordinate columns in the output table will be named using the dimension labels
 * from the source array,  The value column name can be explicitly set using
 * SetValueColumn().
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkSparseArrayToTable_h
#define svtkSparseArrayToTable_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkSparseArrayToTable : public svtkTableAlgorithm
{
public:
  static svtkSparseArrayToTable* New();
  svtkTypeMacro(svtkSparseArrayToTable, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the output table column that contains array values.
   * Default: "value"
   */
  svtkGetStringMacro(ValueColumn);
  svtkSetStringMacro(ValueColumn);
  //@}

protected:
  svtkSparseArrayToTable();
  ~svtkSparseArrayToTable() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* ValueColumn;

private:
  svtkSparseArrayToTable(const svtkSparseArrayToTable&) = delete;
  void operator=(const svtkSparseArrayToTable&) = delete;
};

#endif
