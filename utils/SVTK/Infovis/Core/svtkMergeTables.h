/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeTables.h

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
 * @class   svtkMergeTables
 * @brief   combine two tables
 *
 *
 * Combines the columns of two tables into one larger table.
 * The number of rows in the resulting table is the sum of the number of
 * rows in each of the input tables.
 * The number of columns in the output is generally the sum of the number
 * of columns in each input table, except in the case where column names
 * are duplicated in both tables.
 * In this case, if MergeColumnsByName is on (the default), the two columns
 * will be merged into a single column of the same name.
 * If MergeColumnsByName is off, both columns will exist in the output.
 * You may set the FirstTablePrefix and SecondTablePrefix to define how
 * the columns named are modified.  One of these prefixes may be the empty
 * string, but they must be different.
 */

#ifndef svtkMergeTables_h
#define svtkMergeTables_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkMergeTables : public svtkTableAlgorithm
{
public:
  static svtkMergeTables* New();
  svtkTypeMacro(svtkMergeTables, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The prefix to give to same-named fields from the first table.
   * Default is "Table1.".
   */
  svtkSetStringMacro(FirstTablePrefix);
  svtkGetStringMacro(FirstTablePrefix);
  //@}

  //@{
  /**
   * The prefix to give to same-named fields from the second table.
   * Default is "Table2.".
   */
  svtkSetStringMacro(SecondTablePrefix);
  svtkGetStringMacro(SecondTablePrefix);
  //@}

  //@{
  /**
   * If on, merges columns with the same name.
   * If off, keeps both columns, but calls one
   * FirstTablePrefix + name, and the other SecondTablePrefix + name.
   * Default is on.
   */
  svtkSetMacro(MergeColumnsByName, bool);
  svtkGetMacro(MergeColumnsByName, bool);
  svtkBooleanMacro(MergeColumnsByName, bool);
  //@}

  //@{
  /**
   * If on, all columns will have prefixes except merged columns.
   * If off, only unmerged columns with the same name will have prefixes.
   * Default is off.
   */
  svtkSetMacro(PrefixAllButMerged, bool);
  svtkGetMacro(PrefixAllButMerged, bool);
  svtkBooleanMacro(PrefixAllButMerged, bool);
  //@}

protected:
  svtkMergeTables();
  ~svtkMergeTables() override;

  bool MergeColumnsByName;
  bool PrefixAllButMerged;
  char* FirstTablePrefix;
  char* SecondTablePrefix;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkMergeTables(const svtkMergeTables&) = delete;
  void operator=(const svtkMergeTables&) = delete;
};

#endif
