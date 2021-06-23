/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToSQLiteWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTableToSQLiteWriter
 * @brief   store a svtkTable in an SQLite database
 *
 * svtkTableToSQLiteWriter reads a svtkTable and inserts it into an SQLite
 * database.
 */

#ifndef svtkTableToSQLiteWriter_h
#define svtkTableToSQLiteWriter_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkTableToDatabaseWriter.h"

class svtkSQLiteDatabase;

class SVTKIOSQL_EXPORT svtkTableToSQLiteWriter : public svtkTableToDatabaseWriter
{
public:
  static svtkTableToSQLiteWriter* New();
  svtkTypeMacro(svtkTableToSQLiteWriter, svtkTableToDatabaseWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkTable* GetInput();
  svtkTable* GetInput(int port);
  //@}

protected:
  svtkTableToSQLiteWriter();
  ~svtkTableToSQLiteWriter() override;
  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkTableToSQLiteWriter(const svtkTableToSQLiteWriter&) = delete;
  void operator=(const svtkTableToSQLiteWriter&) = delete;
};

#endif
