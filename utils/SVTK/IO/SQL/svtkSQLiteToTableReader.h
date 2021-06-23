/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLiteToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSQLiteToTableReader
 * @brief   Read an SQLite table as a svtkTable
 *
 * svtkSQLiteToTableReader reads a table from an SQLite database and
 * outputs it as a svtkTable.
 */

#ifndef svtkSQLiteToTableReader_h
#define svtkSQLiteToTableReader_h

#include "svtkDatabaseToTableReader.h"
#include "svtkIOSQLModule.h" // For export macro

class svtkSQLiteDatabase;

class SVTKIOSQL_EXPORT svtkSQLiteToTableReader : public svtkDatabaseToTableReader
{
public:
  static svtkSQLiteToTableReader* New();
  svtkTypeMacro(svtkSQLiteToTableReader, svtkDatabaseToTableReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkSQLiteToTableReader();
  ~svtkSQLiteToTableReader() override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkSQLiteToTableReader(const svtkSQLiteToTableReader&) = delete;
  void operator=(const svtkSQLiteToTableReader&) = delete;
};

#endif
