/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToDatabaseWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTableToDatabaseWriter
 * in a SQL database.
 *
 * svtkTableToDatabaseWriter abstract parent class that reads a svtkTable and
 * inserts it into an SQL database.
 */

#ifndef svtkTableToDatabaseWriter_h
#define svtkTableToDatabaseWriter_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkWriter.h"
#include <string> // STL Header

class svtkSQLDatabase;
class svtkStringArray;
class svtkTable;

class SVTKIOSQL_EXPORT svtkTableToDatabaseWriter : public svtkWriter
{
public:
  svtkTypeMacro(svtkTableToDatabaseWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the database.  Must already be open.
   */
  bool SetDatabase(svtkSQLDatabase* db);

  /**
   * Set the name of the new SQL table that you'd this writer to create.
   * Returns false if the specified table already exists in the database.
   */
  bool SetTableName(const char* name);

  /**
   * Check if the currently specified table name exists in the database.
   */
  bool TableNameIsNew();

  svtkSQLDatabase* GetDatabase() { return this->Database; }

  //@{
  /**
   * Get the input to this writer.
   */
  svtkTable* GetInput();
  svtkTable* GetInput(int port);
  //@}

protected:
  svtkTableToDatabaseWriter();
  ~svtkTableToDatabaseWriter() override;
  void WriteData() override = 0;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkSQLDatabase* Database;
  svtkTable* Input;

  std::string TableName;

private:
  svtkTableToDatabaseWriter(const svtkTableToDatabaseWriter&) = delete;
  void operator=(const svtkTableToDatabaseWriter&) = delete;
};

#endif
