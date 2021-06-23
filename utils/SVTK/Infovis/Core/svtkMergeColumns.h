/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeColumns.h

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
 * @class   svtkMergeColumns
 * @brief   merge two columns into a single column
 *
 *
 * svtkMergeColumns replaces two columns in a table with a single column
 * containing data in both columns.  The columns are set using
 *
 *   SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, "col1")
 *
 * and
 *
 *   SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, "col2")
 *
 * where "col1" and "col2" are the names of the columns to merge.
 * The user may also specify the name of the merged column.
 * The arrays must be of the same type.
 * If the arrays are numeric, the values are summed in the merged column.
 * If the arrays are strings, the values are concatenated.  The strings are
 * separated by a space if they are both nonempty.
 */

#ifndef svtkMergeColumns_h
#define svtkMergeColumns_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkMergeColumns : public svtkTableAlgorithm
{
public:
  static svtkMergeColumns* New();
  svtkTypeMacro(svtkMergeColumns, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name to give the merged column created by this filter.
   */
  svtkSetStringMacro(MergedColumnName);
  svtkGetStringMacro(MergedColumnName);
  //@}

protected:
  svtkMergeColumns();
  ~svtkMergeColumns() override;

  char* MergedColumnName;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkMergeColumns(const svtkMergeColumns&) = delete;
  void operator=(const svtkMergeColumns&) = delete;
};

#endif
