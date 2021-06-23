/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReduceTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkReduceTable
 * @brief   combine some of the rows of a table
 *
 *
 * Collapses the rows of the input table so that one particular
 * column (the IndexColumn) does not contain any duplicate values.
 * Thus the output table will have the same columns as the input
 * table, but potentially fewer rows.  One example use of this
 * class would be to generate a summary table from a table of
 * observations.
 * When two or more rows of the input table share a value in the
 * IndexColumn, the values from these rows will be combined on a
 * column-by-column basis.  By default, such numerical values will be
 * reduced to their mean, and non-numerical values will be reduced to
 * their mode.  This default behavior can be changed by calling
 * SetNumericalReductionMethod() or SetNonNumericalReductionMethod().
 * You can also specify the reduction method to use for a particular
 * column by calling SetReductionMethodForColumn().
 */

#ifndef svtkReduceTable_h
#define svtkReduceTable_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

#include <map>    // For ivar
#include <set>    // For ivar
#include <vector> // For ivar

class svtkVariant;

class SVTKINFOVISCORE_EXPORT svtkReduceTable : public svtkTableAlgorithm
{
public:
  static svtkReduceTable* New();
  svtkTypeMacro(svtkReduceTable, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the column that will be used to reduce the input table.
   * Any rows sharing a value in this column will be collapsed into
   * a single row in the output table.
   */
  svtkGetMacro(IndexColumn, svtkIdType);
  svtkSetMacro(IndexColumn, svtkIdType);
  //@}

  //@{
  /**
   * Get/Set the method that should be used to combine numerical
   * values.
   */
  svtkGetMacro(NumericalReductionMethod, int);
  svtkSetMacro(NumericalReductionMethod, int);
  //@}

  //@{
  /**
   * Get/Set the method that should be used to combine non-numerical
   * values.
   */
  svtkGetMacro(NonNumericalReductionMethod, int);
  svtkSetMacro(NonNumericalReductionMethod, int);
  //@}

  /**
   * Get the method that should be used to combine the values within
   * the specified column.  Returns -1 if no method has been set for
   * this particular column.
   */
  int GetReductionMethodForColumn(svtkIdType col);

  /**
   * Set the method that should be used to combine the values within
   * the specified column.
   */
  void SetReductionMethodForColumn(svtkIdType col, int method);

  /**
   * Enum for methods of reduction
   */
  enum
  {
    MEAN,
    MEDIAN,
    MODE
  };

protected:
  svtkReduceTable();
  ~svtkReduceTable() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Initialize the output table to have the same types of columns as
   * the input table, but no rows.
   */
  void InitializeOutputTable(svtkTable* input, svtkTable* output);

  /**
   * Find the distinct values in the input table's index column.
   * This function also populates the mapping of new row IDs to old row IDs.
   */
  void AccumulateIndexValues(svtkTable* input);

  /**
   * Populate the index column of the output table.
   */
  void PopulateIndexColumn(svtkTable* output);

  /**
   * Populate a non-index column of the output table.  This involves
   * potentially combining multiple values from the input table into
   * a single value for the output table.
   */
  void PopulateDataColumn(svtkTable* input, svtkTable* output, svtkIdType col);

  /**
   * Find the mean of a series of values from the input table
   * and store it in the output table.
   */
  void ReduceValuesToMean(svtkTable* input, svtkTable* output, svtkIdType row, svtkIdType col,
    std::vector<svtkIdType> oldRows);

  /**
   * Find the median of a series of values from the input table
   * and store it in the output table.
   */
  void ReduceValuesToMedian(svtkTable* input, svtkTable* output, svtkIdType row, svtkIdType col,
    std::vector<svtkIdType> oldRows);

  /**
   * Find the mode of a series of values from the input table
   * and store it in the output table.
   */
  void ReduceValuesToMode(svtkTable* input, svtkTable* output, svtkIdType row, svtkIdType col,
    std::vector<svtkIdType> oldRows);

  svtkIdType IndexColumn;
  std::set<svtkVariant> IndexValues;
  std::map<svtkVariant, std::vector<svtkIdType> > NewRowToOldRowsMap;
  std::map<svtkIdType, int> ColumnReductionMethods;

  int NumericalReductionMethod;
  int NonNumericalReductionMethod;

private:
  svtkReduceTable(const svtkReduceTable&) = delete;
  void operator=(const svtkReduceTable&) = delete;
};

#endif
