/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDatabaseToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDatabaseToTableReader
 * @brief   Read an SQL table as a svtkTable
 *
 * svtkDatabaseToTableReader reads a table from an SQL database, outputting
 * it as a svtkTable.
 */

#ifndef svtkDatabaseToTableReader_h
#define svtkDatabaseToTableReader_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkTableAlgorithm.h"
#include <string> // STL Header

class svtkSQLDatabase;
class svtkStringArray;

class SVTKIOSQL_EXPORT svtkDatabaseToTableReader : public svtkTableAlgorithm
{
public:
  svtkTypeMacro(svtkDatabaseToTableReader, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the database associated with this reader
   */
  bool SetDatabase(svtkSQLDatabase* db);

  /**
   * Set the name of the table that you'd like to convert to a svtkTable
   * Returns false if the specified table does not exist in the database.
   */
  bool SetTableName(const char* name);

  /**
   * Check if the currently specified table name exists in the database.
   */
  bool CheckIfTableExists();

  svtkSQLDatabase* GetDatabase() { return this->Database; }

protected:
  svtkDatabaseToTableReader();
  ~svtkDatabaseToTableReader() override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override = 0;
  svtkSQLDatabase* Database;

  std::string TableName;

private:
  svtkDatabaseToTableReader(const svtkDatabaseToTableReader&) = delete;
  void operator=(const svtkDatabaseToTableReader&) = delete;
};

#endif
