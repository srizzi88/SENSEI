/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransposeTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkTransposeTable
 * @brief   Transpose an input table.
 *
 *
 * This algorithm allows to transpose a svtkTable as a matrix.
 * Columns become rows and vice versa. A new column can be added to
 * the result table at index 0 to collect the name of the initial
 * columns (when AddIdColumn is true). Such a column can be used
 * to name the columns of the result.
 * Note that columns of the output table will have a variant type
 * is the columns of the initial table are not consistent.
 */

#ifndef svtkTransposeTable_h
#define svtkTransposeTable_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkTransposeTable : public svtkTableAlgorithm
{
public:
  static svtkTransposeTable* New();
  svtkTypeMacro(svtkTransposeTable, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This flag indicates if a column must be inserted at index 0
   * with the names (ids) of the input columns.
   * Default: true
   */
  svtkGetMacro(AddIdColumn, bool);
  svtkSetMacro(AddIdColumn, bool);
  svtkBooleanMacro(AddIdColumn, bool);
  //@}

  //@{
  /**
   * This flag indicates if the output column must be named using the
   * names listed in the index 0 column.
   * Default: false
   */
  svtkGetMacro(UseIdColumn, bool);
  svtkSetMacro(UseIdColumn, bool);
  svtkBooleanMacro(UseIdColumn, bool);
  //@}

  //@{
  /**
   * Get/Set the name of the id column added by option AddIdColumn.
   * Default: ColName
   */
  svtkGetStringMacro(IdColumnName);
  svtkSetStringMacro(IdColumnName);
  //@}

protected:
  svtkTransposeTable();
  ~svtkTransposeTable() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool AddIdColumn;
  bool UseIdColumn;
  char* IdColumnName;

private:
  svtkTransposeTable(const svtkTransposeTable&) = delete;
  void operator=(const svtkTransposeTable&) = delete;
};

#endif
