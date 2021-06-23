/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTecplotTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2016 Menno Deij - van Rijswijk (MARIN)
-------------------------------------------------------------------------*/

/**
 * @class   svtkTecplotTableReader
 * @brief   reads in Tecplot tabular data
 * and outputs a svtkTable data structure.
 *
 *
 * svtkTecplotTableReader is an interface for reading tabulat data in Tecplot
 * ascii format.
 *
 * @par Thanks:
 * Thanks to svtkDelimitedTextReader authors.
 *
 */

#ifndef svtkTecplotTableReader_h
#define svtkTecplotTableReader_h

#include "svtkIOTecplotTableModule.h" // For export macro
#include "svtkStdString.h"            // Needed for svtkStdString
#include "svtkTableAlgorithm.h"
#include "svtkUnicodeString.h" // Needed for svtkUnicodeString

class SVTKIOTECPLOTTABLE_EXPORT svtkTecplotTableReader : public svtkTableAlgorithm
{
public:
  static svtkTecplotTableReader* New();
  svtkTypeMacro(svtkTecplotTableReader, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specifies the delimited text file to be loaded.
   */
  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specifies the maximum number of records to read from the file.  Limiting the
   * number of records to read is useful for previewing the contents of a file.
   */
  svtkGetMacro(MaxRecords, svtkIdType);
  svtkSetMacro(MaxRecords, svtkIdType);
  //@}

  //@{
  /**
   * Specifies the number of lines that form the header of the file. Default is 2.
   */
  svtkGetMacro(HeaderLines, svtkIdType);
  svtkSetMacro(HeaderLines, svtkIdType);
  //@}

  //@{
  /**
   * Specifies the line number that holds the column names. Default is 1.
   */
  svtkGetMacro(ColumnNamesOnLine, svtkIdType);
  svtkSetMacro(ColumnNamesOnLine, svtkIdType);
  //@}

  //@{
  /**
   * Specifies the number of fields to skip while reading the column names. Default is 1.
   */
  svtkGetMacro(SkipColumnNames, svtkIdType);
  svtkSetMacro(SkipColumnNames, svtkIdType);
  //@}

  //@{
  /**
   * The name of the array for generating or assigning pedigree ids
   * (default "id").
   */
  svtkSetStringMacro(PedigreeIdArrayName);
  svtkGetStringMacro(PedigreeIdArrayName);
  //@}

  //@{
  /**
   * If on (default), generates pedigree ids automatically.
   * If off, assign one of the arrays to be the pedigree id.
   */
  svtkSetMacro(GeneratePedigreeIds, bool);
  svtkGetMacro(GeneratePedigreeIds, bool);
  svtkBooleanMacro(GeneratePedigreeIds, bool);
  //@}

  //@{
  /**
   * If on, assigns pedigree ids to output. Defaults to off.
   */
  svtkSetMacro(OutputPedigreeIds, bool);
  svtkGetMacro(OutputPedigreeIds, bool);
  svtkBooleanMacro(OutputPedigreeIds, bool);
  //@}

  /**
   * Returns a human-readable description of the most recent error, if any.
   * Otherwise, returns an empty string.  Note that the result is only valid
   * after calling Update().
   */
  svtkStdString GetLastError();

protected:
  svtkTecplotTableReader();
  ~svtkTecplotTableReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;
  svtkIdType MaxRecords;
  svtkIdType HeaderLines;
  svtkIdType ColumnNamesOnLine;
  svtkIdType SkipColumnNames;
  char* PedigreeIdArrayName;
  bool GeneratePedigreeIds;
  bool OutputPedigreeIds;
  svtkStdString LastError;

private:
  svtkTecplotTableReader(const svtkTecplotTableReader&) = delete;
  void operator=(const svtkTecplotTableReader&) = delete;
};

#endif
